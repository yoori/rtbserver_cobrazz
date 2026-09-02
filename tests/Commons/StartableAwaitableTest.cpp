#include <coroutine>
#include <functional>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <Commons/AsyncCache.hpp>
#include <Commons/Coro/CallbackAwaiter.hpp>
#include <Commons/Coro/SetAwaitable.hpp>
#include <Commons/Coro/StartableAwaitable.hpp>
#include <Commons/Coro/TupleAwaitable.hpp>
#include <Commons/Coro/Utils.hpp>

namespace
{
  constexpr std::string_view ERROR_MESSAGE = "test exception";

  class RethrowingCoroutine final
  {
  public:
    struct promise_type;
    using Handle = std::coroutine_handle<promise_type>;

    explicit RethrowingCoroutine(Handle handle) noexcept
      : handle_(handle)
    {}

    RethrowingCoroutine(RethrowingCoroutine&& other) noexcept
      : handle_(std::exchange(other.handle_, {}))
    {}

    ~RethrowingCoroutine()
    {
      if (handle_)
      {
        handle_.destroy();
      }
    }

    Handle handle() const noexcept
    {
      return handle_;
    }

    struct promise_type
    {
      RethrowingCoroutine get_return_object() noexcept
      {
        return RethrowingCoroutine(Handle::from_promise(*this));
      }

      std::suspend_always initial_suspend() const noexcept
      {
        return {};
      }

      std::suspend_always final_suspend() const noexcept
      {
        return {};
      }

      void return_void() const noexcept
      {}

      void unhandled_exception()
      {
        throw;
      }
    };

  private:
    Handle handle_;
  };

  struct ThrowOnAssignment final
  {
    ThrowOnAssignment() = default;
    ThrowOnAssignment(const ThrowOnAssignment&) = default;
    ThrowOnAssignment(ThrowOnAssignment&&) noexcept = default;

    ThrowOnAssignment& operator=(const ThrowOnAssignment&)
    {
      throw std::runtime_error(std::string(ERROR_MESSAGE));
    }

    ThrowOnAssignment& operator=(ThrowOnAssignment&&)
    {
      throw std::runtime_error(std::string(ERROR_MESSAGE));
    }
  };

  AdServer::Commons::StartableAwaitable<int>
  make_result(bool fail)
  {
    if (fail)
    {
      throw std::runtime_error(std::string(ERROR_MESSAGE));
    }

    co_return 42;
  }

  AdServer::Commons::StartableAwaitable<void>
  make_void_result(bool fail)
  {
    if (fail)
    {
      throw std::runtime_error(std::string(ERROR_MESSAGE));
    }

    co_return;
  }

  AdServer::Commons::StartableAwaitable<void>
  make_batch(bool fail)
  {
    std::vector<AdServer::Commons::StartableAwaitable<void>> operations;
    operations.emplace_back(make_void_result(false));
    operations.emplace_back(make_void_result(fail));
    co_await AdServer::Commons::SetAwaitable(std::move(operations));
  }

  AdServer::Commons::StartableAwaitable<void>
  make_tuple(bool fail)
  {
    const auto [first, second] = co_await AdServer::Commons::TupleAwaitable(
      make_result(false),
      make_result(fail));
    if (first != 42 || second != 42)
    {
      throw std::runtime_error("unexpected tuple result");
    }
  }

  AdServer::Commons::StartableAwaitable<void>
  await_throwing_callback(
    AdServer::Commons::CallbackAwaiter<ThrowOnAssignment>::Callback& callback)
  {
    co_await AdServer::Commons::CallbackAwaiter<ThrowOnAssignment>(
      [&callback](AdServer::Commons::CallbackAwaiter<ThrowOnAssignment>::Callback value)
      {
        callback = std::move(value);
      });
  }

  using ThrowingCache = AdServer::Commons::AsyncCache<int, ThrowOnAssignment>;

  AdServer::Commons::StartableAwaitable<void>
  await_throwing_cache(const std::shared_ptr<ThrowingCache>& cache)
  {
    co_await cache->co_get(1);
  }

  RethrowingCoroutine
  set_flag(bool& flag)
  {
    flag = true;
    co_return;
  }

  RethrowingCoroutine
  enqueue_and_fail(std::coroutine_handle<> pending)
  {
    AdServer::Commons::resume_coroutine(pending);
    throw std::runtime_error(std::string(ERROR_MESSAGE));
    co_return;
  }

  template<typename Function>
  bool
  check_exception(Function&& function, std::string_view test_name)
  {
    try
    {
      function();
    }
    catch (const std::runtime_error& ex)
    {
      if (ex.what() == ERROR_MESSAGE)
      {
        return true;
      }

      std::cerr << test_name << ": unexpected error: " << ex.what() << '\n';
      return false;
    }
    catch (...)
    {
      std::cerr << test_name << ": unexpected exception type\n";
      return false;
    }

    std::cerr << test_name << ": exception was not propagated\n";
    return false;
  }

