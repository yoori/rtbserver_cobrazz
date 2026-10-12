#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <future>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

#include <pthread.h>

#include <Commons/Coro/CallbackAwaiter.hpp>
#include <Commons/Coro/CooperativeGroup.hpp>

namespace
{
  thread_local bool count_locks = false;
  thread_local unsigned lock_count = 0;
}

extern "C" int __real_pthread_mutex_lock(pthread_mutex_t* mutex);

extern "C" int __wrap_pthread_mutex_lock(pthread_mutex_t* mutex)
{
  if (count_locks)
  {
    ++lock_count;
  }

  return __real_pthread_mutex_lock(mutex);
}

namespace
{
  using namespace AdServer::Commons;
  using namespace std::chrono_literals;
  using Task = StartableAwaitable<void>;
  using Pool = std::shared_ptr<ExecutorPool>;

  class Callback final : public Generics::ActiveObjectCallback,
    public ReferenceCounting::AtomicImpl
  {
  public:
    void report_error(Severity, const String::SubString& text, const char*) noexcept override
    {
      std::cerr << text << std::endl;
      std::abort();
    }
  };

  struct State
  {
    std::atomic<unsigned> executing{0};
    unsigned resumed = 0;
    unsigned completed = 0;
  };

  struct Segment
  {
    explicit Segment(State& state) : state(state)
    {
      assert(state.executing.fetch_add(1) == 0);
      std::this_thread::yield();
    }

    ~Segment()
    {
      assert(state.executing.fetch_sub(1) == 1);
    }

    State& state;
  };

  class Wakeup
  {
  public:
    void publish(std::function<void()> callback)
    {
      {
        std::lock_guard guard(lock_);
        assert(!callback_);
        callback_ = std::move(callback);
      }
      condition_.notify_one();
    }

    void resume()
    {
      std::function<void()> callback;
      {
        std::unique_lock guard(lock_);
        assert(condition_.wait_for(guard, 10s, [&] { return bool(callback_); }));
        callback = std::move(callback_);
        callback_ = {};
      }
      callback();
    }

  private:
    std::mutex lock_;
    std::condition_variable condition_;
    std::function<void()> callback_;
  };

  Task immediate(State& state, bool fail = false)
  {
    Segment segment(state);
    ++state.resumed;
    if (fail)
    {
      throw std::runtime_error("child failure");
    }
    co_return;
  }

  Task synchronous_group(bool fail, unsigned size)
  {
    State state;
    std::vector<Task> operations;
    for (unsigned i = 0; i < size; ++i)
    {
      operations.emplace_back(immediate(state, fail && i == 0));
    }

    bool caught = false;
    lock_count = 0;
    count_locks = true;
    try
    {
      co_await CooperativeGroup<Task>({operations.data(), operations.size()}, {});
    }
    catch (const std::runtime_error&)
    {
      caught = true;
    }
    count_locks = false;
    assert(lock_count == 0);
    assert(caught == fail);
    assert(state.resumed == size);
  }

  Task waiting(Wakeup& wakeup, State& state, unsigned rounds, bool fail)
  {
    for (unsigned i = 0; i < rounds; ++i)
    {
      co_await CallbackAwaiter<>([&](auto callback) { wakeup.publish(std::move(callback)); });
      co_await immediate(state);
    }

    if (fail)
    {
      throw std::runtime_error("asynchronous child failure");
    }
  }

  StartableAwaitable<unsigned> deep_result(unsigned depth)
  {
    if (depth == 0)
    {
      co_return 0;
    }

    co_return 1 + co_await deep_result(depth - 1);
  }

  Task nested_synchronous(State& state)
  {
    const auto depth = co_await deep_result(20000);
    assert(depth == 20000);
    co_await immediate(state);
  }

  void test_symmetric_transfer()
  {
    State state;
    auto operation = nested_synchronous(state);
    unsigned queued = 0;
    unsigned completed = 0;
    const CoroutineResumeScheduler scheduler = [&](std::coroutine_handle<>) { ++queued; };
    {
      ScopedCoroutineResumeQueue queue(scheduler, true);
      operation.start_inline([&](auto error)
      {
        assert(!error);
        ++completed;
      });
    }
    assert(queued == 0);
    assert(completed == 1);
    assert(state.resumed == 1);
    assert(!current_coroutine_symmetric_transfer_ref());
  }

