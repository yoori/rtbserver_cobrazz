#include <atomic>
#include <coroutine>
#include <deque>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <thread>
#include <vector>

#include <Commons/AsyncMutex.hpp>
#include <Commons/Coro/ScopedCoroutineResumeScheduler.hpp>
#include <Commons/Coro/StartableAwaitable.hpp>

namespace
{
  using AdServer::Commons::AsyncMutex;
  using AdServer::Commons::StartableAwaitable;

  StartableAwaitable<void> take_lock(AsyncMutex& mutex, int id, std::vector<int>& order)
  {
    auto guard = co_await mutex.scoped_lock_async();
    order.push_back(id);
  }

  void check(bool condition, const char* message)
  {
    if (!condition)
    {
      throw std::runtime_error(message);
    }
  }

  void drain(std::deque<std::coroutine_handle<>>& scheduled)
  {
    while (!scheduled.empty())
    {
      const auto handle = scheduled.front();
      scheduled.pop_front();
      AdServer::Commons::resume_coroutine(handle);
    }
  }

  void test_cancel_after_handoff()
  {
    std::deque<std::coroutine_handle<>> scheduled;
    AdServer::Commons::CoroutineResumeScheduler scheduler =
      [&scheduled](std::coroutine_handle<> handle) { scheduled.push_back(handle); };
    AdServer::Commons::ScopedCoroutineResumeScheduler scope(scheduler);
    AsyncMutex mutex;
    std::vector<int> order;
    auto owner = mutex.scoped_lock();
    std::optional<StartableAwaitable<void>> first(take_lock(mutex, 1, order));
    std::optional<StartableAwaitable<void>> second(take_lock(mutex, 2, order));
    auto third = take_lock(mutex, 3, order);
    first->start({});
    second->start({});
    third.start({});

    owner.reset();
    first.reset();
    second.reset();
    check(scheduled.size() == 3, "cancelled handoff did not schedule the next waiter");
    // Run the surviving owner before the obsolete wakeups.
    auto last = scheduled.back();
    scheduled.pop_back();
    AdServer::Commons::resume_coroutine(last);
    auto new_owner = mutex.scoped_lock();
    auto fourth = take_lock(mutex, 4, order);
    fourth.start({});
    drain(scheduled);
    check(order == std::vector<int>({3}), "obsolete wakeup released the new owner's lock");
    new_owner.reset();
    drain(scheduled);
    check(order == std::vector<int>({3, 4}), "FIFO failed after cancelling selected waiters");
    check(AsyncMutex::stats().current_waiters == 0, "handoff cancellation leaked waiters");
  }

  void test_cancel_last_waiter()
  {
    std::deque<std::coroutine_handle<>> scheduled;
    AdServer::Commons::CoroutineResumeScheduler scheduler =
      [&scheduled](std::coroutine_handle<> handle) { scheduled.push_back(handle); };
    AdServer::Commons::ScopedCoroutineResumeScheduler scope(scheduler);
    std::vector<int> order;
    {
      AsyncMutex mutex;
      auto owner = mutex.scoped_lock();
      std::optional<StartableAwaitable<void>> first(take_lock(mutex, 1, order));
      first->start({});
      owner.reset();
      first.reset();
      auto second = take_lock(mutex, 2, order);
      second.start({});
      check(order == std::vector<int>({2}), "cancelling the selected last waiter kept the lock");
    }

    // The scheduler can outlive both the cancelled coroutine and its mutex.
    drain(scheduled);
    check(order == std::vector<int>({2}), "cancelled coroutine was resumed");
    check(AsyncMutex::stats().current_waiters == 0, "last-waiter cancellation leaked waiters");
  }

  void test_cancel_during_dispatch()
  {
    AsyncMutex mutex;
    std::vector<int> order;
    auto owner = mutex.scoped_lock();
    std::optional<StartableAwaitable<void>> first(take_lock(mutex, 1, order));
    AdServer::Commons::CoroutineResumeScheduler scheduler =
      [&](std::coroutine_handle<> handle) {
        first.reset();
        AdServer::Commons::resume_coroutine(handle);
      };
    {
      AdServer::Commons::ScopedCoroutineResumeScheduler scope(scheduler);
      first->start({});
    }

    auto second = take_lock(mutex, 2, order);
    second.start({});
    owner.reset();
    check(order == std::vector<int>({2}), "cancellation during dispatch lost the next waiter");
    check(AsyncMutex::stats().current_waiters == 0, "dispatch cancellation leaked waiters");
  }

