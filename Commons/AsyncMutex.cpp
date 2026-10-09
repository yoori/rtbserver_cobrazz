#include "AsyncMutex.hpp"

#include <utility>

#include <Commons/Coro/Utils.hpp>

namespace AdServer::Commons
{
  namespace
  {
    // One allocation per contended wait; no allocation on the uncontended path.
    struct ResumeOperation
    {
      struct promise_type;
      using Handle = std::coroutine_handle<promise_type>;

      static void release(std::coroutine_handle<> handle) noexcept
      {
        const auto typed_handle = Handle::from_address(handle.address());
        if (typed_handle.promise().references.fetch_sub(1, std::memory_order_acq_rel) == 1)
        {
          typed_handle.destroy();
        }
      }

      struct promise_type
      {
        explicit promise_type(std::coroutine_handle<> continuation) noexcept
          : continuation(continuation)
        {}

        ResumeOperation get_return_object() noexcept
        {
          return {Handle::from_promise(*this)};
        }

        std::suspend_always initial_suspend() const noexcept
        {
          return {};
        }

        struct FinalAwaiter
        {
          bool await_ready() const noexcept
          {
            return false;
          }

          std::coroutine_handle<> await_suspend(Handle handle) const noexcept
          {
            const auto next = handle.promise().continuation.exchange({});
            release(handle);
            // Transfer directly: queuing the original handle would reopen the cancellation gap.
            return next ? next : std::noop_coroutine();
          }

          void await_resume() const noexcept
          {}
        };

        FinalAwaiter final_suspend() const noexcept
        {
          return {};
        }

        void return_void() const noexcept
        {}

        void unhandled_exception() const noexcept
        {
          std::terminate();
        }

        // Awaiter and scheduled wakeup; an unscheduled operation is destroyed directly.
        std::atomic<unsigned> references{2};
        std::atomic<std::coroutine_handle<>> continuation;
      };

      Handle handle;
    };

    ResumeOperation make_resume_operation(std::coroutine_handle<> /*continuation*/)
    {
      co_return;
    }
  }

  std::atomic<std::uint64_t> AsyncMutex::immediate_locks_{0};
  std::atomic<std::uint64_t> AsyncMutex::contended_locks_{0};
  std::atomic<std::uint64_t> AsyncMutex::current_waiters_{0};
  std::atomic<std::uint64_t> AsyncMutex::max_waiters_{0};

  AsyncMutex::Guard::Guard(AsyncMutex* mutex) noexcept
    : mutex_(mutex)
  {}

  AsyncMutex::Guard::Guard(Guard&& rhs) noexcept
    : mutex_(std::exchange(rhs.mutex_, nullptr))
  {}

  AsyncMutex::Guard& AsyncMutex::Guard::operator=(Guard&& rhs) noexcept
  {
    if (this != &rhs)
    {
      reset();
      mutex_ = std::exchange(rhs.mutex_, nullptr);
    }

    return *this;
  }

  AsyncMutex::Guard::~Guard() noexcept
  {
    reset();
  }

  void AsyncMutex::Guard::reset() noexcept
  {
    if (mutex_)
    {
      AsyncMutex* mutex = std::exchange(mutex_, nullptr);
      mutex->unlock_();
    }
  }

  AsyncMutex::Guard::operator bool() const noexcept
  {
    return mutex_ != nullptr;
  }

  AsyncMutex::ScopedLockAwaiter::ScopedLockAwaiter(AsyncMutex& mutex) noexcept
    : mutex_(mutex)
  {}

  AsyncMutex::ScopedLockAwaiter::Waiter::~Waiter() noexcept
  {
    if (handle)
    {
      if (granted)
      {
        ResumeOperation::release(handle);
      }
      else
      {
        handle.destroy();
      }
    }
  }

  AsyncMutex::ScopedLockAwaiter::~ScopedLockAwaiter() noexcept
  {
    if (enqueued_)
    {
      mutex_.cancel_(waiter_);
    }
  }

  bool AsyncMutex::ScopedLockAwaiter::await_ready() const noexcept
  {
    return false;
  }

