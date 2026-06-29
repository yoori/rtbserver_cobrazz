#pragma once

#include <coroutine>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <tuple>
#include <type_traits>
#include <utility>

#include <Commons/Coro/ScopedCoroutineResumeScheduler.hpp>
#include <Commons/Coro/Utils.hpp>

namespace AdServer::Commons
{
  template<typename... Args>
  struct CallbackCoroResult
  {
    using Type = std::tuple<std::decay_t<Args>...>;
  };

  template<>
  struct CallbackCoroResult<>
  {
    using Type = void;
  };

  template<typename Arg>
  struct CallbackCoroResult<Arg>
  {
    using Type = std::decay_t<Arg>;
  };

  template<typename... CallbackArgs>
  class CallbackCoro
  {
  public:
    using Result = typename CallbackCoroResult<CallbackArgs...>::Type;

    using Callback = std::function<void(CallbackArgs...)>;
    using StartCallback = std::function<void(Callback)>;

    explicit
    CallbackCoro(StartCallback start_callback);

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
      CoroutineResumeScheduler resume_scheduler;
      bool completed = false;
      bool suspended = false;
    };

    std::shared_ptr<State> state_;
    StartCallback start_callback_;
  };

  template<typename... CallbackArgs, typename Call, typename... Args>
  CallbackCoro<CallbackArgs...>
  async_callback(Call&& call, Args&&... args);

  template<typename... CallbackArgs>
  CallbackCoro<CallbackArgs...>::CallbackCoro(
    StartCallback start_callback)
    : state_(std::make_shared<State>()),
      start_callback_(std::move(start_callback))
  {}

  template<typename... CallbackArgs>
  bool
  CallbackCoro<CallbackArgs...>::await_ready() const noexcept
  {
    return false;
  }

  template<typename... CallbackArgs>
  bool
  CallbackCoro<CallbackArgs...>::await_suspend(
    std::coroutine_handle<> handle)
  {
    state_->handle = handle;
    if(const auto* scheduler = current_coroutine_resume_scheduler())
    {
      state_->resume_scheduler = *scheduler;
    }

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
            if(state->resume_scheduler)
            {
              state->resume_scheduler(state->handle);
            }
            else
            {
              resume_coroutine(state->handle);
            }
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
  typename CallbackCoro<CallbackArgs...>::Result
  CallbackCoro<CallbackArgs...>::await_resume()
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
  CallbackCoro<CallbackArgs...>
  async_callback(Call&& call, Args&&... args)
  {
    return CallbackCoro<CallbackArgs...>(
      [
        call = std::forward<Call>(call),
        ... args = std::forward<Args>(args)
      ](typename CallbackCoro<CallbackArgs...>::Callback callback)
      mutable
      {
        std::invoke(
          std::move(call),
          std::move(args)...,
          std::move(callback));
      });
  }
}