  StartableAwaitable<void> increment(
    AsyncMutex& mutex, std::uint64_t& count, std::atomic<unsigned>& inside,
    std::atomic<bool>& overlap)
  {
    auto guard = co_await mutex.scoped_lock_async();
    if (inside.fetch_add(1) != 0)
    {
      overlap.store(true);
    }

    ++count;
    inside.fetch_sub(1);
  }

  void test_concurrent_handoff()
  {
    AsyncMutex mutex;
    std::uint64_t count = 0;
    std::atomic<unsigned> inside{0};
    std::atomic<bool> overlap{false};
    std::atomic<bool> start{false};
    std::vector<std::thread> threads;
    for (unsigned i = 0; i < 4; ++i)
    {
      threads.emplace_back([&]() {
        while (!start.load())
        {
          std::this_thread::yield();
        }

        for (unsigned iteration = 0; iteration < 10'000; ++iteration)
        {
          AdServer::Commons::sync_wait(increment(mutex, count, inside, overlap));
        }
      });
    }

    start.store(true);
    for (auto& thread : threads)
    {
      thread.join();
    }

    check(!overlap.load() && count == 40'000, "concurrent handoff violated mutual exclusion");
    check(AsyncMutex::stats().current_waiters == 0, "concurrent handoff leaked waiters");
  }
}

int main()
{
  AsyncMutex mutex;
  std::vector<int> order;
  std::deque<std::coroutine_handle<>> scheduled;
  AdServer::Commons::CoroutineResumeScheduler scheduler =
    [&scheduled](std::coroutine_handle<> handle)
    {
      scheduled.push_back(handle);
    };

  auto owner = mutex.scoped_lock();
  auto first = take_lock(mutex, 1, order);
  std::optional<StartableAwaitable<void>> cancelled;
  cancelled.emplace(take_lock(mutex, 2, order));
  auto third = take_lock(mutex, 3, order);

  {
    AdServer::Commons::ScopedCoroutineResumeScheduler scope(scheduler);
    first.start({});
    cancelled->start({});
    third.start({});
  }

  cancelled.reset();
  owner.reset();

  if (!order.empty() || scheduled.size() != 1)
  {
    std::cerr << "AsyncMutexTest: waiter wasn't passed to its scheduler\n";
    return 1;
  }

  while (!scheduled.empty())
  {
    const auto handle = scheduled.front();
    scheduled.pop_front();
    handle.resume();
  }

  if (order != std::vector<int>({1, 3}))
  {
    std::cerr << "AsyncMutexTest: FIFO or cancellation failed\n";
    return 1;
  }

  auto immediate = take_lock(mutex, 4, order);
  {
    AdServer::Commons::ScopedCoroutineResumeScheduler scope(scheduler);
    immediate.start({});
  }

  if (order != std::vector<int>({1, 3, 4}) || !scheduled.empty())
  {
    std::cerr << "AsyncMutexTest: immediate acquisition failed\n";
    return 1;
  }

  const auto stats = AsyncMutex::stats();
  if (stats.lock_attempts != 4 ||
    stats.immediate_locks != 1 ||
    stats.contended_locks != 3 ||
    stats.current_waiters != 0 ||
    stats.max_waiters != 3)
  {
    std::cerr << "AsyncMutexTest: stats mismatch\n";
    return 1;
  }

  auto final_guard = mutex.scoped_lock();
  if (!final_guard)
  {
    std::cerr << "AsyncMutexTest: mutex remained locked\n";
    return 1;
  }

  try
  {
    test_cancel_after_handoff();
    test_cancel_last_waiter();
    test_cancel_during_dispatch();
    test_concurrent_handoff();
  }
  catch (const std::exception& ex)
  {
    std::cerr << "AsyncMutexTest: " << ex.what() << '\n';
    return 1;
  }

  std::cout << "AsyncMutexTest: PASS\n";
  return 0;
}
