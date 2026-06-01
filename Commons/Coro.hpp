#pragma once

#include <coroutine>
#include <deque>
#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>

namespace AdServer::Commons
{
  inline void
  resume_coroutine(std::coroutine_handle<> handle)
  {
    thread_local bool draining = false;
    thread_local std::deque<std::coroutine_handle<>> pending;

    pending.push_back(handle);
    if(draining)
    {
      return;
    }

    draining = true;
    while(!pending.empty())
    {
      std::coroutine_handle<> next = pending.front();
      pending.pop_front();
      next.resume();
    }
    draining = false;
  }

  template<typename... Args>
  struct AsyncCallbackResult
  {
    using Type = std::tuple<std::decay_t<Args>...>;
  };

  template<>
  struct AsyncCallbackResult<>
  {
    using Type = void;
  };

  template<typename Arg>
  struct AsyncCallbackResult<Arg>
  {
    using Type = std::decay_t<Arg>;
  };

  template<typename... CallbackArgs>
  class AsyncCallbackAwaitable
  {
  public:
    using Result = typename AsyncCallbackResult<CallbackArgs...>::Type;

    using Callback = std::function<void(CallbackArgs...)>;
    using StartCallback = std::function<void(Callback)>;

    explicit
    AsyncCallbackAwaitable(StartCallback start_callback);

    bool
    await_ready() const noexcept;

    bool
    await_suspend(std::coroutine_handle<> handle);

    Result
    await_resume();

  private:
    using StoredResult = std::tuple<std::decay_t<CallbackArgs>...>;

    struct State
    {
      std::mutex lock;
      std::coroutine_handle<> handle;
      StoredResult result;
      std::exception_ptr exception;
      bool completed = false;
      bool suspended = false;
    };

    std::shared_ptr<State> state_;
    StartCallback start_callback_;
  };

  template<typename... CallbackArgs, typename Call, typename... Args>
  AsyncCallbackAwaitable<CallbackArgs...>
  async_callback(Call&& call, Args&&... args);

  template<typename ResultType>
  class Task
  {
  public:
    struct promise_type;
    using Handle = std::coroutine_handle<promise_type>;

    explicit Task(Handle handle) noexcept;
    Task(Task&& other) noexcept;
    Task& operator=(Task&& other) noexcept;
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;
    ~Task();

    bool await_ready() const noexcept;
    void await_suspend(std::coroutine_handle<> continuation) noexcept;
    ResultType await_resume();
    ResultType sync_wait();

  private:
    Handle handle_;
  };

  template<typename ResultType>
  struct Task<ResultType>::promise_type
  {
    Task get_return_object() noexcept;
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
    std::exception_ptr exception;
  };

  template<typename... CallbackArgs>
  AsyncCallbackAwaitable<CallbackArgs...>::AsyncCallbackAwaitable(
    StartCallback start_callback)
    : state_(std::make_shared<State>()),
      start_callback_(std::move(start_callback))
  {}

  template<typename... CallbackArgs>
  bool
  AsyncCallbackAwaitable<CallbackArgs...>::await_ready() const noexcept
  {
    return false;
  }

  template<typename... CallbackArgs>
  bool
  AsyncCallbackAwaitable<CallbackArgs...>::await_suspend(
    std::coroutine_handle<> handle)
  {
    state_->handle = handle;

    try
    {
      start_callback_(
        [state = state_](CallbackArgs... callback_args) mutable
        {
          bool resume = false;
          {
            std::lock_guard<std::mutex> guard(state->lock);
            state->result = StoredResult(std::forward<CallbackArgs>(
              callback_args)...);
            state->completed = true;
            resume = state->suspended;
          }

          if(resume)
          {
            resume_coroutine(state->handle);
          }
        });
    }
    catch(...)
    {
      std::lock_guard<std::mutex> guard(state_->lock);
      state_->exception = std::current_exception();
      state_->completed = true;
    }

    std::lock_guard<std::mutex> guard(state_->lock);
    if(state_->completed)
    {
      return false;
    }

    state_->suspended = true;
    return true;
  }

