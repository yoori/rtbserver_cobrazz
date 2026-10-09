#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <future>
#include <iostream>
#include <map>
#include <mutex>
#include <set>
#include <thread>

#include <Commons/AsyncMutex.hpp>
#include <Commons/Coro/CallbackAwaiter.hpp>
#include <Commons/Coro/TupleAwaitable.hpp>
#include <Commons/ExecutorPool.hpp>
#include <Commons/Grpc/GrpcServiceBase.hpp>

namespace
{
  using namespace std::chrono_literals;
  using Request = adserver::grpc::BatchRequest;
  using Response = adserver::grpc::BatchResponse;
  using Item = adserver::grpc::BatchRequestItem;
  using Strategy = AdServer::Grpc::BatchExecutionStrategy;
  using ServiceBase = AdServer::Grpc::GrpcServiceBase;
  using Pool = AdServer::Commons::ExecutorPool;

  class Callback final : public Generics::ActiveObjectCallback,
    public ReferenceCounting::AtomicImpl
  {
  public:
    void report_error(Severity, const String::SubString& text, const char*) noexcept override
    {
      std::cerr << text << std::endl;
      std::abort();
    }
  };

  class Service final : public ServiceBase
  {
  public:
    Service(std::shared_ptr<Pool> pool, Strategy strategy, bool benchmark = false,
      bool distinct_hashes = false)
      : pool_(std::move(pool)), benchmark_(benchmark)
    {
      const auto hash = [benchmark, distinct_hashes](const Item& request) -> std::size_t
      {
        if (benchmark)
        {
          return std::hash<std::string>()(request.full_method());
        }

        // Exercise both full-hash collisions and distinct hashes mapped to the same lane.
        return distinct_hashes ? 64 * static_cast<unsigned char>(request.full_method().front()) : 0;
      };
      const auto key = [](const Item& request) -> std::string_view
      {
        return request.full_method();
      };
      const auto handler = [this](Item&& request, Item& response, grpc::Status& status)
      {
        return handle(std::move(request), response, status);
      };
      register_batch_coro_method<Item, Item>(
        "/test.Match", handler, hash, true, true, strategy, key);
      register_batch_coro_method<Item, Item>("/test.Legacy", handler, hash, true, true);
    }

    using ServiceBase::start_handle_batch_request;

    AdServer::Commons::AsyncMutex::Guard hold_mutex()
    {
      return mutex_.scoped_lock();
    }

    bool wait_started(std::uint64_t id, std::chrono::milliseconds timeout = 2s)
    {
      std::unique_lock guard(lock_);
      return condition_.wait_for(guard, timeout, [&] { return started_.count(id); });
    }

    bool started(std::uint64_t id)
    {
      std::lock_guard guard(lock_);
      return started_.count(id);
    }

    void release(std::uint64_t id)
    {
      std::function<void()> callback;
      {
        std::unique_lock guard(lock_);
        assert(condition_.wait_for(guard, 2s, [&] { return callbacks_.count(id); }));
        callback = std::move(callbacks_.at(id));
        callbacks_.erase(id);
      }
      callback();
    }

    bool wait_finished(std::uint64_t id)
    {
      std::unique_lock guard(lock_);
      return condition_.wait_for(guard, 2s, [&] { return finished_.count(id); });
    }

  private:
    struct Segment
    {
      explicit Segment(Service& service) : service(service)
      {
        assert(service.executing_.fetch_add(1) == 0);
        assert(service.pool_->running_in_this_thread());
        std::this_thread::yield();
      }
      ~Segment()
      {
        assert(service.executing_.fetch_sub(1) == 1);
      }
      Service& service;
    };

    AdServer::Commons::StartableAwaitable<int> nested()
    {
      Segment segment(*this);
      co_return 1;
    }

