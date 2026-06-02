#pragma once

#include <coroutine>
#include <cstdint>
#include <condition_variable>
#include <mutex>
#include <atomic>

#include <boost/intrusive/list.hpp>

namespace AdServer::Commons
{
  class AsyncMutex final
  {
  public:
    struct Stats
    {
      std::uint64_t lock_attempts = 0;
      std::uint64_t immediate_locks = 0;
      std::uint64_t contended_locks = 0;
      std::uint64_t current_waiters = 0;
      std::uint64_t max_waiters = 0;
    };

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
      struct Waiter
        : boost::intrusive::list_base_hook<
            boost::intrusive::link_mode<boost::intrusive::auto_unlink>>
      {
        std::coroutine_handle<> handle;
      };

      explicit ScopedLockAwaiter(AsyncMutex& mutex) noexcept;
      ~ScopedLockAwaiter() noexcept;

      bool await_ready() const noexcept;
      bool await_suspend(std::coroutine_handle<> handle);
      Guard await_resume() noexcept;

    private:
      AsyncMutex& mutex_;
      Waiter waiter_;
    };

    AsyncMutex() noexcept = default;
    AsyncMutex(const AsyncMutex&) = delete;
    AsyncMutex& operator=(const AsyncMutex&) = delete;
    ~AsyncMutex() noexcept = default;

    Guard scoped_lock();
    ScopedLockAwaiter scoped_lock_async() noexcept;
    static Stats stats() noexcept;

  private:
    friend class Guard;
    friend class ScopedLockAwaiter;

    bool try_lock_or_enqueue_(
      ScopedLockAwaiter::Waiter& waiter,
      std::coroutine_handle<> handle);
    void cancel_(ScopedLockAwaiter::Waiter& waiter) noexcept;
    void unlock_() noexcept;
    static void update_max_waiters_(std::uint64_t value) noexcept;

  private:
    static std::atomic<std::uint64_t> lock_attempts_;
    static std::atomic<std::uint64_t> immediate_locks_;
    static std::atomic<std::uint64_t> contended_locks_;
    static std::atomic<std::uint64_t> current_waiters_;
    static std::atomic<std::uint64_t> max_waiters_;

    std::mutex mutex_;
    std::condition_variable condition_;
    bool locked_ = false;
    boost::intrusive::list<
      ScopedLockAwaiter::Waiter,
      boost::intrusive::constant_time_size<false>> waiters_;
  };
}
