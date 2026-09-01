#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <cerrno>
#include <cstring>
#include <deque>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <ftw.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>

#include <Generics/AppUtils.hpp>
#include <Generics/MemBuf.hpp>
#include <Generics/Time.hpp>

#include <ProfilingCommons/ProfileMap/RocksDBBatchingProfileMap.hpp>
#include <ProfilingCommons/ProfileMap/RocksDBProfileMap.hpp>

namespace
{
  constexpr std::size_t KEY_SIZE = 16;
  constexpr std::size_t BODY_SIZE = 16;

  struct CpuTimes
  {
    double user = 0.0;
    double sys = 0.0;
  };

  class TaskExecutor final
  {
  public:
    explicit TaskExecutor(unsigned int threads)
    {
      workers_.reserve(threads);
      for (unsigned int i = 0; i < threads; ++i)
      {
        workers_.emplace_back([this]() { worker_loop_(); });
      }
    }

    ~TaskExecutor()
    {
      stop();
    }

    void post(std::function<void()> task)
    {
      {
        std::lock_guard<std::mutex> lock(lock_);
        if (stopped_)
        {
          return;
        }

        tasks_.emplace_back(std::move(task));
      }

      cond_.notify_one();
    }

    void stop()
    {
      {
        std::lock_guard<std::mutex> lock(lock_);
        if (stopped_)
        {
          return;
        }

        stopped_ = true;
      }

      cond_.notify_all();

      for (auto& worker : workers_)
      {
        if (worker.joinable())
        {
          worker.join();
        }
      }

      workers_.clear();
    }

  private:
    void worker_loop_()
    {
      while (true)
      {
        std::function<void()> task;
        {
          std::unique_lock<std::mutex> lock(lock_);
          cond_.wait(lock, [this]() { return stopped_ || !tasks_.empty(); });
          if (stopped_ && tasks_.empty())
          {
            return;
          }

          task = std::move(tasks_.front());
          tasks_.pop_front();
        }

        task();
      }
    }

    std::mutex lock_;
    std::condition_variable cond_;
    std::deque<std::function<void()>> tasks_;
    bool stopped_ = false;
    std::vector<std::thread> workers_;
  };

  void
  update_max(std::atomic<std::uint64_t>& value, std::uint64_t candidate) noexcept
  {
    auto current = value.load(std::memory_order_relaxed);
    while (current < candidate &&
      !value.compare_exchange_weak(current, candidate, std::memory_order_relaxed))
    {}
  }

  std::string
  format_stat_float(double value)
  {
    std::ostringstream out;
    out << std::fixed << std::setprecision(6) << value;
    auto result = out.str();
    const auto dot_pos = result.find('.');
    if (dot_pos != std::string::npos)
    {
      while (!result.empty() && result.back() == '0')
      {
        result.pop_back();
      }

      if (!result.empty() && result.back() == '.')
      {
        result.pop_back();
      }
    }
    return result;
  }

  double
  timeval_seconds(const timeval& value)
  {
    return static_cast<double>(value.tv_sec) +
      static_cast<double>(value.tv_usec) / 1000000.0;
  }

  CpuTimes
  process_cpu_times()
  {
    rusage usage{};
    getrusage(RUSAGE_SELF, &usage);
    return CpuTimes{
      timeval_seconds(usage.ru_utime),
      timeval_seconds(usage.ru_stime)
    };
  }

  std::string
  random_ascii(std::mt19937& gen, std::size_t size)
  {
    static constexpr char first = '!';
    static constexpr char last = '~';
    std::uniform_int_distribution<int> dist(first, last);

    std::string value;
    value.reserve(size);
    for (std::size_t i = 0; i < size; ++i)
    {
      value.push_back(static_cast<char>(dist(gen)));
    }
    return value;
  }

  Generics::ConstSmartMemBuf_var
  make_profile(const std::string& body)
  {
    return Generics::ConstSmartMemBuf_var(new Generics::ConstSmartMemBuf(body.data(), body.size()));
  }

  double
  avg_batch_size(const AdServer::ProfilingCommons::ProfileMap<std::string>::Stats& stats)
  {
    const auto logical_operations = stats.logical_read_operations + stats.logical_write_operations;
    const auto physical_operations =
      stats.physical_read_operations + stats.physical_write_operations;

    return physical_operations == 0 ?
      0.0 :
      static_cast<double>(logical_operations) /
        static_cast<double>(physical_operations);
  }

