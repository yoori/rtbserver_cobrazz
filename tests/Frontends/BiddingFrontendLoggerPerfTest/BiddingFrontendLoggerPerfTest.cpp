#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <sys/resource.h>

#include <Generics/AppUtils.hpp>
#include <Logger/ActiveObjectCallback.hpp>
#include <Logger/Logger.hpp>

#include <Frontends/Modules/BiddingFrontend/BiddingFrontendLogger.hpp>

namespace
{
  struct Options
  {
    std::uint64_t count = 1000000;
    std::uint64_t threads = 1;
    unsigned long geo_shards = 16;
    std::uint64_t keys = 1024;
    unsigned long geo_period = 24 * 60 * 60;
    std::filesystem::path log_root = "/tmp/BiddingFrontendLoggerPerfTest";
  };

  struct CpuTimes
  {
    double user = 0.0;
    double sys = 0.0;
  };

  struct GeoSample
  {
    std::string ip;
    std::string source;
    AdServer::CampaignSvcs::CoordDecimal lat;
    AdServer::CampaignSvcs::CoordDecimal lon;
    std::string type;
    std::string country;
    std::string region;
    std::string city;
  };

  void
  print_usage()
  {
    std::cerr
      << "Usage: BiddingFrontendLoggerPerfTest [OPTIONS]\n"
      << "Options:\n"
      << "  --count <N>       process_geo calls count (default: 1000000)\n"
      << "  --threads <N>     producer threads count (default: 1)\n"
      << "  --geo-shards <N>  GeoLogger shard count (default: 16)\n"
      << "  --keys <N>        unique ip/source key pool size (default: 1024)\n"
      << "  --geo-period <S>  flush period in seconds (default: 86400)\n"
      << "  --log-root <P>    Geo log output dir "
        "(default: /tmp/BiddingFrontendLoggerPerfTest)\n";
  }

