#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <sys/resource.h>

#include <Generics/AppUtils.hpp>
#include <LogCommons/LogHolder.hpp>
#include <LogCommons/UserProperties.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

namespace
{
  constexpr std::uint64_t RECORDS_PER_REQUEST = 5;

  constexpr char COUNTRY[] = "CountryCode";
  constexpr char CLIENT_VERSION[] = "ClientVersion";
  constexpr char OS_VERSION[] = "OsVersion";
  constexpr char BROWSER_VERSION[] = "BrowserVersion";
  constexpr char CLIENT_APP[] = "Client";

  struct Options
  {
    std::uint64_t count = 1000000;
    std::uint64_t threads = 1;
    std::uint64_t keys = 1024;
    unsigned long shards = 16;
  };

  struct CpuTimes
  {
    double user = 0.0;
    double sys = 0.0;
  };

  struct BenchmarkResult
  {
    double elapsed = 0.0;
    double user_cpu = 0.0;
    double sys_cpu = 0.0;
    std::uint64_t checksum = 0;
  };

  struct RequestInfoSample
  {
    Generics::Time time;
    Generics::Time isp_time;
    std::uint32_t colo_id = 1;
    char user_status = 'I';
    bool test_request = false;
    std::string country;
    std::string client_app_version;
    std::string os_version;
    std::string browser_version;
    std::string client_app;
  };

  using UserPropertiesTraits = AdServer::LogProcessing::UserPropertiesTraits;
  using ShardedLogHolder = AdServer::LogProcessing::LogHolderSharded<UserPropertiesTraits>;

  template<typename LogHolderType>
  class UserPropertiesLogger final: public LogHolderType
  {
  public:
    using Collector = UserPropertiesTraits::CollectorType;

    template<typename... Args>
    explicit UserPropertiesLogger(Args&&... args)
      : LogHolderType(std::forward<Args>(args)...),
        stat_unverified_imp_one_(0, 0, 1, 0, 0, 0)
    {}

    void
    process_request(const RequestInfoSample& request_info)
    {
      log_record_(request_info, stat_unverified_imp_one_);
    }

  protected:
    ~UserPropertiesLogger() noexcept override = default;

  private:
    void
    log_record_(const RequestInfoSample& request_info, const Collector::DataT& data)
    {
      if (!request_info.test_request)
      {
        this->add_record(
          Collector::KeyT(
            request_info.time,
            request_info.isp_time,
            request_info.colo_id,
            request_info.user_status,
            COUNTRY,
            request_info.country),
          data);
        this->add_record(
          Collector::KeyT(
            request_info.time,
            request_info.isp_time,
            request_info.colo_id,
            request_info.user_status,
            CLIENT_VERSION,
            request_info.client_app_version),
          data);
        this->add_record(
          Collector::KeyT(
            request_info.time,
            request_info.isp_time,
            request_info.colo_id,
            request_info.user_status,
            OS_VERSION,
            request_info.os_version),
          data);
        this->add_record(
          Collector::KeyT(
            request_info.time,
            request_info.isp_time,
            request_info.colo_id,
            request_info.user_status,
            BROWSER_VERSION,
            request_info.browser_version),
          data);
        this->add_record(
          Collector::KeyT(
            request_info.time,
            request_info.isp_time,
            request_info.colo_id,
            request_info.user_status,
            CLIENT_APP,
            request_info.client_app),
          data);
      }
    }

    const Collector::DataT stat_unverified_imp_one_;
  };

  using ShardedUserPropertiesLogger = UserPropertiesLogger<ShardedLogHolder>;

  template<typename Logger>
  using Logger_var = ReferenceCounting::SmartPtr<Logger>;

  void
  print_usage()
  {
    std::cerr
      << "Usage: UserPropertiesLoggerPerfTest [OPTIONS]\n"
      << "Options:\n"
      << "  --count <N>    process_request calls count (default: 1000000)\n"
      << "  --threads <N>  producer threads count (default: 1)\n"
      << "  --keys <N>     request property combinations count (default: 1024)\n"
      << "  --shards <N>   LogHolderSharded shard count (default: 16)\n";
  }

