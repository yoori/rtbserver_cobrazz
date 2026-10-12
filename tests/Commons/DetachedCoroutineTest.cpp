#include <array>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <future>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <Commons/Coro/CallbackAwaiter.hpp>
#include <Commons/Coro/CooperativeGroup.hpp>
#include <Generics/HashTableAdapters.hpp>
#include <ProfilingCommons/ProfileMap/TransactionProfileMap.hpp>

namespace
{
  using namespace AdServer::Commons;
  using namespace std::chrono_literals;
  using Key = Generics::StringHashAdapter;
  using AsyncMap = AdServer::ProfilingCommons::AsyncProfileMap<Key>;
  using Map = AdServer::ProfilingCommons::TransactionProfileMap<Key>;
  using Task = StartableAwaitable<void>;
  using Pool = std::shared_ptr<ExecutorPool>;
  using Callback = std::function<void()>;

  void require(bool condition, const char* message)
  {
    if (!condition)
    {
      std::cerr << "DetachedCoroutineTest: " << message << std::endl;
      std::exit(1);
    }
  }

  template<typename T>
  T ready(std::future<T>& future)
  {
    require(future.wait_for(3s) == std::future_status::ready, "continuation was lost");
    return future.get();
  }

  class ErrorCallback final : public Generics::ActiveObjectCallback,
    public ReferenceCounting::AtomicImpl
  {
  public:
    void report_error(Severity, const String::SubString&, const char*) noexcept override;
  };

  void ErrorCallback::report_error(Severity, const String::SubString&, const char*) noexcept
  {
    require(false, "executor error");
  }

  class Storage final : public AsyncMap, public ReferenceCounting::AtomicImpl
  {
  public:
    void check_profile_async(const Key&, CheckCallback callback) const override;
    Generics::ConstSmartMemBuf_var get_profile_async(
      const Key&, GetCallback callback, std::optional<Generics::Time>) override;
    void save_profile_async(
      const Key&, const Generics::ConstSmartMemBuf*, const Generics::Time&, SaveCallback callback)
      override;
    void remove_profile_async(
      const Key&, AdServer::ProfilingCommons::OperationPriority, RemoveCallback callback) override;
    void clear_expired_async(const Generics::Time&, CompleteCallback callback) override;
    unsigned long size() const noexcept override;

    std::promise<Callback> pending;
    std::optional<std::string> error;
  };

  void Storage::check_profile_async(const Key&, CheckCallback callback) const
  {
    callback(false, {});
  }

  Generics::ConstSmartMemBuf_var Storage::get_profile_async(
    const Key&, GetCallback callback, std::optional<Generics::Time>)
  {
    callback({}, {});
    return {};
  }

  void Storage::save_profile_async(
    const Key&, const Generics::ConstSmartMemBuf*, const Generics::Time&, SaveCallback callback)
  {
    pending.set_value([callback = std::move(callback), error = error] { callback(error); });
  }

  void Storage::remove_profile_async(
    const Key&, AdServer::ProfilingCommons::OperationPriority, RemoveCallback callback)
  {
    pending.set_value([callback = std::move(callback), error = error]
    {
      callback(!error, error);
    });
  }

  void Storage::clear_expired_async(const Generics::Time&, CompleteCallback callback)
  {
    if (callback)
    {
      callback();
    }
  }

  unsigned long Storage::size() const noexcept
  {
    return 0;
  }

  struct Request
  {
    Task run();
    void complete(std::optional<std::string> error);

    Map& map;
    Pool pool;
    bool remove;
    bool fail;
    bool suspend;
    bool same_context;
    std::optional<ExecutorPool::ContextIndex> context;
    std::promise<Callback> pending;
    std::promise<void> write_completed;
    unsigned completions = 0;
  };

  Task Request::run()
  {
    context = pool ? pool->current_context_index() : std::nullopt;
    const auto* scheduler = current_coroutine_resume_scheduler();
    require(scheduler != nullptr, "request did not enter the group");
    if (remove)
    {
      map.remove_profile_async(Key("user"), AdServer::ProfilingCommons::OP_RUNTIME,
        [this](bool result, auto error)
        {
          require(result == !fail, "remove result changed");
          complete(std::move(error));
        });
    }
    else
    {
      Generics::ConstSmartMemBuf_var profile = new Generics::ConstSmartMemBuf("value", 5);
      map.save_profile_async(Key("user"), profile, Generics::Time::ZERO,
        [this](auto error) { complete(std::move(error)); });
    }

    require(current_coroutine_resume_scheduler() == scheduler, "request scheduler changed");
    require(current_coroutine_symmetric_transfer_ref(), "request transfer mode was not restored");
    if (suspend)
    {
      co_await async_callback<>([this](Callback callback)
      {
        pending.set_value(std::move(callback));
      });
    }
  }

  void Request::complete(std::optional<std::string> error)
  {
    require(bool(error) == fail, "storage error was not propagated");
    require(++completions == 1, "write completed twice");
    if (pool)
    {
      require(pool->running_in_this_thread(), "write resumed outside its executor pool");
      require(!same_context || context == pool->current_context_index(), "context changed");
    }

    write_completed.set_value();
  }

  Task acquire(Map& map)
  {
    auto transaction = co_await map.co_get_transaction(Key("user"), false);
    require(bool(transaction), "transaction was not acquired");
  }

  Task invoke_callback(Callback callback)
  {
    const auto* scheduler = current_coroutine_resume_scheduler();
    callback();
    require(current_coroutine_resume_scheduler() == scheduler, "callback replaced group scheduler");
    co_return;
  }

  struct LifetimeGuard
  {
    ~LifetimeGuard();
    unsigned& destroyed;
  };

