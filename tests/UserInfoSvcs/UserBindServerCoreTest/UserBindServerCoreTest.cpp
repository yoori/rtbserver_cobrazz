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

#include <Generics/AppUtils.hpp>
#include <Logger/StreamLogger.hpp>
#include <UserInfoSvcs/UserBindServer/UserBindServerCore.hpp>

namespace
{
  struct Options
  {
    std::filesystem::path cache_root;
    std::size_t chunk_count = 16;
    std::size_t threads = 1;
    std::uint64_t count = 0;
  };

  struct CpuTimes
  {
    double user = 0.0;
    double sys = 0.0;
  };

  void
  print_usage()
  {
    std::cerr
      << "Usage: UserBindServerCoreTest --cache-root <path> --count <N> [OPTIONS]\n"
      << "Options:\n"
      << "  --chunk-count <N>  chunks count (default: 16)\n"
      << "  --threads <N>      worker threads count (default: 1)\n"
      << "  --count <N>        total get_user_id operations\n";
  }

  Options
  parse_options(int argc, char** argv)
  {
    using namespace Generics::AppUtils;

    StringOption opt_cache_root;
    Option<unsigned long> opt_chunk_count(16);
    Option<unsigned long> opt_threads(1);
    Option<unsigned long> opt_count(0);
    CheckOption opt_help;

    Args args(-1);
    args.add(equal_name("cache-root"), opt_cache_root);
    args.add(equal_name("chunk-count"), opt_chunk_count);
    args.add(equal_name("threads"), opt_threads);
    args.add(equal_name("count"), opt_count);
    args.add(equal_name("help") || short_name("h"), opt_help);

    args.parse(argc - 1, argv + 1);

    if(opt_help.enabled())
    {
      print_usage();
      std::exit(0);
    }

    Options options;
    options.cache_root = *opt_cache_root;
    options.chunk_count = *opt_chunk_count;
    options.threads = *opt_threads;
    options.count = *opt_count;

    if(options.cache_root.empty())
    {
      throw std::runtime_error("--cache-root is required");
    }

    if(options.chunk_count == 0)
    {
      throw std::runtime_error("--chunk-count must be > 0");
    }

    if(options.threads == 0)
    {
      throw std::runtime_error("--threads must be > 0");
    }

    if(options.count == 0)
    {
      throw std::runtime_error("--count must be > 0");
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

  void
  prepare_cache_root(
    const std::filesystem::path& cache_root,
    const std::size_t chunk_count)
  {
    std::filesystem::create_directories(cache_root);

    for(std::size_t i = 0; i < chunk_count; ++i)
    {
      std::ostringstream name;
      name << "Chunk_" << i << '_' << chunk_count;
      std::filesystem::create_directories(cache_root / name.str());
    }
  }

  AdServer::UserInfoSvcs::UserBindServerCore::Config
  make_config(
    const std::filesystem::path& cache_root,
    const std::size_t chunk_count)
  {
    AdServer::UserInfoSvcs::UserBindServerCore::Config config;
    config.storage.chunks_root = cache_root.string() + "/";
    config.storage.prefix = "UserSeen";
    config.storage.bound_prefix = "UserBind";
    config.storage.common_chunks_number = chunk_count;
    config.storage.expire_time = Generics::Time(604800);
    config.storage.bound_expire_time = Generics::Time(7257600);
    config.storage.dump_period = Generics::Time::ZERO;
    config.storage.portions = 1024;
    config.storage.load_slave = true;

    config.bind_request_storage.prefix = "BindRequest";
    config.bind_request_storage.common_chunks_number = chunk_count;
    config.bind_request_storage.expire_time = Generics::Time(259200);
    config.bind_request_storage.portions = 1024;

    config.min_age = Generics::Time::ZERO;
    config.bind_on_min_age = false;
    config.max_bad_event = 10000;
    config.partition_index = 0;
    config.partitions_number = 1;
    return config;
  }

  void
  wait_ready(AdServer::UserInfoSvcs::UserBindServerCore* core)
  {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(60);

    while(std::chrono::steady_clock::now() < deadline)
    {
      const auto user_bind_processor = core->user_bind_container()->get_object();
      const auto bind_request_processor = core->bind_request_container()->get_object();
      if(user_bind_processor.in() && bind_request_processor.in())
      {
        return;
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    throw std::runtime_error("UserBindServerCore is not ready after 60 seconds");
  }
}

int
main(int argc, char** argv)
{
  try
  {
    const Options options = parse_options(argc, argv);
    const auto cache_root = std::filesystem::absolute(options.cache_root);

    prepare_cache_root(cache_root, options.chunk_count);
    const auto config = make_config(cache_root, options.chunk_count);

    Logging::Logger_var logger(
      new Logging::OStream::Logger(Logging::OStream::Config(std::cerr, 3)));

    AdServer::UserInfoSvcs::UserBindServerCore_var core =
      new AdServer::UserInfoSvcs::UserBindServerCore(config, logger);

    core->activate_object();
    wait_ready(core);

    std::atomic<std::uint64_t> next{0};
    std::atomic<std::uint64_t> errors{0};
    std::atomic<std::uint64_t> created{0};
    std::atomic<std::uint64_t> found{0};

    const auto started_at = std::chrono::steady_clock::now();
    const CpuTimes cpu_started = current_cpu_times();

    std::vector<std::thread> workers;
    workers.reserve(options.threads);

    for(std::size_t thread_index = 0; thread_index < options.threads; ++thread_index)
    {
      workers.emplace_back([&, thread_index]()
      {
        const auto timestamp = Generics::Time::get_time_of_day();
        while(true)
        {
          const auto operation_index = next.fetch_add(1, std::memory_order_relaxed);
          if(operation_index >= options.count)
          {
            break;
          }

          try
          {
            AdServer::UserInfoSvcs::UserBindServerCore::GetUserRequestInfo request;
            request.id = "UserBindServerCoreTest/" + std::to_string(operation_index);
            request.timestamp = timestamp;
            request.create_timestamp = Generics::Time::ZERO;
            request.generate_user_id = false;

            const auto response = core->co_get_user_id(request).sync_wait();
            if(response.created)
            {
              created.fetch_add(1, std::memory_order_relaxed);
            }

            if(response.user_found)
            {
              found.fetch_add(1, std::memory_order_relaxed);
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
      });
    }

    for(auto& worker : workers)
    {
      worker.join();
    }

    const CpuTimes cpu_finished = current_cpu_times();
    const auto finished_at = std::chrono::steady_clock::now();
    const double elapsed = std::chrono::duration<double>(finished_at - started_at).count();
    const double user_cpu = cpu_finished.user - cpu_started.user;
    const double sys_cpu = cpu_finished.sys - cpu_started.sys;
    const auto stats = core->stats();

    core->deactivate_object();
    core->wait_object();

    std::cout
      << "cache_root=" << cache_root.string() << '\n'
      << "chunk_count=" << options.chunk_count << '\n'
      << "threads=" << options.threads << '\n'
      << "operations=" << options.count << '\n'
      << "errors=" << errors.load(std::memory_order_relaxed) << '\n'
      << "created=" << created.load(std::memory_order_relaxed) << '\n'
      << "found=" << found.load(std::memory_order_relaxed) << '\n'
      << "core_get_user_id_total=" << stats.get_user_id_total_requests << '\n'
      << "elapsed_sec=" << format_float(elapsed) << '\n'
      << "rps=" << format_float(options.count / elapsed) << '\n'
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
