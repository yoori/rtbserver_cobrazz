#include "AsyncMutex.hpp"

#include <utility>

#include <Commons/Coro/Utils.hpp>

namespace AdServer::Commons
{
  std::atomic<std::uint64_t> AsyncMutex::lock_attempts_{0};
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

  AsyncMutex::Guard&
  AsyncMutex::Guard::operator=(Guard&& rhs) noexcept
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

  void
  AsyncMutex::Guard::reset() noexcept
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

  AsyncMutex::ScopedLockAwaiter::~ScopedLockAwaiter() noexcept
  {
    mutex_.cancel_(waiter_);
  }

  bool
  AsyncMutex::ScopedLockAwaiter::await_ready() const noexcept
  {
    return false;
  }

  bool
  AsyncMutex::ScopedLockAwaiter::await_suspend(std::coroutine_handle<> handle)
  {
    return mutex_.try_lock_or_enqueue_(waiter_, handle);
  }

  AsyncMutex::Guard
  AsyncMutex::ScopedLockAwaiter::await_resume() noexcept
  {
    return Guard(&mutex_);
  }

  AsyncMutex::ScopedLockAwaiter
  AsyncMutex::scoped_lock_async() noexcept
  {
    return ScopedLockAwaiter(*this);
  }

  AsyncMutex::Stats
  AsyncMutex::stats() noexcept
  {
    return Stats{
      lock_attempts_.load(std::memory_order_relaxed),
      immediate_locks_.load(std::memory_order_relaxed),
      contended_locks_.load(std::memory_order_relaxed),
      current_waiters_.load(std::memory_order_relaxed),
      max_waiters_.load(std::memory_order_relaxed)
    };
  }

  AsyncMutex::Guard
  AsyncMutex::scoped_lock()
  {
    std::unique_lock<std::mutex> guard(mutex_);
    condition_.wait(guard, [this]() noexcept { return !locked_ && waiters_.empty(); });

    locked_ = true;
    return Guard(this);
  }

  bool
  AsyncMutex::try_lock_or_enqueue_(
    ScopedLockAwaiter::Waiter& waiter,
    std::coroutine_handle<> handle)
  {
    lock_attempts_.fetch_add(1, std::memory_order_relaxed);

    std::lock_guard<std::mutex> guard(mutex_);
    if (!locked_)
    {
      locked_ = true;
      immediate_locks_.fetch_add(1, std::memory_order_relaxed);
      return false;
    }

    waiter.handle = handle;
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

  void
  AsyncMutex::cancel_(ScopedLockAwaiter::Waiter& waiter) noexcept
  {
    std::lock_guard<std::mutex> guard(mutex_);
    if (waiter.is_linked())
    {
      waiter.unlink();
      current_waiters_.fetch_sub(1, std::memory_order_relaxed);
    }
  }

  void
  AsyncMutex::unlock_() noexcept
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

  void
  AsyncMutex::update_max_waiters_(std::uint64_t value) noexcept
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
