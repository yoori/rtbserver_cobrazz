#include <atomic>
#include <chrono>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <sys/resource.h>

#include <Commons/Coro/StartableAwaitable.hpp>
#include <Generics/AppUtils.hpp>
#include <Generics/MemBuf.hpp>
#include <Generics/Time.hpp>
#include <ProfilingCommons/ProfileMap/RocksDBBatchingProfileMap.hpp>

namespace
{
  enum class Mode
  {
    READ,
    WRITE,
    READ_WRITE
  };

  struct Options
  {
    std::filesystem::path data_root;
    std::uint64_t count = 0;
    std::size_t threads = 1;
    std::size_t batching_threads = 1;
    std::size_t enqueue_buckets = 32;
    std::size_t key_count = 0;
    Mode mode = Mode::READ_WRITE;
    bool disable_wal = false;
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

  struct CpuTimes
  {
    double user = 0.0;
    double sys = 0.0;
  };

  void
  print_usage()
  {
    std::cerr
      << "Usage: RocksDBBatchingProfileMapPerfTest --data-root <path> "
      << "--count <N> [OPTIONS]\n"
      << "Options:\n"
      << "  --threads <N>           load worker threads count (default: 1)\n"
      << "  --batching-threads <N>  RocksDB batching worker threads count (default: 1)\n"
      << "  --enqueue-buckets <N>   producer queue buckets count (default: 32)\n"
      << "  --key-count <N>         hot keys count (default: --threads)\n"
      << "  --mode <MODE>           read, write, or read-write (default: read-write)\n"
      << "  --disable-wal  disable RocksDB WAL for writes\n";
  }

  Options
  parse_options(int argc, char** argv)
  {
    using namespace Generics::AppUtils;

    StringOption opt_data_root;
    Option<unsigned long> opt_count(0);
    Option<unsigned long> opt_threads(1);
    Option<unsigned long> opt_batching_threads(1);
    Option<unsigned long> opt_enqueue_buckets(32);
    Option<unsigned long> opt_key_count(0);
    StringOption opt_mode("read-write");
    CheckOption opt_disable_wal;
    CheckOption opt_help;

    Args args(-1);
    args.add(equal_name("data-root"), opt_data_root);
    args.add(equal_name("count"), opt_count);
    args.add(equal_name("threads"), opt_threads);
    args.add(equal_name("batching-threads"), opt_batching_threads);
    args.add(equal_name("enqueue-buckets"), opt_enqueue_buckets);
    args.add(equal_name("key-count"), opt_key_count);
    args.add(equal_name("mode"), opt_mode);
    args.add(equal_name("disable-wal"), opt_disable_wal);
    args.add(equal_name("help") || short_name("h"), opt_help);

    args.parse(argc - 1, argv + 1);

    if(opt_help.enabled())
    {
      print_usage();
      std::exit(0);
    }

    Options options;
    options.data_root = *opt_data_root;
    options.count = *opt_count;
    options.threads = *opt_threads;
    options.batching_threads = *opt_batching_threads;
    options.enqueue_buckets = *opt_enqueue_buckets;
    options.key_count = *opt_key_count;
    options.disable_wal = opt_disable_wal.enabled();

    if(options.data_root.empty())
    {
      throw std::runtime_error("--data-root is required");
    }

    if(options.count == 0)
    {
      throw std::runtime_error("--count must be > 0");
    }

    if(options.threads == 0)
    {
      throw std::runtime_error("--threads must be > 0");
    }

    if(options.batching_threads == 0)
    {
      throw std::runtime_error("--batching-threads must be > 0");
    }

    if(options.enqueue_buckets == 0)
    {
      throw std::runtime_error("--enqueue-buckets must be > 0");
    }

    if(options.key_count == 0)
    {
      options.key_count = options.threads;
    }

    if(*opt_mode == "read")
    {
      options.mode = Mode::READ;
    }
    else if(*opt_mode == "write")
    {
      options.mode = Mode::WRITE;
    }
    else if(*opt_mode == "read-write")
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
    if(getrusage(RUSAGE_SELF, &usage) != 0)
    {
      throw std::runtime_error("getrusage failed");
    }

    return {
      usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1000000.0,
      usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / 1000000.0
    };
  }

  std::string
  format_float(double value)
  {
    std::ostringstream out;
    out << std::fixed << std::setprecision(6) << value;
    return out.str();
  }

  Generics::ConstSmartMemBuf_var
  make_profile(
    const std::uint64_t thread_index,
    const std::uint64_t operation_index)
  {
    const std::string body =
      "RocksDBBatchingProfileMapPerfTest/" +
      std::to_string(thread_index) + "/" +
      std::to_string(operation_index);
    return Generics::ConstSmartMemBuf_var(
      new Generics::ConstSmartMemBuf(body.data(), body.size()));
  }

  template<typename ProfileMap>
  AdServer::Commons::StartableAwaitable<Generics::ConstSmartMemBuf_var>
  co_read_profile(
    ProfileMap& profile_map,
    const std::string& key)
  {
    co_return co_await profile_map.co_get_profile(key);
  }