  bool
  check_resume_coroutine_exception_safety()
  {
    bool queued_resumed = false;
    auto queued = set_flag(queued_resumed);
    auto failing = enqueue_and_fail(queued.handle());
    if (!check_exception(
      [&failing]()
      {
        AdServer::Commons::resume_coroutine(failing.handle());
      },
      "resume_coroutine"))
    {
      return false;
    }

    bool next_resumed = false;
    auto next = set_flag(next_resumed);
    AdServer::Commons::resume_coroutine(next.handle());
    if (!queued_resumed || !next_resumed)
    {
      std::cerr << "resume_coroutine abandoned pending handles or remained draining\n";
      return false;
    }

    return true;
  }

  bool
  check_callback_result_exception()
  {
    using Callback = AdServer::Commons::CallbackAwaiter<ThrowOnAssignment>::Callback;

    Callback callback;
    std::optional<std::exception_ptr> completion_exception;
    bool completed = false;
    auto operation = await_throwing_callback(callback);
    operation.start(
      [&completed, &completion_exception](std::optional<std::exception_ptr> exception) noexcept
      {
        completed = true;
        completion_exception = std::move(exception);
      });

    if (!callback || completed)
    {
      std::cerr << "CallbackAwaiter didn't suspend\n";
      return false;
    }

    try
    {
      callback(ThrowOnAssignment());
    }
    catch (...)
    {
      std::cerr << "CallbackAwaiter leaked a result materialization exception\n";
      return false;
    }

    if (!completed || !completion_exception)
    {
      std::cerr << "CallbackAwaiter lost a result materialization exception\n";
      return false;
    }

    if (!check_exception(
      [&completion_exception]()
      {
        std::rethrow_exception(*completion_exception);
      },
      "CallbackAwaiter result materialization completion"))
    {
      return false;
    }

    return check_exception(
      [&operation]()
      {
        operation.await_resume();
      },
      "CallbackAwaiter result materialization await_resume");
  }

  bool
  check_cache_result_exception()
  {
    ThrowingCache::UpdateCallback update;
    auto cache = std::make_shared<ThrowingCache>(
      ThrowingCache::UNLIMITED_SIZE,
      Generics::Time::ONE_SECOND,
      Generics::Time::ONE_SECOND,
      [](const int&, const ThrowingCache::HolderPtr&) -> ThrowingCache::HolderPtr
      {
        return {};
      },
      [&update](int, ThrowingCache::HolderPtr, ThrowingCache::UpdateCallback callback)
      {
        update = std::move(callback);
      });

    std::optional<std::exception_ptr> completion_exception;
    bool completed = false;
    auto operation = await_throwing_cache(cache);
    operation.start(
      [&completed, &completion_exception](std::optional<std::exception_ptr> exception) noexcept
      {
        completed = true;
        completion_exception = std::move(exception);
      });

    if (!update || completed)
    {
      std::cerr << "AsyncCacheAwaiter didn't suspend\n";
      return false;
    }

    update(std::make_shared<ThrowingCache::Holder>(
      ThrowOnAssignment(),
      Generics::Time::ZERO,
      Generics::Time::ZERO,
      0));
    if (!completed || !completion_exception)
    {
      std::cerr << "AsyncCacheAwaiter lost a result materialization exception\n";
      return false;
    }

    if (!check_exception(
      [&completion_exception]()
      {
        std::rethrow_exception(*completion_exception);
      },
      "AsyncCacheAwaiter result materialization completion"))
    {
      return false;
    }

    return check_exception(
      [&operation]()
      {
        operation.await_resume();
      },
      "AsyncCacheAwaiter result materialization await_resume");
  }
}

int
main()
{
  if (AdServer::Commons::sync_wait(make_result(false)) != 42)
  {
    std::cerr << "result sync_wait returned an unexpected value\n";
    return 1;
  }

  AdServer::Commons::sync_wait(make_void_result(false));

  if (!check_exception(
      []()
      {
        AdServer::Commons::sync_wait(make_result(true));
      },
      "result sync_wait"))
  {
    return 1;
  }

  if (!check_exception(
      []()
      {
        AdServer::Commons::sync_wait(make_void_result(true));
      },
      "void sync_wait"))
  {
    return 1;
  }

  AdServer::Commons::sync_wait(make_batch(false));
  if (!check_exception(
      []()
      {
        AdServer::Commons::sync_wait(make_batch(true));
      },
      "batch sync_wait"))
  {
    return 1;
  }

  AdServer::Commons::sync_wait(make_tuple(false));
  if (!check_exception(
      []()
      {
        AdServer::Commons::sync_wait(make_tuple(true));
      },
      "tuple sync_wait"))
  {
    return 1;
  }

  if (!check_callback_result_exception() ||
    !check_cache_result_exception() ||
    !check_resume_coroutine_exception_safety())
  {
    return 1;
  }

  std::cout << "StartableAwaitableTest: PASS\n";
  return 0;
}
