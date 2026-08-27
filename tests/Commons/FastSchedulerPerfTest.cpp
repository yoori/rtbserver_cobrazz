#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

#include <sys/resource.h>

#include <boost/asio.hpp>

#include <Generics/AppUtils.hpp>

#include <Commons/FastScheduler.hpp>

namespace
{
  using Clock = std::chrono::steady_clock;
  using TimePoint = Clock::time_point;

  struct Options
  {
    std::size_t count = 100000;
    std::size_t threads = 16;
    std::size_t custom_threads = 16;
    std::size_t io_threads = 16;
    std::size_t steps = 3;
    std::size_t step_us = 1000;
    std::size_t start_delay_us = 0;
  };

  struct CpuTimes
  {
    double user = 0.0;
    double system = 0.0;
  };

  struct LatenessStats
  {
    double average_us = 0.0;
    double p50_us = 0.0;
    double p90_us = 0.0;
    double p99_us = 0.0;
    double max_us = 0.0;
  };

  struct BucketStats
  {
    std::size_t count = 0;
    double average_items = 0.0;
    std::size_t max_items = 0;
    LatenessStats initial_callback_lateness;
    LatenessStats first_callback_lateness;
    LatenessStats processing_span;
  };

  struct MeasureResult
  {
    double setup_elapsed = 0.0;
    double setup_user_cpu = 0.0;
    double setup_system_cpu = 0.0;
    double enqueue_elapsed = 0.0;
    double enqueue_user_cpu = 0.0;
    double enqueue_system_cpu = 0.0;
    double total_elapsed = 0.0;
    double total_user_cpu = 0.0;
    double total_system_cpu = 0.0;
    LatenessStats first_lateness;
    LatenessStats final_lateness;
    BucketStats bucket_stats;
  };

  CpuTimes
  operator-(const CpuTimes& lhs, const CpuTimes& rhs) noexcept
  {
    return {lhs.user - rhs.user, lhs.system - rhs.system};
  }

  CpuTimes
  current_cpu_times()
  {
    rusage usage{};
    if (getrusage(RUSAGE_SELF, &usage) != 0)
    {
      throw std::runtime_error("getrusage failed");
    }

    return {
      usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1000000.0,
      usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / 1000000.0
    };
  }

  double
  elapsed_seconds(const TimePoint begin, const TimePoint end) noexcept
  {
    return std::chrono::duration<double>(end - begin).count();
  }

  void
  print_usage()
  {
    std::cerr
      << "Usage: FastSchedulerPerfTest [OPTIONS]\n"
      << "Options:\n"
      << "  --count <N>       timer count (default: 100000)\n"
      << "  --threads <N>     producer threads (default: 16)\n"
      << "  --custom-threads <N> custom scheduler threads (default: 16)\n"
      << "  --io-threads <N>  asio io_context count (default: 16)\n"
      << "  --steps <N>       callbacks per timer (default: 3)\n"
      << "  --step-us <N>     interval between callbacks (default: 1000)\n"
      << "  --start-delay-us <N> common first deadline delay (default: 0)\n";
  }

  Options
  parse_options(int argc, char** argv)
  {
    using namespace Generics::AppUtils;

    Option<unsigned long> count(100000);
    Option<unsigned long> threads(16);
    Option<unsigned long> custom_threads(16);
    Option<unsigned long> io_threads(16);
    Option<unsigned long> steps(3);
    Option<unsigned long> step_us(1000);
    Option<unsigned long> start_delay_us(0);
    CheckOption help;

    Args args(-1);
    args.add(equal_name("count"), count);
    args.add(equal_name("threads"), threads);
    args.add(equal_name("custom-threads"), custom_threads);
    args.add(equal_name("io-threads"), io_threads);
    args.add(equal_name("steps"), steps);
    args.add(equal_name("step-us"), step_us);
    args.add(equal_name("start-delay-us"), start_delay_us);
    args.add(equal_name("help") || short_name("h"), help);
    args.parse(argc - 1, argv + 1);

    if (help.enabled())
    {
      print_usage();
      std::exit(0);
    }

    Options result{
      *count,
      *threads,
      *custom_threads,
      *io_threads,
      *steps,
      *step_us,
      *start_delay_us
    };

    if (!result.count || !result.threads || !result.custom_threads ||
      !result.io_threads || !result.steps || !result.step_us)
    {
      throw std::runtime_error("all numeric options must be greater than zero");
    }
    return result;
  }