  AdServer::ProfilingCommons::ProfileMap<std::string>::Stats
  subtract_stats(
    const AdServer::ProfilingCommons::ProfileMap<std::string>::Stats& lhs,
    const AdServer::ProfilingCommons::ProfileMap<std::string>::Stats& rhs)
  {
    return {
      lhs.logical_read_operations - rhs.logical_read_operations,
      lhs.logical_write_operations - rhs.logical_write_operations,
      lhs.physical_read_operations - rhs.physical_read_operations,
      lhs.physical_write_operations - rhs.physical_write_operations
    };
  }

  int
  remove_path_callback(const char* path, const struct stat*, int, struct FTW*)
  {
    return ::remove(path);
  }

  void
  remove_all(const std::string& path)
  {
    if (::nftw(path.c_str(), remove_path_callback, 64, FTW_DEPTH | FTW_PHYS) != 0 &&
      errno != ENOENT)
    {
      const int error = errno;
      throw std::runtime_error("can't remove '" + path + "': " + std::strerror(error));
    }
  }

  void
  print_usage()
  {
    std::cerr
      << "Usage: RocksDBProfileMapTest [OPTIONS]\n"
      << "Options:\n"
      << "  --path <path>       RocksDB path (default: /tmp/RocksDBProfileMapTest.<pid>)\n"
      << "  --count <N>         number of read/write operations (default: 1000000)\n"
      << "  --threads <N>       test threads count; callback workers in batch mode (default: 16)\n"
      << "  --keys <N>          number of pre-generated random keys (default: 10000)\n"
      << "  --mode <direct|batch>  map mode (default: direct)\n"
      << "  --workers <N>       batch mode workers count (default: 4)\n"
      << "  --batch-size <N>    batch mode batch size (default: 128)\n"
      << "  --max-inflight <N>  maximum async scenarios in flight for batch mode (default: 12000)\n"
      << "  --max-delay-us <N>  maximum batch accumulation delay for batch mode,\n"
      << "                      0 disables (default: 0)\n"
      << "  --disable-wal <0|1> disable RocksDB WAL writes (default: 0)\n"
      << "  --prepare-db <0|1>  fill, flush and reopen DB before test (default: 0)\n"
      << "  --keep-db <0|1>     keep DB directory after test (default: 0)\n";
  }
}

