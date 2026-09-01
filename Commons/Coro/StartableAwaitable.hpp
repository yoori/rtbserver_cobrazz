#pragma once

#include <coroutine>
#include <exception>
#include <functional>
#include <future>
#include <optional>
#include <stdexcept>
#include <utility>

#include <Commons/Coro/Utils.hpp>

namespace AdServer::Commons
{
  template<typename ResultType>
  class StartableAwaitable
  {
  public:
    struct promise_type;
    using Handle = std::coroutine_handle<promise_type>;
    using Completion = std::function<void(std::optional<std::exception_ptr>)>;

    explicit StartableAwaitable(Handle handle) noexcept;
    StartableAwaitable(StartableAwaitable&& other) noexcept;
    StartableAwaitable& operator=(StartableAwaitable&& other) noexcept;
    StartableAwaitable(const StartableAwaitable&) = delete;
    StartableAwaitable& operator=(const StartableAwaitable&) = delete;
    ~StartableAwaitable();

    void start(Completion completion);
    void start_detached(Completion completion);
    bool await_ready() const noexcept;
    void await_suspend(std::coroutine_handle<> continuation) noexcept;
    ResultType await_resume();

  private:
    Handle handle_;
  };

  template<>
  class StartableAwaitable<void>
  {
  public:
    struct promise_type;
    using Handle = std::coroutine_handle<promise_type>;
    using Completion = std::function<void(std::optional<std::exception_ptr>)>;

    explicit StartableAwaitable(Handle handle) noexcept;
    StartableAwaitable(StartableAwaitable&& other) noexcept;
    StartableAwaitable& operator=(StartableAwaitable&& other) noexcept;
    StartableAwaitable(const StartableAwaitable&) = delete;
    StartableAwaitable& operator=(const StartableAwaitable&) = delete;
    ~StartableAwaitable();

    void start(Completion completion);
    void start_detached(Completion completion);
    bool await_ready() const noexcept;
    void await_suspend(std::coroutine_handle<> continuation) noexcept;
    void await_resume();

  private:
    Handle handle_;
  };

  template<typename ResultType>
  struct StartableAwaitable<ResultType>::promise_type
  {
    StartableAwaitable get_return_object() noexcept;
    std::suspend_always initial_suspend() const noexcept;

    struct FinalAwaiter
    {
      bool await_ready() const noexcept;
      void await_suspend(Handle handle) const noexcept;
      void await_resume() const noexcept;
    };

    FinalAwaiter final_suspend() const noexcept;

    template<typename ValueType>
    void return_value(ValueType&& value);

    void unhandled_exception() noexcept;

    std::coroutine_handle<> continuation;
    Completion completion;
    std::optional<ResultType> result;
    std::optional<std::exception_ptr> exception;
    bool destroy_on_completion = false;
  };

  struct StartableAwaitable<void>::promise_type
  {
    StartableAwaitable get_return_object() noexcept;
    std::suspend_always initial_suspend() const noexcept;

    struct FinalAwaiter
    {
      bool await_ready() const noexcept;
      void await_suspend(Handle handle) const noexcept;
      void await_resume() const noexcept;
    };

    FinalAwaiter final_suspend() const noexcept;
    void return_void() const noexcept;
    void unhandled_exception() noexcept;

    std::coroutine_handle<> continuation;
    Completion completion;
    std::optional<std::exception_ptr> exception;
    bool destroy_on_completion = false;
  };

  template<typename ResultType>
  ResultType sync_wait(StartableAwaitable<ResultType> awaitable);

  void sync_wait(StartableAwaitable<void> awaitable);

  template<typename ResultType>
  inline StartableAwaitable<ResultType>::StartableAwaitable(Handle handle) noexcept
    : handle_(handle)
  {}

  template<typename ResultType>
  inline StartableAwaitable<ResultType>::StartableAwaitable(StartableAwaitable&& other) noexcept
    : handle_(std::exchange(other.handle_, {}))
  {}