  void test_direct_start(const Pool& pool, unsigned count, bool reschedule, bool asynchronous)
  {
    State state;
    Wakeup wakeup;
    std::vector<Task> operations;
    for (unsigned i = 0; i < count; ++i)
    {
      operations.emplace_back(asynchronous && i == 0 ?
        waiting(wakeup, state, 1, true) : immediate(state));
    }

    std::promise<void> completion;
    auto future = completion.get_future();
    CooperativeGroup group({operations.data(), operations.size()}, pool);
    group.start([&](auto error)
    {
      assert(bool(error) == (asynchronous && count != 0));
      assert(state.resumed == count);
      assert(++state.completed == 1);
      operations.clear();
      completion.set_value();
    }, reschedule);
    if (asynchronous && count != 0)
    {
      assert(future.wait_for(0s) == std::future_status::timeout);
      wakeup.resume();
    }
    assert(future.wait_for(10s) == std::future_status::ready);
  }

  Task wait_for_delivery(State& state, const std::atomic<bool>& delivered)
  {
    Segment segment(state);
    const auto deadline = std::chrono::steady_clock::now() + 10s;
    while (!delivered.load(std::memory_order_acquire))
    {
      assert(std::chrono::steady_clock::now() < deadline);
      std::this_thread::yield();
    }
    co_return;
  }

  Task run_group(std::vector<Task> operations, Pool pool, State& state, bool fail)
  {
    bool caught = false;
    try
    {
      co_await CooperativeGroup<Task>({operations.data(), operations.size()}, std::move(pool));
    }
    catch (const std::runtime_error&)
    {
      caught = true;
    }
    assert(caught == fail);
    operations.clear();
    assert(++state.completed == 1);
  }

  void test_wakeup_during_run(const Pool& pool)
  {
    State state;
    Wakeup wakeup;
    std::atomic<bool> delivered{false};
    std::vector<Task> operations;
    operations.emplace_back(waiting(wakeup, state, 1, false));
    operations.emplace_back(wait_for_delivery(state, delivered));
    std::thread producer([&]
    {
      wakeup.resume();
      delivered.store(true, std::memory_order_release);
    });
    sync_wait(run_group(std::move(operations), pool, state, false));
    producer.join();
    assert(state.resumed == 1);
  }

  void test_wakeups(const Pool& pool, unsigned count, unsigned rounds, bool fail,
    bool wake_after_start = false)
  {
    std::array<Wakeup, 64> wakeups;
    State state;
    std::vector<Task> operations;
    for (unsigned i = 0; i < count; ++i)
    {
      operations.emplace_back(waiting(wakeups[i], state, rounds, fail && i == 0));
    }

    std::vector<std::thread> producers;
    const auto threads = std::min(count, 8u);
    const auto start_producers = [&]
    {
      for (unsigned t = 0; t < threads; ++t)
      {
        producers.emplace_back([&, t]
        {
          for (unsigned round = 0; round < rounds; ++round)
          {
            for (unsigned i = t; i < count; i += threads)
            {
              wakeups[i].resume();
            }
          }
        });
      }
    };

    if (!wake_after_start)
    {
      start_producers();
    }

    std::promise<void> completion;
    auto future = completion.get_future();
    run_group(std::move(operations), pool, state, fail).start_detached([&](auto error)
    {
      assert(!error);
      completion.set_value();
    });

    if (wake_after_start)
    {
      assert(future.wait_for(0s) == std::future_status::timeout);
      start_producers();
    }

    assert(future.wait_for(10s) == std::future_status::ready);
    for (auto& producer : producers)
    {
      producer.join();
    }
    assert(state.resumed == count * rounds);
    assert(state.completed == 1);
  }
}

int main()
{
  test_symmetric_transfer();
  for (unsigned size : {0, 1, 2, 8, 16})
  {
    sync_wait(synchronous_group(false, size));
    if (size)
    {
      sync_wait(synchronous_group(true, size));
    }
  }

  Generics::ActiveObjectCallback_var callback(new Callback());
  for (unsigned threads : {0, 1, 4})
  {
    Pool pool;
    if (threads)
    {
      pool = std::make_shared<ExecutorPool>(
        callback, threads, ExecutorPool::ResumeStrategy::CurrentContext);
      pool->activate_object();
    }

    test_wakeup_during_run(pool);
    for (unsigned size : {0, 1, 2, 8, 257, 1024})
    {
      test_direct_start(pool, size, false, false);
      test_direct_start(pool, size, true, false);
      test_direct_start(pool, size, false, true);
      test_direct_start(pool, size, true, true);
    }
    for (unsigned i = 0; i < 40; ++i)
    {
      test_wakeups(pool, 1, 1, i % 2, (i / 2) % 2);
      test_wakeups(pool, 8, 8, i % 2, (i / 2) % 2);
    }
    test_wakeups(pool, 64, 16, false);
    test_wakeups(pool, 64, 16, true);

    if (pool)
    {
      pool->deactivate_object();
      pool->wait_object();
    }
  }
  std::cout << "CooperativeGroupTest passed; synchronous group mutex locks: 0" << std::endl;
}
