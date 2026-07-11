#pragma once

#include <coroutine>
#include <exception>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include <Commons/Coro/ScopedCoroutineResumeScheduler.hpp>
#include <Commons/Coro/Utils.hpp>

namespace AdServer::Commons
{
  template<typename CoroutineType>
  class CoroSet final
  {
  public:
    explicit CoroSet(std::vector<CoroutineType> operations);

    bool await_ready() const noexcept;
    bool await_suspend(std::coroutine_handle<> continuation);
    void await_resume();

  private:
    struct State
    {
      std::mutex lock;
      std::coroutine_handle<> continuation;
      CoroutineResumeScheduler resume_scheduler;
      std::exception_ptr exception;
      std::size_t remaining = 0;
      bool suspended = false;
    };

    std::vector<CoroutineType> operations_;
    std::shared_ptr<State> state_;
  };

  template<typename CoroutineType>
  CoroSet<CoroutineType>::CoroSet(std::vector<CoroutineType> operations)
    : operations_(std::move(operations)),
      state_(std::make_shared<State>())
  {}

  template<typename CoroutineType>
  bool
  CoroSet<CoroutineType>::await_ready() const noexcept
  {
    return operations_.empty();
  }

  template<typename CoroutineType>
  bool
  CoroSet<CoroutineType>::await_suspend(std::coroutine_handle<> continuation)
  {
    state_->continuation = continuation;
    if(const auto* scheduler = current_coroutine_resume_scheduler())
    {
      state_->resume_scheduler = *scheduler;
    }
    state_->remaining = operations_.size();

    for(auto& operation : operations_)
    {
      operation.start([state = state_](std::exception_ptr exception) mutable {
        bool resume = false;
        {
          std::lock_guard<std::mutex> guard(state->lock);
          if(exception && !state->exception)
          {
            state->exception = exception;
          }

          if(--state->remaining == 0)
          {
            resume = state->suspended;
          }
        }

        if(resume)
        {
          if(state->resume_scheduler)
          {
            state->resume_scheduler(state->continuation);
          }
          else
          {
            AdServer::Commons::resume_coroutine(state->continuation);
          }
        }
      });
    }

    std::lock_guard<std::mutex> guard(state_->lock);
    if(state_->remaining == 0)
    {
      return false;
    }

    state_->suspended = true;
    return true;
  }

  template<typename CoroutineType>
  void
  CoroSet<CoroutineType>::await_resume()
  {
    if(state_->exception)
    {
      std::rethrow_exception(state_->exception);
    }
  }
}