  class CompletionCounter final
  {
  public:
    explicit CompletionCounter(const std::size_t target) noexcept
      : target_(target)
    {}

    void complete() noexcept
    {
      if (completed_.fetch_add(1, std::memory_order_acq_rel) + 1 == target_)
      {
        std::lock_guard<std::mutex> lock(lock_);
        condition_.notify_one();
      }
    }

    void wait()
    {
      std::unique_lock<std::mutex> lock(lock_);
      if (!condition_.wait_for(
        lock,
        std::chrono::minutes(2),
        [this]() noexcept
        {
          return completed_.load(std::memory_order_acquire) == target_;
        }))
      {
        throw std::runtime_error("timer completion timeout");
      }
    }

  private:
    const std::size_t target_;
    std::atomic<std::size_t> completed_{0};
    std::mutex lock_;
    std::condition_variable condition_;
  };

  class CompletionShard final
  {
  public:
    CompletionShard(CompletionCounter& completion, const std::size_t count) noexcept
      : completion_(completion),
        remaining_(count)
    {}

    void complete() noexcept
    {
      if (remaining_.fetch_sub(1, std::memory_order_acq_rel) == 1)
      {
        completion_.complete();
      }
    }

  private:
    CompletionCounter& completion_;
    std::atomic<std::size_t> remaining_;
  };

  std::int64_t
  lateness_ns(const TimePoint now, const TimePoint deadline) noexcept
  {
    return now > deadline ?
      std::chrono::duration_cast<std::chrono::nanoseconds>(now - deadline).count() :
      0;
  }

  std::int64_t
  lateness_ns(const Generics::Time& now, const Generics::Time& deadline) noexcept
  {
    return now > deadline ? (now - deadline).microseconds() * 1000 : 0;
  }

  Generics::Time
  time_from_microseconds(const std::uint64_t value) noexcept
  {
    return Generics::Time(value / Generics::Time::USEC_MAX, value % Generics::Time::USEC_MAX);
  }

  LatenessStats
  calculate_lateness_stats(std::vector<std::int64_t> values)
  {
    long double total = 0;
    for (const auto value : values)
    {
      total += value;
    }
    std::sort(values.begin(), values.end());

    const auto percentile = [&](const std::size_t value) noexcept
    {
      return static_cast<double>(values[(values.size() - 1) * value / 100]) / 1000.0;
    };
    return {
      static_cast<double>(total / values.size()) / 1000.0,
      percentile(50),
      percentile(90),
      percentile(99),
      static_cast<double>(values.back()) / 1000.0
    };
  }

  template<typename State, typename Getter>
  LatenessStats
  calculate_lateness_stats(const std::vector<std::shared_ptr<State>>& states, Getter getter)
  {
    std::vector<std::int64_t> values;
    values.reserve(states.size());
    for (const auto& state : states)
    {
      values.emplace_back(getter(*state));
    }
    return calculate_lateness_stats(std::move(values));
  }

  template<typename Fun>
  std::pair<double, CpuTimes>
  run_producers(const std::size_t threads, const std::size_t count, Fun fun)
  {
    std::mutex start_lock;
    std::condition_variable ready_condition;
    std::condition_variable start_condition;
    std::size_t ready = 0;
    bool start = false;
    std::vector<std::thread> producers;
    producers.reserve(threads);

    std::size_t begin = 0;
    for (std::size_t thread_index = 0; thread_index < threads; ++thread_index)
    {
      const std::size_t thread_count = count / threads + (thread_index < count % threads ? 1 : 0);
      const std::size_t end = begin + thread_count;
      producers.emplace_back(
        [&, begin, end]()
        {
          {
            std::unique_lock<std::mutex> lock(start_lock);
            ++ready;
            ready_condition.notify_one();
            start_condition.wait(lock, [&]() noexcept { return start; });
          }

          for (std::size_t index = begin; index < end; ++index)
          {
            fun(index);
          }
        });
      begin = end;
    }

    {
      std::unique_lock<std::mutex> lock(start_lock);
      ready_condition.wait(lock, [&]() noexcept { return ready == threads; });
    }

    const auto cpu_started = current_cpu_times();
    const auto started = Clock::now();

    {
      std::lock_guard<std::mutex> lock(start_lock);
      start = true;
    }
    start_condition.notify_all();

    for (auto& producer : producers)
    {
      producer.join();
    }

    const auto finished = Clock::now();
    const auto cpu_finished = current_cpu_times();

    return {elapsed_seconds(started, finished), cpu_finished - cpu_started};
  }