    AdServer::Commons::StartableAwaitable<void>
    handle(Item&& request, Item& response, grpc::Status&)
    {
      if (benchmark_)
      {
        response.set_request_id(request.request_id());
        co_return;
      }

      const auto id = request.request_id();
      {
        Segment segment(*this);
        std::lock_guard guard(lock_);
        assert(started_.insert(id).second);
      }
      condition_.notify_all();
      AdServer::Commons::AsyncMutex::Guard mutex_guard;
      if (request.payload() == "mutex")
      {
        mutex_guard = co_await mutex_.scoped_lock_async();
      }

      if (request.payload() == "wait")
      {
        co_await AdServer::Commons::async_callback<>([this, id](std::function<void()> callback)
        {
          {
            std::lock_guard guard(lock_);
            callbacks_.emplace(id, std::move(callback));
          }
          condition_.notify_all();
        });
      }

      const auto [first, second] = co_await AdServer::Commons::TupleAwaitable(nested(), nested());
      assert(first + second == 2);
      {
        Segment segment(*this);
        response.set_request_id(id);
        std::lock_guard guard(lock_);
        finished_.insert(id);
      }
      condition_.notify_all();
      if (request.payload() == "throw")
      {
        throw std::runtime_error("test handler failure");
      }

      if (request.payload() == "throw_unknown")
      {
        throw 1;
      }
    }

    void register_in_queue(grpc::ServerCompletionQueue*) override
    {}

    std::size_t distributed_batch_max_sequential_ops() const noexcept override
    {
      return 4;
    }

    std::shared_ptr<Pool> batch_processing_executor_pool() const noexcept override
    {
      return pool_;
    }

    const std::shared_ptr<Pool> pool_;
    const bool benchmark_;
    std::mutex lock_;
    std::condition_variable condition_;
    std::set<std::uint64_t> started_;
    std::set<std::uint64_t> finished_;
    std::map<std::uint64_t, std::function<void()>> callbacks_;
    std::atomic<unsigned> executing_{0};
    AdServer::Commons::AsyncMutex mutex_;
  };

  class Publisher final : public ServiceBase::BatchResponsePublisher,
    public std::enable_shared_from_this<Publisher>
  {
  public:
    google::protobuf::Arena& response_arena() noexcept override
    {
      return arena_;
    }

    std::shared_ptr<BatchResponsePublisher> retain() noexcept override
    {
      return shared_from_this();
    }

    Response* create_response() override
    {
      return google::protobuf::Arena::CreateMessage<Response>(&arena_);
    }

    void publish_response(Response* response) noexcept override
    {
      std::unique_lock guard(lock_);
      publishing_ = true;
      condition_.notify_all();
      condition_.wait(guard, [&] { return !pause_; });
      assert(!completed_);
      assert(response->items_size() > 0);
      for (const auto& item : response->items())
      {
        assert(statuses_.emplace(item.request_id(), item.status_code()).second);
      }
      condition_.notify_all();
    }

    void complete(std::optional<std::exception_ptr> exception)
    {
      assert(!exception);
      std::lock_guard guard(lock_);
      completed_ = true;
      condition_.notify_all();
    }

    bool wait_item(std::uint64_t id)
    {
      std::unique_lock guard(lock_);
      return condition_.wait_for(guard, 2s, [&] { return statuses_.count(id); });
    }

    bool wait_complete()
    {
      std::unique_lock guard(lock_);
      return condition_.wait_for(guard, 2s, [&] { return completed_; });
    }

    bool completed()
    {
      std::lock_guard guard(lock_);
      return completed_;
    }

    void pause(bool value)
    {
      std::lock_guard guard(lock_);
      pause_ = value;
      condition_.notify_all();
    }

    bool wait_publishing()
    {
      std::unique_lock guard(lock_);
      return condition_.wait_for(guard, 2s, [&] { return publishing_; });
    }

    std::map<std::uint64_t, int> statuses()
    {
      std::lock_guard guard(lock_);
      return statuses_;
    }

  private:
    google::protobuf::Arena arena_;
    std::mutex lock_;
    std::condition_variable condition_;
    std::map<std::uint64_t, int> statuses_;
    bool pause_ = false;
    bool publishing_ = false;
    bool completed_ = false;
  };