  Options
  parse_options(int argc, char** argv)
  {
    using namespace Generics::AppUtils;

    Option<unsigned long> opt_count(1000000);
    Option<unsigned long> opt_threads(1);
    Option<unsigned long> opt_keys(1024);
    Option<unsigned long> opt_shards(16);
    CheckOption opt_help;

    Args args(-1);
    args.add(equal_name("count"), opt_count);
    args.add(equal_name("threads"), opt_threads);
    args.add(equal_name("keys"), opt_keys);
    args.add(equal_name("shards"), opt_shards);
    args.add(equal_name("help") || short_name("h"), opt_help);
    args.parse(argc - 1, argv + 1);

    if (opt_help.enabled())
    {
      print_usage();
      std::exit(0);
    }

    Options options;
    options.count = *opt_count;
    options.threads = *opt_threads;
    options.keys = *opt_keys;
    options.shards = *opt_shards;

    if (options.count == 0)
    {
      throw std::runtime_error("--count must be > 0");
    }

    if (options.count > std::numeric_limits<std::uint64_t>::max() / RECORDS_PER_REQUEST)
    {
      throw std::runtime_error("--count is too large");
    }

    if (options.threads == 0)
    {
      throw std::runtime_error("--threads must be > 0");
    }

    if (options.keys == 0)
    {
      throw std::runtime_error("--keys must be > 0");
    }

    if (options.shards == 0)
    {
      throw std::runtime_error("--shards must be > 0");
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

  std::string
  format_float(double value)
  {
    std::ostringstream out;
    out << std::fixed << std::setprecision(6) << value;
    return out.str();
  }

  std::vector<RequestInfoSample>
  make_samples(std::uint64_t key_count)
  {
    constexpr const char* countries[] = {"RUS", "USA", "DEU", "BRA", "IND"};
    constexpr const char* clients[] = {"Chrome", "Firefox", "Safari", "YandexBrowser"};
    constexpr const char* operating_systems[] = {"Linux", "Windows", "Android", "iOS"};

    std::vector<RequestInfoSample> samples;
    samples.reserve(key_count);

    const Generics::Time base_time(1787958000);
    for (std::uint64_t i = 0; i < key_count; ++i)
    {
      RequestInfoSample sample;
      sample.time = base_time + Generics::Time(i % 24 * 60 * 60);
      sample.isp_time = sample.time;
      sample.colo_id = 1 + i % 3;
      sample.country = countries[i % std::size(countries)];
      sample.client_app_version = std::to_string(100 + i % 32) + ".0";
      sample.os_version = operating_systems[i % std::size(operating_systems)];
      sample.browser_version = std::to_string(100 + i % 64) + ".0.1";
      sample.client_app = clients[i % std::size(clients)];
      samples.emplace_back(std::move(sample));
    }

    return samples;
  }

  template<typename Logger>
  __attribute__((noinline))
  std::uint64_t
  user_properties_logger_process_request_benchmark(
    Logger& logger,
    const std::vector<RequestInfoSample>& samples,
    std::uint64_t count,
    std::uint64_t offset)
  {
    std::uint64_t checksum = 0;

    for (std::uint64_t i = 0; i < count; ++i)
    {
      const RequestInfoSample& sample = samples[(i + offset) % samples.size()];
      logger.process_request(sample);
      checksum += sample.country.size() + sample.client_app_version.size() +
        sample.os_version.size() + sample.browser_version.size() + sample.client_app.size();
    }

    return checksum;
  }

  template<typename Logger>
  BenchmarkResult
  run_benchmark(
    Logger& logger,
    const Options& options,
    const std::vector<RequestInfoSample>& samples)
  {
    std::uint64_t checksum = 0;
    CpuTimes cpu_started;
    std::chrono::steady_clock::time_point started_at;

    if (options.threads == 1)
    {
      cpu_started = current_cpu_times();
      started_at = std::chrono::steady_clock::now();
      checksum = user_properties_logger_process_request_benchmark(
        logger,
        samples,
        options.count,
        0);
    }
    else
    {
      std::atomic<std::uint64_t> ready_threads = 0;
      std::atomic<bool> start = false;
      std::vector<std::thread> threads;
      std::vector<std::uint64_t> checksums(options.threads, 0);
      threads.reserve(options.threads);

      const std::uint64_t base_count = options.count / options.threads;
      const std::uint64_t extra_count = options.count % options.threads;
      for (std::uint64_t thread_i = 0; thread_i < options.threads; ++thread_i)
      {
        const std::uint64_t thread_count = base_count + (thread_i < extra_count ? 1 : 0);
        const std::uint64_t offset = thread_i * base_count +
          (thread_i < extra_count ? thread_i : extra_count);
        threads.emplace_back(
          [
            &logger,
            &samples,
            &checksums,
            &ready_threads,
            &start,
            thread_i,
            thread_count,
            offset
          ]()
          {
            ready_threads.fetch_add(1, std::memory_order_release);
            while (!start.load(std::memory_order_acquire))
            {
              std::this_thread::yield();
            }

            checksums[thread_i] = user_properties_logger_process_request_benchmark(
              logger,
              samples,
              thread_count,
              offset);
          });
      }

      while (ready_threads.load(std::memory_order_acquire) != options.threads)
      {
        std::this_thread::yield();
      }

      cpu_started = current_cpu_times();
      started_at = std::chrono::steady_clock::now();
      start.store(true, std::memory_order_release);

      for (auto& thread : threads)
      {
        thread.join();
      }

      for (const std::uint64_t thread_checksum : checksums)
      {
        checksum += thread_checksum;
      }
    }

    const auto finished_at = std::chrono::steady_clock::now();
    const CpuTimes cpu_finished = current_cpu_times();

    logger.flush_if_required(Generics::Time::get_time_of_day());

    return {
      std::chrono::duration<double>(finished_at - started_at).count(),
      cpu_finished.user - cpu_started.user,
      cpu_finished.sys - cpu_started.sys,
      checksum
    };
  }

  void
  print_result(const Options& options, const BenchmarkResult& result)
  {
    const std::uint64_t records = options.count * RECORDS_PER_REQUEST;

    std::cout
      << "requests=" << options.count << '\n'
      << "records=" << records << '\n'
      << "threads=" << options.threads << '\n'
      << "keys=" << options.keys << '\n'
      << "shards=" << options.shards << '\n'
      << "elapsed_sec=" << format_float(result.elapsed) << '\n'
      << "requests_per_sec="
      << format_float(static_cast<double>(options.count) / result.elapsed) << '\n'
      << "records_per_sec="
      << format_float(static_cast<double>(records) / result.elapsed) << '\n'
      << "user_cpu_sec=" << format_float(result.user_cpu) << '\n'
      << "sys_cpu_sec=" << format_float(result.sys_cpu) << '\n'
      << "cpu_sec=" << format_float(result.user_cpu + result.sys_cpu) << '\n'
      << "checksum=" << result.checksum << '\n';
  }
}

int
main(int argc, char** argv)
{
  try
  {
    const Options options = parse_options(argc, argv);
    const std::vector<RequestInfoSample> samples = make_samples(options.keys);
    const AdServer::LogProcessing::LogFlushTraits flush_traits(Generics::Time::ONE_DAY, "");
    using SavePolicy = AdServer::LogProcessing::DefaultSavePolicy<UserPropertiesTraits>;
    Logger_var<ShardedUserPropertiesLogger> logger(
      new ShardedUserPropertiesLogger(flush_traits, SavePolicy(), options.shards));
    const BenchmarkResult result = run_benchmark(*logger, options, samples);

    print_result(options, result);

    return 0;
  }
  catch (const std::exception& ex)
  {
    std::cerr << "Caught std::exception: " << ex.what() << std::endl;
  }

  return 1;
}