  struct SchedulerState final
  {
    using Scheduler = AdServer::Commons::FastScheduler;

    SchedulerState(
      Scheduler& scheduler_val,
      CompletionShard& completion_val,
      const std::size_t steps_val,
      const Generics::Time& step_val) noexcept
      : scheduler(scheduler_val),
        completion(completion_val),
        steps(steps_val),
        step(step_val)
    {}

    static std::optional<Generics::Time> on_timer(void* owner) noexcept
    {
      auto& state = *static_cast<SchedulerState*>(owner);
      const auto deadline = state.first_deadline +
        state.step * static_cast<int>(state.completed_steps);
      const auto callback_at = Generics::Time::get_time_of_day();
      const auto callback_lateness = lateness_ns(callback_at, deadline);
      if (state.completed_steps++ == 0)
      {
        state.first_callback_at = callback_at;
        state.first_lateness_ns = callback_lateness;
      }

      if (state.completed_steps < state.steps)
      {
        return state.first_deadline + state.step * static_cast<int>(state.completed_steps);
      }

      state.final_lateness_ns = callback_lateness;
      state.completion.complete();
      return std::nullopt;
    }

    Scheduler::Task task;
    Scheduler& scheduler;
    CompletionShard& completion;
    const std::size_t steps;
    const Generics::Time step;
    Generics::Time first_deadline;
    Generics::Time first_callback_at;
    std::size_t completed_steps = 0;
    std::int64_t first_lateness_ns = 0;
    std::int64_t final_lateness_ns = 0;
  };

  BucketStats
  calculate_bucket_stats(const std::vector<std::shared_ptr<SchedulerState>>& states)
  {
    struct Record
    {
      std::int64_t deadline_ms;
      std::int64_t callback_ns;
    };

    std::vector<Record> records;
    records.reserve(states.size());
    for (const auto& state : states)
    {
      const auto deadline_us = state->first_deadline.microseconds();
      auto deadline_ms = deadline_us / 1000;
      if (deadline_us % 1000 != 0)
      {
        ++deadline_ms;
      }
      records.emplace_back(Record{ deadline_ms, state->first_callback_at.microseconds() * 1000 });
    }
    std::sort(
      records.begin(),
      records.end(),
      [](const Record& lhs, const Record& rhs) noexcept
      {
        return lhs.deadline_ms < rhs.deadline_ms;
      });

    std::vector<std::int64_t> first_lateness;
    std::vector<std::int64_t> initial_lateness;
    std::vector<std::int64_t> processing_spans;
    std::size_t total_items = 0;
    std::size_t max_items = 0;

    for (std::size_t begin = 0; begin < records.size();)
    {
      std::size_t end = begin + 1;
      auto first_callback = records[begin].callback_ns;
      auto last_callback = first_callback;
      while (end < records.size() && records[end].deadline_ms == records[begin].deadline_ms)
      {
        first_callback = std::min(first_callback, records[end].callback_ns);
        last_callback = std::max(last_callback, records[end].callback_ns);
        ++end;
      }

      const auto items = end - begin;
      total_items += items;
      max_items = std::max(max_items, items);
      const auto bucket_lateness = std::max<std::int64_t>(
        0,
        first_callback - records[begin].deadline_ms * 1000000);
      first_lateness.emplace_back(bucket_lateness);
      if (begin == 0)
      {
        initial_lateness.emplace_back(bucket_lateness);
      }
      processing_spans.emplace_back(last_callback - first_callback);
      begin = end;
    }

    return {
      first_lateness.size(),
      static_cast<double>(total_items) / first_lateness.size(),
      max_items,
      calculate_lateness_stats(std::move(initial_lateness)),
      calculate_lateness_stats(std::move(first_lateness)),
      calculate_lateness_stats(std::move(processing_spans))
    };
  }

