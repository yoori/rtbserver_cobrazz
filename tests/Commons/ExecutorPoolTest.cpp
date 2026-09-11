#include <chrono>
#include <condition_variable>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>

#include <Commons/ExecutorPool.hpp>

namespace
{
  class NullActiveObjectCallback final:
    public virtual Generics::ActiveObjectCallback,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    void
    report_error(Severity, const String::SubString&, const char* = nullptr) noexcept override
    {}

  protected:
    ~NullActiveObjectCallback() noexcept override = default;
  };
}

int
main()
{
  Generics::ActiveObjectCallback_var callback(new NullActiveObjectCallback());
  auto executor_pool = std::make_shared<AdServer::Commons::ExecutorPool>(
    callback,
    4,
    AdServer::Commons::ExecutorPool::ResumeStrategy::CurrentContext,
    "executor-test",
    4);
  executor_pool->activate_object();

  std::mutex mutex;
  std::condition_variable condition;
  bool blocker_started = false;
  bool release_blocker = false;
  executor_pool->post(
    [&]()
    {
      std::unique_lock lock(mutex);
      blocker_started = true;
      condition.notify_all();
      condition.wait(lock, [&]() { return release_blocker; });
    });

  bool blocker_started_in_time = false;
  {
    std::unique_lock lock(mutex);
    blocker_started_in_time = condition.wait_for(
      lock,
      std::chrono::seconds(1),
      [&]() { return blocker_started; });
    if (!blocker_started_in_time)
    {
      release_blocker = true;
    }
  }

  if (!blocker_started_in_time)
  {
    condition.notify_all();
    executor_pool->deactivate_object();
    executor_pool->wait_object();
    std::cerr << "ExecutorPoolTest: blocking task didn't start\n";
    return 1;
  }

  executor_pool->post([]() {});
  executor_pool->post([]() {});
  executor_pool->post([]() {});

  std::promise<void> completed;
  auto completed_future = completed.get_future();
  executor_pool->post([&completed]() { completed.set_value(); });

  const bool completed_while_worker_blocked =
    completed_future.wait_for(std::chrono::seconds(1)) == std::future_status::ready;

  {
    std::lock_guard lock(mutex);
    release_blocker = true;
  }
  condition.notify_all();

  executor_pool->deactivate_object();
  executor_pool->wait_object();

  if (!completed_while_worker_blocked)
  {
    std::cerr << "ExecutorPoolTest: a blocked worker stalled its context\n";
    return 1;
  }

  std::cout << "ExecutorPoolTest passed\n";
  return 0;
}