  void add(Request& batch, std::uint64_t id, std::string key, std::string action = {},
    std::string method = "/test.Match")
  {
    Item request;
    request.set_request_id(id);
    request.set_full_method(std::move(key));
    request.set_payload(std::move(action));
    auto* item = batch.add_items();
    item->set_request_id(id);
    item->set_full_method(std::move(method));
    assert(request.SerializeToString(item->mutable_payload()));
  }

  void steps(Request& request)
  {
    for (unsigned step : {10'000, 30'000, 100'000, 300'000, 1'000'000})
    {
      request.add_response_time_steps_us(step);
    }
  }

  void test_independent_keys(const std::shared_ptr<Pool>& pool, bool partial,
    bool distinct_hashes = false)
  {
    Service service(pool, Strategy::CooperativeByKey, false, distinct_hashes);
    Request request;
    request.set_batch_id(10);
    if (partial)
    {
      steps(request);
    }
    add(request, 10, "a", "wait");
    add(request, 11, "a");
    add(request, 12, "b");
    add(request, 13, "c", "throw");
    add(request, 14, "d");
    request.mutable_items(4)->set_payload("\xff");
    add(request, 15, "c", "throw_unknown");
    add(request, 16, "c");

    auto publisher = std::make_shared<Publisher>();
    ServiceBase::BatchProcessingHandle handle;
    service.start_handle_batch_request(handle, request, *publisher,
      [publisher](auto error) { publisher->complete(std::move(error)); });
    assert(service.wait_finished(12));
    assert(service.wait_finished(16));
    assert(!service.started(11));
    if (partial)
    {
      assert(publisher->wait_item(12));
      assert(!publisher->statuses().count(10));
    }
    assert(!publisher->completed());
    service.release(10);
    assert(publisher->wait_complete());
    const auto statuses = publisher->statuses();
    assert(statuses.size() == 7);
    assert(statuses.at(10) == grpc::StatusCode::OK);
    assert(statuses.at(11) == grpc::StatusCode::OK);
    assert(statuses.at(13) == grpc::StatusCode::INTERNAL);
    assert(statuses.at(14) == grpc::StatusCode::INVALID_ARGUMENT);
    assert(statuses.at(15) == grpc::StatusCode::INTERNAL);
    assert(statuses.at(16) == grpc::StatusCode::OK);
  }

  void test_legacy_and_mixed(const std::shared_ptr<Pool>& pool, Strategy strategy, bool mixed)
  {
    Service service(pool, strategy);
    Request request;
    steps(request);
    add(request, 20, "a", "wait", mixed ? "/test.Legacy" : "/test.Match");
    add(request, 21, "b", {}, mixed ? "/test.Legacy" : "/test.Match");
    if (mixed)
    {
      add(request, 22, "c");
    }
    auto publisher = std::make_shared<Publisher>();
    ServiceBase::BatchProcessingHandle handle;
    service.start_handle_batch_request(handle, request, *publisher,
      [publisher](auto error) { publisher->complete(std::move(error)); });
    assert(service.wait_started(20));
    if (mixed)
    {
      assert(publisher->wait_item(22));
    }
    assert(!service.wait_started(21, 20ms));
    service.release(20);
    assert(publisher->wait_complete());
    assert(publisher->statuses().size() == (mixed ? 3 : 2));
  }

  void test_timer_completion_race(const std::shared_ptr<Pool>& pool)
  {
    Service service(pool, Strategy::CooperativeByKey);
    Request request;
    steps(request);
    add(request, 30, "a", "wait");
    add(request, 31, "b");
    auto publisher = std::make_shared<Publisher>();
    publisher->pause(true);
    ServiceBase::BatchProcessingHandle handle;
    service.start_handle_batch_request(handle, request, *publisher,
      [publisher](auto error) { publisher->complete(std::move(error)); });
    assert(publisher->wait_publishing());
    service.release(30);
    assert(service.wait_finished(30));
    assert(!publisher->completed());
    publisher->pause(false);
    assert(publisher->wait_complete());
    assert(publisher->statuses().size() == 2);
  }