int
main(int argc, char** argv)
{
  try
  {
    using namespace Generics::AppUtils;
    using ProfileMapBase = AdServer::ProfilingCommons::ProfileMap<std::string>;
    using DirectProfileMap = AdServer::ProfilingCommons::RocksDBProfileMap<std::string>;
    using BatchProfileMap = AdServer::ProfilingCommons::RocksDBBatchingProfileMap<std::string>;

    StringOption opt_path(std::string("/tmp/RocksDBProfileMapTest.") + std::to_string(::getpid()));
    Option<unsigned long> opt_count(1000000);
    Option<unsigned int> opt_threads(16);
    Option<unsigned long> opt_keys(10000);
    StringOption opt_mode("direct");
    Option<unsigned long> opt_workers(4);
    Option<unsigned long> opt_batch_size(128);
    Option<unsigned long> opt_max_inflight(12000);
    Option<unsigned long> opt_max_delay_us(0);
    Option<unsigned int> opt_disable_wal(0);
    Option<unsigned int> opt_prepare_db(0);
    Option<unsigned int> opt_keep_db(0);
    CheckOption opt_help;

    Args args(-1);
    args.add(equal_name("path") || short_name("p"), opt_path);
    args.add(equal_name("count") || short_name("c"), opt_count);
    args.add(equal_name("threads") || short_name("t"), opt_threads);
    args.add(equal_name("keys") || short_name("k"), opt_keys);
    args.add(equal_name("mode"), opt_mode);
    args.add(equal_name("workers"), opt_workers);
    args.add(equal_name("batch-size"), opt_batch_size);
    args.add(equal_name("max-inflight"), opt_max_inflight);
    args.add(equal_name("max-delay-us"), opt_max_delay_us);
    args.add(equal_name("disable-wal"), opt_disable_wal);
    args.add(equal_name("prepare-db"), opt_prepare_db);
    args.add(equal_name("keep-db"), opt_keep_db);
    args.add(equal_name("help") || short_name("h"), opt_help);

    args.parse(argc - 1, argv + 1);

    if (opt_help.enabled())
    {
      print_usage();
      return 0;
    }

    if (*opt_threads == 0)
    {
      std::cerr << "--threads must be > 0" << std::endl;
      return 1;
    }

    if (*opt_keys == 0)
    {
      std::cerr << "--keys must be > 0" << std::endl;
      return 1;
    }

    if (*opt_mode != "direct" && *opt_mode != "batch")
    {
      std::cerr << "--mode must be direct or batch" << std::endl;
      return 1;
    }

    if (*opt_workers == 0)
    {
      std::cerr << "--workers must be > 0" << std::endl;
      return 1;
    }

    if (*opt_batch_size == 0)
    {
      std::cerr << "--batch-size must be > 0" << std::endl;
      return 1;
    }

    if (*opt_max_inflight == 0)
    {
      std::cerr << "--max-inflight must be > 0" << std::endl;
      return 1;
    }

    remove_all(*opt_path);

    std::vector<std::string> keys;
    keys.reserve(*opt_keys);
    {
      std::mt19937 gen(std::random_device{}());
      for (unsigned long i = 0; i < *opt_keys; ++i)
      {
        keys.emplace_back(random_ascii(gen, KEY_SIZE));
      }
    }

    if (*opt_prepare_db != 0)
    {
      DirectProfileMap prepare_map(
        String::SubString(*opt_path),
        Generics::Time::ZERO,
        *opt_disable_wal != 0);

      std::mt19937 gen(std::random_device{}());
      for (const auto& key : keys)
      {
        const std::string body = random_ascii(gen, BODY_SIZE);
        Generics::ConstSmartMemBuf_var write_buf = make_profile(body);
        prepare_map.save_profile(key, write_buf.in(), Generics::Time::get_time_of_day());
      }

      prepare_map.flush();
    }

    std::unique_ptr<DirectProfileMap> direct_map;
    std::unique_ptr<BatchProfileMap> batch_map;
    ProfileMapBase* map = nullptr;

    if (*opt_mode == "batch")
    {
      batch_map.reset(
        new BatchProfileMap(
          String::SubString(*opt_path),
          Generics::Time::ZERO,
          *opt_workers,
          *opt_batch_size,
          Generics::Time(*opt_max_delay_us / 1000000, *opt_max_delay_us % 1000000),
          *opt_disable_wal != 0));
      batch_map->activate_object();
      map = batch_map.get();
    }
    else
    {
      direct_map.reset(
        new DirectProfileMap(
          String::SubString(*opt_path),
          Generics::Time::ZERO,
          *opt_disable_wal != 0));
      map = direct_map.get();
    }

    std::atomic<std::uint64_t> sent_count{0};
    std::atomic<std::uint64_t> done_count{0};
    std::atomic<std::uint64_t> error_count{0};
    std::atomic<std::uint64_t> read_hit_count{0};
    std::atomic<std::uint64_t> latency_count{0};
    std::atomic<std::uint64_t> latency_sum_us{0};
    std::atomic<std::uint64_t> latency_max_us{0};
    std::mutex error_lock;

    std::thread reporter_thread([&]() {
      std::uint64_t prev_done = 0;
      std::uint64_t prev_errors = 0;
      std::uint64_t prev_hits = 0;
      std::uint64_t prev_latency_count = 0;
      std::uint64_t prev_latency_sum_us = 0;
      auto prev_stats = map->stats();

      while (done_count.load(std::memory_order_relaxed) < *opt_count)
      {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        const auto now = std::chrono::system_clock::now();
        const std::time_t now_tt = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
        localtime_r(&now_tt, &tm);

        const auto current_done = done_count.load(std::memory_order_relaxed);
        const auto current_errors = error_count.load(std::memory_order_relaxed);
        const auto current_hits = read_hit_count.load(std::memory_order_relaxed);
        const auto current_latency_count = latency_count.load(std::memory_order_relaxed);
        const auto current_latency_sum_us = latency_sum_us.load(std::memory_order_relaxed);
        const auto current_stats = map->stats();

        const auto done_delta = current_done - prev_done;
        const auto errors_delta = current_errors - prev_errors;
        const auto hits_delta = current_hits - prev_hits;
        const auto latency_count_delta = current_latency_count - prev_latency_count;
        const auto latency_sum_delta_us = current_latency_sum_us - prev_latency_sum_us;
        const auto avg_latency_us = latency_count_delta == 0 ?
          0.0 :
          static_cast<double>(latency_sum_delta_us) /
            static_cast<double>(latency_count_delta);
        const auto stats_delta = subtract_stats(current_stats, prev_stats);

        prev_done = current_done;
        prev_errors = current_errors;
        prev_hits = current_hits;
        prev_latency_count = current_latency_count;
        prev_latency_sum_us = current_latency_sum_us;
        prev_stats = current_stats;

        std::cout << std::put_time(&tm, "%T") <<
          ": " << done_delta <<
          ", errors=" << errors_delta <<
          ", hits=" << hits_delta <<
          ", avg_latency=" << format_stat_float(avg_latency_us) << "us" <<
          ", avg_batch_size=" << format_stat_float(avg_batch_size(stats_delta)) <<
          ", max_latency=" << latency_max_us.load(std::memory_order_relaxed) << "us" << std::endl;
      }
    });

    const auto run_start = std::chrono::steady_clock::now();
    const auto cpu_start = process_cpu_times();

    if (*opt_mode == "direct")
    {
      std::vector<std::thread> threads;
      threads.reserve(*opt_threads);

      for (unsigned int i = 0; i < *opt_threads; ++i)
      {
        threads.emplace_back([&]() {
          std::mt19937 gen(std::random_device{}());
          std::uniform_int_distribution<std::size_t> key_dist(0, keys.size() - 1);

          while (true)
          {
            const auto index = sent_count.fetch_add(1, std::memory_order_relaxed);
            if (index >= *opt_count)
            {
              return;
            }

            const auto& key = keys[key_dist(gen)];
            const std::string body = random_ascii(gen, BODY_SIZE);
            const auto start = std::chrono::steady_clock::now();

            try
            {
              Generics::ConstSmartMemBuf_var read_buf = map->get_profile(key);
              if (read_buf.in())
              {
                read_hit_count.fetch_add(1, std::memory_order_relaxed);
                if (read_buf->membuf().size() != BODY_SIZE)
                {
                  std::lock_guard<std::mutex> lock(error_lock);
                  std::cerr << "unexpected profile size for key '" << key <<
                    "': " << read_buf->membuf().size() << std::endl;
                  error_count.fetch_add(1, std::memory_order_relaxed);
                }
              }

              Generics::ConstSmartMemBuf_var write_buf = make_profile(body);
              map->save_profile(key, write_buf.in(), Generics::Time::get_time_of_day());
            }
            catch(const eh::Exception& ex)
            {
              error_count.fetch_add(1, std::memory_order_relaxed);
              std::lock_guard<std::mutex> lock(error_lock);
              std::cerr << "operation error: " << ex.what() << std::endl;
            }

            const auto finish = std::chrono::steady_clock::now();
            const auto latency_us = static_cast<std::uint64_t>(
              std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count());
            latency_count.fetch_add(1, std::memory_order_relaxed);
            latency_sum_us.fetch_add(latency_us, std::memory_order_relaxed);
            update_max(latency_max_us, latency_us);

            done_count.fetch_add(1, std::memory_order_relaxed);
          }
        });
      }

      for (auto& thread : threads)
      {
        thread.join();
      }
    }
    else
    {
      struct Scenario final
      {
        std::string key;
        std::string body;
        std::chrono::steady_clock::time_point start;
      };

      TaskExecutor callback_executor(*opt_threads);
      std::mutex done_lock;
      std::condition_variable done_cond;
      std::function<void()> start_scenario;

      auto log_error =
        [&](const std::string& message)
      {
        error_count.fetch_add(1, std::memory_order_relaxed);
        std::lock_guard<std::mutex> lock(error_lock);
        std::cerr << message << std::endl;
      };

      auto complete_scenario =
        [&](const std::chrono::steady_clock::time_point& start, bool start_next)
      {
        const auto finish = std::chrono::steady_clock::now();
        const auto latency_us = static_cast<std::uint64_t>(
          std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count());
        latency_count.fetch_add(1, std::memory_order_relaxed);
        latency_sum_us.fetch_add(latency_us, std::memory_order_relaxed);
        update_max(latency_max_us, latency_us);

        const auto done = done_count.fetch_add(1, std::memory_order_relaxed) + 1;
        if (done >= *opt_count)
        {
          done_cond.notify_all();
        }

        if (start_next)
        {
          start_scenario();
        }
      };

      start_scenario =
        [&]()
      {
        const auto index = sent_count.fetch_add(1, std::memory_order_relaxed);
        if (index >= *opt_count)
        {
          return;
        }

        thread_local std::mt19937 gen(std::random_device{}());
        std::uniform_int_distribution<std::size_t> key_dist(0, keys.size() - 1);

        auto scenario = std::make_shared<Scenario>();
        scenario->key = keys[key_dist(gen)];
        scenario->body = random_ascii(gen, BODY_SIZE);
        scenario->start = std::chrono::steady_clock::now();

        try
        {
          batch_map->get_profile_async(
            scenario->key,
            [
              &,
              scenario
            ](Generics::ConstSmartMemBuf_var read_buf, std::optional<std::string> error)
            {
              callback_executor.post(
                [
                  &,
                  scenario,
                  read_buf = std::move(read_buf),
                  error = std::move(error)
                ]() mutable
              {
                if (error)
                {
                  log_error("operation error: " + *error);
                  complete_scenario(scenario->start, true);
                  return;
                }

                if (read_buf.in())
                {
                  read_hit_count.fetch_add(1, std::memory_order_relaxed);
                  if (read_buf->membuf().size() != BODY_SIZE)
                  {
                    std::ostringstream ostr;
                    ostr << "unexpected profile size for key '" <<
                      scenario->key << "': " << read_buf->membuf().size();
                    log_error(ostr.str());
                  }
                }

                Generics::ConstSmartMemBuf_var write_buf = make_profile(scenario->body);
                try
                {
                  batch_map->save_profile_async(
                    scenario->key,
                    write_buf.in(),
                    Generics::Time::get_time_of_day(),
                    [
                      &,
                      scenario,
                      write_buf
                    ](std::optional<std::string> error)
                  {
                    callback_executor.post(
                      [
                        &,
                        scenario,
                        write_buf,
                        error = std::move(error)
                      ]() mutable
                    {
                      if (error)
                      {
                        log_error("operation error: " + *error);
                      }

                      complete_scenario(scenario->start, true);
                    });
                  });
                }
                catch(const eh::Exception& ex)
                {
                  log_error(std::string("operation error: ") + ex.what());
                  complete_scenario(scenario->start, true);
                }
              });
            });
        }
        catch(const eh::Exception& ex)
        {
          log_error(std::string("operation error: ") + ex.what());
          complete_scenario(scenario->start, true);
        }
      };

      const std::uint64_t initial_inflight = std::min<std::uint64_t>(*opt_max_inflight, *opt_count);
      for (std::uint64_t i = 0; i < initial_inflight; ++i)
      {
        start_scenario();
      }

      {
        std::unique_lock<std::mutex> lock(done_lock);
        done_cond.wait(
          lock,
          [&]()
          {
            return done_count.load(std::memory_order_relaxed) >= *opt_count;
          });
      }

      callback_executor.stop();
    }

    const auto run_finish = std::chrono::steady_clock::now();
    const auto cpu_finish = process_cpu_times();

    reporter_thread.join();
    const auto profile_map_stats = map->stats();
    const auto profile_map_size = map->size();
    if (*opt_count != 0 && profile_map_size == 0)
    {
      std::cerr << "profile map size is zero after writes" << std::endl;
      error_count.fetch_add(1, std::memory_order_relaxed);
    }

    if (batch_map)
    {
      batch_map->deactivate_object();
      batch_map->wait_object();
      batch_map.reset();
    }
    direct_map.reset();

    const auto total_latency_count = latency_count.load(std::memory_order_relaxed);
    const auto total_avg_latency_us = total_latency_count == 0 ?
      0.0 :
      static_cast<double>(latency_sum_us.load(std::memory_order_relaxed)) /
        static_cast<double>(total_latency_count);
    const auto run_seconds = std::chrono::duration<double>(run_finish - run_start).count();
    const auto rps = run_seconds == 0.0 ?
      0.0 :
      static_cast<double>(done_count.load(std::memory_order_relaxed)) /
        run_seconds;
    const auto user_cpu_seconds = cpu_finish.user - cpu_start.user;
    const auto sys_cpu_seconds = cpu_finish.sys - cpu_start.sys;
    const auto cpu_seconds = user_cpu_seconds + sys_cpu_seconds;

    std::cout << "completed: " << done_count.load(std::memory_order_relaxed) <<
      ", errors: " << error_count.load(std::memory_order_relaxed) <<
      ", hits: " << read_hit_count.load(std::memory_order_relaxed) <<
      ", rps: " << format_stat_float(rps) <<
      ", cpu_time: " << format_stat_float(cpu_seconds) << "s" <<
      ", user_cpu_time: " << format_stat_float(user_cpu_seconds) << "s" <<
      ", sys_cpu_time: " << format_stat_float(sys_cpu_seconds) << "s" <<
      ", avg_latency: " << format_stat_float(total_avg_latency_us) << "us" <<
      ", avg_batch_size: " << format_stat_float(avg_batch_size(profile_map_stats)) <<
      ", max_latency: " << latency_max_us.load(std::memory_order_relaxed) << "us" <<
      ", profiles: " << profile_map_size <<
      ", mode: " << *opt_mode <<
      ", disable_wal: " << (*opt_disable_wal != 0 ? 1 : 0) << ", path: " << *opt_path << std::endl;

    if (*opt_keep_db == 0)
    {
      remove_all(*opt_path);
    }

    return error_count.load(std::memory_order_relaxed) == 0 ? 0 : 1;
  }
  catch(const std::exception& ex)
  {
    std::cerr << "Caught exception: " << ex.what() << std::endl;
  }

  return 1;
}
