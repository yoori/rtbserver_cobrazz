#include <coroutine>
#include <deque>
#include <iostream>
#include <optional>
#include <vector>

#include <Commons/AsyncMutex.hpp>
#include <Commons/Coro/ScopedCoroutineResumeScheduler.hpp>
#include <Commons/Coro/StartableAwaitable.hpp>

namespace
{
  using AdServer::Commons::AsyncMutex;
  using AdServer::Commons::StartableAwaitable;

  StartableAwaitable<void>
  take_lock(AsyncMutex& mutex, int id, std::vector<int>& order)
  {
    auto guard = co_await mutex.scoped_lock_async();
    order.push_back(id);
  }
}

int
main()
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

  if(!order.empty() || scheduled.size() != 1)
  {
    std::cerr << "AsyncMutexTest: waiter wasn't passed to its scheduler\n";
    return 1;
  }

  while(!scheduled.empty())
  {
    const auto handle = scheduled.front();
    scheduled.pop_front();
    handle.resume();
  }

  if(order != std::vector<int>({1, 3}))
  {
    std::cerr << "AsyncMutexTest: FIFO or cancellation failed\n";
    return 1;
  }

  auto immediate = take_lock(mutex, 4, order);
  {
    AdServer::Commons::ScopedCoroutineResumeScheduler scope(scheduler);
    immediate.start({});
  }

  if(order != std::vector<int>({1, 3, 4}) || !scheduled.empty())
  {
    std::cerr << "AsyncMutexTest: immediate acquisition failed\n";
    return 1;
  }

  const auto stats = AsyncMutex::stats();
  if(stats.lock_attempts != 4 ||
    stats.immediate_locks != 1 ||
    stats.contended_locks != 3 ||
    stats.current_waiters != 0 ||
    stats.max_waiters != 3)
  {
    std::cerr << "AsyncMutexTest: stats mismatch\n";
    return 1;
  }

  auto final_guard = mutex.scoped_lock();
  if(!final_guard)
  {
    std::cerr << "AsyncMutexTest: mutex remained locked\n";
    return 1;
  }

  std::cout << "AsyncMutexTest: PASS\n";
  return 0;
}