  void test_mutex_wakeups(const std::shared_ptr<Pool>& pool)
  {
    Service service(pool, Strategy::CooperativeByKey);
    auto owner = service.hold_mutex();
    Request request;
    steps(request);
    add(request, 40, "a", "mutex");
    add(request, 41, "b", "mutex");
    add(request, 42, "c");
    auto publisher = std::make_shared<Publisher>();
    ServiceBase::BatchProcessingHandle handle;
    service.start_handle_batch_request(handle, request, *publisher,
      [publisher](auto error) { publisher->complete(std::move(error)); });
    assert(publisher->wait_item(42));
    assert(service.started(40));
    assert(service.started(41));
    assert(publisher->statuses().size() == 1);
    assert(AdServer::Commons::AsyncMutex::stats().current_waiters == 2);
    owner.reset();
    assert(publisher->wait_complete());
    assert(publisher->statuses().size() == 3);
    assert(AdServer::Commons::AsyncMutex::stats().current_waiters == 0);
  }

  void test_concurrent_wakeups(const std::shared_ptr<Pool>& pool)
  {
    Service service(pool, Strategy::CooperativeByKey);
    Request request;
    steps(request);
    for (unsigned i = 0; i < 64; ++i)
    {
      add(request, i, std::to_string(i), "wait");
    }
    auto publisher = std::make_shared<Publisher>();
    ServiceBase::BatchProcessingHandle handle;
    service.start_handle_batch_request(handle, request, *publisher,
      [publisher](auto error) { publisher->complete(std::move(error)); });
    for (unsigned i = 0; i < 64; ++i)
    {
      assert(service.wait_started(i));
    }
    handle.reset();
    std::vector<std::thread> threads;
    for (unsigned t = 0; t < 8; ++t)
    {
      threads.emplace_back([&, t]
      {
        for (unsigned i = t; i < 64; i += 8)
        {
          service.release(i);
        }
      });
    }
    for (auto& thread : threads)
    {
      thread.join();
    }
    assert(publisher->wait_complete());
    assert(publisher->statuses().size() == 64);
  }

  void test_ready_queue_wraparound(const std::shared_ptr<Pool>& pool)
  {
    for (unsigned keys : {1, 2, 7, 8, 9, 16, 17, 64, 65})
    {
      Service service(pool, Strategy::CooperativeByKey);
      Request request;
      steps(request);
      // Nested TupleAwaitable children grow the ready ring after its head has advanced.
      for (unsigned round = 0; round < 6; ++round)
      {
        for (unsigned key = 0; key < keys; ++key)
        {
          add(request, round * keys + key, std::to_string(key));
        }
      }

      auto publisher = std::make_shared<Publisher>();
      ServiceBase::BatchProcessingHandle handle;
      service.start_handle_batch_request(handle, request, *publisher,
        [publisher](auto error) { publisher->complete(std::move(error)); });
      assert(publisher->wait_complete());
      const auto statuses = publisher->statuses();
      assert(statuses.size() == 6 * keys);
      for (const auto& [id, status] : statuses)
      {
        assert(status == grpc::StatusCode::OK);
      }
    }
  }

