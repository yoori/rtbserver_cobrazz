#pragma once

#include <atomic>
#include <coroutine>
#include <exception>
#include <functional>
#include <memory>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>

#include <Commons/Coro/ScopedCoroutineResumeScheduler.hpp>
#include <Commons/Coro/Utils.hpp>

namespace AdServer::Commons
{
  template<typename... Args>
  struct CallbackAwaiterResult
  {
    using Type = std::tuple<std::decay_t<Args>...>;
  };

  template<>
  struct CallbackAwaiterResult<>
  {
    using Type = void;
  };

  template<typename Arg>
  struct CallbackAwaiterResult<Arg>
  {
    using Type = std::decay_t<Arg>;
  };

  template<typename... CallbackArgs>
  class CallbackAwaiter
  {
  public:
    using Result = typename CallbackAwaiterResult<CallbackArgs...>::Type;

    using Callback = std::function<void(CallbackArgs...)>;
    using StartCallback = std::function<void(Callback)>;

    explicit
    CallbackAwaiter(StartCallback start_callback);

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
      enum class Status
      {
        S_RUNNING,
        S_SUSPENDED,
        S_COMPLETED
      };

      std::atomic<Status> status{Status::S_RUNNING};
      std::coroutine_handle<> handle;
      StoredResult result;
      std::optional<std::exception_ptr> exception;
      CoroutineResumeScheduler resume_scheduler;
    };

    static void
    resume_(const std::shared_ptr<State>& state);

    std::shared_ptr<State> state_;
    StartCallback start_callback_;
  };
}

namespace AdServer::Commons
{
  template<typename... CallbackArgs, typename Call, typename... Args>
  CallbackAwaiter<CallbackArgs...>
  async_callback(Call&& call, Args&&... args);

  template<typename... CallbackArgs>
  CallbackAwaiter<CallbackArgs...>::CallbackAwaiter(
    StartCallback start_callback)
    : state_(std::make_shared<State>()),
      start_callback_(std::move(start_callback))
  {}

  template<typename... CallbackArgs>
  bool
  CallbackAwaiter<CallbackArgs...>::await_ready() const noexcept
  {
    return false;
  }

  template<typename... CallbackArgs>
  bool
  CallbackAwaiter<CallbackArgs...>::await_suspend(std::coroutine_handle<> handle)
  {
    auto state = state_;
    state->handle = handle;
    if(const auto* scheduler = current_coroutine_resume_scheduler())
    {
      state->resume_scheduler = *scheduler;
    }

    try
    {
      start_callback_(
        [state](CallbackArgs... callback_args) mutable
        {
          state->result = StoredResult(std::forward<CallbackArgs>(callback_args)...);

          const auto previous_status = state->status.exchange(
            State::Status::S_COMPLETED,
            std::memory_order_acq_rel);
          if(previous_status == State::Status::S_SUSPENDED)
          {
            resume_(state);
          }
        }
      );
    }
    catch(...)
    {
      state->exception.emplace(std::current_exception());
      const auto previous_status = state->status.exchange(
        State::Status::S_COMPLETED,
        std::memory_order_acq_rel);
      if(previous_status == State::Status::S_SUSPENDED)
      {
        resume_(state);
      }
    }

    auto expected_status = State::Status::S_RUNNING;
    if(state->status.compare_exchange_strong(
      expected_status,
      State::Status::S_SUSPENDED,
      std::memory_order_acq_rel,
      std::memory_order_acquire))
    {
      return true;
    }

    return false;
  }

  template<typename... CallbackArgs>
  typename CallbackAwaiter<CallbackArgs...>::Result
  CallbackAwaiter<CallbackArgs...>::await_resume()
  {
    state_->status.load(std::memory_order_acquire);

    if(state_->exception)
    {
      std::rethrow_exception(std::move(*state_->exception));
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

  template<typename... CallbackArgs>
  void
  CallbackAwaiter<CallbackArgs...>::resume_(const std::shared_ptr<State>& state)
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

  template<typename... CallbackArgs, typename Call, typename... Args>
  CallbackAwaiter<CallbackArgs...>
  async_callback(Call&& call, Args&&... args)
  {
    return CallbackAwaiter<CallbackArgs...>(
      [
        call = std::forward<Call>(call),
        ... args = std::forward<Args>(args)
      ](typename CallbackAwaiter<CallbackArgs...>::Callback callback)
      mutable
      {
        std::invoke(std::move(call), std::move(args)..., std::move(callback));
      });
  }
}
