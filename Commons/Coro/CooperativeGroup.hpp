#pragma once

#include <algorithm>
#include <atomic>
#include <mutex>
#include <span>

#include <boost/container/small_vector.hpp>

#include <Commons/Coro/StartableAwaitable.hpp>
#include <Commons/ExecutorPool.hpp>

namespace AdServer::Commons
{
  // The caller keeps the group and operation frames alive until completion.
  // Children must honor the current resume scheduler rather than hop to another executor.
  template<typename CoroutineType = StartableAwaitable<void>>
  class CooperativeGroup final
  {
  public:
    using Operations = std::span<CoroutineType>;
    using Completion = std::function<void(std::optional<std::exception_ptr>)>;

    CooperativeGroup(Operations operations, std::shared_ptr<ExecutorPool> executor_pool);

    void start(Completion completion, bool reschedule = false);

    bool await_ready() const noexcept;

    bool await_suspend(std::coroutine_handle<> continuation);

    void await_resume();

  private:
    struct ReadyQueue
    {
      ReadyQueue();

      bool empty() const noexcept;

      void push(std::coroutine_handle<> handle);

      std::coroutine_handle<> pop() noexcept;

      boost::container::small_vector<std::coroutine_handle<>, 8> handles;
      std::size_t head = 0;
      std::size_t size = 0;
    };

    struct State final : std::enable_shared_from_this<State>
    {
      enum class CompletionState
      {
        STARTING,
        SUSPENDED,
        COMPLETED
      };

      static constexpr unsigned RUNNING = 1;
      static constexpr unsigned PENDING = 2;

      struct CurrentScope
      {
        explicit CurrentScope(State* state);

        ~CurrentScope();

        State* previous;
      };

      State(Operations operations_val, std::shared_ptr<ExecutorPool> executor_pool_val);

      void enqueue(std::coroutine_handle<> handle);

      void schedule();

      bool run(bool awaiting);

      inline static thread_local State* current = nullptr;
      const Operations operations;
      const std::shared_ptr<ExecutorPool> executor_pool;
      std::optional<ExecutorPool::ContextIndex> context_index;
      CoroutineResumeScheduler parent_scheduler;
      std::coroutine_handle<> continuation;
      std::optional<std::exception_ptr> exception;
      std::size_t remaining;
      std::size_t next_operation = 0;
      Completion on_complete;
      ReadyQueue queues[2];
      ReadyQueue* ready = &queues[0];
      ReadyQueue* pending = &queues[1];
      std::mutex lock;
      std::atomic<unsigned> run_state{RUNNING};
      std::atomic<CompletionState> completion{CompletionState::STARTING};
    };

    std::shared_ptr<State> state_;
  };

  template<typename CoroutineType>
  inline CooperativeGroup<CoroutineType>::CooperativeGroup(
    Operations operations, std::shared_ptr<ExecutorPool> executor_pool)
    : state_(std::make_shared<State>(operations, std::move(executor_pool)))
  {}

  template<typename CoroutineType>
  inline void CooperativeGroup<CoroutineType>::start(Completion completion, bool reschedule)
  {
    auto state = state_;
    state->on_complete = std::move(completion);
    const auto& pool = state->executor_pool;
    if (pool && (reschedule || !pool->running_in_this_thread()))
    {
      state->context_index = pool->get_next_context_index();
      state->schedule();
    }
    else
    {
      state->run(false);
    }
  }

  template<typename CoroutineType>
  inline bool CooperativeGroup<CoroutineType>::await_ready() const noexcept
  {
    return state_->operations.empty();
  }

  template<typename CoroutineType>
  inline bool CooperativeGroup<CoroutineType>::await_suspend(std::coroutine_handle<> continuation)
  {
    auto state = state_;
    state->continuation = continuation;
    if (const auto* scheduler = current_coroutine_resume_scheduler())
    {
      state->parent_scheduler = *scheduler;
    }

    if (state->run(true))
    {
      return false;
    }

    auto expected = State::CompletionState::STARTING;
    return state->completion.compare_exchange_strong(
      expected, State::CompletionState::SUSPENDED,
      std::memory_order_acq_rel, std::memory_order_acquire);
  }

  template<typename CoroutineType>
  inline void CooperativeGroup<CoroutineType>::await_resume()
  {
    if (state_->exception)
    {
      std::rethrow_exception(*state_->exception);
    }
  }

  template<typename CoroutineType>
  inline CooperativeGroup<CoroutineType>::ReadyQueue::ReadyQueue() : handles(8)
  {}

  template<typename CoroutineType>
  [[gnu::always_inline]]
  inline bool CooperativeGroup<CoroutineType>::ReadyQueue::empty() const noexcept
  {
    return size == 0;
  }

