#include <algorithm>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include <Generics/ActiveObject.hpp>

#include <Commons/ExecutorPool.hpp>
#include <Commons/Grpc/GrpcServiceBase.hpp>

namespace
{
  using BatchRequest = adserver::grpc::BatchRequest;
  using BatchResponse = adserver::grpc::BatchResponse;
  using Clock = std::chrono::steady_clock;

  void test_inprogress_stats()
  {
    AdServer::Grpc::GrpcServiceBase::InprogressStats stats;
    auto snapshot = stats.snapshot();
    assert(snapshot.call_inflight == 0);
    assert(!snapshot.min_time_of_request_in_progress);

    const auto later = stats.add(3, Generics::Time(20));
    const auto earlier = stats.add(2, Generics::Time(10));
    const auto same_time = stats.add(4, Generics::Time(10));
    snapshot = stats.snapshot();
    assert(snapshot.call_inflight == 9);
    assert(snapshot.min_time_of_request_in_progress == Generics::Time(10));

    stats.remove(earlier);
    stats.remove(earlier);
    snapshot = stats.snapshot();
    assert(snapshot.call_inflight == 7);
    assert(snapshot.min_time_of_request_in_progress == Generics::Time(10));

    stats.remove(same_time);
    snapshot = stats.snapshot();
    assert(snapshot.call_inflight == 3);
    assert(snapshot.min_time_of_request_in_progress == Generics::Time(20));

    stats.remove(later);
    snapshot = stats.snapshot();
    assert(snapshot.call_inflight == 0);
    assert(!snapshot.min_time_of_request_in_progress);
  }

  void test_inprogress_stats_cross_thread_removal()
  {
    AdServer::Grpc::GrpcServiceBase::InprogressStats stats;
    constexpr std::size_t thread_count = 80;
    constexpr std::size_t requests_per_thread = 32;
    std::vector<std::uint64_t> ids(thread_count * requests_per_thread);
    std::vector<std::thread> threads;
    for (std::size_t thread = 0; thread < thread_count; ++thread)
    {
      threads.emplace_back([&, thread]
      {
        for (std::size_t i = 0; i < requests_per_thread; ++i)
        {
          ids[thread * requests_per_thread + i] = stats.add(2, Generics::Time(10 + i));
        }
      });
    }

    for (auto& thread : threads)
    {
      thread.join();
    }

    std::sort(ids.begin(), ids.end());
    assert(std::adjacent_find(ids.begin(), ids.end()) == ids.end());
    auto snapshot = stats.snapshot();
    assert(snapshot.call_inflight == ids.size() * 2);
    assert(snapshot.min_time_of_request_in_progress == Generics::Time(10));

    threads.clear();
    std::atomic<bool> done{false};
    std::thread reader([&]
    {
      while (!done.load(std::memory_order_relaxed))
      {
        const auto current = stats.snapshot();
        assert(current.call_inflight <= ids.size() * 2);
        assert(bool(current.min_time_of_request_in_progress) == (current.call_inflight != 0));
      }
    });
    for (std::size_t thread = 0; thread < 8; ++thread)
    {
      threads.emplace_back([&, thread]
      {
        for (std::size_t i = thread; i < ids.size(); i += 8)
        {
          stats.remove(ids[i]);
        }
      });
    }

    for (auto& thread : threads)
    {
      thread.join();
    }

    done.store(true, std::memory_order_relaxed);
    reader.join();
    snapshot = stats.snapshot();
    assert(snapshot.call_inflight == 0);
    assert(!snapshot.min_time_of_request_in_progress);
  }

  class NullActiveObjectCallback final:
    public virtual Generics::ActiveObjectCallback,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    void report_error(Severity, const String::SubString&, const char* = nullptr) noexcept override
    {}

