#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>

#include <Generics/TaskRunner.hpp>

#include <LogProcessing/SyncLogs/FeedRouteProcessor.hpp>
#include <LogProcessing/SyncLogs/RouteHelpers.hpp>

namespace
{
  constexpr unsigned HARD_THREADS = 5;
  constexpr unsigned SOFT_THREADS = 1;
  const Generics::Time SOFT_MAX_FILE_AGE(60);

  class Callback final :
    public Generics::ActiveObjectCallback,
    public ReferenceCounting::AtomicImpl
  {
  public:
    void
    report_error(
      Severity,
      const String::SubString& description,
      const char* = nullptr) noexcept override
    {
      std::cerr << description << std::endl;
    }

  protected:
    ~Callback() override = default;
  };

  class TaskState
  {
  public:
    void execute()
    {
      std::unique_lock lock(mutex_);
      ++started_;
      ++active_;
      max_active_ = std::max(max_active_, active_);
      condition_.notify_all();

      condition_.wait(lock, [this]() { return released_; });

      --active_;
      ++completed_;
      condition_.notify_all();
    }

    bool wait_started(unsigned expected, std::chrono::milliseconds timeout)
    {
      std::unique_lock lock(mutex_);
      return condition_.wait_for(
        lock,
        timeout,
        [this, expected]() { return started_ >= expected; });
    }

    bool
    wait_completed(unsigned expected, std::chrono::milliseconds timeout)
    {
      std::unique_lock lock(mutex_);
      return condition_.wait_for(
        lock,
        timeout,
        [this, expected]() { return completed_ >= expected; });
    }

    void
    release()
    {
      std::lock_guard lock(mutex_);
      released_ = true;
      condition_.notify_all();
    }

    unsigned
    started() const
    {
      std::lock_guard lock(mutex_);
      return started_;
    }

    unsigned
    max_active() const
    {
      std::lock_guard lock(mutex_);
      return max_active_;
    }

  private:
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    unsigned started_ = 0;
    unsigned completed_ = 0;
    unsigned active_ = 0;
    unsigned max_active_ = 0;
    bool released_ = false;
  };

  class BlockingTask final : public Generics::TaskImpl
  {
  public:
    explicit BlockingTask(TaskState& state)
      : state_(state)
    {}

    void
    execute() noexcept override
    {
      state_.execute();
    }

  protected:
    ~BlockingTask() override = default;

  private:
    TaskState& state_;
  };

  class FixedRouteHelper final:
    public AdServer::LogProcessing::RouteBasicHelper,
    public ReferenceCounting::AtomicImpl
  {
  public:
    explicit FixedRouteHelper(unsigned long host_check_period)
      : RouteBasicHelper(
          AdServer::LogProcessing::ST_DEFINITEHASH,
          host_check_period)
    {}

    std::string
    get_dest_host(const char*) override
    {
      return "destination";
    }

  protected:
    ~FixedRouteHelper() noexcept override = default;
  };

  class Fixture
  {
  public:
    Fixture()
      : callback_(new Callback),
        task_runner_(new Generics::TaskRunner(callback_, HARD_THREADS, 0, 0, HARD_THREADS)),
        scheduler_(new AdServer::LogProcessing::MoveTaskScheduler(
          task_runner_,
          HARD_THREADS,
          SOFT_THREADS,
          SOFT_MAX_FILE_AGE))
    {
      task_runner_->activate_object();
    }

    ~Fixture()
    {
      task_runner_->deactivate_object();
      task_runner_->wait_object();
    }

    void
    enqueue(TaskState& state, const Generics::Time& modification_time)
    {
      scheduler_->enqueue_task(Generics::Task_var(new BlockingTask(state)), modification_time);
    }

  private:
    Generics::ActiveObjectCallback_var callback_;
    Generics::TaskRunner_var task_runner_;
    AdServer::LogProcessing::MoveTaskScheduler_var scheduler_;
  };