  template<typename CoroutineType>
  [[gnu::always_inline]]
  inline void CooperativeGroup<CoroutineType>::ReadyQueue::push(std::coroutine_handle<> handle)
  {
    if (size == handles.size())
    {
      const auto old_capacity = handles.size();
      handles.resize(old_capacity * 2);
      // Unwrap the full ring into the newly allocated half, keeping its FIFO order.
      std::copy_n(handles.begin(), head, handles.begin() + old_capacity);
    }

    auto tail = head + size;
    if (tail >= handles.size())
    {
      tail -= handles.size();
    }

    handles[tail] = handle;
    ++size;
  }

  template<typename CoroutineType>
  [[gnu::always_inline]]
  inline std::coroutine_handle<> CooperativeGroup<CoroutineType>::ReadyQueue::pop() noexcept
  {
    if (empty())
    {
      return {};
    }

    const auto result = handles[head++];
    if (head == handles.size())
    {
      head = 0;
    }

    --size;
    return result;
  }

  template<typename CoroutineType>
  inline CooperativeGroup<CoroutineType>::State::CurrentScope::CurrentScope(State* state)
    : previous(std::exchange(current, state))
  {}

  template<typename CoroutineType>
  inline CooperativeGroup<CoroutineType>::State::CurrentScope::~CurrentScope()
  {
    current = previous;
  }

  template<typename CoroutineType>
  inline CooperativeGroup<CoroutineType>::State::State(
    Operations operations_val, std::shared_ptr<ExecutorPool> executor_pool_val)
    : operations(operations_val),
      executor_pool(std::move(executor_pool_val)),
      context_index(executor_pool ? executor_pool->current_context_index() : std::nullopt),
      remaining(operations.size())
  {}

  template<typename CoroutineType>
  inline void CooperativeGroup<CoroutineType>::State::enqueue(std::coroutine_handle<> handle)
  {
    if (current == this)
    {
      ready->push(handle);
      return;
    }

    bool wake;
    {
      std::lock_guard guard(lock);
      pending->push(handle);
      wake = !(run_state.fetch_or(RUNNING | PENDING, std::memory_order_acq_rel) & RUNNING);
    }

    if (wake)
    {
      schedule();
    }
  }

  template<typename CoroutineType>
  inline void CooperativeGroup<CoroutineType>::State::schedule()
  {
    auto self = this->shared_from_this();
    if (executor_pool)
    {
      try
      {
        executor_pool->post([self] { self->run(false); }, context_index);
        return;
      }
      catch (...)
      {
        // As in ExecutorPool, keep accepted continuations alive if posting fails.
      }
    }

    run(false);
  }

  template<typename CoroutineType>
  inline bool CooperativeGroup<CoroutineType>::State::run(bool awaiting)
  {
    auto self = this->shared_from_this();
    bool completed = false;
    bool repost = false;
    {
      CurrentScope current_scope(this);

      // The owning group and runner keep State alive until all children complete.
      // A borrowed capture also keeps scheduler copies inside std::function's small buffer.
      const CoroutineResumeScheduler scheduler = [this](std::coroutine_handle<> handle)
      {
        enqueue(handle);
      };
      ScopedCoroutineResumeScheduler scheduler_scope(scheduler);
      ScopedCoroutineResumeQueue queue_scope(scheduler, true);

      std::size_t resumed = 0;
      while (true)
      {
        if (remaining == 0)
        {
          completed = true;
          break;
        }

        if (next_operation < operations.size())
        {
          operations[next_operation++].start_inline([this](std::optional<std::exception_ptr> error)
          {
            if (error && !exception)
            {
              exception = std::move(error);
            }
            --remaining;
          });
        }
        else
        {
          auto next = ready->pop();
          if (!next)
          {
            // Either observe published work or hand runner ownership to the next producer.
            auto expected = RUNNING;
            if (run_state.compare_exchange_strong(
              expected, 0, std::memory_order_acq_rel, std::memory_order_acquire))
            {
              break;
            }

            std::lock_guard guard(lock);
            // Swap queues while external producers cannot publish to the old pending queue.
            std::swap(ready, pending);
            run_state.store(RUNNING, std::memory_order_relaxed);
            next = ready->pop();
          }

          next.resume();
        }

        // Bound a runnable group without introducing a post for every operation.
        if (++resumed == 256 && executor_pool)
        {
          repost = true;
          break;
        }
      }
    }

    if (repost)
    {
      schedule();
    }
    else if (completed && on_complete)
    {
      auto callback = std::move(on_complete);
      callback(std::move(exception));
    }
    else if (completed && !awaiting && completion.exchange(
      CompletionState::COMPLETED, std::memory_order_acq_rel) == CompletionState::SUSPENDED)
    {
      if (parent_scheduler)
      {
        parent_scheduler(continuation);
      }
      else
      {
        resume_coroutine(continuation);
      }
    }

    return completed;
  }
}
