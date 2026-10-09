#include <atomic>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <grpcpp/server_builder.h>

#include <Commons/DeferredDeletionQueue.hpp>
#include <Commons/Grpc/AsyncBatchingClientBase.hpp>
#include <Commons/Grpc/Batch.grpc.pb.h>
#include <Commons/Grpc/DistributedPartitionPool.hpp>
#include <Commons/Grpc/RefPool.hpp>

namespace
{
  using namespace std::chrono_literals;
  using BatchRequest = adserver::grpc::BatchRequest;
  using BatchResponse = adserver::grpc::BatchResponse;
  using Runner = AdServer::Commons::BoostAsioContextRunActiveObject;
  using Queue = AdServer::Commons::DeferredDeletionQueue;

  void require(bool condition, const char* message)
  {
    if (!condition)
    {
      std::cerr << message << std::endl;
      std::abort();
    }
  }

  template<typename T>
  T await(std::future<T>& future)
  {
    require(future.wait_for(5s) == std::future_status::ready, "completion timed out");
    return future.get();
  }

  class Callback final:
    public virtual Generics::ActiveObjectCallback,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    void report_error(Severity, const String::SubString&, const char* = nullptr) noexcept override
    {}
  };

  struct Lifetime
  {
    std::promise<std::thread::id> deleted;
    std::atomic<unsigned int> stops{0};
    std::atomic<unsigned int> waits{0};
  };

  class Client final: public AdServer::Grpc::AsyncBatchingClientBase
  {
  public:
    using Response = AdServer::Grpc::ResponseHolder<BatchRequest>;
    using Result = std::function<void(const grpc::Status&, Response)>;

    Client(const std::string& endpoint, std::shared_ptr<AdServer::Grpc::GrpcExecutor> executor,
      std::shared_ptr<Runner> runner, std::shared_ptr<Lifetime> lifetime)
      : AsyncBatchingClientBase(endpoint, std::move(executor), std::move(runner), options_()),
        lifetime_(std::move(lifetime))
    {}

    ~Client() override
    {
      lifetime_->deleted.set_value(std::this_thread::get_id());
    }

    void submit(const char* method, Result callback)
    {
      BatchRequest request;
      request.set_batch_id(42);
      enqueue_request_<BatchRequest, BatchRequest>(
        method, request, std::move(callback), "invalid echo response");
    }

    void deactivate_object() override
    {
      ++lifetime_->stops;
      AsyncBatchingClientBase::deactivate_object();
    }

    void wait_object() override
    {
      ++lifetime_->waits;
      AsyncBatchingClientBase::wait_object();
    }

  private:
    static AdServer::Grpc::BatchingOptions options_()
    {
      AdServer::Grpc::BatchingOptions options;
      options.channels_number = 1;
      options.max_batch_size = 1;
      options.stream_shrink_period = Generics::Time::ZERO;
      options.reconnect_period = Generics::Time::ZERO;
      return options;
    }

    const std::shared_ptr<Lifetime> lifetime_;
  };

  using Deleter =
    AdServer::Commons::DeferredDeleter<Client, AdServer::Commons::ActiveObjectDeleter<Client>>;

  class Service final: public adserver::grpc::BatchTransport::Service
  {
  public:
    grpc::Status stream_batches(grpc::ServerContext* context,
      grpc::ServerReaderWriter<BatchResponse, BatchRequest>* stream) override
    {
      BatchRequest request;
      BatchRequest deferred;
      while (stream->Read(&request))
      {
        if (request.items(0).full_method() == "/test.Retire")
        {
          deferred = request;
          continue;
        }

        if (request.items(0).full_method() == "/test.Pending")
        {
          require(deferred.items_size() == 1, "missing first batch");
          pending_received.store(true);
          if (!reply_(stream, deferred))
          {
            break;
          }

          while (!context->IsCancelled())
          {
            std::this_thread::sleep_for(1ms);
          }

          return grpc::Status(grpc::StatusCode::CANCELLED, "client retired");
        }

        if (!reply_(stream, request))
        {
          break;
        }
      }

      return grpc::Status::OK;
    }

    std::atomic<bool> pending_received{false};

  private:
    static bool reply_(grpc::ServerReaderWriter<BatchResponse, BatchRequest>* stream,
      const BatchRequest& request)
    {
      BatchResponse response;
      response.set_batch_id(request.batch_id());
      for (const auto& item : request.items())
      {
        auto* result = response.add_items();
        result->set_request_id(item.request_id());
        result->set_payload(item.payload());
      }

      return stream->Write(response);
    }
  };

  struct Holder
  {
    std::shared_ptr<Client> client;
  };

  using Pool = AdServer::Grpc::RefPool<Holder>;

  void test_last_pool_ref_in_cq()
  {
    Service service;
    grpc::ServerBuilder builder;
    int port = 0;
    builder.AddListeningPort("127.0.0.1:0", grpc::InsecureServerCredentials(), &port);
    builder.RegisterService(&service);
    auto server = builder.BuildAndStart();
    require(server && port, "failed to start test server");

    Generics::ActiveObjectCallback_var callback(new Callback());
    auto runner = std::make_shared<Runner>(callback, std::make_shared<Runner::IoService>(), 1);
    auto executor = std::make_shared<AdServer::Grpc::GrpcExecutor>(1);
    runner->activate_object();
    const auto endpoint = "127.0.0.1:" + std::to_string(port);
    auto lifetime = std::make_shared<Lifetime>();
    auto deleted = lifetime->deleted.get_future();
    auto client = AdServer::Commons::make_shared_with_deleter<Client>(
      Deleter(executor->deletion_queue()), endpoint, executor, runner, lifetime);
    client->activate_object();
    auto holder = std::make_shared<Holder>(Holder{client});
    auto old_pool = std::make_shared<Pool>(std::vector<std::shared_ptr<Holder>>{holder}, runner);
    auto new_pool = std::make_shared<Pool>(std::vector<std::shared_ptr<Holder>>{holder}, runner);
    old_pool->activate_object();
    new_pool->activate_object();
    auto old_ref = old_pool->get_object();
    auto new_ref = new_pool->get_object();
    old_pool->deactivate_object();
    old_pool->wait_object();
    old_pool.reset();
    old_ref.reset();
    holder.reset();
    client.reset();
    executor->deletion_queue()->wait();
    require(lifetime->stops == 0, "old pool stopped a client still owned by the new pool");

    std::promise<std::thread::id> response_thread;
    auto responded = response_thread.get_future();
    std::promise<void> pending_completed;
    auto completed = pending_completed.get_future();
    auto* borrowed = (*new_ref)->client.get();
    borrowed->submit("/test.Retire",
      [ref = std::move(*new_ref), &response_thread, &service](const auto& status, auto response)
      {
        require(status.ok() && response->batch_id() == 42, "first response failed");
        require(service.pending_received, "test needs another outstanding request");
        response_thread.set_value(std::this_thread::get_id());
      });
    new_ref.reset();
    borrowed->submit("/test.Pending", [&pending_completed](const auto& status, auto)
      {
        require(!status.ok(), "outstanding request must complete with cancellation");
        pending_completed.set_value();
      });
    new_pool->deactivate_object();
    new_pool->wait_object();
    new_pool.reset();
    executor->activate_object();

    const auto cq_thread = await(responded);
    await(completed);
    require(await(deleted) != cq_thread, "client was destroyed on its completion queue");
    require(lifetime->stops == 1 && lifetime->waits == 1, "client shutdown must run once");

    auto stopped_lifetime = std::make_shared<Lifetime>();
    auto stopped_deleted = stopped_lifetime->deleted.get_future();
    auto stopped = AdServer::Commons::make_shared_with_deleter<Client>(
      Deleter(executor->deletion_queue()), endpoint, executor, runner, stopped_lifetime);
    stopped->activate_object();
    stopped->deactivate_object();
    stopped->wait_object();
    Deleter::complete(stopped);
    stopped.reset();
    await(stopped_deleted);
    require(stopped_lifetime->stops == 1 && stopped_lifetime->waits == 1,
      "deletion repeated an explicit shutdown");

    auto other_lifetime = std::make_shared<Lifetime>();
    auto other_deleted = other_lifetime->deleted.get_future();
    auto other = AdServer::Commons::make_shared_with_deleter<Client>(
      Deleter(executor->deletion_queue()), endpoint, executor, runner, other_lifetime);
    other->activate_object();
    std::promise<std::thread::id> other_response;
    auto other_responded = other_response.get_future();
    other->submit("/test.Echo", [&other_response](const auto& status, auto response)
      {
        require(status.ok() && response->batch_id() == 42, "other client failed");
        other_response.set_value(std::this_thread::get_id());
      });
    require(await(other_responded) == cq_thread, "other client must progress on the same CQ");

    // Shutdown must drain a queued deletion before closing its completion queue.
    other.reset();
    executor->deactivate_object();
    executor->wait_object();
    await(other_deleted);
    require(other_lifetime->stops == 1 && other_lifetime->waits == 1,
      "executor shutdown did not drain client deletion");
    runner->deactivate_object();
    runner->wait_object();
    server->Shutdown();
    server->Wait();
  }

  void test_wait_includes_cascaded_deletions()
  {
    auto queue = std::make_shared<Queue>();
    std::promise<void> entered;
    auto started = entered.get_future();
    std::promise<void> release;
    auto resume = release.get_future().share();
    std::atomic<unsigned int> deleted{0};
    queue->enqueue([queue, &entered, resume, &deleted]
      {
        entered.set_value();
        resume.wait();
        queue->enqueue([&deleted] { ++deleted; });
      });
    await(started);
    auto waiting = std::async(std::launch::async, [queue] { queue->wait(); });
    require(waiting.wait_for(20ms) == std::future_status::timeout,
      "wait ignored a running deletion");
    release.set_value();
    await(waiting);
    require(deleted == 1, "wait ignored a cascaded deletion");
  }

  void test_queue_owned_by_deleted_object()
  {
    struct Object
    {
      std::shared_ptr<Queue> queue;
      std::promise<void>& deleted;

      ~Object()
      {
        queue.reset();
        deleted.set_value();
      }
    };

    auto queue = std::make_shared<Queue>();
    std::promise<void> release;
    auto resume = release.get_future().share();
    queue->enqueue([resume] { resume.wait(); });
    std::promise<void> deleted;
    auto completed = deleted.get_future();
    auto object = AdServer::Commons::make_shared_with_deleter<Object>(
      AdServer::Commons::DeferredDeleter<Object>(queue), queue, deleted);
    object.reset();
    queue.reset();
    release.set_value();
    await(completed);
  }

  struct DeletionPause
  {
    std::promise<void> entered;
    std::promise<void> release;
    std::shared_future<void> resume = release.get_future().share();

    void wait()
    {
      entered.set_value();
      resume.wait();
    }
  };

  class ShutdownClient final: public Generics::SimpleActiveObject
  {
  public:
    ShutdownClient(std::shared_ptr<AdServer::Grpc::GrpcExecutor> executor,
      std::shared_ptr<Lifetime> lifetime)
      : executor_(std::move(executor)), lifetime_(std::move(lifetime))
    {}

    ~ShutdownClient() override
    {
      lifetime_->deleted.set_value(std::this_thread::get_id());
    }

  private:
    void deactivate_object_() override
    {
      require(executor_->execute([] {}), "CQ closed before delayed client shutdown");
      ++lifetime_->stops;
    }

    void wait_object_() override
    {
      ++lifetime_->waits;
    }

    const std::shared_ptr<AdServer::Grpc::GrpcExecutor> executor_;
    const std::shared_ptr<Lifetime> lifetime_;
  };

  using ShutdownDeleter = AdServer::Commons::DeferredDeleter<
    ShutdownClient, AdServer::Commons::ActiveObjectDeleter<ShutdownClient>>;

  struct PausedDeleter
  {
    ShutdownDeleter deleter;
    std::shared_ptr<DeletionPause> pause;

    auto bind() const
    {
      return [bound = deleter.bind(), pause = pause](ShutdownClient* client)
        {
          pause->wait();
          bound(client);
        };
    }
  };

  void test_shutdown_waits_for_deletion_transfer(bool pause_in_holder)
  {
    struct PausedHolder
    {
      std::shared_ptr<ShutdownClient> client;
      std::shared_ptr<DeletionPause> pause;

      ~PausedHolder()
      {
        if (pause)
        {
          pause->wait();
        }
      }
    };

    auto executor = std::make_shared<AdServer::Grpc::GrpcExecutor>(1);
    executor->activate_object();
    auto queue = executor->deletion_queue();
    auto lifetime = std::make_shared<Lifetime>();
    auto deleted = lifetime->deleted.get_future();
    auto pause = std::make_shared<DeletionPause>();
    auto entered = pause->entered.get_future();
    ShutdownDeleter deleter(queue);
    auto client = pause_in_holder ?
      AdServer::Commons::make_shared_with_deleter<ShutdownClient>(deleter, executor, lifetime) :
      AdServer::Commons::make_shared_with_deleter<ShutdownClient>(
        PausedDeleter{deleter, pause}, executor, lifetime);
    client->activate_object();
    std::weak_ptr<ShutdownClient> weak_client = client;
    auto holder = std::make_shared<PausedHolder>(client, pause_in_holder ? pause : nullptr);
    std::weak_ptr<PausedHolder> weak_holder = holder;
    client.reset();
    auto dropping = std::async(std::launch::async, [holder = std::move(holder)]() mutable
      {
        holder.reset();
      });
    await(entered);
    require(weak_holder.expired(), "shutdown must be unable to lock the old holder");
    require(weak_client.expired() != pause_in_holder, "wrong deletion transfer window");

    auto stopping = std::async(std::launch::async, [executor]
      {
        executor->deactivate_object();
        executor->wait_object();
      });
    bool registrations_closed = false;
    const auto deadline = std::chrono::steady_clock::now() + 5s;
    while (std::chrono::steady_clock::now() < deadline)
    {
      try
      {
        queue->register_deletion()->complete();
      }
      catch (const std::logic_error&)
      {
        registrations_closed = true;
        break;
      }

      require(stopping.wait_for(0ms) != std::future_status::ready,
        "executor shutdown missed a deletion not yet enqueued");
      std::this_thread::sleep_for(1ms);
    }

    require(registrations_closed, "shutdown did not close deletion registration");
    require(stopping.wait_for(20ms) == std::future_status::timeout,
      "shutdown did not wait for deletion transfer");
    require(executor->execute([] {}), "CQ shut down while deletion transfer was paused");
    pause->release.set_value();
    await(dropping);
    await(stopping);
    await(deleted);
    require(lifetime->stops == 1 && lifetime->waits == 1, "delayed shutdown must run once");
    require(!executor->execute([] {}), "executor did not shut down after deletion completed");
  }

  void test_shutdown_with_retained_stopped_client()
  {
    auto executor = std::make_shared<AdServer::Grpc::GrpcExecutor>(1);
    executor->activate_object();
    auto lifetime = std::make_shared<Lifetime>();
    auto deleted = lifetime->deleted.get_future();
    auto client = AdServer::Commons::make_shared_with_deleter<ShutdownClient>(
      ShutdownDeleter(executor->deletion_queue()), executor, lifetime);
    client->activate_object();
    client->deactivate_object();
    client->wait_object();
    ShutdownDeleter::complete(client);
    auto stopping = std::async(std::launch::async, [executor]
      {
        executor->deactivate_object();
        executor->wait_object();
      });
    await(stopping);
    require(deleted.wait_for(0ms) == std::future_status::timeout,
      "shutdown destroyed a retained client");
    client.reset();
    await(deleted);
    executor->deletion_queue()->wait();
    require(lifetime->stops == 1 && lifetime->waits == 1, "late deletion repeated shutdown");
  }

  void test_registration_and_construction_failure()
  {
    struct Object
    {
      Object(unsigned int& constructed, bool fail)
      {
        ++constructed;
        if (fail)
        {
          throw std::runtime_error("constructor failed");
        }
      }
    };

    auto queue = std::make_shared<Queue>();
    AdServer::Commons::DeferredDeleter<Object> deleter(queue);
    unsigned int constructed = 0;
    try
    {
      AdServer::Commons::make_shared_with_deleter<Object>(deleter, constructed, true);
      require(false, "constructor did not fail");
    }
    catch (const std::runtime_error&)
    {}

    auto draining = std::async(std::launch::async, [queue] { queue->drain(); });
    await(draining);
    try
    {
      AdServer::Commons::make_shared_with_deleter<Object>(deleter, constructed, false);
      require(false, "drained queue accepted a new object");
    }
    catch (const std::logic_error&)
    {}

    require(constructed == 1, "object was constructed before registering its deletion");
  }

  struct PolicyCounters
  {
    std::atomic<unsigned int> deleted{0};
    std::atomic<unsigned int> waited{0};
  };

  class PolicyClient final:
    public Generics::SimpleActiveObject,
    public virtual AdServer::Grpc::Client
  {
  public:
    PolicyClient(const std::string&, std::shared_ptr<AdServer::Grpc::GrpcExecutor>,
      std::shared_ptr<Runner>, AdServer::Grpc::BatchingOptions)
    {}

    ~PolicyClient() override
    {
      ++counters.deleted;
    }

    static PolicyCounters counters;

  private:
    void wait_object_() override
    {
      ++counters.waited;
    }
  };

  PolicyCounters PolicyClient::counters;

  void test_partition_pool_policy()
  {
    using Policy = AdServer::Commons::DeferredDeleter<
      PolicyClient, AdServer::Commons::ActiveObjectDeleter<PolicyClient>>;
    using PartitionPool = AdServer::Grpc::DistributedPartitionPool<
      PolicyClient, AdServer::Grpc::BasicControllerRefHolder, Policy>;
    auto queue = std::make_shared<Queue>();
    std::atomic<unsigned int> version{0};
    auto pool = std::make_shared<PartitionPool>(
      "DeletionTest", "Test", PartitionPool::ControllerRefArray{{"controller-0"}, {"controller-1"}},
      AdServer::Grpc::BatchingOptions{}, nullptr, nullptr, nullptr,
      [&version](auto&) -> std::optional<PartitionPool::EndpointChunksArray>
      {
        return PartitionPool::EndpointChunksArray{{"endpoint-" + std::to_string(version), {0}}};
      },
      [](const std::string&, unsigned long) { return 0; },
      [](const std::string&, unsigned long) { return 0; },
      Generics::Time::ZERO, Generics::Time(0, 20000), Policy(queue));
    pool->activate_object();
    auto old_ref = pool->get_ref("key");
    require(old_ref && (*old_ref)->endpoint == "endpoint-0", "initial partition missing");

    auto replace = [&](unsigned int next_version)
      {
        version = next_version;
        const auto deadline = std::chrono::steady_clock::now() + 5s;
        while (std::chrono::steady_clock::now() < deadline)
        {
          pool->try_to_reresolve_partition(0);
          pool->try_to_reresolve_partition(1);
          auto ref = pool->get_ref("key");
          if (ref && (*ref)->endpoint == "endpoint-" + std::to_string(next_version))
          {
            return;
          }

          std::this_thread::sleep_for(1ms);
        }

        require(false, "partition was not replaced");
      };

    replace(1);
    require(PolicyClient::counters.deleted == 0, "replacement invalidated a borrowed ref");
    old_ref.reset();
    const auto deadline = std::chrono::steady_clock::now() + 5s;
    while (PolicyClient::counters.deleted == 0 && std::chrono::steady_clock::now() < deadline)
    {
      std::this_thread::sleep_for(1ms);
    }

    queue->wait();
    require(PolicyClient::counters.deleted == 1 && PolicyClient::counters.waited == 1,
      "partition pool ignored its deletion policy");

    old_ref = pool->get_ref("key");
    replace(2);
    pool->deactivate_object();
    pool->wait_object();
    require(!(*old_ref)->client->active(), "shutdown missed a client from an old partition");
    auto draining = std::async(std::launch::async, [queue] { queue->drain(); });
    await(draining);
    old_ref.reset();
    pool.reset();
    queue->wait();
    require(PolicyClient::counters.deleted == 3 && PolicyClient::counters.waited == 3,
      "pool shutdown lost a client or repeated its shutdown");
  }
}

int main()
{
  test_shutdown_waits_for_deletion_transfer(true);
  test_shutdown_waits_for_deletion_transfer(false);
  test_shutdown_with_retained_stopped_client();
  test_registration_and_construction_failure();
  test_wait_includes_cascaded_deletions();
  test_queue_owned_by_deleted_object();
  test_partition_pool_policy();
  test_last_pool_ref_in_cq();
  std::cout << "DeferredClientDeletionTest passed" << std::endl;
}