  LifetimeGuard::~LifetimeGuard()
  {
    ++destroyed;
  }

  StartableAwaitable<int> detached_result(
    Callback& callback, unsigned& destroyed, bool asynchronous, bool fail)
  {
    LifetimeGuard guard{destroyed};
    if (asynchronous)
    {
      co_await async_callback<>([&](Callback resume) { callback = std::move(resume); });
    }

    if (fail)
    {
      throw std::runtime_error("detached failure");
    }

    co_return 42;
  }

  Task launch_result(Callback& callback, unsigned& destroyed, unsigned& completed,
    bool asynchronous, bool fail)
  {
    detached_result(callback, destroyed, asynchronous, fail).start_detached(
      [&completed, fail](auto error)
      {
        require(bool(error) == fail, "detached result lost its exception");
        ++completed;
      });
    co_return;
  }

  void test_detached_result(bool asynchronous, bool fail)
  {
    Callback callback;
    unsigned destroyed = 0;
    unsigned completed = 0;
    {
      std::array<Task, 1> operations{
        launch_result(callback, destroyed, completed, asynchronous, fail)};
      CooperativeGroup<> group(operations, {});
      bool request_completed = false;
      group.start([&](auto error)
      {
        require(!error, "detached exception escaped into its request");
        request_completed = true;
      });
      require(request_completed, "request waited for a detached result");
    }

    if (asynchronous)
    {
      require(completed == 0 && destroyed == 0 && callback, "detached result did not suspend");
      callback();
    }

    require(completed == 1 && destroyed == 1, "detached result did not complete exactly once");
  }

  void test_profile(const Pool& pool, bool same_context, bool remove, bool fail,
    bool blocked, bool suspend_request, bool nested_callback, bool destroy_group = true)
  {
    ReferenceCounting::SmartPtr<Storage> storage = new Storage();
    if (fail)
    {
      storage->error = "injected write error";
    }

    ReferenceCounting::SmartPtr<Map> map = new Map(storage);
    auto blocker = blocked ? sync_wait(map->co_get_transaction(Key("user"), false)) :
      Map::Transaction_var();
    Request request{*map, pool, remove, fail, suspend_request, same_context, {}, {}, {}, 0};
    auto write_started = storage->pending.get_future();
    auto request_suspended = request.pending.get_future();
    auto write_completed = request.write_completed.get_future();
    std::promise<void> request_completion;
    auto request_completed = request_completion.get_future();
    std::array<Task, 1> operations{request.run()};
    auto group = std::make_unique<CooperativeGroup<>>(operations, pool);
    group->start([&](auto error)
    {
      require(!error, "request failed");
      request_completion.set_value();
    }, bool(pool));

    if (suspend_request)
    {
      // Keep the group alive until the detached operation has reached its first real wait.
      auto resume_request = ready(request_suspended);
      if (!blocked)
      {
        require(write_started.wait_for(3s) == std::future_status::ready, "write did not start");
      }

      resume_request();
    }

    ready(request_completed);
    if (!blocked)
    {
      require(write_started.wait_for(3s) == std::future_status::ready, "write did not start");
    }

    if (destroy_group)
    {
      group.reset();
      operations[0] = Task(Task::Handle{});
    }

    blocker.reset();
    auto complete_write = ready(write_started);

    auto waiter = acquire(*map);
    std::promise<void> acquired;
    auto acquired_future = acquired.get_future();
    waiter.start([&](auto error)
    {
      require(!error, "next transaction failed");
      acquired.set_value();
    });
    require(acquired_future.wait_for(0s) == std::future_status::timeout,
      "transaction lock was released before the write callback");

    if (nested_callback)
    {
      std::array<Task, 1> callbacks{invoke_callback(std::move(complete_write))};
      CooperativeGroup<> callback_group(callbacks, pool);
      std::promise<void> callback_completion;
      auto callback_completed = callback_completion.get_future();
      auto start = [&]
      {
        callback_group.start([&](auto error)
        {
          require(!error, "callback group failed");
          callback_completion.set_value();
        });
      };
      if (pool)
      {
        pool->post(start, request.context);
      }
      else
      {
        start();
      }

      ready(callback_completed);
    }
    else
    {
      complete_write();
    }

    ready(write_completed);
    ready(acquired_future);
  }
}

int main()
{
  test_profile({}, false, false, false, false, true, false, false);
  for (const bool asynchronous : {false, true})
  {
    for (const bool fail : {false, true})
    {
      test_detached_result(asynchronous, fail);
    }
  }

  Generics::ActiveObjectCallback_var callback = new ErrorCallback();
  for (const unsigned threads : {0, 1, 4})
  {
    for (const auto strategy : {ExecutorPool::ResumeStrategy::CurrentContext,
      ExecutorPool::ResumeStrategy::AnyContext})
    {
      Pool pool;
      if (threads)
      {
        pool = std::make_shared<ExecutorPool>(callback, threads, strategy, "detached-test", 1);
        pool->activate_object();
      }

      for (const bool remove : {false, true})
      {
        for (const bool fail : {false, true})
        {
          for (const bool blocked : {false, true})
          {
            for (const bool suspend : {false, true})
            {
              for (const bool nested : {false, true})
              {
                test_profile(pool, strategy == ExecutorPool::ResumeStrategy::CurrentContext,
                  remove, fail, blocked, suspend, nested);
              }
            }
          }
        }
      }

      if (pool)
      {
        pool->deactivate_object();
        pool->wait_object();
      }
    }
  }

  std::cout << "DetachedCoroutineTest: PASS (193 profile cases)\n";
}
