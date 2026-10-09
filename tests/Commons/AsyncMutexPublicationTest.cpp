#include <atomic>
#include <iostream>
#include <thread>

#include <pthread.h>

#include <Commons/AsyncMutex.hpp>
#include <Commons/Coro/StartableAwaitable.hpp>

namespace
{
  thread_local bool pause_after_unlock = false;
  std::atomic<bool> published{false};
  std::atomic<bool> continue_suspend{false};

  AdServer::Commons::StartableAwaitable<void> take_lock(
    AdServer::Commons::AsyncMutex& mutex, bool& completed)
  {
    auto guard = co_await mutex.scoped_lock_async();
    completed = true;
  }
}

extern "C" int __real_pthread_mutex_unlock(pthread_mutex_t* mutex);

extern "C" int __wrap_pthread_mutex_unlock(pthread_mutex_t* mutex)
{
  const int result = __real_pthread_mutex_unlock(mutex);
  if (pause_after_unlock)
  {
    pause_after_unlock = false;
    published.store(true);
    while (!continue_suspend.load())
    {
      std::this_thread::yield();
    }
  }

  return result;
}

int main()
{
  AdServer::Commons::AsyncMutex mutex;
  bool completed = false;
  auto owner = mutex.scoped_lock();
  std::thread suspender([&]() {
    auto operation = take_lock(mutex, completed);
    pause_after_unlock = true;
    operation.start_detached({});
  });
  while (!published.load())
  {
    std::this_thread::yield();
  }

  // Finish and destroy the coroutine before its await_suspend returns. Run under Valgrind
  // or ASan to detect any subsequent access to the freed awaiter in the publishing thread.
  owner.reset();
  continue_suspend.store(true);
  suspender.join();
  if (!completed || AdServer::Commons::AsyncMutex::stats().current_waiters != 0)
  {
    std::cerr << "AsyncMutexPublicationTest: handoff did not complete\n";
    return 1;
  }

  std::cout << "AsyncMutexPublicationTest: PASS\n";
  return 0;
}
