#include <atomic>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <future>
#include <iostream>
#include <mutex>
#include <pthread.h>
#include <thread>
#include <utility>
#include <vector>

#include <Commons/Grpc/GrpcClient.hpp>

namespace
{
  using namespace std::chrono_literals;
  using AdServer::Grpc::InflightLimiter;

  thread_local std::function<void()> before_wait;
  thread_local bool count_mutex_calls = false;
  thread_local std::size_t mutex_calls = 0;

  void require(bool condition, const char* message)
  {
    if (!condition)
    {
      std::cerr << "InflightLimiterTest: " << message << std::endl;
      std::abort();
    }
  }

  void await(std::future<void>& future)
  {
    require(future.wait_for(5s) == std::future_status::ready, "acquire/release timed out");
    future.get();
  }

  template<typename Predicate>
  void await(Predicate predicate)
  {
    const auto deadline = std::chrono::steady_clock::now() + 5s;
    while (!predicate())
    {
      require(std::chrono::steady_clock::now() < deadline, "waiter registration timed out");
      std::this_thread::yield();
    }
  }

  void test_fast_path()
  {
    InflightLimiter unlimited;
    InflightLimiter limiter(4);
    std::mutex mutex;
    mutex_calls = 0;
    count_mutex_calls = true;
    {
      std::lock_guard lock(mutex);
    }

    count_mutex_calls = false;
    require(mutex_calls == 1, "mutex interception is inactive");
    mutex_calls = 0;
    count_mutex_calls = true;
    unlimited.acquire(100);
    require(unlimited.try_acquire(200), "unlimited acquire failed");
    unlimited.release(300);
    require(unlimited.count() == 0, "unlimited limiter started accounting requests");

    for (unsigned i = 0; i < 1000; ++i)
    {
      limiter.acquire(3);
      require(limiter.try_acquire(3), "batch was split at the limit");
      require(limiter.count() == 6, "batch accounting changed");
      require(!limiter.try_acquire(), "full limiter accepted another request");
      limiter.release(3);
      limiter.release(3);
    }

    count_mutex_calls = false;
    require(mutex_calls == 0, "uncontended limiter acquired a mutex");
    require(limiter.count() == 0, "fast path leaked inflight requests");
  }

  void test_release_between_predicate_and_wait()
  {
    InflightLimiter limiter(1);
    limiter.acquire();
    std::promise<void> entered;
    auto waiting = entered.get_future();
    std::promise<void> resume;
    auto resuming = resume.get_future();
    auto acquiring = std::async(std::launch::async, [&]
      {
        before_wait = [&]
          {
            entered.set_value();
            resuming.wait();
          };
        limiter.acquire();
      });
    await(waiting);

    auto releasing = std::async(std::launch::async, [&] { limiter.release(); });
    await([&] { return limiter.count() == 0; });
    // Before the fix, release completes here and notifies before the actual wait.
    releasing.wait_for(50ms);
    resume.set_value();
    await(releasing);
    await(acquiring);
    require(limiter.count() == 1, "awakened waiter did not acquire the limit");
    limiter.release();
  }

  void test_concurrent_batches()
  {
    constexpr unsigned workers = 8;
    constexpr unsigned iterations = 2000;
    InflightLimiter limiter(4);
    limiter.acquire(4);
    std::atomic<unsigned> waiting{0};
    std::atomic<unsigned> completed{0};
    std::vector<std::future<void>> tasks;
    for (unsigned i = 0; i < workers; ++i)
    {
      tasks.emplace_back(std::async(std::launch::async, [&]
        {
          before_wait = [&] { waiting.fetch_add(1); };
          for (unsigned iteration = 0; iteration < iterations; ++iteration)
          {
            limiter.acquire(3);
            if (iteration % 8 == 0)
            {
              std::this_thread::yield();
            }

            limiter.release(3);
            completed.fetch_add(1, std::memory_order_relaxed);
          }
        }));
    }

    await([&] { return waiting.load() == workers; });
    limiter.release(4);
    for (auto& task : tasks)
    {
      await(task);
    }

    require(completed.load() == workers * iterations, "batch operations were lost");
    require(limiter.count() == 0, "concurrent batches leaked inflight requests");
  }
}

// Pause the real implementation after its predicate, without hooks in production code.
extern "C" void real_wait(std::condition_variable*, std::unique_lock<std::mutex>&)
  asm("__real__ZNSt18condition_variable4waitERSt11unique_lockISt5mutexE");
extern "C" void wrapped_wait(std::condition_variable*, std::unique_lock<std::mutex>&)
  asm("__wrap__ZNSt18condition_variable4waitERSt11unique_lockISt5mutexE");

extern "C" void wrapped_wait(std::condition_variable* cv, std::unique_lock<std::mutex>& lock)
{
  if (auto callback = std::exchange(before_wait, {}))
  {
    callback();
  }

  real_wait(cv, lock);
}

extern "C" int __real_pthread_mutex_lock(pthread_mutex_t* mutex);

extern "C" int __wrap_pthread_mutex_lock(pthread_mutex_t* mutex)
{
  if (count_mutex_calls)
  {
    ++mutex_calls;
  }

  return __real_pthread_mutex_lock(mutex);
}

int main()
{
  test_fast_path();
  test_release_between_predicate_and_wait();
  test_concurrent_batches();
  std::cout << "InflightLimiterTest: PASS" << std::endl;
}