  protected:
    ~NullActiveObjectCallback() noexcept override = default;
  };

  class TestService final:
    public AdServer::Grpc::GrpcServiceBase
  {
  public:
    explicit TestService(std::shared_ptr<AdServer::Commons::ExecutorPool> executor_pool)
      : executor_pool_(std::move(executor_pool))
    {
      register_batch_method<BatchRequest, BatchResponse>(
        "/test.Delay",
        [](const BatchRequest& request, BatchResponse& response, grpc::Status&)
        {
          std::this_thread::sleep_for(std::chrono::microseconds(request.batch_id()));
          response.set_batch_id(request.batch_id());
        });
    }

    using GrpcServiceBase::start_handle_batch_request;

  private:
    void register_in_queue(grpc::ServerCompletionQueue*) override
    {}

    std::size_t distributed_batch_max_sequential_ops() const noexcept override
    {
      return 1;
    }

    std::shared_ptr<AdServer::Commons::ExecutorPool>
    batch_processing_executor_pool() const noexcept override
    {
      return executor_pool_;
    }

    const std::shared_ptr<AdServer::Commons::ExecutorPool> executor_pool_;
  };

  class TestPublisher final:
    public AdServer::Grpc::GrpcServiceBase::BatchResponsePublisher,
    public std::enable_shared_from_this<TestPublisher>
  {
  public:
    struct Chunk
    {
      Clock::time_point time;
      std::uint64_t batch_id = 0;
      std::vector<std::uint64_t> request_ids;
    };

    google::protobuf::Arena& response_arena() noexcept override
    {
      return arena_;
    }

    std::shared_ptr<BatchResponsePublisher> retain() noexcept override
    {
      return shared_from_this();
    }

    BatchResponse* create_response() override
    {
      return google::protobuf::Arena::CreateMessage<BatchResponse>(&arena_);
    }

    void publish_response(BatchResponse* response) noexcept override
    {
      Chunk chunk;
      chunk.time = Clock::now();
      chunk.batch_id = response->batch_id();
      chunk.request_ids.reserve(response->items_size());
      for (const auto& item : response->items())
      {
        chunk.request_ids.emplace_back(item.request_id());
      }

      {
        std::lock_guard<std::mutex> lock(lock_);
        chunks_.emplace_back(std::move(chunk));
      }
      condition_.notify_all();
    }

    void complete(std::optional<std::exception_ptr> exception)
    {
      {
        std::lock_guard<std::mutex> lock(lock_);
        exception_ = std::move(exception);
        completed_ = true;
      }
      condition_.notify_all();
    }

    bool wait_for_chunks(const std::size_t count, const std::chrono::milliseconds timeout)
    {
      std::unique_lock<std::mutex> lock(lock_);
      return condition_.wait_for(
        lock,
        timeout,
        [this, count]() noexcept
        {
          return chunks_.size() >= count;
        });
    }

    bool wait_for_completion(const std::chrono::milliseconds timeout)
    {
      std::unique_lock<std::mutex> lock(lock_);
      return condition_.wait_for(lock, timeout, [this]() noexcept { return completed_; });
    }

    std::vector<Chunk> chunks() const
    {
      std::lock_guard<std::mutex> lock(lock_);
      return chunks_;
    }

    bool has_exception() const
    {
      std::lock_guard<std::mutex> lock(lock_);
      return exception_.has_value();
    }

  private:
    google::protobuf::Arena arena_;
    mutable std::mutex lock_;
    std::condition_variable condition_;
    std::vector<Chunk> chunks_;
    std::optional<std::exception_ptr> exception_;
    bool completed_ = false;
  };

  class TestReadWaiter final:
    public AdServer::Grpc::GrpcServiceBase::BatchStreamReadLimiter::Waiter
  {
  public:
    void start_read_from_limiter() noexcept override
    {
      started = true;
    }

    bool started = false;
  };

  void test_read_limiter_stats()
  {
    using ReadLimiter = AdServer::Grpc::GrpcServiceBase::BatchStreamReadLimiter;
    ReadLimiter limiter(ReadLimiter::Options(true, 2));

    auto stats = limiter.stats();
    assert(stats.read_ahead_enabled);
    assert(stats.max_requests_in_progress == 2);
    assert(stats.requests_in_progress == 0);
    assert(stats.read_reservations == 0);
    assert(stats.waiting_streams == 0);

    assert(limiter.reserve_read_or_enqueue(std::make_shared<TestReadWaiter>()));
    stats = limiter.stats();
    assert(stats.read_reservations == 1);

    limiter.complete_read_reservation(2);
    auto waiter = std::make_shared<TestReadWaiter>();
    assert(!limiter.reserve_read_or_enqueue(waiter));
    stats = limiter.stats();
    assert(stats.requests_in_progress == 2);
    assert(stats.read_reservations == 0);
    assert(stats.waiting_streams == 1);

    limiter.complete_requests(2);
    stats = limiter.stats();
    assert(waiter->started);
    assert(stats.requests_in_progress == 0);
    assert(stats.read_reservations == 1);
    assert(stats.waiting_streams == 0);

    limiter.cancel_read_reservation();
    assert(limiter.stats().read_reservations == 0);
  }

  void add_item(BatchRequest& batch, const std::uint64_t request_id, const std::uint64_t delay_us)
  {
    BatchRequest request;
    request.set_batch_id(delay_us);

    auto* item = batch.add_items();
    item->set_request_id(request_id);
    item->set_full_method("/test.Delay");
    assert(request.SerializeToString(item->mutable_payload()));
  }

  std::shared_ptr<TestPublisher> run_batch(
    TestService& service,
    BatchRequest& request,
    AdServer::Grpc::GrpcServiceBase::BatchProcessingHandle& handle)
  {
    auto publisher = std::make_shared<TestPublisher>();
    service.start_handle_batch_request(
      handle,
      request,
      *publisher,
      [publisher](std::optional<std::exception_ptr> exception)
      {
        publisher->complete(std::move(exception));
      });
    return publisher;
  }

  void test_fast_batch_is_published_before_first_step(TestService& service)
  {
    BatchRequest request;
    request.set_batch_id(100);
    request.add_response_time_steps_us(100'000);
    add_item(request, 100, 0);
    add_item(request, 101, 0);

    AdServer::Grpc::GrpcServiceBase::BatchProcessingHandle handle;
    const auto started_at = Clock::now();
    auto publisher = run_batch(service, request, handle);

    assert(publisher->wait_for_completion(std::chrono::seconds(1)));
    assert(publisher->wait_for_chunks(1, std::chrono::seconds(1)));
    assert(!publisher->has_exception());

    const auto chunks = publisher->chunks();
    assert(chunks.size() == 1);
    assert(chunks.front().request_ids == (std::vector<std::uint64_t>{100, 101}));
    assert(chunks.front().time - started_at < std::chrono::milliseconds(50));
  }

  void test_batch_without_steps_uses_single_response(TestService& service)
  {
    BatchRequest request;
    request.set_batch_id(150);
    add_item(request, 150, 0);
    add_item(request, 151, 0);

    AdServer::Grpc::GrpcServiceBase::BatchProcessingHandle handle;
    auto publisher = run_batch(service, request, handle);

    assert(publisher->wait_for_completion(std::chrono::seconds(1)));
    assert(publisher->wait_for_chunks(1, std::chrono::seconds(1)));
    assert(!publisher->has_exception());

    const auto chunks = publisher->chunks();
    assert(chunks.size() == 1);
    assert(chunks.front().batch_id == 150);
    assert(chunks.front().request_ids == (std::vector<std::uint64_t>{150, 151}));
  }

  void test_slow_lane_is_published_separately(TestService& service)
  {
    BatchRequest request;
    request.set_batch_id(200);
    request.add_response_time_steps_us(10'000);
    request.add_response_time_steps_us(30'000);
    add_item(request, 200, 0);
    add_item(request, 201, 60'000);
    add_item(request, 202, 0);

    AdServer::Grpc::GrpcServiceBase::BatchProcessingHandle handle;
    const auto started_at = Clock::now();
    auto publisher = run_batch(service, request, handle);

    assert(publisher->wait_for_chunks(1, std::chrono::milliseconds(45)));
    auto chunks = publisher->chunks();
    assert(chunks.size() == 1);
    assert(chunks.front().time - started_at >= std::chrono::milliseconds(5));
    assert(chunks.front().time - started_at < std::chrono::milliseconds(45));

    assert(publisher->wait_for_completion(std::chrono::seconds(1)));
    assert(publisher->wait_for_chunks(2, std::chrono::seconds(1)));
    assert(!publisher->has_exception());

    chunks = publisher->chunks();
    assert(chunks.size() == 2);
    std::vector<std::uint64_t> request_ids;
    for (const auto& chunk : chunks)
    {
      request_ids.insert(request_ids.end(), chunk.request_ids.begin(), chunk.request_ids.end());
    }
    std::sort(request_ids.begin(), request_ids.end());
    assert(request_ids == (std::vector<std::uint64_t>{200, 201, 202}));
  }
}

int main()
{
  test_inprogress_stats();
  test_inprogress_stats_cross_thread_removal();
  test_read_limiter_stats();

  Generics::ActiveObjectCallback_var callback(new NullActiveObjectCallback());
  auto executor_pool = std::make_shared<AdServer::Commons::ExecutorPool>(
    callback,
    4,
    AdServer::Commons::ExecutorPool::ResumeStrategy::CurrentContext,
    "batch-partial");
  executor_pool->activate_object();

  TestService service(executor_pool);
  test_batch_without_steps_uses_single_response(service);
  test_fast_batch_is_published_before_first_step(service);
  test_slow_lane_is_published_separately(service);

  executor_pool->deactivate_object();
  executor_pool->wait_object();

  std::cout << "BatchPartialResponseTest passed" << std::endl;
  return 0;
}