  void test_ready_queue_handoffs(const std::shared_ptr<Pool>& pool)
  {
    constexpr unsigned rounds = 4;
    for (unsigned keys : {2, 9, 65})
    {
      Service service(pool, Strategy::CooperativeByKey);
      Request request;
      steps(request);
      for (unsigned round = 0; round < rounds; ++round)
      {
        for (unsigned key = 0; key < keys; ++key)
        {
          add(request, round * keys + key, std::to_string(key), "wait");
        }
      }

      auto publisher = std::make_shared<Publisher>();
      ServiceBase::BatchProcessingHandle handle;
      service.start_handle_batch_request(handle, request, *publisher,
        [publisher](auto error) { publisher->complete(std::move(error)); });
      handle.reset();
      for (unsigned round = 0; round < rounds; ++round)
      {
        for (unsigned key = 0; key < keys; ++key)
        {
          assert(service.wait_started(round * keys + key));
        }

        if (round + 1 < rounds)
        {
          assert(!service.started((round + 1) * keys));
        }

        std::vector<std::thread> threads;
        for (unsigned thread = 0; thread < 4; ++thread)
        {
          threads.emplace_back([&, round, thread]
          {
            for (unsigned key = thread; key < keys; key += 4)
            {
              service.release(round * keys + key);
            }
          });
        }
        for (auto& thread : threads)
        {
          thread.join();
        }
      }
      assert(publisher->wait_complete());
      const auto statuses = publisher->statuses();
      assert(statuses.size() == rounds * keys);
      for (const auto& [id, status] : statuses)
      {
        assert(status == grpc::StatusCode::OK);
      }
    }
  }

  void test_full_response(const std::shared_ptr<Pool>& pool, bool single_key)
  {
    Service service(pool, Strategy::CooperativeByKey);
    Request request;
    add(request, 0, "a", "wait");
    add(request, 1, "a", "throw");
    add(request, 2, "a");
    if (!single_key)
    {
      add(request, 3, "b");
    }

    google::protobuf::Arena arena;
    auto& response = *google::protobuf::Arena::CreateMessage<Response>(&arena);
    ServiceBase::BatchProcessingHandle handle;
    std::promise<void> completion;
    auto future = completion.get_future();
    service.start_handle_batch_request(handle, request, response, [&](auto error)
    {
      assert(!error);
      completion.set_value();
    });
    assert(service.wait_started(0));
    assert(!service.started(1));
    if (!single_key)
    {
      assert(service.wait_finished(3));
    }

    handle.reset();
    service.release(0);
    assert(future.wait_for(2s) == std::future_status::ready);
    assert(response.items_size() == request.items_size());
    for (const auto& item : response.items())
    {
      assert(item.status_code() ==
        (item.request_id() == 1 ? grpc::StatusCode::INTERNAL : grpc::StatusCode::OK));
    }
  }

  void test_multi_lane_batch(const std::shared_ptr<Pool>& pool, bool partial)
  {
    Service service(pool, Strategy::CooperativeByKey, true);
    Request request;
    if (partial)
    {
      steps(request);
    }

    std::map<std::uint64_t, int> expected;
    for (unsigned i = 0; i < 257; ++i)
    {
      add(request, i, std::to_string(i % 97), {}, i % 11 ? "/test.Match" : "/test.Legacy");
      if (i % 31 == 0)
      {
        request.mutable_items(i)->set_payload("\xff");
      }
      expected.emplace(i, i % 31 ? grpc::StatusCode::OK : grpc::StatusCode::INVALID_ARGUMENT);
    }

    for (unsigned round = 0; round < 6; ++round)
    {
      ServiceBase::BatchProcessingHandle handle;
      std::promise<void> started;
      auto started_future = started.get_future();
      if (partial)
      {
        auto publisher = std::make_shared<Publisher>();
        auto start = [&]
        {
          service.start_handle_batch_request(handle, request, *publisher,
            [publisher](auto error) { publisher->complete(std::move(error)); });
          handle.reset();
          started.set_value();
        };
        if (round % 2)
        {
          pool->post(start);
        }
        else
        {
          start();
        }
        assert(started_future.wait_for(2s) == std::future_status::ready);
        assert(publisher->wait_complete());
        assert(publisher->statuses() == expected);
      }
      else
      {
        Response response;
        std::promise<void> completion;
        auto future = completion.get_future();
        auto start = [&]
        {
          service.start_handle_batch_request(handle, request, response, [&](auto error)
          {
            assert(!error);
            completion.set_value();
          });
          handle.reset();
          started.set_value();
        };
        if (round % 2)
        {
          pool->post(start);
        }
        else
        {
          start();
        }
        assert(started_future.wait_for(2s) == std::future_status::ready);
        assert(future.wait_for(2s) == std::future_status::ready);
        assert(response.items_size() == request.items_size());
        std::map<std::uint64_t, int> actual;
        for (const auto& item : response.items())
        {
          assert(actual.emplace(item.request_id(), item.status_code()).second);
          if (item.status_code() == grpc::StatusCode::OK)
          {
            Item response_item;
            assert(response_item.ParseFromString(item.payload()));
            assert(response_item.request_id() == item.request_id());
          }
        }
        assert(actual == expected);
      }
    }
  }

