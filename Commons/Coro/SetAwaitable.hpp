#pragma once

#include <coroutine>
#include <exception>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

#include <Commons/Coro/ScopedCoroutineResumeScheduler.hpp>
#include <Commons/Coro/Utils.hpp>

namespace AdServer::Commons
{
  template<
    typename CoroutineType,
    typename Allocator = std::allocator<CoroutineType>>
  class SetAwaitable final
  {
  public:
    using Operations = std::vector<CoroutineType, Allocator>;

    explicit SetAwaitable(Operations operations);

    bool await_ready() const noexcept;
    bool await_suspend(std::coroutine_handle<> continuation);
    void await_resume();

  private:
    struct State
    {
      std::mutex lock;
      std::coroutine_handle<> continuation;
      CoroutineResumeScheduler resume_scheduler;
      std::optional<std::exception_ptr> exception;
      std::size_t remaining = 0;
      bool suspended = false;
    };

    Operations operations_;
    std::shared_ptr<State> state_;
  };
}

namespace AdServer::Commons
{
  template<typename CoroutineType, typename Allocator>
  SetAwaitable<CoroutineType, Allocator>::SetAwaitable(Operations operations)
    : operations_(std::move(operations)),
      state_(std::make_shared<State>())
  {}

  template<typename CoroutineType, typename Allocator>
  bool
  SetAwaitable<CoroutineType, Allocator>::await_ready() const noexcept
  {
    return operations_.empty();
  }

  template<typename CoroutineType, typename Allocator>
  bool
  SetAwaitable<CoroutineType, Allocator>::await_suspend(std::coroutine_handle<> continuation)
  {
    state_->continuation = continuation;
    if (const auto* scheduler = current_coroutine_resume_scheduler())
    {
      state_->resume_scheduler = *scheduler;
    }
    state_->remaining = operations_.size();

    for (auto& operation : operations_)
    {
      operation.start(
        [state = state_](std::optional<std::exception_ptr> exception) mutable
        {
          bool resume = false;

          {
            std::lock_guard<std::mutex> guard(state->lock);
            if (exception && !state->exception)
            {
              state->exception = std::move(exception);
            }

            if (--state->remaining == 0)
            {
              resume = state->suspended;
            }
          }

          if (resume)
          {
            if (state->resume_scheduler)
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
    if (state_->remaining == 0)
    {
      return false;
    }

    state_->suspended = true;
    return true;
  }

  template<typename CoroutineType, typename Allocator>
  void
  SetAwaitable<CoroutineType, Allocator>::await_resume()
  {
    if (state_->exception)
    {
      std::rethrow_exception(std::move(*state_->exception));
    }
  }
}