  template<typename... CallbackArgs>
  typename AsyncCallbackAwaitable<CallbackArgs...>::Result
  AsyncCallbackAwaitable<CallbackArgs...>::await_resume()
  {
    if(state_->exception)
    {
      std::rethrow_exception(state_->exception);
    }

    if constexpr(sizeof...(CallbackArgs) == 0)
    {
      return;
    }
    else if constexpr(sizeof...(CallbackArgs) == 1)
    {
      return std::move(std::get<0>(state_->result));
    }
    else
    {
      return std::move(state_->result);
    }
  }

  template<typename... CallbackArgs, typename Call, typename... Args>
  AsyncCallbackAwaitable<CallbackArgs...>
  async_callback(Call&& call, Args&&... args)
  {
    return AsyncCallbackAwaitable<CallbackArgs...>(
      [
        call = std::forward<Call>(call),
        ... args = std::forward<Args>(args)
      ](typename AsyncCallbackAwaitable<CallbackArgs...>::Callback callback)
      mutable
      {
        std::invoke(
          std::move(call),
          std::move(args)...,
          std::move(callback));
      });
  }

  template<typename ResultType>
  Task<ResultType>::Task(Handle handle) noexcept
    : handle_(handle)
  {}

  template<typename ResultType>
  Task<ResultType>::Task(Task&& other) noexcept
    : handle_(std::exchange(other.handle_, {}))
  {}

  template<typename ResultType>
  Task<ResultType>&
  Task<ResultType>::operator=(Task&& other) noexcept
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
  Task<ResultType>::~Task()
  {
    if(handle_)
    {
      handle_.destroy();
    }
  }

  template<typename ResultType>
  bool
  Task<ResultType>::await_ready() const noexcept
  {
    return false;
  }

  template<typename ResultType>
  void
  Task<ResultType>::await_suspend(
    std::coroutine_handle<> continuation) noexcept
  {
    handle_.promise().continuation = continuation;
    resume_coroutine(handle_);
  }

  template<typename ResultType>
  ResultType
  Task<ResultType>::await_resume()
  {
    if(handle_.promise().exception)
    {
      std::rethrow_exception(handle_.promise().exception);
    }

    return std::move(*handle_.promise().result);
  }

  template<typename ResultType>
  ResultType
  Task<ResultType>::sync_wait()
  {
    std::promise<void> promise;
    auto future = promise.get_future();
    handle_.promise().completion = [&promise]() {
      promise.set_value();
    };
    resume_coroutine(handle_);
    future.get();
    return await_resume();
  }

  template<typename ResultType>
  Task<ResultType>
  Task<ResultType>::promise_type::get_return_object() noexcept
  {
    return Task(Handle::from_promise(*this));
  }

  template<typename ResultType>
  std::suspend_always
  Task<ResultType>::promise_type::initial_suspend() const noexcept
  {
    return {};
  }

  template<typename ResultType>
  bool
  Task<ResultType>::promise_type::FinalAwaiter::await_ready() const noexcept
  {
    return false;
  }

  template<typename ResultType>
  void
  Task<ResultType>::promise_type::FinalAwaiter::await_suspend(
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
  Task<ResultType>::promise_type::FinalAwaiter::await_resume() const noexcept
  {}

  template<typename ResultType>
  typename Task<ResultType>::promise_type::FinalAwaiter
  Task<ResultType>::promise_type::final_suspend() const noexcept
  {
    return {};
  }

  template<typename ResultType>
  template<typename ValueType>
  void
  Task<ResultType>::promise_type::return_value(ValueType&& value)
  {
    result.emplace(std::forward<ValueType>(value));
  }

  template<typename ResultType>
  void
  Task<ResultType>::promise_type::unhandled_exception() noexcept
  {
    exception = std::current_exception();
  }
}