  template<typename ResultType>
  inline StartableAwaitable<ResultType>&
  StartableAwaitable<ResultType>::operator=(StartableAwaitable&& other) noexcept
  {
    if (this != &other)
    {
      if (handle_)
      {
        handle_.destroy();
      }
      handle_ = std::exchange(other.handle_, {});
    }

    return *this;
  }

  template<typename ResultType>
  inline StartableAwaitable<ResultType>::~StartableAwaitable()
  {
    if (handle_)
    {
      handle_.destroy();
    }
  }

  template<typename ResultType>
  inline void
  StartableAwaitable<ResultType>::start(Completion completion)
  {
    handle_.promise().completion = std::move(completion);
    resume_coroutine(handle_);
  }

  template<typename ResultType>
  inline void
  StartableAwaitable<ResultType>::start_detached(Completion completion)
  {
    const Handle handle = std::exchange(handle_, {});
    handle.promise().completion = std::move(completion);
    handle.promise().destroy_on_completion = true;
    resume_coroutine(handle);
  }

  template<typename ResultType>
  inline bool
  StartableAwaitable<ResultType>::await_ready() const noexcept
  {
    return !handle_ || handle_.done();
  }

  template<typename ResultType>
  inline void
  StartableAwaitable<ResultType>::await_suspend(std::coroutine_handle<> continuation) noexcept
  {
    handle_.promise().continuation = continuation;
    resume_coroutine(handle_);
  }

  template<typename ResultType>
  inline ResultType
  StartableAwaitable<ResultType>::await_resume()
  {
    if (handle_.promise().exception)
    {
      std::rethrow_exception(std::move(*handle_.promise().exception));
    }

    if (!handle_.promise().result)
    {
      throw std::logic_error("StartableAwaitable result is not initialized");
    }

    return std::move(*handle_.promise().result);
  }

  template<typename ResultType>
  inline StartableAwaitable<ResultType>
  StartableAwaitable<ResultType>::promise_type::get_return_object() noexcept
  {
    return StartableAwaitable(Handle::from_promise(*this));
  }

  template<typename ResultType>
  inline std::suspend_always
  StartableAwaitable<ResultType>::promise_type::initial_suspend() const noexcept
  {
    return {};
  }

  template<typename ResultType>
  inline bool
  StartableAwaitable<ResultType>::promise_type::FinalAwaiter::await_ready()
    const noexcept
  {
    return false;
  }

  template<typename ResultType>
  inline void
  StartableAwaitable<ResultType>::promise_type::FinalAwaiter::await_suspend(
    Handle handle) const noexcept
  {
    auto& promise = handle.promise();
    const bool destroy_on_completion = promise.destroy_on_completion;
    auto completion = std::move(promise.completion);
    if (completion)
    {
      completion(std::move(promise.exception));
    }
    else if (promise.continuation)
    {
      resume_coroutine(promise.continuation);
    }

    if (destroy_on_completion)
    {
      handle.destroy();
    }
  }

  template<typename ResultType>
  inline void
  StartableAwaitable<ResultType>::promise_type::FinalAwaiter::await_resume()
    const noexcept
  {}

  template<typename ResultType>
  inline typename StartableAwaitable<ResultType>::promise_type::FinalAwaiter
  StartableAwaitable<ResultType>::promise_type::final_suspend() const noexcept
  {
    return {};
  }

  template<typename ResultType>
  template<typename ValueType>
  inline void
  StartableAwaitable<ResultType>::promise_type::return_value(ValueType&& value)
  {
    result.emplace(std::forward<ValueType>(value));
  }

  template<typename ResultType>
  inline void
  StartableAwaitable<ResultType>::promise_type::unhandled_exception() noexcept
  {
    exception.emplace(std::current_exception());
  }

  template<typename ResultType>
  inline ResultType
  sync_wait(StartableAwaitable<ResultType> awaitable)
  {
    using CompletionResult = std::optional<std::exception_ptr>;
    auto promise = std::make_shared<std::promise<CompletionResult>>();
    auto future = promise->get_future();
    awaitable.start(
      [promise = std::move(promise)](CompletionResult exception) mutable
      {
        promise->set_value(std::move(exception));
      });
    auto exception = future.get();
    if (exception)
    {
      std::rethrow_exception(std::move(*exception));
    }

    return awaitable.await_resume();
  }