  MeasureResult
  run_scheduler(const Options& options, const std::size_t scheduler_threads)
  {
    AdServer::Commons::FastScheduler scheduler(scheduler_threads);
    const auto active_threads = std::min(options.count, scheduler_threads);
    CompletionCounter completion(active_threads);
    std::vector<std::unique_ptr<CompletionShard>> completion_shards;
    completion_shards.reserve(active_threads);
    for (std::size_t i = 0; i < active_threads; ++i)
    {
      completion_shards.emplace_back(std::make_unique<CompletionShard>(
        completion,
        options.count / active_threads + (i < options.count % active_threads ? 1 : 0)));
    }
    const auto step = time_from_microseconds(options.step_us);

    const auto total_cpu_started = current_cpu_times();
    const auto total_started = Clock::now();
    std::vector<std::shared_ptr<SchedulerState>> states;
    states.reserve(options.count);
    for (std::size_t i = 0; i < options.count; ++i)
    {
      const auto scheduler_index = i % active_threads;
      states.emplace_back(std::make_shared<SchedulerState>(
        scheduler,
        *completion_shards[scheduler_index],
        options.steps,
        step));
    }
    const auto setup_finished = Clock::now();
    const auto scheduler_setup_finished = Generics::Time::get_time_of_day();
    const auto setup_cpu_finished = current_cpu_times();

    const auto [enqueue_elapsed, enqueue_cpu] = run_producers(
      options.threads,
      options.count,
      [&](const std::size_t index)
      {
        auto& state = states[index];
        state->first_deadline = options.start_delay_us ?
          scheduler_setup_finished + time_from_microseconds(options.start_delay_us) :
          Generics::Time::get_time_of_day() + step;
        state->scheduler.schedule(
          state->task,
          state->first_deadline,
          state,
          &SchedulerState::on_timer);
      });
    completion.wait();

    const auto total_finished = Clock::now();
    const auto total_cpu_finished = current_cpu_times();
    const auto setup_cpu = setup_cpu_finished - total_cpu_started;
    const auto total_cpu = total_cpu_finished - total_cpu_started;
    const auto first_lateness = calculate_lateness_stats(
      states,
      [](const SchedulerState& state) noexcept
      {
        return state.first_lateness_ns;
      });
    const auto final_lateness = calculate_lateness_stats(
      states,
      [](const SchedulerState& state) noexcept
      {
        return state.final_lateness_ns;
      });
    const auto bucket_stats = calculate_bucket_stats(states);
    return {
      elapsed_seconds(total_started, setup_finished),
      setup_cpu.user,
      setup_cpu.system,
      enqueue_elapsed,
      enqueue_cpu.user,
      enqueue_cpu.system,
      elapsed_seconds(total_started, total_finished),
      total_cpu.user,
      total_cpu.system,
      first_lateness,
      final_lateness,
      bucket_stats
    };
  }

  class IoContextPool final
  {
  public:
    using IoContext = boost::asio::io_context;
    using WorkGuard = boost::asio::executor_work_guard<IoContext::executor_type>;

    explicit IoContextPool(const std::size_t size)
    {
      contexts_.reserve(size);
      guards_.reserve(size);
      threads_.reserve(size);
      for (std::size_t i = 0; i < size; ++i)
      {
        contexts_.emplace_back(std::make_unique<IoContext>());
        guards_.emplace_back(std::make_unique<WorkGuard>(contexts_.back()->get_executor()));
      }

      for (auto& context : contexts_)
      {
        auto* io_context = context.get();
        threads_.emplace_back([io_context]() { io_context->run(); });
      }
    }

    ~IoContextPool()
    {
      guards_.clear();
      for (auto& thread : threads_)
      {
        thread.join();
      }
    }

    IoContext& io_service(const std::size_t index) noexcept
    {
      return *contexts_[index];
    }

  private:
    std::vector<std::unique_ptr<IoContext>> contexts_;
    std::vector<std::unique_ptr<WorkGuard>> guards_;
    std::vector<std::thread> threads_;
  };

