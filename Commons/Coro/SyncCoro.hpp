#pragma once

#include <coroutine>
#include <exception>
#include <functional>
#include <future>
#include <optional>
#include <utility>

#include <Commons/Coro/Utils.hpp>

namespace AdServer::Commons
{
  // SyncCoro is the basic coroutine return object used by service code that needs
  // an awaitable value-producing coroutine. It owns the coroutine frame,
  // resumes a parent coroutine on final suspend, and also provides sync_wait()
  // for bridge code that must execute the coroutine from a synchronous path.
  template<typename ResultType>
  class SyncCoro
  {
  public:
    struct promise_type;
    using Handle = std::coroutine_handle<promise_type>;
    using Completion = std::function<void(std::optional<std::exception_ptr>)>;

    explicit SyncCoro(Handle handle) noexcept;
    SyncCoro(SyncCoro&& other) noexcept;
    SyncCoro& operator=(SyncCoro&& other) noexcept;
    SyncCoro(const SyncCoro&) = delete;
    SyncCoro& operator=(const SyncCoro&) = delete;
    ~SyncCoro();

    void start(Completion completion);
    bool await_ready() const noexcept;
    void await_suspend(std::coroutine_handle<> continuation) noexcept;
    ResultType await_resume();
    ResultType sync_wait();

  private:
    Handle handle_;
  };

  template<typename ResultType>
  struct SyncCoro<ResultType>::promise_type
  {
    SyncCoro get_return_object() noexcept;
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
    std::function<void()> completion;
    std::optional<ResultType> result;
    std::optional<std::exception_ptr> exception;
  };

  template<typename ResultType>
  SyncCoro<ResultType>::SyncCoro(Handle handle) noexcept
    : handle_(handle)
  {}

  template<typename ResultType>
  SyncCoro<ResultType>::SyncCoro(SyncCoro&& other) noexcept
    : handle_(std::exchange(other.handle_, {}))
  {}

  template<typename ResultType>
  SyncCoro<ResultType>&
  SyncCoro<ResultType>::operator=(SyncCoro&& other) noexcept
  {
    if(this != &other)
    {
      if(handle_)
      {
        handle_.destroy();
      }
      handle_ = std::exchange(other.handle_, {});
    }

    return *this;
  }

  template<typename ResultType>
  SyncCoro<ResultType>::~SyncCoro()
  {
    if(handle_)
    {
      handle_.destroy();
    }
  }

  template<typename ResultType>
  void
  SyncCoro<ResultType>::start(Completion completion)
  {
    handle_.promise().completion =
      [this, completion = std::move(completion)]() mutable
      {
        completion(std::move(handle_.promise().exception));
      };
    resume_coroutine(handle_);
  }

  template<typename ResultType>
  bool
  SyncCoro<ResultType>::await_ready() const noexcept
  {
    return false;
  }

  template<typename ResultType>
  void
  SyncCoro<ResultType>::await_suspend(
    std::coroutine_handle<> continuation) noexcept
  {
    handle_.promise().continuation = continuation;
    resume_coroutine(handle_);
  }

  template<typename ResultType>
  ResultType
  SyncCoro<ResultType>::await_resume()
  {
    if(handle_.promise().exception)
    {
      std::rethrow_exception(std::move(*handle_.promise().exception));
    }

    return std::move(*handle_.promise().result);
  }

  template<typename ResultType>
  ResultType
  SyncCoro<ResultType>::sync_wait()
  {
    std::promise<void> promise;
    auto future = promise.get_future();
    handle_.promise().completion = [&promise]()
    {
      promise.set_value();
    };
    resume_coroutine(handle_);
    future.get();
    return await_resume();
  }

  template<typename ResultType>
  SyncCoro<ResultType>
  SyncCoro<ResultType>::promise_type::get_return_object() noexcept
  {
    return SyncCoro(Handle::from_promise(*this));
  }

  template<typename ResultType>
  std::suspend_always
  SyncCoro<ResultType>::promise_type::initial_suspend() const noexcept
  {
    return {};
  }

  template<typename ResultType>
  bool
  SyncCoro<ResultType>::promise_type::FinalAwaiter::await_ready() const noexcept
  {
    return false;
  }

  template<typename ResultType>
  void
  SyncCoro<ResultType>::promise_type::FinalAwaiter::await_suspend(
    Handle handle) const noexcept
  {
    if(handle.promise().completion)
    {
      handle.promise().completion();
    }
    else if(handle.promise().continuation)
    {
      resume_coroutine(handle.promise().continuation);
    }
  }

  template<typename ResultType>
  void
  SyncCoro<ResultType>::promise_type::FinalAwaiter::await_resume() const noexcept
  {}

  template<typename ResultType>
  typename SyncCoro<ResultType>::promise_type::FinalAwaiter
  SyncCoro<ResultType>::promise_type::final_suspend() const noexcept
  {
    return {};
  }

  template<typename ResultType>
  template<typename ValueType>
  void
  SyncCoro<ResultType>::promise_type::return_value(ValueType&& value)
  {
    result.emplace(std::forward<ValueType>(value));
  }

  template<typename ResultType>
  void
  SyncCoro<ResultType>::promise_type::unhandled_exception() noexcept
  {
    exception.emplace(std::current_exception());
  }
}
