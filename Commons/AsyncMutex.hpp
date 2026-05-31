#pragma once

#include <coroutine>
#include <condition_variable>
#include <deque>
#include <mutex>

namespace AdServer::Commons
{
  class AsyncMutex final
  {
  public:
    class Guard final
    {
    public:
      Guard() noexcept = default;
      explicit Guard(AsyncMutex* mutex) noexcept;
      Guard(Guard&& rhs) noexcept;
      Guard& operator=(Guard&& rhs) noexcept;
      Guard(const Guard&) = delete;
      Guard& operator=(const Guard&) = delete;
      ~Guard() noexcept;

      void reset() noexcept;
      explicit operator bool() const noexcept;

    private:
      AsyncMutex* mutex_ = nullptr;
    };

    class ScopedLockAwaiter final
    {
    public:
      explicit ScopedLockAwaiter(AsyncMutex& mutex) noexcept;

      bool await_ready() const noexcept;
      bool await_suspend(std::coroutine_handle<> handle);
      Guard await_resume() noexcept;

    private:
      AsyncMutex& mutex_;
    };

    AsyncMutex() noexcept = default;
    AsyncMutex(const AsyncMutex&) = delete;
    AsyncMutex& operator=(const AsyncMutex&) = delete;
    ~AsyncMutex() noexcept = default;

    Guard scoped_lock();
    ScopedLockAwaiter scoped_lock_async() noexcept;

  private:
    friend class Guard;
    friend class ScopedLockAwaiter;

    bool try_lock_or_enqueue_(std::coroutine_handle<> handle);
    void unlock_() noexcept;

  private:
    std::mutex mutex_;
    std::condition_variable condition_;
    bool locked_ = false;
    std::deque<std::coroutine_handle<>> waiters_;
  };
}
