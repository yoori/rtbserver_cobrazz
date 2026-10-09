#include <chrono>
#include <cstdlib>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <utility>

#include <Commons/Grpc/AsyncBatchingClientBase.hpp>

namespace AdServer::Grpc
{
  struct AsyncBatchingClientBaseTestAccess
  {
    using Holder = AsyncBatchingClientBase::StreamHolder;

    static auto make_holder(AsyncBatchingClientBase& client)
    {
      return client.make_stream_();
    }

    static auto
    make_callback(AsyncBatchingClientBase& client, const std::shared_ptr<Holder>& holder)
    {
      return client.make_drain_callback_(holder);
    }

    static auto gate(AsyncBatchingClientBase& client)
    {
      return client.drain_callbacks_gate_;
    }

    static void add_draining(AsyncBatchingClientBase& client, std::shared_ptr<Holder> holder)
    {
      std::lock_guard<std::mutex> lock(client.streams_registry_lock_);
      client.draining_streams_.emplace_back(std::move(holder));
    }
  };
}

namespace
{
  using namespace std::chrono_literals;
  using Access = AdServer::Grpc::AsyncBatchingClientBaseTestAccess;
  using Runner = AdServer::Commons::BoostAsioContextRunActiveObject;

  void require(bool condition, const char* message)
  {
    if (!condition)
    {
      std::cerr << message << std::endl;
      std::abort();
    }
  }

  void await(std::future<void>& future)
  {
    require(future.wait_for(5s) == std::future_status::ready, "completion timed out");
    future.get();
  }

  struct Pause
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

  class Callback final:
    public virtual Generics::ActiveObjectCallback,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    void report_error(Severity, const String::SubString&, const char* = nullptr) noexcept override
    {}
  };

  class Client final: public AdServer::Grpc::AsyncBatchingClientBase
  {
  public:
    Client(std::shared_ptr<AdServer::Grpc::GrpcExecutor> executor, std::shared_ptr<Runner> runner)
      : AsyncBatchingClientBase("127.0.0.1:1", std::move(executor), std::move(runner), options_())
    {}

    using AsyncBatchingClientBase::stats;

  private:
    static AdServer::Grpc::BatchingOptions options_()
    {
      AdServer::Grpc::BatchingOptions options;
      options.stream_shrink_period = Generics::Time::ZERO;
      return options;
    }
  };

  struct Fixture
  {
    Fixture()
      : callback(new Callback()),
        runner(std::make_shared<Runner>(callback, std::make_shared<Runner::IoService>(), 1)),
        executor(std::make_shared<AdServer::Grpc::GrpcExecutor>(1)),
        client(std::make_unique<Client>(executor, runner))
    {
      client->activate_object();
    }

    void stop()
    {
      client->deactivate_object();
      client->wait_object();
    }

    Generics::ActiveObjectCallback_var callback;
    std::shared_ptr<Runner> runner;
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> executor;
    std::unique_ptr<Client> client;
  };

  void test_callback_after_client_destruction()
  {
    Fixture fixture;
    auto holder = Access::make_holder(*fixture.client);
    auto callback = Access::make_callback(*fixture.client, holder);
    auto gate = Access::gate(*fixture.client);
    Pause pause;
    auto entered = pause.entered.get_future();
    auto completing = std::async(std::launch::async,
      [callback = std::move(callback), stream = holder->stream.get(), &pause]
      {
        pause.wait();
        callback(stream);
      });
    await(entered);
    fixture.stop();
    fixture.client.reset();
    require(!gate->enter(), "shutdown left the callback gate open");

    // Keep the weak holder lockable after client deletion, as an already running callback can.
    pause.release.set_value();
    await(completing);
    require(!gate->has_activities(), "late callback left a gate activity behind");
  }

  void test_wait_includes_holder_destruction()
  {
    Fixture fixture;
    auto holder = Access::make_holder(*fixture.client);
    auto* stream = holder->stream.get();
    auto* borrowed_holder = holder.get();
    Pause pause;
    auto entered = pause.entered.get_future();
    auto paused_holder = std::shared_ptr<Access::Holder>(borrowed_holder,
      [holder = std::move(holder), &pause](auto*) mutable
      {
        pause.wait();
        holder.reset();
      });
    auto callback = Access::make_callback(*fixture.client, paused_holder);
    auto gate = Access::gate(*fixture.client);
    Access::add_draining(*fixture.client, std::move(paused_holder));
    auto completing = std::async(std::launch::async, [callback = std::move(callback), stream]
      {
        callback(stream);
      });
    await(entered);
    require(gate->has_activities(), "callback released its gate before destroying its holder");
    require(fixture.client->stats().draining_streams == 0, "callback did not remove the stream");
    require(fixture.client->stats().deferred_streams == 1, "callback did not finish stream wait");

    fixture.client->deactivate_object();
    std::promise<void> wait_entered;
    auto waiting = wait_entered.get_future();
    auto stopping = std::async(std::launch::async, [&fixture, &wait_entered]
      {
        wait_entered.set_value();
        fixture.client->wait_object();
      });
    await(waiting);
    require(stopping.wait_for(20ms) == std::future_status::timeout,
      "client wait ignored the drain callback's holder destruction");
    require(fixture.client->stats().deferred_streams == 1,
      "client cleared its streams before the drain callback returned");
    pause.release.set_value();
    await(completing);
    await(stopping);
    require(!gate->has_activities(), "completed callback retained its gate activity");
    require(fixture.client->stats().deferred_streams == 0, "shutdown retained a deferred stream");
  }

  void test_reactivation_does_not_reopen_old_callbacks()
  {
    Fixture fixture;
    auto old_holder = Access::make_holder(*fixture.client);
    auto old_callback = Access::make_callback(*fixture.client, old_holder);
    auto old_gate = Access::gate(*fixture.client);
    fixture.stop();
    fixture.client->activate_object();
    require(Access::gate(*fixture.client) != old_gate, "reactivation reused the old callback gate");
    require(!old_gate->enter(), "reactivation reopened an old callback");
    old_callback(old_holder->stream.get());

    auto holder = Access::make_holder(*fixture.client);
    auto callback = Access::make_callback(*fixture.client, holder);
    auto* stream = holder->stream.get();
    Access::add_draining(*fixture.client, std::move(holder));
    callback(stream);
    require(fixture.client->stats().draining_streams == 0,
      "reactivated client did not accept its new drain callback");
    require(fixture.client->stats().deferred_streams == 1,
      "reactivated client's drain callback did not finish stream wait");
    fixture.stop();
    fixture.client.reset();
    old_callback(old_holder->stream.get());
  }
}

int main()
{
  test_callback_after_client_destruction();
  test_wait_includes_holder_destruction();
  test_reactivation_does_not_reopen_old_callbacks();
  std::cout << "DrainCallbackLifetimeTest passed" << std::endl;
}