  bool AsyncMutex::ScopedLockAwaiter::await_suspend(std::coroutine_handle<> handle)
  {
    // Another thread may resume and destroy this awaiter as soon as it is published.
    enqueued_ = true;
    return mutex_.try_lock_or_enqueue_(waiter_, handle);
  }

  AsyncMutex::Guard AsyncMutex::ScopedLockAwaiter::await_resume() noexcept
  {
    enqueued_ = false;
    return Guard(&mutex_);
  }

  AsyncMutex::ScopedLockAwaiter AsyncMutex::scoped_lock_async() noexcept
  {
    return ScopedLockAwaiter(*this);
  }

  AsyncMutex::Stats AsyncMutex::stats() noexcept
  {
    const auto immediate_locks = immediate_locks_.load(std::memory_order_relaxed);
    const auto contended_locks = contended_locks_.load(std::memory_order_relaxed);
    return Stats{
      immediate_locks + contended_locks,
      immediate_locks,
      contended_locks,
      current_waiters_.load(std::memory_order_relaxed),
      max_waiters_.load(std::memory_order_relaxed)
    };
  }

  AsyncMutex::Guard AsyncMutex::scoped_lock()
  {
    std::unique_lock<std::mutex> guard(mutex_);
    condition_.wait(guard, [this]() noexcept { return !locked_ && waiters_.empty(); });

    locked_ = true;
    return Guard(this);
  }

  bool AsyncMutex::try_lock_or_enqueue_(
    ScopedLockAwaiter::Waiter& waiter, std::coroutine_handle<> handle)
  {
    std::lock_guard<std::mutex> guard(mutex_);
    if (!locked_)
    {
      locked_ = true;
      immediate_locks_.fetch_add(1, std::memory_order_relaxed);
      return false;
    }

    waiter.handle = make_resume_operation(handle).handle;
    if (const auto* scheduler = current_coroutine_resume_scheduler())
    {
      waiter.resume_scheduler = *scheduler;
    }
    waiters_.push_back(waiter);
    contended_locks_.fetch_add(1, std::memory_order_relaxed);
    const auto waiters = current_waiters_.fetch_add(1, std::memory_order_relaxed) + 1;
    update_max_waiters_(waiters);
    return true;
  }

  void AsyncMutex::cancel_(ScopedLockAwaiter::Waiter& waiter) noexcept
  {
    bool granted = false;

    {
      std::lock_guard<std::mutex> guard(mutex_);
      if (waiter.handle)
      {
        const auto handle = ResumeOperation::Handle::from_address(waiter.handle.address());
        handle.promise().continuation.store({});
      }

      if (waiter.is_linked())
      {
        waiter.unlink();
        current_waiters_.fetch_sub(1, std::memory_order_relaxed);
      }

      granted = waiter.granted;
    }

    if (granted)
    {
      unlock_();
    }
  }

  void AsyncMutex::unlock_() noexcept
  {
    std::coroutine_handle<> next;
    CoroutineResumeScheduler resume_scheduler;

    {
      std::lock_guard<std::mutex> guard(mutex_);
      if (!waiters_.empty())
      {
        ScopedLockAwaiter::Waiter& waiter = waiters_.front();
        waiter.unlink();
        current_waiters_.fetch_sub(1, std::memory_order_relaxed);
        waiter.granted = true;
        next = waiter.handle;
        resume_scheduler = std::move(waiter.resume_scheduler);
      }

      if (!next)
      {
        locked_ = false;
      }
    }

    if (!next)
    {
      condition_.notify_one();
      return;
    }

    if (resume_scheduler)
    {
      resume_scheduler(next);
    }
    else
    {
      resume_coroutine(next);
    }
  }

  void AsyncMutex::update_max_waiters_(std::uint64_t value) noexcept
  {
    auto current = max_waiters_.load(std::memory_order_relaxed);
    while (current < value &&
      !max_waiters_.compare_exchange_weak(
        current,
        value,
        std::memory_order_relaxed,
        std::memory_order_relaxed))
    {}
  }
}
