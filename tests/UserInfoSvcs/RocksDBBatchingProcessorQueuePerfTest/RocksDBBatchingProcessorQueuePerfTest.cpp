#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <sys/resource.h>

#include <Generics/AppUtils.hpp>
#include <ProfilingCommons/ProfileMap/RocksDBBatchingProcessorQueue.hpp>

#include "SimpleRocksDBBatchingProcessorQueue.hpp"

namespace
{
  using BucketQueue = AdServer::ProfilingCommons::RocksDBBatchingProcessorQueue;
  using SimpleQueue = AdServer::ProfilingCommons::Test::SimpleRocksDBBatchingProcessorQueue;

  enum class Mode
  {
    READ,
    WRITE,
    READ_WRITE
  };

  struct Options final
  {
    std::string implementation;
    std::uint64_t count = 0;
    std::size_t threads = 1;
    std::size_t batching_threads = 16;
    std::size_t batch_size = 128;
    std::size_t enqueue_buckets = 32;
    std::size_t key_count = 0;
    unsigned long max_delay_us = 0;
    unsigned long process_delay_us = 20;
    Mode mode = Mode::READ_WRITE;
  };

  struct CpuTimes final
  {
    double user = 0.0;
    double sys = 0.0;
  };

  class StartBarrier final
  {
  public:
    explicit StartBarrier(std::size_t participants)
      : participants_(participants)
    {}

    void
    arrive_and_wait()
    {
      std::unique_lock guard(lock_);
      if (++arrived_ == participants_)
      {
        released_ = true;
        condition_.notify_all();
        return;
      }
      condition_.wait(guard, [this]() { return released_; });
    }

  private:
    const std::size_t participants_;
    std::size_t arrived_ = 0;
    bool released_ = false;
    std::mutex lock_;
    std::condition_variable condition_;
  };

  const char*
  mode_name(Mode mode) noexcept
  {
    switch(mode)
    {
      case Mode::READ:
        return "read";
      case Mode::WRITE:
        return "write";
      case Mode::READ_WRITE:
        return "read-write";
    }
    return "unknown";
  }

  void
  print_usage()
  {
    std::cerr
      << "Usage: RocksDBBatchingProcessorQueuePerfTest "
      << "--implementation <simple|bucket> --count <N> [OPTIONS]\n"
      << "Options:\n"
      << "  --threads <N>           producer threads count (default: 1)\n"
      << "  --batching-threads <N>  consumer threads count (default: 16)\n"
      << "  --batch-size <N>        unique keys per batch (default: 128)\n"
      << "  --enqueue-buckets <N>   producer buckets for bucket queue (default: 32)\n"
      << "  --key-count <N>         distinct keys count (default: --count)\n"
      << "  --max-delay-us <N>      batch delay in microseconds (default: 0)\n"
      << "  --process-delay-us <N>  batch processing delay in microseconds (default: 20)\n"
      << "  --mode <MODE>           read, write, or read-write (default: read-write)\n";
  }

