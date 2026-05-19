#pragma once

#include <coroutine>
#include <exception>
#include <utility>

namespace FrontendCommons
{
  template<typename Result>
  class ValueTask
  {
  public:
    struct promise_type;
    using Handle = std::coroutine_handle<promise_type>;

    explicit ValueTask(Handle handle) noexcept
      : handle_(handle)
    {}

    ValueTask(const ValueTask&) = delete;
    ValueTask& operator=(const ValueTask&) = delete;

    ValueTask(ValueTask&& rhs) noexcept
      : handle_(rhs.handle_)
    {
      rhs.handle_ = nullptr;
    }

    ValueTask&
    operator=(ValueTask&& rhs) noexcept
    {
      if(this != &rhs)
      {
        if(handle_)
        {
          handle_.destroy();
        }
        handle_ = rhs.handle_;
        rhs.handle_ = nullptr;
      }
      return *this;
    }

    ~ValueTask() noexcept
    {
      if(handle_)
      {
        handle_.destroy();
      }
    }

    bool
    await_ready() const noexcept
    {
      return false;
    }

    std::coroutine_handle<>
    await_suspend(std::coroutine_handle<> continuation) noexcept
    {
      handle_.promise().continuation = continuation;
      return handle_;
    }

    Result
    await_resume()
    {
      if(handle_.promise().exception)
      {
        std::rethrow_exception(handle_.promise().exception);
      }
      return std::move(handle_.promise().result);
    }

    struct promise_type
    {
      struct FinalAwaiter
      {
        bool
        await_ready() noexcept
        {
          return false;
        }

        void
        await_resume() noexcept
        {}

        std::coroutine_handle<>
        await_suspend(Handle handle) noexcept
        {
          auto continuation = handle.promise().continuation;
          return continuation ? continuation : std::noop_coroutine();
        }
      };

      ValueTask
      get_return_object() noexcept
      {
        return ValueTask{Handle::from_promise(*this)};
      }

      std::suspend_always
      initial_suspend() noexcept
      {
        return {};
      }

      FinalAwaiter
      final_suspend() noexcept
      {
        return {};
      }

      void
      return_value(Result result_value) noexcept
      {
        result = std::move(result_value);
      }

      void
      unhandled_exception() noexcept
      {
        exception = std::current_exception();
      }

      std::exception_ptr exception;
      Result result;
      std::coroutine_handle<> continuation;
    };

  private:
    Handle handle_;
  };
}