  struct AsioState final:
    public std::enable_shared_from_this<AsioState>
  {
    AsioState(
      IoContextPool::IoContext& io_context,
      CompletionShard& completion_val,
      const std::size_t steps_val,
      const std::chrono::microseconds step_val)
      : timer(io_context),
        completion(completion_val),
        steps(steps_val),
        step(step_val)
    {}

    void schedule(const TimePoint common_deadline)
    {
      first_deadline = common_deadline == TimePoint{} ? Clock::now() + step : common_deadline;
      timer.expires_at(first_deadline);
      async_wait_();
    }

    void async_wait_()
    {
      timer.async_wait(
        [self = shared_from_this()](const boost::system::error_code& error)
        {
          self->on_timer_(error);
        });
    }

    void on_timer_(const boost::system::error_code& error)
    {
      const auto deadline = first_deadline + step * completed_steps;
      const auto callback_lateness = lateness_ns(Clock::now(), deadline);
      if (completed_steps++ == 0)
      {
        first_lateness_ns = callback_lateness;
      }

      if (!error && completed_steps < steps)
      {
        timer.expires_at(first_deadline + step * completed_steps);
        async_wait_();
        return;
      }

      final_lateness_ns = callback_lateness;
      completion.complete();
    }

    boost::asio::steady_timer timer;
    CompletionShard& completion;
    const std::size_t steps;
    const std::chrono::microseconds step;
    TimePoint first_deadline;
    std::size_t completed_steps = 0;
    std::int64_t first_lateness_ns = 0;
    std::int64_t final_lateness_ns = 0;
  };

  MeasureResult
  run_asio(const Options& options)
  {
    IoContextPool io_contexts(options.io_threads);
    const auto active_threads = std::min(options.count, options.io_threads);
    CompletionCounter completion(active_threads);
    std::vector<std::unique_ptr<CompletionShard>> completion_shards;
    completion_shards.reserve(active_threads);
    for (std::size_t i = 0; i < active_threads; ++i)
    {
      completion_shards.emplace_back(std::make_unique<CompletionShard>(
        completion,
        options.count / active_threads + (i < options.count % active_threads ? 1 : 0)));
    }
    const auto step = std::chrono::microseconds(options.step_us);

    const auto total_cpu_started = current_cpu_times();
    const auto total_started = Clock::now();
    std::vector<std::shared_ptr<AsioState>> states;
    states.reserve(options.count);
    for (std::size_t i = 0; i < options.count; ++i)
    {
      const auto io_index = i % active_threads;
      states.emplace_back(std::make_shared<AsioState>(
        io_contexts.io_service(io_index),
        *completion_shards[io_index],
        options.steps,
        step));
    }
    const auto setup_finished = Clock::now();
    const auto setup_cpu_finished = current_cpu_times();

    const auto [enqueue_elapsed, enqueue_cpu] = run_producers(
      options.threads,
      options.count,
      [&](const std::size_t index)
      {
        states[index]->schedule(
          options.start_delay_us ?
            setup_finished +
              std::chrono::microseconds(options.start_delay_us) :
            TimePoint{});
      });
    completion.wait();

    const auto total_finished = Clock::now();
    const auto total_cpu_finished = current_cpu_times();
    const auto setup_cpu = setup_cpu_finished - total_cpu_started;
    const auto total_cpu = total_cpu_finished - total_cpu_started;
    const auto first_lateness = calculate_lateness_stats(
      states,
      [](const AsioState& state) noexcept
      {
        return state.first_lateness_ns;
      });
    const auto final_lateness = calculate_lateness_stats(
      states,
      [](const AsioState& state) noexcept
      {
        return state.final_lateness_ns;
      });

    return {
      elapsed_seconds(total_started, setup_finished),
      setup_cpu.user,
      setup_cpu.system,
      enqueue_elapsed,
      enqueue_cpu.user,
      enqueue_cpu.system,
      elapsed_seconds(total_started, total_finished),
      total_cpu.user,
      total_cpu.system,
      first_lateness,
      final_lateness,
      {}
    };
  }

