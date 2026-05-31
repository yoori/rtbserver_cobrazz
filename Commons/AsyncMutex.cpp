#include "AsyncMutex.hpp"

#include <utility>

namespace AdServer::Commons
{
  AsyncMutex::Guard::Guard(AsyncMutex* mutex) noexcept
    : mutex_(mutex)
  {}

  AsyncMutex::Guard::Guard(Guard&& rhs) noexcept
    : mutex_(std::exchange(rhs.mutex_, nullptr))
  {}

  AsyncMutex::Guard&
  AsyncMutex::Guard::operator=(Guard&& rhs) noexcept
  {
    if(this != &rhs)
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
    if(mutex_)
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

  bool
  AsyncMutex::ScopedLockAwaiter::await_ready() const noexcept
  {
    return false;
  }

  bool
  AsyncMutex::ScopedLockAwaiter::await_suspend(
    std::coroutine_handle<> handle)
  {
    return mutex_.try_lock_or_enqueue_(handle);
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

  AsyncMutex::Guard
  AsyncMutex::scoped_lock()
  {
    std::unique_lock<std::mutex> guard(mutex_);
    condition_.wait(
      guard,
      [this]() noexcept
      {
        return !locked_ && waiters_.empty();
      });

    locked_ = true;
    return Guard(this);
  }

  bool
  AsyncMutex::try_lock_or_enqueue_(std::coroutine_handle<> handle)
  {
    std::lock_guard<std::mutex> guard(mutex_);
    if(!locked_)
    {
      locked_ = true;
      return false;
    }

    waiters_.push_back(handle);
    return true;
  }

  void
  AsyncMutex::unlock_() noexcept
  {
    std::coroutine_handle<> next;
    {
      std::lock_guard<std::mutex> guard(mutex_);
      if(waiters_.empty())
      {
        locked_ = false;
        condition_.notify_one();
        return;
      }

      next = waiters_.front();
      waiters_.pop_front();
    }

    next.resume();
  }
}
