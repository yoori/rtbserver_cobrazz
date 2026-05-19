#pragma once

#include <coroutine>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <tuple>
#include <type_traits>
#include <utility>

namespace AdServer::Commons
{
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
            state->handle.resume();
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
}