  void benchmark(const std::shared_ptr<Pool>& pool, Strategy strategy)
  {
    Service service(pool, strategy, true);
    Request request;
    for (unsigned i = 0; i < 256; ++i)
    {
      add(request, i, std::to_string(i));
    }
    constexpr unsigned iterations = 4000;
    const auto start = std::chrono::steady_clock::now();
    for (unsigned i = 0; i < iterations; ++i)
    {
      google::protobuf::Arena arena;
      auto& response = *google::protobuf::Arena::CreateMessage<Response>(&arena);
      ServiceBase::BatchProcessingHandle handle;
      std::promise<void> completion;
      service.start_handle_batch_request(handle, request, response, [&](auto error)
      {
        assert(!error);
        completion.set_value();
      });
      completion.get_future().get();
      assert(response.items_size() == request.items_size());
    }
    const auto elapsed = std::chrono::steady_clock::now() - start;
    std::cout << (strategy == Strategy::CooperativeByKey ? "cooperative" : "sequential") <<
      " ns/item=" <<
      std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count() /
        double(iterations * request.items_size()) << std::endl;
  }
}

int main(int argc, char** argv)
{
  Generics::ActiveObjectCallback_var callback(new Callback());
  auto pool = std::make_shared<Pool>(callback, 4, Pool::ResumeStrategy::CurrentContext);
  pool->activate_object();
  if (argc == 2)
  {
    benchmark(pool, std::string_view(argv[1]) == "cooperative" ?
      Strategy::CooperativeByKey : Strategy::Sequential);
  }
  else
  {
    test_independent_keys(pool, true);
    test_independent_keys(pool, false);
    test_independent_keys(pool, true, true);
    test_legacy_and_mixed(pool, Strategy::Sequential, false);
    test_legacy_and_mixed(pool, Strategy::CooperativeByKey, true);
    test_timer_completion_race(pool);
    test_mutex_wakeups(pool);
    test_ready_queue_wraparound(pool);
    test_ready_queue_handoffs(pool);
    test_full_response(pool, true);
    test_full_response(pool, false);
    test_multi_lane_batch(pool, true);
    test_multi_lane_batch(pool, false);
    for (unsigned i = 0; i < 20; ++i)
    {
      test_concurrent_wakeups(pool);
    }

    auto single_pool = std::make_shared<Pool>(callback, 1, Pool::ResumeStrategy::CurrentContext);
    single_pool->activate_object();
    test_independent_keys(single_pool, true);
    test_independent_keys(single_pool, false);
    test_independent_keys(single_pool, false, true);
    test_mutex_wakeups(single_pool);
    test_concurrent_wakeups(single_pool);
    test_ready_queue_wraparound(single_pool);
    test_ready_queue_handoffs(single_pool);
    test_full_response(single_pool, true);
    test_full_response(single_pool, false);
    test_multi_lane_batch(single_pool, true);
    test_multi_lane_batch(single_pool, false);
    single_pool->deactivate_object();
    single_pool->wait_object();
    std::cout << "BatchCooperativeTest passed" << std::endl;
  }
  pool->deactivate_object();
  pool->wait_object();
  pool.reset();
  google::protobuf::ShutdownProtobufLibrary();
}
