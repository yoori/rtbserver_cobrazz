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
  struct DefaultCoroutineFrameAllocator
  {};

  template<typename ResultType, typename FrameAllocator = DefaultCoroutineFrameAllocator>
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
    void start_inline(Completion completion);
    void start_detached(Completion completion);
    bool await_ready() const noexcept;
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> continuation);
    ResultType await_resume();

  private:
    Handle handle_;
  };

  template<typename FrameAllocator>
  class StartableAwaitable<void, FrameAllocator>
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
    void start_inline(Completion completion);
    void start_detached(Completion completion);
    bool await_ready() const noexcept;
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> continuation);
    void await_resume();

  private:
    Handle handle_;
  };

  template<typename ResultType, typename FrameAllocator>
  struct StartableAwaitable<ResultType, FrameAllocator>::promise_type : FrameAllocator
  {
    StartableAwaitable get_return_object() noexcept;
    std::suspend_always initial_suspend() const noexcept;

    struct FinalAwaiter
    {
      bool await_ready() const noexcept;
      std::coroutine_handle<> await_suspend(Handle handle) const noexcept;
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

  template<typename FrameAllocator>
  struct StartableAwaitable<void, FrameAllocator>::promise_type : FrameAllocator
  {
    StartableAwaitable get_return_object() noexcept;
    std::suspend_always initial_suspend() const noexcept;

    struct FinalAwaiter
    {
      bool await_ready() const noexcept;
      std::coroutine_handle<> await_suspend(Handle handle) const noexcept;
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

  template<typename ResultType, typename FrameAllocator>
  ResultType sync_wait(StartableAwaitable<ResultType, FrameAllocator> awaitable);

  template<typename FrameAllocator>
  void sync_wait(StartableAwaitable<void, FrameAllocator> awaitable);

  template<typename ResultType, typename FrameAllocator>
  inline StartableAwaitable<ResultType, FrameAllocator>::StartableAwaitable(Handle handle) noexcept
    : handle_(handle)
  {}

  template<typename ResultType, typename FrameAllocator>
  inline
  StartableAwaitable<ResultType, FrameAllocator>::StartableAwaitable(StartableAwaitable&& other)
    noexcept
    : handle_(std::exchange(other.handle_, {}))
  {}

  template<typename ResultType, typename FrameAllocator>
  inline StartableAwaitable<ResultType, FrameAllocator>&
  StartableAwaitable<ResultType, FrameAllocator>::operator=(StartableAwaitable&& other) noexcept
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

  template<typename ResultType, typename FrameAllocator>
  inline StartableAwaitable<ResultType, FrameAllocator>::~StartableAwaitable()
  {
    if (handle_)
    {
      handle_.destroy();
    }
  }

  template<typename ResultType, typename FrameAllocator>
  inline void StartableAwaitable<ResultType, FrameAllocator>::start(Completion completion)
  {
    handle_.promise().completion = std::move(completion);
    resume_coroutine(handle_);
  }

  template<typename ResultType, typename FrameAllocator>
  inline void StartableAwaitable<ResultType, FrameAllocator>::start_inline(Completion completion)
  {
    handle_.promise().completion = std::move(completion);
    handle_.resume();
  }

  template<typename ResultType, typename FrameAllocator>
  inline void StartableAwaitable<ResultType, FrameAllocator>::start_detached(Completion completion)
  {
    const Handle handle = std::exchange(handle_, {});
    handle.promise().completion = std::move(completion);
    handle.promise().destroy_on_completion = true;
    resume_coroutine(handle);
  }

  template<typename ResultType, typename FrameAllocator>
  inline bool StartableAwaitable<ResultType, FrameAllocator>::await_ready() const noexcept
  {
    return !handle_ || handle_.done();
  }

  template<typename ResultType, typename FrameAllocator>
  inline std::coroutine_handle<> StartableAwaitable<ResultType, FrameAllocator>::await_suspend(
    std::coroutine_handle<> continuation)
  {
    handle_.promise().continuation = continuation;
    if (current_coroutine_symmetric_transfer_ref())
    {
      return handle_;
    }

    resume_coroutine(handle_);
    return std::noop_coroutine();
  }

  template<typename ResultType, typename FrameAllocator>
  inline ResultType StartableAwaitable<ResultType, FrameAllocator>::await_resume()
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

  template<typename ResultType, typename FrameAllocator>
  inline StartableAwaitable<ResultType, FrameAllocator>
  StartableAwaitable<ResultType, FrameAllocator>::promise_type::get_return_object() noexcept
  {
    return StartableAwaitable(Handle::from_promise(*this));
  }

  template<typename ResultType, typename FrameAllocator>
  inline std::suspend_always
  StartableAwaitable<ResultType, FrameAllocator>::promise_type::initial_suspend() const noexcept
  {
    return {};
  }

  template<typename ResultType, typename FrameAllocator>
  inline bool
  StartableAwaitable<ResultType, FrameAllocator>::promise_type::FinalAwaiter::await_ready()
    const noexcept
  {
    return false;
  }

  template<typename ResultType, typename FrameAllocator>
  inline std::coroutine_handle<>
  StartableAwaitable<ResultType, FrameAllocator>::promise_type::FinalAwaiter::await_suspend(
    Handle handle) const noexcept
  {
    auto& promise = handle.promise();
    const bool destroy_on_completion = promise.destroy_on_completion;
    auto completion = std::move(promise.completion);
    std::coroutine_handle<> next = std::noop_coroutine();
    if (completion)
    {
      completion(promise.exception);
    }
    else if (promise.continuation)
    {
      if (current_coroutine_symmetric_transfer_ref())
      {
        next = promise.continuation;
      }
      else
      {
        resume_coroutine(promise.continuation);
      }
    }

    if (destroy_on_completion)
    {
      handle.destroy();
    }

    return next;
  }

  template<typename ResultType, typename FrameAllocator>
  inline void
  StartableAwaitable<ResultType, FrameAllocator>::promise_type::FinalAwaiter::await_resume()
    const noexcept
  {}

  template<typename ResultType, typename FrameAllocator>
  inline typename StartableAwaitable<ResultType, FrameAllocator>::promise_type::FinalAwaiter
  StartableAwaitable<ResultType, FrameAllocator>::promise_type::final_suspend() const noexcept
  {
    return {};
  }

  template<typename ResultType, typename FrameAllocator>
  template<typename ValueType>
  inline void
  StartableAwaitable<ResultType, FrameAllocator>::promise_type::return_value(ValueType&& value)
  {
    result.emplace(std::forward<ValueType>(value));
  }

  template<typename ResultType, typename FrameAllocator>
  inline void
  StartableAwaitable<ResultType, FrameAllocator>::promise_type::unhandled_exception() noexcept
  {
    exception.emplace(std::current_exception());
  }

  template<typename ResultType, typename FrameAllocator>
  inline ResultType sync_wait(StartableAwaitable<ResultType, FrameAllocator> awaitable)
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

  template<typename FrameAllocator>
  inline StartableAwaitable<void, FrameAllocator>::StartableAwaitable(Handle handle) noexcept
    : handle_(handle)
  {}

  template<typename FrameAllocator>
  inline StartableAwaitable<void, FrameAllocator>::StartableAwaitable(StartableAwaitable&& other)
    noexcept
    : handle_(std::exchange(other.handle_, {}))
  {}

  template<typename FrameAllocator>
  inline StartableAwaitable<void, FrameAllocator>&
  StartableAwaitable<void, FrameAllocator>::operator=(StartableAwaitable&& other) noexcept
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

  template<typename FrameAllocator>
  inline StartableAwaitable<void, FrameAllocator>::~StartableAwaitable()
  {
    if (handle_)
    {
      handle_.destroy();
    }
  }

  template<typename FrameAllocator>
  inline void StartableAwaitable<void, FrameAllocator>::start(Completion completion)
  {
    handle_.promise().completion = std::move(completion);
    resume_coroutine(handle_);
  }

  template<typename FrameAllocator>
  inline void StartableAwaitable<void, FrameAllocator>::start_inline(Completion completion)
  {
    handle_.promise().completion = std::move(completion);
    handle_.resume();
  }

  template<typename FrameAllocator>
  inline void StartableAwaitable<void, FrameAllocator>::start_detached(Completion completion)
  {
    const Handle handle = std::exchange(handle_, {});
    handle.promise().completion = std::move(completion);
    handle.promise().destroy_on_completion = true;
    resume_coroutine(handle);
  }

  template<typename FrameAllocator>
  inline bool StartableAwaitable<void, FrameAllocator>::await_ready() const noexcept
  {
    return !handle_ || handle_.done();
  }

  template<typename FrameAllocator>
  inline std::coroutine_handle<>
  StartableAwaitable<void, FrameAllocator>::await_suspend(std::coroutine_handle<> continuation)
  {
    handle_.promise().continuation = continuation;
    if (current_coroutine_symmetric_transfer_ref())
    {
      return handle_;
    }

    resume_coroutine(handle_);
    return std::noop_coroutine();
  }

  template<typename FrameAllocator>
  inline void StartableAwaitable<void, FrameAllocator>::await_resume()
  {
    if (handle_.promise().exception)
    {
      std::rethrow_exception(std::move(*handle_.promise().exception));
    }
  }

  template<typename FrameAllocator>
  inline StartableAwaitable<void, FrameAllocator>
  StartableAwaitable<void, FrameAllocator>::promise_type::get_return_object() noexcept
  {
    return StartableAwaitable(Handle::from_promise(*this));
  }

  template<typename FrameAllocator>
  inline std::suspend_always
  StartableAwaitable<void, FrameAllocator>::promise_type::initial_suspend() const noexcept
  {
    return {};
  }

  template<typename FrameAllocator>
  inline bool StartableAwaitable<void, FrameAllocator>::promise_type::FinalAwaiter::await_ready()
    const noexcept
  {
    return false;
  }

  template<typename FrameAllocator>
  inline std::coroutine_handle<>
  StartableAwaitable<void, FrameAllocator>::promise_type::FinalAwaiter::await_suspend(Handle handle)
    const noexcept
  {
    auto& promise = handle.promise();
    const bool destroy_on_completion = promise.destroy_on_completion;
    auto completion = std::move(promise.completion);
    std::coroutine_handle<> next = std::noop_coroutine();
    if (completion)
    {
      completion(promise.exception);
    }
    else if (promise.continuation)
    {
      if (current_coroutine_symmetric_transfer_ref())
      {
        next = promise.continuation;
      }
      else
      {
        resume_coroutine(promise.continuation);
      }
    }

    if (destroy_on_completion)
    {
      handle.destroy();
    }

    return next;
  }

  template<typename FrameAllocator>
  inline void StartableAwaitable<void, FrameAllocator>::promise_type::FinalAwaiter::await_resume()
    const noexcept
  {}

  template<typename FrameAllocator>
  inline typename StartableAwaitable<void, FrameAllocator>::promise_type::FinalAwaiter
  StartableAwaitable<void, FrameAllocator>::promise_type::final_suspend() const noexcept
  {
    return {};
  }

  template<typename FrameAllocator>
  inline void StartableAwaitable<void, FrameAllocator>::promise_type::return_void() const noexcept
  {}

  template<typename FrameAllocator>
  inline void StartableAwaitable<void, FrameAllocator>::promise_type::unhandled_exception() noexcept
  {
    exception.emplace(std::current_exception());
  }

  template<typename FrameAllocator>
  inline void sync_wait(StartableAwaitable<void, FrameAllocator> awaitable)
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
