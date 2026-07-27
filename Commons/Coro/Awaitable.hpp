#pragma once

#include <coroutine>
#include <exception>
#include <optional>
#include <stdexcept>
#include <utility>

#include <Commons/Coro/Utils.hpp>

namespace AdServer::Commons
{
  template<typename ResultType>
  class Awaitable
  {
  public:
    struct promise_type;
    using Handle = std::coroutine_handle<promise_type>;

    explicit Awaitable(Handle handle) noexcept;
    Awaitable(Awaitable&& other) noexcept;
    Awaitable& operator=(Awaitable&& other) noexcept;
    Awaitable(const Awaitable&) = delete;
    Awaitable& operator=(const Awaitable&) = delete;
    ~Awaitable();

    bool await_ready() const noexcept;
    std::coroutine_handle<> await_suspend(
      std::coroutine_handle<> continuation) noexcept;
    ResultType await_resume();

  private:
    Handle handle_;
  };

  template<>
  class Awaitable<void>
  {
  public:
    struct promise_type;
    using Handle = std::coroutine_handle<promise_type>;

    explicit Awaitable(Handle handle) noexcept;
    Awaitable(Awaitable&& other) noexcept;
    Awaitable& operator=(Awaitable&& other) noexcept;
    Awaitable(const Awaitable&) = delete;
    Awaitable& operator=(const Awaitable&) = delete;
    ~Awaitable();

    bool await_ready() const noexcept;
    std::coroutine_handle<> await_suspend(
      std::coroutine_handle<> continuation) noexcept;
    void await_resume();

  private:
    Handle handle_;
  };

  template<typename ResultType>
  struct Awaitable<ResultType>::promise_type
  {
    Awaitable get_return_object() noexcept;
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
    std::optional<ResultType> result;
    std::optional<std::exception_ptr> exception;
  };

  struct Awaitable<void>::promise_type
  {
    Awaitable get_return_object() noexcept;
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
    std::optional<std::exception_ptr> exception;
  };

  template<typename ResultType>
  inline Awaitable<ResultType>::Awaitable(Handle handle) noexcept
    : handle_(handle)
  {}

  template<typename ResultType>
  inline Awaitable<ResultType>::Awaitable(Awaitable&& other) noexcept
    : handle_(std::exchange(other.handle_, {}))
  {}

  template<typename ResultType>
  inline Awaitable<ResultType>&
  Awaitable<ResultType>::operator=(Awaitable&& other) noexcept
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
  inline Awaitable<ResultType>::~Awaitable()
  {
    if(handle_)
    {
      handle_.destroy();
    }
  }

  template<typename ResultType>
  inline bool
  Awaitable<ResultType>::await_ready() const noexcept
  {
    return !handle_ || handle_.done();
  }

  template<typename ResultType>
  inline std::coroutine_handle<>
  Awaitable<ResultType>::await_suspend(
    std::coroutine_handle<> continuation) noexcept
  {
    handle_.promise().continuation = continuation;
    return handle_;
  }

  template<typename ResultType>
  inline ResultType
  Awaitable<ResultType>::await_resume()
  {
    if(handle_.promise().exception)
    {
      std::rethrow_exception(std::move(*handle_.promise().exception));
    }

    if(!handle_.promise().result)
    {
      throw std::logic_error("Awaitable result is not initialized");
    }

    return std::move(*handle_.promise().result);
  }

  template<typename ResultType>
  inline Awaitable<ResultType>
  Awaitable<ResultType>::promise_type::get_return_object() noexcept
  {
    return Awaitable(Handle::from_promise(*this));
  }

  template<typename ResultType>
  inline std::suspend_always
  Awaitable<ResultType>::promise_type::initial_suspend() const noexcept
  {
    return {};
  }

  template<typename ResultType>
  inline bool
  Awaitable<ResultType>::promise_type::FinalAwaiter::await_ready() const noexcept
  {
    return false;
  }

  template<typename ResultType>
  inline std::coroutine_handle<>
  Awaitable<ResultType>::promise_type::FinalAwaiter::await_suspend(
    Handle handle) const noexcept
  {
    if(handle.promise().continuation)
    {
      return handle.promise().continuation;
    }

    return std::noop_coroutine();
  }

  template<typename ResultType>
  inline void
  Awaitable<ResultType>::promise_type::FinalAwaiter::await_resume() const noexcept
  {}

  template<typename ResultType>
  inline typename Awaitable<ResultType>::promise_type::FinalAwaiter
  Awaitable<ResultType>::promise_type::final_suspend() const noexcept
  {
    return {};
  }

  template<typename ResultType>
  template<typename ValueType>
  inline void
  Awaitable<ResultType>::promise_type::return_value(ValueType&& value)
  {
    result.emplace(std::forward<ValueType>(value));
  }

  template<typename ResultType>
  inline void
  Awaitable<ResultType>::promise_type::unhandled_exception() noexcept
  {
    exception.emplace(std::current_exception());
  }

  inline Awaitable<void>::Awaitable(Handle handle) noexcept
    : handle_(handle)
  {}

  inline Awaitable<void>::Awaitable(Awaitable&& other) noexcept
    : handle_(std::exchange(other.handle_, {}))
  {}

  inline Awaitable<void>&
  Awaitable<void>::operator=(Awaitable&& other) noexcept
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

  inline Awaitable<void>::~Awaitable()
  {
    if(handle_)
    {
      handle_.destroy();
    }
  }

  inline bool
  Awaitable<void>::await_ready() const noexcept
  {
    return !handle_ || handle_.done();
  }

  inline std::coroutine_handle<>
  Awaitable<void>::await_suspend(std::coroutine_handle<> continuation) noexcept
  {
    handle_.promise().continuation = continuation;
    return handle_;
  }

  inline void
  Awaitable<void>::await_resume()
  {
    if(handle_.promise().exception)
    {
      std::rethrow_exception(std::move(*handle_.promise().exception));
    }
  }

  inline Awaitable<void>
  Awaitable<void>::promise_type::get_return_object() noexcept
  {
    return Awaitable(Handle::from_promise(*this));
  }

  inline std::suspend_always
  Awaitable<void>::promise_type::initial_suspend() const noexcept
  {
    return {};
  }

  inline bool
  Awaitable<void>::promise_type::FinalAwaiter::await_ready() const noexcept
  {
    return false;
  }

  inline std::coroutine_handle<>
  Awaitable<void>::promise_type::FinalAwaiter::await_suspend(
    Handle handle) const noexcept
  {
    if(handle.promise().continuation)
    {
      return handle.promise().continuation;
    }

    return std::noop_coroutine();
  }

  inline void
  Awaitable<void>::promise_type::FinalAwaiter::await_resume() const noexcept
  {}

  inline Awaitable<void>::promise_type::FinalAwaiter
  Awaitable<void>::promise_type::final_suspend() const noexcept
  {
    return {};
  }

  inline void
  Awaitable<void>::promise_type::return_void() const noexcept
  {}

  inline void
  Awaitable<void>::promise_type::unhandled_exception() noexcept
  {
    exception.emplace(std::current_exception());
  }
}