  void
  print_result(const char* name, const MeasureResult& result, const std::size_t callbacks)
  {
    const double setup_cpu = result.setup_user_cpu + result.setup_system_cpu;
    const double enqueue_cpu = result.enqueue_user_cpu + result.enqueue_system_cpu;
    const double total_cpu = result.total_user_cpu + result.total_system_cpu;
    const double run_cpu = total_cpu - setup_cpu;

    std::cout
      << name << ":\n"
      << "  setup_elapsed_sec=" << result.setup_elapsed << '\n'
      << "  setup_cpu_sec=" << setup_cpu << '\n'
      << "  enqueue_elapsed_sec=" << result.enqueue_elapsed << '\n'
      << "  enqueue_cpu_sec=" << enqueue_cpu << '\n'
      << "  total_elapsed_sec=" << result.total_elapsed << '\n'
      << "  total_cpu_sec=" << total_cpu << '\n'
      << "  run_cpu_sec=" << run_cpu << '\n'
      << "  callbacks_per_cpu_sec="
      << static_cast<double>(callbacks) / run_cpu << '\n'
      << "  first_lateness_us="
      << "avg:" << result.first_lateness.average_us
      << " p50:" << result.first_lateness.p50_us
      << " p90:" << result.first_lateness.p90_us
      << " p99:" << result.first_lateness.p99_us
      << " max:" << result.first_lateness.max_us << '\n'
      << "  final_lateness_us="
      << "avg:" << result.final_lateness.average_us
      << " p50:" << result.final_lateness.p50_us
      << " p90:" << result.final_lateness.p90_us
      << " p99:" << result.final_lateness.p99_us << " max:" << result.final_lateness.max_us << '\n';
    if (result.bucket_stats.count)
    {
      std::cout
        << "  buckets=" << result.bucket_stats.count
        << " avg_items:" << result.bucket_stats.average_items
        << " max_items:" << result.bucket_stats.max_items << '\n'
        << "  initial_bucket_lateness_us="
        << "avg:" << result.bucket_stats.initial_callback_lateness.average_us
        << " p50:" << result.bucket_stats.initial_callback_lateness.p50_us
        << " p99:" << result.bucket_stats.initial_callback_lateness.p99_us
        << " max:" << result.bucket_stats.initial_callback_lateness.max_us << '\n'
        << "  bucket_first_lateness_us="
        << "avg:" << result.bucket_stats.first_callback_lateness.average_us
        << " p50:" << result.bucket_stats.first_callback_lateness.p50_us
        << " p99:" << result.bucket_stats.first_callback_lateness.p99_us
        << " max:" << result.bucket_stats.first_callback_lateness.max_us << '\n'
        << "  bucket_processing_span_us="
        << "avg:" << result.bucket_stats.processing_span.average_us
        << " p50:" << result.bucket_stats.processing_span.p50_us
        << " p99:" << result.bucket_stats.processing_span.p99_us
        << " max:" << result.bucket_stats.processing_span.max_us << '\n';
    }
  }
}

int
main(int argc, char** argv)
{
  try
  {
    const auto options = parse_options(argc, argv);
    std::cout << std::fixed << std::setprecision(6)
      << "count=" << options.count << '\n'
      << "threads=" << options.threads << '\n'
      << "custom_threads=" << options.custom_threads << '\n'
      << "io_threads=" << options.io_threads << '\n'
      << "steps=" << options.steps << '\n'
      << "step_us=" << options.step_us << '\n'
      << "start_delay_us=" << options.start_delay_us << '\n';

    const auto scheduler_result = run_scheduler(options, 1);
    const auto scheduler_pool_result = run_scheduler(options, options.custom_threads);
    const auto asio_result = run_asio(options);
    const auto callbacks = options.count * options.steps;
    print_result("fast_scheduler", scheduler_result, callbacks);
    print_result("worker_custom_scheduler", scheduler_pool_result, callbacks);
    print_result("asio_steady_timer", asio_result, callbacks);

    const double scheduler_run_cpu =
      scheduler_result.total_user_cpu + scheduler_result.total_system_cpu -
      scheduler_result.setup_user_cpu - scheduler_result.setup_system_cpu;
    const double asio_run_cpu =
      asio_result.total_user_cpu + asio_result.total_system_cpu -
      asio_result.setup_user_cpu - asio_result.setup_system_cpu;
    const double scheduler_pool_run_cpu =
      scheduler_pool_result.total_user_cpu +
      scheduler_pool_result.total_system_cpu -
      scheduler_pool_result.setup_user_cpu -
      scheduler_pool_result.setup_system_cpu;
    std::cout << "scheduler_pool_to_single_run_cpu_ratio="
      << scheduler_pool_run_cpu / scheduler_run_cpu << '\n';
    std::cout << "asio_to_scheduler_run_cpu_ratio="
      << asio_run_cpu / scheduler_run_cpu << '\n'
      << "asio_to_scheduler_pool_run_cpu_ratio="
      << asio_run_cpu / scheduler_pool_run_cpu << std::endl;
    return 0;
  }
  catch (const std::exception& ex)
  {
    std::cerr << "error: " << ex.what() << std::endl;
    return 1;
  }
}