  Options
  parse_options(int argc, char** argv)
  {
    using namespace Generics::AppUtils;

    StringOption opt_implementation;
    Option<unsigned long> opt_count(0);
    Option<unsigned long> opt_threads(1);
    Option<unsigned long> opt_batching_threads(16);
    Option<unsigned long> opt_batch_size(128);
    Option<unsigned long> opt_enqueue_buckets(32);
    Option<unsigned long> opt_key_count(0);
    Option<unsigned long> opt_max_delay_us(0);
    Option<unsigned long> opt_process_delay_us(20);
    StringOption opt_mode("read-write");
    CheckOption opt_help;

    Args args(-1);
    args.add(equal_name("implementation"), opt_implementation);
    args.add(equal_name("count"), opt_count);
    args.add(equal_name("threads"), opt_threads);
    args.add(equal_name("batching-threads"), opt_batching_threads);
    args.add(equal_name("batch-size"), opt_batch_size);
    args.add(equal_name("enqueue-buckets"), opt_enqueue_buckets);
    args.add(equal_name("key-count"), opt_key_count);
    args.add(equal_name("max-delay-us"), opt_max_delay_us);
    args.add(equal_name("process-delay-us"), opt_process_delay_us);
    args.add(equal_name("mode"), opt_mode);
    args.add(equal_name("help") || short_name("h"), opt_help);
    args.parse(argc - 1, argv + 1);

    if (opt_help.enabled())
    {
      print_usage();
      std::exit(0);
    }

    Options options;
    options.implementation = *opt_implementation;
    options.count = *opt_count;
    options.threads = *opt_threads;
    options.batching_threads = *opt_batching_threads;
    options.batch_size = *opt_batch_size;
    options.enqueue_buckets = *opt_enqueue_buckets;
    options.key_count = *opt_key_count;
    options.max_delay_us = *opt_max_delay_us;
    options.process_delay_us = *opt_process_delay_us;

    if (options.implementation != "simple" && options.implementation != "bucket")
    {
      throw std::runtime_error("--implementation must be simple or bucket");
    }

    if (options.count == 0)
    {
      throw std::runtime_error("--count must be > 0");
    }

    if (options.threads == 0 || options.batching_threads == 0 ||
      options.batch_size == 0 || options.enqueue_buckets == 0)
    {
      throw std::runtime_error("thread, batch, and bucket counts must be > 0");
    }

    if (options.key_count == 0)
    {
      options.key_count = options.count;
    }

    if (*opt_mode == "read")
    {
      options.mode = Mode::READ;
    }
    else if (*opt_mode == "write")
    {
      options.mode = Mode::WRITE;
    }
    else if (*opt_mode == "read-write")
    {
      options.mode = Mode::READ_WRITE;
    }
    else
    {
      throw std::runtime_error("--mode must be read, write, or read-write");
    }

    return options;
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

  std::uint64_t
  current_thread_cpu_ns()
  {
    rusage usage{};
    if (getrusage(RUSAGE_THREAD, &usage) != 0)
    {
      throw std::runtime_error("getrusage for thread failed");
    }

    return
      (static_cast<std::uint64_t>(usage.ru_utime.tv_sec) +
        static_cast<std::uint64_t>(usage.ru_stime.tv_sec)) * 1000000000ULL +
      (static_cast<std::uint64_t>(usage.ru_utime.tv_usec) +
        static_cast<std::uint64_t>(usage.ru_stime.tv_usec)) * 1000ULL;
  }

  std::string
  format_float(double value)
  {
    std::ostringstream out;
    out << std::fixed << std::setprecision(6) << value;
    return out.str();
  }

  template<typename Queue>
  class QueueScheduler final
  {
  public:
    explicit QueueScheduler(Queue& queue)
      : queue_(queue)
    {}

    void
    publish(const typename Queue::ReadyState& state)
    {
      if (!state.has_operation)
      {
        return;
      }

      {
        std::lock_guard guard(lock_);
        if (ready_ && ready_time_ <= state.ready_time)
        {
          return;
        }

        ready_ = true;
        ready_time_ = state.ready_time;
      }
      condition_.notify_one();
    }

    bool
    acquire_batch()
    {
      std::unique_lock guard(lock_);
      while (true)
      {
        if (ready_)
        {
          const Generics::Time now = Generics::Time::get_time_of_day();
          if (now < ready_time_)
          {
            const auto deadline = std::chrono::system_clock::time_point(
              std::chrono::duration_cast<std::chrono::system_clock::duration>(
                std::chrono::microseconds(ready_time_.microseconds())));
            condition_.wait_until(guard, deadline);
            continue;
          }

          ready_ = false;
          queue_.start_batch();
          return true;
        }

        if (producers_done_ && queue_.drained())
        {
          return false;
        }
        condition_.wait(guard);
      }
    }

    void
    finish_batch()
    {
      queue_.finish_batch();
      bool drained = false;
      {
        std::lock_guard guard(lock_);
        drained = producers_done_ && queue_.drained();
      }

      if (drained)
      {
        condition_.notify_all();
      }
    }

    void
    finish_producers()
    {
      publish(queue_.flush_pending());
      {
        std::lock_guard guard(lock_);
        producers_done_ = true;
      }
      condition_.notify_all();
    }

  private:
    Queue& queue_;
    std::mutex lock_;
    std::condition_variable condition_;
    bool ready_ = false;
    bool producers_done_ = false;
    Generics::Time ready_time_;
  };

  template<typename Queue>
  int
  run(const Options& options)
  {
    Queue queue(
      options.batch_size,
      Generics::Time(0, options.max_delay_us),
      options.enqueue_buckets);
    QueueScheduler<Queue> scheduler(queue);

    std::vector<std::string> keys;
    keys.reserve(options.key_count);
    for (std::size_t i = 0; i < options.key_count; ++i)
    {
      keys.emplace_back("profile/" + std::to_string(i));
    }

    std::atomic<std::uint64_t> next_operation{0};
    std::atomic<std::uint64_t> enqueued{0};
    std::atomic<std::uint64_t> enqueue_call_count{0};
    std::atomic<std::uint64_t> enqueue_call_total_ns{0};
    std::atomic<std::uint64_t> done_count{0};
    std::atomic<std::uint64_t> processed{0};
    std::atomic<std::uint64_t> read_operations{0};
    std::atomic<std::uint64_t> write_operations{0};
    std::atomic<std::uint64_t> batches{0};
    std::atomic<std::uint64_t> checksum{0};
    std::atomic<std::uint64_t> errors{0};
    std::atomic<std::uint64_t> enqueue_threads_cpu_ns{0};
    std::atomic<std::uint64_t> collect_threads_cpu_ns{0};

    StartBarrier start_barrier(options.threads + options.batching_threads + 1);

    std::vector<std::thread> consumers;
    consumers.reserve(options.batching_threads);
    for (std::size_t i = 0; i < options.batching_threads; ++i)
    {
      consumers.emplace_back(
        [&]()
        {
          typename Queue::Operations batch;
          typename Queue::SelectedKeys selected_keys;
          start_barrier.arrive_and_wait();
          const std::uint64_t cpu_started_at = current_thread_cpu_ns();

          while (scheduler.acquire_batch())
          {
            scheduler.publish(queue.collect_batch(batch, selected_keys));
            if (batch.empty())
            {
              scheduler.finish_batch();
              continue;
            }

            std::uint64_t local_checksum = 0;
            for (auto& operation : batch)
            {
              local_checksum += operation.key.text().size() + operation.type;
              if (operation.get_callback)
              {
                (*operation.get_callback)(Generics::ConstSmartMemBuf_var(), std::nullopt);
              }

              if (operation.save_callback)
              {
                (*operation.save_callback)(std::nullopt);
              }
            }

            if (options.process_delay_us != 0)
            {
              std::this_thread::sleep_for(std::chrono::microseconds(options.process_delay_us));
            }

            processed.fetch_add(batch.size(), std::memory_order_relaxed);
            batches.fetch_add(1, std::memory_order_relaxed);
            checksum.fetch_add(local_checksum, std::memory_order_relaxed);
            scheduler.publish(queue.complete_batch(batch));
            scheduler.finish_batch();
            selected_keys.clear();
            selected_keys.reserve(batch.size());
            batch.clear();
          }

          collect_threads_cpu_ns.fetch_add(
            current_thread_cpu_ns() - cpu_started_at,
            std::memory_order_relaxed);
        });
    }

    std::vector<std::thread> producers;
    producers.reserve(options.threads);
    for (std::size_t thread_index = 0; thread_index < options.threads; ++thread_index)
    {
      producers.emplace_back(
        [&]()
        {
          std::uint64_t local_enqueue_call_count = 0;
          std::uint64_t local_enqueue_call_total_ns = 0;
          start_barrier.arrive_and_wait();
          const std::uint64_t cpu_started_at = current_thread_cpu_ns();

          while (true)
          {
            const std::uint64_t operation_index =
              next_operation.fetch_add(1, std::memory_order_relaxed);
            if (operation_index >= options.count)
            {
              break;
            }

            const bool write_operation = options.mode == Mode::WRITE ||
              (options.mode == Mode::READ_WRITE && operation_index % 2 != 0);
            typename Queue::Operation operation;
            operation.type = write_operation ? BucketQueue::OT_SAVE : BucketQueue::OT_GET;
            operation.key = keys[operation_index % keys.size()];
            if (write_operation)
            {
              operation.save_callback =
                [&done_count](std::optional<std::string>)
                {
                  done_count.fetch_add(1, std::memory_order_relaxed);
                };
              write_operations.fetch_add(1, std::memory_order_relaxed);
            }
            else
            {
              operation.get_callback =
                [&done_count](Generics::ConstSmartMemBuf_var, std::optional<std::string>)
                {
                  done_count.fetch_add(1, std::memory_order_relaxed);
                };
              read_operations.fetch_add(1, std::memory_order_relaxed);
            }

            typename Queue::Operations operations;
            operations.emplace_back(std::move(operation));
            const auto enqueue_started_at = std::chrono::steady_clock::now();
            auto result = queue.enqueue(std::move(operations));
            const auto enqueue_finished_at = std::chrono::steady_clock::now();
            local_enqueue_call_total_ns += static_cast<std::uint64_t>(
              std::chrono::duration_cast<std::chrono::nanoseconds>(
                enqueue_finished_at - enqueue_started_at).count());
            ++local_enqueue_call_count;

            enqueued.fetch_add(1, std::memory_order_relaxed);

            if (result.ready_state)
            {
              scheduler.publish(*result.ready_state);
            }
          }

          enqueue_call_count.fetch_add(local_enqueue_call_count, std::memory_order_relaxed);
          enqueue_call_total_ns.fetch_add(local_enqueue_call_total_ns, std::memory_order_relaxed);
          enqueue_threads_cpu_ns.fetch_add(
            current_thread_cpu_ns() - cpu_started_at,
            std::memory_order_relaxed);
        });
    }

    const CpuTimes cpu_started = current_cpu_times();
    const auto started_at = std::chrono::steady_clock::now();
    start_barrier.arrive_and_wait();

    for (auto& producer : producers)
    {
      producer.join();
    }
    scheduler.finish_producers();
    for (auto& consumer : consumers)
    {
      consumer.join();
    }

    const auto finished_at = std::chrono::steady_clock::now();
    const CpuTimes cpu_finished = current_cpu_times();
    const double elapsed = std::chrono::duration<double>(finished_at - started_at).count();
    const double user_cpu = cpu_finished.user - cpu_started.user;
    const double sys_cpu = cpu_finished.sys - cpu_started.sys;
    const std::uint64_t enqueued_count = enqueued.load(std::memory_order_relaxed);
    const std::uint64_t measured_enqueue_call_count =
      enqueue_call_count.load(std::memory_order_relaxed);
    const std::uint64_t measured_enqueue_call_total_ns =
      enqueue_call_total_ns.load(std::memory_order_relaxed);
    const std::uint64_t done = done_count.load(std::memory_order_relaxed);
    const std::uint64_t processed_count = processed.load(std::memory_order_relaxed);
    const std::uint64_t batch_count = batches.load(std::memory_order_relaxed);
    const double enqueue_threads_cpu =
      enqueue_threads_cpu_ns.load(std::memory_order_relaxed) / 1000000000.0;
    const double collect_threads_cpu =
      collect_threads_cpu_ns.load(std::memory_order_relaxed) / 1000000000.0;

    std::cout
      << "implementation=" << options.implementation << '\n'
      << "threads=" << options.threads << '\n'
      << "batching_threads=" << options.batching_threads << '\n'
      << "batch_size=" << options.batch_size << '\n'
      << "enqueue_buckets=" << options.enqueue_buckets << '\n'
      << "key_count=" << options.key_count << '\n'
      << "max_delay_us=" << options.max_delay_us << '\n'
      << "process_delay_us=" << options.process_delay_us << '\n'
      << "mode=" << mode_name(options.mode) << '\n'
      << "operations=" << options.count << '\n'
      << "enqueued=" << enqueued_count << '\n'
      << "enqueue_call_count=" << measured_enqueue_call_count << '\n'
      << "enqueue_call_total_sec=" << format_float(
        measured_enqueue_call_total_ns / 1000000000.0) << '\n'
      << "average_enqueue_call_ns=" << format_float(
        measured_enqueue_call_count ?
          static_cast<double>(measured_enqueue_call_total_ns) /
            measured_enqueue_call_count : 0.0) << '\n'
      << "done=" << done << '\n'
      << "processed=" << processed_count << '\n'
      << "reads=" << read_operations.load(std::memory_order_relaxed) << '\n'
      << "writes=" << write_operations.load(std::memory_order_relaxed) << '\n'
      << "batches=" << batch_count << '\n'
      << "average_batch_size=" << format_float(
        batch_count ? static_cast<double>(processed_count) / batch_count : 0.0) << '\n'
      << "errors=" << errors.load(std::memory_order_relaxed) << '\n'
      << "checksum=" << checksum.load(std::memory_order_relaxed) << '\n'
      << "elapsed_sec=" << format_float(elapsed) << '\n'
      << "operations_per_sec=" << format_float(options.count / elapsed) << '\n'
      << "enqueue_threads_cpu_sec=" << format_float(enqueue_threads_cpu) << '\n'
      << "collect_threads_cpu_sec=" << format_float(collect_threads_cpu) << '\n'
      << "worker_threads_cpu_sec=" << format_float(
        enqueue_threads_cpu + collect_threads_cpu) << '\n'
      << "cpu_sec=" << format_float(user_cpu + sys_cpu) << '\n'
      << "user_cpu_sec=" << format_float(user_cpu) << '\n'
      << "sys_cpu_sec=" << format_float(sys_cpu) << std::endl;

    return errors.load(std::memory_order_relaxed) == 0 &&
      enqueued_count == options.count &&
      measured_enqueue_call_count == options.count &&
      done == options.count && processed_count == options.count ? 0 : 2;
  }
}

int
main(int argc, char** argv)
{
  try
  {
    const Options options = parse_options(argc, argv);
    return options.implementation == "simple" ?
      run<SimpleQueue>(options) : run<BucketQueue>(options);
  }
  catch(const std::exception& ex)
  {
    std::cerr << "Caught std::exception: " << ex.what() << std::endl;
    print_usage();
    return 1;
  }
}