  inline StartableAwaitable<void>::StartableAwaitable(Handle handle) noexcept
    : handle_(handle)
  {}

  inline StartableAwaitable<void>::StartableAwaitable(StartableAwaitable&& other) noexcept
    : handle_(std::exchange(other.handle_, {}))
  {}

  inline StartableAwaitable<void>&
  StartableAwaitable<void>::operator=(StartableAwaitable&& other) noexcept
  {
    if (this != &other)
    {
      if (handle_)
      {
        handle_.destroy();
      }
      handle_ = std::exchange(other.handle_, {});
    }

    return *this;
  }

  inline StartableAwaitable<void>::~StartableAwaitable()
  {
    if (handle_)
    {
      handle_.destroy();
    }
  }

  inline void
  StartableAwaitable<void>::start(Completion completion)
  {
    handle_.promise().completion = std::move(completion);
    resume_coroutine(handle_);
  }

  inline void
  StartableAwaitable<void>::start_detached(Completion completion)
  {
    const Handle handle = std::exchange(handle_, {});
    handle.promise().completion = std::move(completion);
    handle.promise().destroy_on_completion = true;
    resume_coroutine(handle);
  }

  inline bool
  StartableAwaitable<void>::await_ready() const noexcept
  {
    return !handle_ || handle_.done();
  }

  inline void
  StartableAwaitable<void>::await_suspend(std::coroutine_handle<> continuation) noexcept
  {
    handle_.promise().continuation = continuation;
    resume_coroutine(handle_);
  }

  inline void
  StartableAwaitable<void>::await_resume()
  {
    if (handle_.promise().exception)
    {
      std::rethrow_exception(std::move(*handle_.promise().exception));
    }
  }

  inline StartableAwaitable<void>
  StartableAwaitable<void>::promise_type::get_return_object() noexcept
  {
    return StartableAwaitable(Handle::from_promise(*this));
  }

  inline std::suspend_always
  StartableAwaitable<void>::promise_type::initial_suspend() const noexcept
  {
    return {};
  }

  inline bool
  StartableAwaitable<void>::promise_type::FinalAwaiter::await_ready()
    const noexcept
  {
    return false;
  }

  inline void
  StartableAwaitable<void>::promise_type::FinalAwaiter::await_suspend(Handle handle) const noexcept
  {
    auto& promise = handle.promise();
    const bool destroy_on_completion = promise.destroy_on_completion;
    auto completion = std::move(promise.completion);
    if (completion)
    {
      completion(std::move(promise.exception));
    }
    else if (promise.continuation)
    {
      resume_coroutine(promise.continuation);
    }

    if (destroy_on_completion)
    {
      handle.destroy();
    }
  }

  inline void
  StartableAwaitable<void>::promise_type::FinalAwaiter::await_resume()
    const noexcept
  {}

  inline StartableAwaitable<void>::promise_type::FinalAwaiter
  StartableAwaitable<void>::promise_type::final_suspend() const noexcept
  {
    return {};
  }

  inline void
  StartableAwaitable<void>::promise_type::return_void() const noexcept
  {}

  inline void
  StartableAwaitable<void>::promise_type::unhandled_exception() noexcept
  {
    exception.emplace(std::current_exception());
  }

  inline void
  sync_wait(StartableAwaitable<void> awaitable)
  {
    using CompletionResult = std::optional<std::exception_ptr>;
    auto promise = std::make_shared<std::promise<CompletionResult>>();
    auto future = promise->get_future();
    awaitable.start(
      [promise = std::move(promise)](CompletionResult exception) mutable
      {
        promise->set_value(std::move(exception));
      });
    auto exception = future.get();
    if (exception)
    {
      std::rethrow_exception(std::move(*exception));
    }

    awaitable.await_resume();
  }
}
