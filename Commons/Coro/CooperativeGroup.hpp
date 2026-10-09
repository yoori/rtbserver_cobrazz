#pragma once

#include <algorithm>
#include <span>

#include <boost/container/small_vector.hpp>

#include <Commons/Coro/StartableAwaitable.hpp>
#include <Commons/ExecutorPool.hpp>

namespace AdServer::Commons
{
  // The caller owns the operation frames and keeps them alive until the group completes.
  // Children must honor the current resume scheduler rather than hop to another executor.
  template<typename CoroutineType = StartableAwaitable<void>>
  class CooperativeGroup final
  {
  public:
    using Operations = std::span<CoroutineType>;

    CooperativeGroup(Operations operations, std::shared_ptr<ExecutorPool> executor_pool)
      : state_(std::make_shared<State>(operations, std::move(executor_pool)))
    {}

    bool await_ready() const noexcept
    {
      return state_->operations.empty();
    }

    bool await_suspend(std::coroutine_handle<> continuation)
    {
      auto state = state_;
      state->continuation = continuation;
      if (const auto* scheduler = current_coroutine_resume_scheduler())
      {
        state->parent_scheduler = *scheduler;
      }

      state->run(true);
      std::lock_guard guard(state->lock);
      if (state->completed)
      {
        return false;
      }

      state->suspended = true;
      return true;
    }

    void await_resume()
    {
      if (state_->exception)
      {
        std::rethrow_exception(*state_->exception);
      }
    }

  private:
    struct ReadyQueue
    {
      ReadyQueue() : handles(8)
      {}

      bool empty() const noexcept
      {
        return size == 0;
      }

      void push(std::coroutine_handle<> handle)
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

      std::coroutine_handle<> pop() noexcept
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

      boost::container::small_vector<std::coroutine_handle<>, 8> handles;
      std::size_t head = 0;
      std::size_t size = 0;
    };

    struct State final : std::enable_shared_from_this<State>
    {
      State(Operations operations_val, std::shared_ptr<ExecutorPool> executor_pool_val)
        : operations(operations_val),
          executor_pool(std::move(executor_pool_val)),
          context_index(executor_pool ? executor_pool->current_context_index() : std::nullopt),
          remaining(operations.size())
      {}

      void enqueue(std::coroutine_handle<> handle)
      {
        if (current == this)
        {
          ready->push(handle);
          return;
        }

        bool wake = false;
        {
          std::lock_guard guard(lock);
          pending->push(handle);
          if (!running)
          {
            running = true;
            wake = true;
          }
        }

        if (wake)
        {
          schedule();
        }
      }

      void schedule()
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

      void run(bool start)
      {
        auto self = this->shared_from_this();
        bool notify = false;
        bool repost = false;
        {
          struct CurrentScope
          {
            explicit CurrentScope(State* state) : previous(std::exchange(current, state))
            {}
            ~CurrentScope()
            {
              current = previous;
            }
            State* previous;
          } current_scope(this);

          // The awaiting frame and the runner own State until all children complete.
          // A borrowed capture also keeps scheduler copies inside std::function's small buffer.
          const CoroutineResumeScheduler scheduler = [this](std::coroutine_handle<> handle)
          {
            enqueue(handle);
          };
          ScopedCoroutineResumeScheduler scheduler_scope(scheduler);
          ScopedCoroutineResumeQueue queue_scope(scheduler);

          if (start)
          {
            for (auto& operation : operations)
            {
              operation.start([this](std::optional<std::exception_ptr> error)
              {
                if (error && !exception)
                {
                  exception = std::move(error);
                }
                --remaining;
              });
            }
          }

          std::size_t resumed = 0;
          while (true)
          {
            auto next = ready->pop();
            if (!next)
            {
              std::lock_guard guard(lock);
              if (pending->empty())
              {
                running = false;
                completed = remaining == 0;
                notify = completed && suspended;
                break;
              }

              // Exchange roles without moving small_vector buffers or allocating under the lock.
              std::swap(ready, pending);
              next = ready->pop();
            }

            next.resume();

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
        else if (notify)
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
      }

      inline static thread_local State* current = nullptr;
      const Operations operations;
      const std::shared_ptr<ExecutorPool> executor_pool;
      const std::optional<ExecutorPool::ContextIndex> context_index;
      CoroutineResumeScheduler parent_scheduler;
      std::coroutine_handle<> continuation;
      std::optional<std::exception_ptr> exception;
      std::size_t remaining;
      ReadyQueue queues[2];
      ReadyQueue* ready = &queues[0];
      ReadyQueue* pending = &queues[1];
      std::mutex lock;
      bool running = true;
      bool completed = false;
      bool suspended = false;
    };

    std::shared_ptr<State> state_;
  };
}
