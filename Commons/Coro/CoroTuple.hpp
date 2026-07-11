#pragma once

#include <coroutine>
#include <exception>
#include <memory>
#include <mutex>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>

#include <Commons/Coro/ScopedCoroutineResumeScheduler.hpp>
#include <Commons/Coro/Utils.hpp>

namespace AdServer::Commons
{
  template<typename... CoroutineTypes>
  class CoroTuple final
  {
  public:
    explicit CoroTuple(CoroutineTypes&&... operations);

    bool await_ready() const noexcept;
    bool await_suspend(std::coroutine_handle<> continuation);
    auto await_resume();

  private:
    template<typename CoroutineType>
    using ResultType = decltype(std::declval<CoroutineType&>().await_resume());

    template<std::size_t Index>
    void start_operation_();

    void complete_(std::exception_ptr exception);

  private:
    struct State
    {
      std::mutex lock;
      std::coroutine_handle<> continuation;
      CoroutineResumeScheduler resume_scheduler;
      std::exception_ptr exception;
      std::size_t remaining = sizeof...(CoroutineTypes);
      bool suspended = false;
    };

    std::tuple<CoroutineTypes...> operations_;
    std::tuple<std::optional<ResultType<CoroutineTypes>>...> results_;
    std::shared_ptr<State> state_;
  };

  template<typename... CoroutineTypes>
  CoroTuple(CoroutineTypes&&...) -> CoroTuple<CoroutineTypes...>;

  template<typename... CoroutineTypes>
  CoroTuple<CoroutineTypes...>::CoroTuple(CoroutineTypes&&... operations)
    : operations_(std::forward<CoroutineTypes>(operations)...),
      state_(std::make_shared<State>())
  {}

  template<typename... CoroutineTypes>
  bool
  CoroTuple<CoroutineTypes...>::await_ready() const noexcept
  {
    return sizeof...(CoroutineTypes) == 0;
  }

  template<typename... CoroutineTypes>
  bool
  CoroTuple<CoroutineTypes...>::await_suspend(
    std::coroutine_handle<> continuation)
  {
    state_->continuation = continuation;
    if(const auto* scheduler = current_coroutine_resume_scheduler())
    {
      state_->resume_scheduler = *scheduler;
    }

    [&]<std::size_t... Indexes>(std::index_sequence<Indexes...>)
    {
      (start_operation_<Indexes>(), ...);
    }(std::index_sequence_for<CoroutineTypes...>{});

    std::lock_guard<std::mutex> guard(state_->lock);
    if(state_->remaining == 0)
    {
      return false;
    }

    state_->suspended = true;
    return true;
  }

  template<typename... CoroutineTypes>
  auto
  CoroTuple<CoroutineTypes...>::await_resume()
  {
    if(state_->exception)
    {
      std::rethrow_exception(state_->exception);
    }

    return [&]<std::size_t... Indexes>(std::index_sequence<Indexes...>)
    {
      return std::make_tuple(
        std::move(*std::get<Indexes>(results_))...);
    }(std::index_sequence_for<CoroutineTypes...>{});
  }

  template<typename... CoroutineTypes>
  template<std::size_t Index>
  void
  CoroTuple<CoroutineTypes...>::start_operation_()
  {
    auto& operation = std::get<Index>(operations_);
    operation.start([this, &operation](std::exception_ptr exception) mutable
    {
      if(!exception)
      {
        try
        {
          std::get<Index>(results_).emplace(operation.await_resume());
        }
        catch(...)
        {
          exception = std::current_exception();
        }
      }

      complete_(exception);
    });
  }

  template<typename... CoroutineTypes>
  void
  CoroTuple<CoroutineTypes...>::complete_(std::exception_ptr exception)
  {
    bool resume = false;
    {
      std::lock_guard<std::mutex> guard(state_->lock);
      if(exception && !state_->exception)
      {
        state_->exception = exception;
      }

      if(--state_->remaining == 0)
      {
        resume = state_->suspended;
      }
    }

    if(resume)
    {
      if(state_->resume_scheduler)
      {
        state_->resume_scheduler(state_->continuation);
      }
      else
      {
        AdServer::Commons::resume_coroutine(state_->continuation);
      }
    }
  }
}