  Options
  parse_options(int argc, char** argv)
  {
    using namespace Generics::AppUtils;

    Option<unsigned long> opt_count(1000000);
    Option<unsigned long> opt_threads(1);
    Option<unsigned long> opt_geo_shards(16);
    Option<unsigned long> opt_keys(1024);
    Option<unsigned long> opt_geo_period(24 * 60 * 60);
    StringOption opt_log_root("/tmp/BiddingFrontendLoggerPerfTest");
    CheckOption opt_help;

    Args args(-1);
    args.add(equal_name("count"), opt_count);
    args.add(equal_name("threads"), opt_threads);
    args.add(equal_name("geo-shards"), opt_geo_shards);
    args.add(equal_name("keys"), opt_keys);
    args.add(equal_name("geo-period"), opt_geo_period);
    args.add(equal_name("log-root"), opt_log_root);
    args.add(equal_name("help") || short_name("h"), opt_help);

    args.parse(argc - 1, argv + 1);

    if(opt_help.enabled())
    {
      print_usage();
      std::exit(0);
    }

    Options options;
    options.count = *opt_count;
    options.threads = *opt_threads;
    options.geo_shards = *opt_geo_shards;
    options.keys = *opt_keys;
    options.geo_period = *opt_geo_period;
    options.log_root = *opt_log_root;

    if(options.count == 0)
    {
      throw std::runtime_error("--count must be > 0");
    }

    if(options.threads == 0)
    {
      throw std::runtime_error("--threads must be > 0");
    }

    if(options.geo_shards == 0)
    {
      throw std::runtime_error("--geo-shards must be > 0");
    }

    if(options.keys == 0)
    {
      throw std::runtime_error("--keys must be > 0");
    }

    if(options.geo_period == 0)
    {
      throw std::runtime_error("--geo-period must be > 0");
    }

    if(options.log_root.empty())
    {
      throw std::runtime_error("--log-root must not be empty");
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

  std::vector<GeoSample>
  make_geo_samples(std::uint64_t key_count)
  {
    std::vector<GeoSample> samples;
    samples.reserve(key_count);

    for(std::uint64_t i = 0; i < key_count; ++i)
    {
      GeoSample sample;
      sample.ip =
        "85.249." + std::to_string((i / 256) % 256) + "." +
        std::to_string(i % 256);
      sample.source = (i % 2 == 0 ? "msc" : "app");
      sample.lat = AdServer::CampaignSvcs::CoordDecimal("45.0355");
      sample.lon = AdServer::CampaignSvcs::CoordDecimal("38.975");
      sample.type = "2";
      sample.country = "RUS";
      sample.region = "RU-KDA";
      sample.city = "Krasnodar";

      samples.emplace_back(std::move(sample));
    }

    return samples;
  }

  void
  assign_mono_string(Generics::MonoString& target, const std::string& value)
  {
    target.assign(value.data(), value.size());
  }

  void
  fill_geo_params(
    AdServer::Bidding::BiddingFrontendLogger::GeoParams& params,
    const GeoSample& sample)
  {
    assign_mono_string(params.ip, sample.ip);
    assign_mono_string(params.source, sample.source);
    params.lat = sample.lat;
    params.lon = sample.lon;
    assign_mono_string(params.type, sample.type);
    assign_mono_string(params.country, sample.country);
    assign_mono_string(params.region, sample.region);
    assign_mono_string(params.city, sample.city);
  }
}

extern "C"
{
  __attribute__((noinline))
  std::uint64_t
  bidding_frontend_logger_process_geo_benchmark(
    AdServer::Bidding::BiddingFrontendLogger& logger,
    const std::vector<GeoSample>& samples,
    std::uint64_t count,
    std::uint64_t offset)
  {
    std::uint64_t checksum = 0;

    for(std::uint64_t i = 0; i < count; ++i)
    {
      const GeoSample& sample = samples[(i + offset) % samples.size()];
      AdServer::Bidding::BiddingFrontendLogger::GeoParams params;
      fill_geo_params(params, sample);
      checksum += sample.ip.size() + sample.source.size();
      logger.process_geo(std::move(params));
    }

    return checksum;
  }
}

int
main(int argc, char** argv)
{
  try
  {
    const Options options = parse_options(argc, argv);
    std::filesystem::create_directories(options.log_root);

    Logging::Logger_var logger(new Logging::Null::Logger);
    Generics::ActiveObjectCallback_var callback(
      new Logging::ActiveObjectCallbackImpl(
        logger,
        "BiddingFrontendLoggerPerfTest"));
    AdServer::Bidding::BiddingFrontendLogger_var frontend_logger(
      new AdServer::Bidding::BiddingFrontendLogger(
        callback,
        logger,
        options.log_root.string(),
        Generics::Time(options.geo_period),
        options.geo_shards));

    frontend_logger->activate_object();

    const std::vector<GeoSample> samples = make_geo_samples(options.keys);
    const auto started_at = std::chrono::steady_clock::now();
    const CpuTimes cpu_started = current_cpu_times();

    std::uint64_t checksum = 0;
    if(options.threads == 1)
    {
      checksum = bidding_frontend_logger_process_geo_benchmark(
        *frontend_logger,
        samples,
        options.count,
        0);
    }
    else
    {
      std::vector<std::thread> threads;
      std::vector<std::uint64_t> checksums(options.threads, 0);
      threads.reserve(options.threads);

      const std::uint64_t base_count = options.count / options.threads;
      const std::uint64_t extra_count = options.count % options.threads;
      for(std::uint64_t thread_i = 0; thread_i < options.threads; ++thread_i)
      {
        const std::uint64_t thread_count =
          base_count + (thread_i < extra_count ? 1 : 0);
        threads.emplace_back(
          [
            &frontend_logger,
            &samples,
            &checksums,
            thread_i,
            thread_count
          ]()
          {
            checksums[thread_i] =
              bidding_frontend_logger_process_geo_benchmark(
                *frontend_logger,
                samples,
                thread_count,
                thread_i);
          });
      }

      for(auto& thread : threads)
      {
        thread.join();
      }

      for(const std::uint64_t thread_checksum : checksums)
      {
        checksum += thread_checksum;
      }
    }

    const CpuTimes cpu_finished = current_cpu_times();
    const auto finished_at = std::chrono::steady_clock::now();

    frontend_logger->deactivate_object();
    frontend_logger->wait_object();

    const double elapsed = std::chrono::duration<double>(
      finished_at - started_at).count();
    const double user_cpu = cpu_finished.user - cpu_started.user;
    const double sys_cpu = cpu_finished.sys - cpu_started.sys;
    const double rate = static_cast<double>(options.count) / elapsed;

    std::cout
      << "count=" << options.count << '\n'
      << "threads=" << options.threads << '\n'
      << "geo_shards=" << options.geo_shards << '\n'
      << "keys=" << options.keys << '\n'
      << "log_root=" << options.log_root.string() << '\n'
      << "elapsed_sec=" << format_float(elapsed) << '\n'
      << "rate_per_sec=" << format_float(rate) << '\n'
      << "user_cpu_sec=" << format_float(user_cpu) << '\n'
      << "sys_cpu_sec=" << format_float(sys_cpu) << '\n'
      << "cpu_sec=" << format_float(user_cpu + sys_cpu) << '\n'
      << "checksum=" << checksum << '\n';

    return 0;
  }
  catch(const std::exception& ex)
  {
    std::cerr << "Caught std::exception: " << ex.what() << std::endl;
  }

  return 1;
}