  bool
  test_fresh_files_use_soft_limit()
  {
    Fixture fixture;
    TaskState state;
    const auto now = Generics::Time::get_time_of_day();

    for (unsigned i = 0; i < HARD_THREADS; ++i)
    {
      fixture.enqueue(state, now);
    }

    const bool first_started = state.wait_started(1, std::chrono::seconds(2));
    const bool second_did_not_start = !state.wait_started(2, std::chrono::milliseconds(200));
    state.release();
    const bool all_completed = state.wait_completed(HARD_THREADS, std::chrono::seconds(2));

    return first_started && second_did_not_start && all_completed &&
      state.max_active() == SOFT_THREADS;
  }

  bool test_overdue_files_use_hard_limit()
  {
    Fixture fixture;
    TaskState state;
    const auto old_time = Generics::Time::get_time_of_day() - Generics::Time(120);

    for (unsigned i = 0; i < HARD_THREADS; ++i)
    {
      fixture.enqueue(state, old_time);
    }

    const bool all_started = state.wait_started(HARD_THREADS, std::chrono::seconds(2));
    state.release();
    const bool all_completed = state.wait_completed(HARD_THREADS, std::chrono::seconds(2));

    return all_started && all_completed && state.max_active() == HARD_THREADS;
  }

  bool
  test_scheduler_returns_to_soft_limit()
  {
    Fixture fixture;
    TaskState fresh_state;
    TaskState overdue_state;
    const auto now = Generics::Time::get_time_of_day();
    const auto old_time = now - Generics::Time(120);

    for (unsigned i = 0; i < 3; ++i)
    {
      fixture.enqueue(fresh_state, now);
    }

    const bool first_fresh_started = fresh_state.wait_started(1, std::chrono::seconds(2));

    for (unsigned i = 0; i < 4; ++i)
    {
      fixture.enqueue(overdue_state, old_time);
    }

    const bool all_overdue_started = overdue_state.wait_started(4, std::chrono::seconds(2));
    overdue_state.release();
    const bool all_overdue_completed = overdue_state.wait_completed(4, std::chrono::seconds(2));
    const bool next_fresh_waits_for_soft_slot =
      !fresh_state.wait_started(2, std::chrono::milliseconds(200));

    fresh_state.release();
    const bool all_fresh_completed = fresh_state.wait_completed(3, std::chrono::seconds(2));

    return first_fresh_started && all_overdue_started &&
      all_overdue_completed && next_fresh_waits_for_soft_slot &&
      all_fresh_completed && fresh_state.max_active() == SOFT_THREADS;
  }

  bool
  test_destination_host_backoff()
  {
    using AdServer::LogProcessing::RouteBasicHelper;
    using AdServer::LogProcessing::RouteBasicHelper_var;

    RouteBasicHelper_var delayed_route_helper = new FixedRouteHelper(60);
    const auto delayed_failure = delayed_route_helper->get_destination("file");
    delayed_route_helper->bad_host(delayed_failure);

    try
    {
      delayed_route_helper->get_destination("file");
      return false;
    }
    catch (const RouteBasicHelper::NotReady&)
    {}

    RouteBasicHelper_var route_helper = new FixedRouteHelper(0);
    const auto failed = route_helper->get_destination("file");
    const auto stale = route_helper->get_destination("file");

    route_helper->bad_host(failed);
    const auto failed_probe = route_helper->get_destination("file");
    if (!failed_probe.probe)
    {
      return false;
    }
    route_helper->bad_host(failed_probe);

    const auto successful_probe = route_helper->get_destination("file");
    if (!successful_probe.probe)
    {
      return false;
    }

    try
    {
      route_helper->get_destination("file");
      return false;
    }
    catch (const RouteBasicHelper::NotReady&)
    {}

    route_helper->good_host(successful_probe);
    route_helper->bad_host(stale);
    return !route_helper->get_destination("file").probe;
  }
}

int
main()
{
  bool success = true;

  const auto run = [&success](const char* name, bool (*test)())
  {
    const bool result = test();
    std::cout << name << ": " << (result ? "PASS" : "FAIL") << std::endl;
    success &= result;
  };

  run("fresh files use soft limit", test_fresh_files_use_soft_limit);
  run("overdue files use hard limit", test_overdue_files_use_hard_limit);
  run("scheduler returns to soft limit", test_scheduler_returns_to_soft_limit);
  run("destination host backoff", test_destination_host_backoff);

  return success ? 0 : 1;
}
