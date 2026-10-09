#include <atomic>
#include <chrono>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

#include <grpcpp/server_builder.h>
#include <grpcpp/alarm.h>

#include <Commons/Grpc/AsyncBatchingClientBase.hpp>
#include <Commons/Grpc/Batch.grpc.pb.h>

namespace
{
  using Clock = std::chrono::steady_clock;
  using BatchRequest = adserver::grpc::BatchRequest;
  using BatchResponse = adserver::grpc::BatchResponse;
  using Runner = AdServer::Commons::BoostAsioContextRunActiveObject;

  void require(bool condition, const char* message)
  {
    if (!condition)
    {
      throw std::runtime_error(message);
    }
  }

  class Callback final:
    public virtual Generics::ActiveObjectCallback,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    void report_error(Severity, const String::SubString&, const char* = nullptr) noexcept override
    {}
  };

  class Service final: public adserver::grpc::BatchTransport::Service
  {
  public:
    grpc::Status stream_batches(grpc::ServerContext*,
      grpc::ServerReaderWriter<BatchResponse, BatchRequest>* stream) override
    {
      BatchRequest request;
      while (stream->Read(&request))
      {
        {
          std::lock_guard<std::mutex> lock(lock_);
          batch_sizes_.push_back(request.items_size());
        }

        BatchResponse response;
        response.set_batch_id(request.batch_id());
        for (const auto& item : request.items())
        {
          auto* result = response.add_items();
          result->set_request_id(item.request_id());
          result->set_payload(item.payload());
        }

        if (!stream->Write(response))
        {
          break;
        }
      }

      return grpc::Status::OK;
    }

    std::vector<int> batch_sizes()
    {
      std::lock_guard<std::mutex> lock(lock_);
      return batch_sizes_;
    }

  private:
    std::mutex lock_;
    std::vector<int> batch_sizes_;
  };

  class Client final: public AdServer::Grpc::AsyncBatchingClientBase
  {
  public:
    Client(const std::string& endpoint, std::shared_ptr<AdServer::Grpc::GrpcExecutor> executor,
      std::shared_ptr<Runner> runner, AdServer::Grpc::BatchingOptions options)
      : AsyncBatchingClientBase(
          endpoint, std::move(executor), std::move(runner), std::move(options))
    {}

    using AsyncBatchingClientBase::stats;

    void submit(std::atomic<unsigned int>& completed, std::atomic<unsigned int>& errors)
    {
      BatchRequest request;
      request.set_batch_id(42);
      using Response = AdServer::Grpc::ResponseHolder<BatchRequest>;
      std::function<void(const grpc::Status&, Response)> callback =
        [&completed, &errors](const grpc::Status& status, auto response)
        {
          if (!status.ok() || response->batch_id() != 42)
          {
            errors.fetch_add(1);
          }
          completed.fetch_add(1);
        };
      enqueue_request_<BatchRequest, BatchRequest>(
        "/test.Echo", request, std::move(callback), "invalid echo response");
    }
  };

  class CompletionPause
  {
  public:
    explicit CompletionPause(const std::shared_ptr<AdServer::Grpc::GrpcExecutor>& executor)
    {
      auto entered = state_->entered.get_future();
      auto* event = new Event(state_);
      alarm_.Set(&executor->queue(0)->completion_queue(), gpr_now(GPR_CLOCK_MONOTONIC), event);
      if (entered.wait_for(std::chrono::seconds(5)) != std::future_status::ready)
      {
        state_->release.set_value();
        throw std::runtime_error("failed to pause completion processing");
      }
    }

    ~CompletionPause()
    {
      state_->release.set_value();
    }

  private:
    struct State
    {
      std::promise<void> entered;
      std::promise<void> release;
    };

    struct Event final: AdServer::Grpc::GrpcCompletionEvent
    {
      explicit Event(std::shared_ptr<State> state)
        : state_(std::move(state)), release_(state_->release.get_future())
      {}

      void proceed(bool) override
      {
        state_->entered.set_value();
        release_.wait();
      }

      std::shared_ptr<State> state_;
      std::future<void> release_;
    };

    std::shared_ptr<State> state_ = std::make_shared<State>();
    grpc::Alarm alarm_;
  };

  class Fixture
  {
  public:
    explicit Fixture(std::size_t batch_size = 2, std::size_t streams = 1)
      : callback_(new Callback()),
        io_(std::make_shared<Runner::IoService>()),
        runner_(std::make_shared<Runner>(callback_, io_, 4)),
        executor_(std::make_shared<AdServer::Grpc::GrpcExecutor>(2))
    {
      grpc::ServerBuilder builder;
      int port = 0;
      builder.AddListeningPort("127.0.0.1:0", grpc::InsecureServerCredentials(), &port);
      builder.RegisterService(&service_);
      server_ = builder.BuildAndStart();
      require(server_ && port, "failed to start test server");

      AdServer::Grpc::BatchingOptions options;
      options.channels_number = streams;
      options.max_batch_size = batch_size;
      options.hot_buckets_count = 4;
      options.max_batch_delay = Generics::Time(0, 1000);
      options.stream_shrink_period = Generics::Time::ZERO;
      options.reconnect_period = Generics::Time::ZERO;
      client_ = std::make_unique<Client>(
        "127.0.0.1:" + std::to_string(port), executor_, runner_, options);
      client_->activate_object();
    }

    ~Fixture()
    {
      start_executor();
      client_->deactivate_object();
      client_->wait_object();
      if (runner_->active())
      {
        runner_->deactivate_object();
        runner_->wait_object();
      }

      executor_->deactivate_object();
      executor_->wait_object();
      server_->Shutdown();
      server_->Wait();
    }

    void submit(unsigned int count)
    {
      for (unsigned int i = 0; i < count; ++i)
      {
        client_->submit(completed_, errors_);
      }
    }

    void start_executor()
    {
      if (!executor_started_)
      {
        executor_->activate_object();
        executor_started_ = true;
      }
    }

    void check_blocked_timer()
    {
      const auto stats = client_->stats();
      require(stats.available_streams == 0, "blocked client must have no available streams");
      require(stats.queue_items != 0, "test needs an unflushed tail");
      unsigned int callbacks = 0;
      const auto until = Clock::now() + std::chrono::milliseconds(30);
      while (Clock::now() < until)
      {
        callbacks += io_->poll_one();
        require(callbacks <= 1, "blocked client repeatedly rearms its coalescing timer");
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }

      require(client_->stats().queue_items == stats.queue_items, "blocked timer consumed the tail");
    }

    void await_completion(unsigned int count)
    {
      const auto until = Clock::now() + std::chrono::seconds(5);
      while (completed_.load() < count && Clock::now() < until)
      {
        io_->poll_one();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }

      require(completed_.load() == count, "lost timer wakeup: requests did not finish");
      require(client_->stats().queue_items == 0, "requests remain queued after completion");
    }

    Service service_;
    std::unique_ptr<grpc::Server> server_;
    Generics::ActiveObjectCallback_var callback_;
    std::shared_ptr<Runner::IoService> io_;
    std::shared_ptr<Runner> runner_;
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> executor_;
    std::unique_ptr<Client> client_;
    std::atomic<unsigned int> completed_{0};
    std::atomic<unsigned int> errors_{0};
    bool executor_started_ = false;
  };

  void test_connect_and_write_resume()
  {
    Fixture fixture;
    fixture.submit(5);
    fixture.check_blocked_timer();
    fixture.start_executor();
    fixture.await_completion(5);
    require(fixture.errors_ == 0, "healthy stream returned errors");
    require(fixture.service_.batch_sizes() == std::vector<int>({2, 2, 1}),
      "resume must preserve full batches and flush the tail");
  }

  void test_ready_before_timer_runs()
  {
    Fixture fixture;
    fixture.submit(5);
    fixture.start_executor();
    const auto until = Clock::now() + std::chrono::seconds(5);
    while (fixture.completed_ < 4 && Clock::now() < until)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    require(fixture.completed_ == 4, "full batches must finish before the first timer callback");
    fixture.await_completion(5);
    require(fixture.errors_ == 0, "early readiness returned errors");
  }

  void test_failed_connect_resume()
  {
    Fixture fixture;
    fixture.server_->Shutdown();
    fixture.server_->Wait();
    fixture.submit(5);
    fixture.check_blocked_timer();
    fixture.start_executor();
    fixture.await_completion(5);
    require(fixture.errors_ == 5, "unreachable endpoint must fail requests, including the tail");
  }

  void test_busy_stream_resume()
  {
    Fixture fixture;
    fixture.submit(2);
    fixture.start_executor();
    fixture.await_completion(2);
    const auto until = Clock::now() + std::chrono::seconds(5);
    while (fixture.client_->stats().available_streams == 0 && Clock::now() < until)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    require(fixture.client_->stats().available_streams == 1, "warm stream did not become ready");
    {
      CompletionPause pause(fixture.executor_);
      fixture.submit(3);
      const auto stats = fixture.client_->stats();
      require(stats.connecting_streams == 0 && stats.active_streams == 1,
        "test must exhaust existing streams without connecting");
      fixture.check_blocked_timer();
    }

    fixture.await_completion(5);
    require(fixture.errors_ == 0, "stream write completion failed to resume the tail");
    require(fixture.service_.batch_sizes() == std::vector<int>({2, 2, 1}),
      "stream write completion must preserve batching");
  }

  void test_concurrent_producers_and_readiness()
  {
    Fixture fixture(16, 4);
    fixture.submit(33);
    fixture.check_blocked_timer();
    fixture.runner_->activate_object();
    fixture.start_executor();
    std::vector<std::thread> producers;
    for (unsigned int i = 0; i < 8; ++i)
    {
      producers.emplace_back([&fixture] { fixture.submit(101); });
    }

    for (auto& producer : producers)
    {
      producer.join();
    }

    fixture.await_completion(33 + 8 * 101);
    require(fixture.errors_ == 0, "concurrent producers returned errors");
  }
}

int main()
{
  try
  {
    test_connect_and_write_resume();
    test_ready_before_timer_runs();
    test_failed_connect_resume();
    test_busy_stream_resume();
    test_concurrent_producers_and_readiness();
    std::cout << "BatchingTimerTest passed\n";
    return 0;
  }
  catch (const std::exception& ex)
  {
    std::cerr << "BatchingTimerTest: " << ex.what() << '\n';
    return 1;
  }
}