  template<typename ProfileMap>
  AdServer::Commons::StartableAwaitable<bool>
  co_write_profile(
    ProfileMap& profile_map,
    const std::string& key,
    const Generics::ConstSmartMemBuf* profile)
  {
    co_await profile_map.co_save_profile(
      key,
      profile,
      Generics::Time::get_time_of_day());
    co_return true;
  }
}

int
main(int argc, char** argv)
{
  try
  {
    using ProfileMap =
      AdServer::ProfilingCommons::RocksDBBatchingProfileMap<std::string>;

    const Options options = parse_options(argc, argv);
    const auto data_root = std::filesystem::absolute(options.data_root);
    std::filesystem::create_directories(data_root);

    ProfileMap profile_map(
      data_root.string(),
      Generics::Time(604800),
      options.batching_threads,
      128,
      Generics::Time::ZERO,
      options.disable_wal,
      options.enqueue_buckets);
    profile_map.activate_object();

    for(std::size_t key_index = 0; key_index < options.key_count; ++key_index)
    {
      const std::string key = "profile/" + std::to_string(key_index);
      const auto profile = make_profile(0, key_index);
      profile_map.save_profile(
        key,
        profile.in(),
        Generics::Time::get_time_of_day());
    }

    std::atomic<std::uint64_t> next{0};
    std::atomic<std::uint64_t> errors{0};
    std::atomic<std::uint64_t> misses{0};
    std::atomic<std::uint64_t> reads{0};
    std::atomic<std::uint64_t> writes{0};

    const auto stats_started = profile_map.stats();
    const auto started_at = std::chrono::steady_clock::now();
    const CpuTimes cpu_started = current_cpu_times();

    std::vector<std::thread> workers;
    workers.reserve(options.threads);

    for(std::size_t thread_index = 0; thread_index < options.threads; ++thread_index)
    {
      workers.emplace_back(
        [&, thread_index]()
        {
          const auto run = [&]<bool read_profiles, bool write_profiles>()
          {
            while(true)
            {
              const auto operation_index =
                next.fetch_add(1, std::memory_order_relaxed);
              if(operation_index >= options.count)
              {
                break;
              }

              try
              {
                const std::string key = "profile/" + std::to_string(
                  thread_index % options.key_count);
                if constexpr(read_profiles)
                {
                  const auto profile = AdServer::Commons::sync_wait(
                    co_read_profile(profile_map, key));
                  reads.fetch_add(1, std::memory_order_relaxed);
                  if(!profile.in())
                  {
                    misses.fetch_add(1, std::memory_order_relaxed);
                  }
                }

                if constexpr(write_profiles)
                {
                  const Generics::ConstSmartMemBuf_var new_profile =
                    make_profile(thread_index, operation_index);
                  AdServer::Commons::sync_wait(
                    co_write_profile(
                      profile_map,
                      key,
                      new_profile.in()));
                  writes.fetch_add(1, std::memory_order_relaxed);
                }
              }
              catch(const std::exception& ex)
              {
                if(errors.fetch_add(1, std::memory_order_relaxed) < 10)
                {
                  std::cerr
                    << "worker #" << thread_index
                    << " operation #" << operation_index
                    << " failed: " << ex.what() << std::endl;
                }
              }
            }
          };

          switch(options.mode)
          {
            case Mode::READ:
              run.template operator()<true, false>();
              break;
            case Mode::WRITE:
              run.template operator()<false, true>();
              break;
            case Mode::READ_WRITE:
              run.template operator()<true, true>();
              break;
          }
        });
    }

    for(auto& worker : workers)
    {
      worker.join();
    }

    auto stats = profile_map.stats();
    stats.logical_read_operations -= stats_started.logical_read_operations;
    stats.logical_write_operations -= stats_started.logical_write_operations;
    stats.physical_read_operations -= stats_started.physical_read_operations;
    stats.physical_write_operations -= stats_started.physical_write_operations;
    const CpuTimes cpu_finished = current_cpu_times();
    const auto finished_at = std::chrono::steady_clock::now();
    const double elapsed =
      std::chrono::duration<double>(finished_at - started_at).count();
    const double user_cpu = cpu_finished.user - cpu_started.user;
    const double sys_cpu = cpu_finished.sys - cpu_started.sys;

    profile_map.deactivate_object();
    profile_map.wait_object();

    std::cout
      << "data_root=" << data_root.string() << '\n'
      << "threads=" << options.threads << '\n'
      << "batching_threads=" << options.batching_threads << '\n'
      << "enqueue_buckets=" << options.enqueue_buckets << '\n'
      << "key_count=" << options.key_count << '\n'
      << "mode=" << mode_name(options.mode) << '\n'
      << "disable_wal=" << options.disable_wal << '\n'
      << "operations=" << options.count << '\n'
      << "reads=" << reads.load(std::memory_order_relaxed) << '\n'
      << "writes=" << writes.load(std::memory_order_relaxed) << '\n'
      << "misses=" << misses.load(std::memory_order_relaxed) << '\n'
      << "errors=" << errors.load(std::memory_order_relaxed) << '\n'
      << "logical_read_operations=" << stats.logical_read_operations << '\n'
      << "logical_write_operations=" << stats.logical_write_operations << '\n'
      << "physical_read_operations=" << stats.physical_read_operations << '\n'
      << "physical_write_operations=" << stats.physical_write_operations << '\n'
      << "elapsed_sec=" << format_float(elapsed) << '\n'
      << "cycles_per_sec=" << format_float(options.count / elapsed) << '\n'
      << "profile_ops_per_sec=" << format_float(
        (reads.load(std::memory_order_relaxed) +
          writes.load(std::memory_order_relaxed)) / elapsed) << '\n'
      << "cpu_sec=" << format_float(user_cpu + sys_cpu) << '\n'
      << "user_cpu_sec=" << format_float(user_cpu) << '\n'
      << "sys_cpu_sec=" << format_float(sys_cpu) << std::endl;

    return errors.load(std::memory_order_relaxed) == 0 ? 0 : 2;
  }
  catch(const std::exception& ex)
  {
    std::cerr << "Caught std::exception: " << ex.what() << std::endl;
    print_usage();
    return 1;
  }
}
