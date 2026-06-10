#include <atomic>
#include <chrono>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <sys/resource.h>

#include <Commons/ExecutorPool.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Generics/AppUtils.hpp>
#include <Logger/ActiveObjectCallback.hpp>
#include <Logger/StreamLogger.hpp>
#include <ProfilingCommons/ProfileMap/ProfileMapFactory.hpp>
#include <UserInfoSvcs/UserInfoCommons/ChannelDictionary.hpp>
#include <UserInfoSvcs/UserInfoCommons/FreqCapConfig.hpp>

#define private public
#define protected public
#include <UserInfoSvcs/UserInfoManager/UserInfoManagerCore.hpp>
#undef protected
#undef private

#include <xsd/UserInfoSvcs/UserInfoManagerConfig.hpp>

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
      << "Usage: UserInfoManagerCorePerfTest --cache-root <path> --count <N> [OPTIONS]\n"
      << "Options:\n"
      << "  --chunk-count <N>  chunks count (default: 16)\n"
      << "  --threads <N>      worker threads count (default: 1)\n"
      << "  --count <N>        total match operations\n";
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
      name << "UserChunk_" << i;
      std::filesystem::create_directories(cache_root / name.str());
    }
  }

  void
  write_chunks_config(std::ostream& out, const char* name)
  {
    out
      << "      <cfg:" << name << " rw_buffer_size=\"1048576\""
      << " rwlevel_max_size=\"10485760\" max_undumped_size=\"10485760\""
      << " max_levels0=\"20\" expire_time=\"604800\"/>\n";
  }

  std::filesystem::path
  write_config(
    const std::filesystem::path& cache_root,
    const std::size_t chunk_count)
  {
    const auto config_path = cache_root / "UserInfoManagerCorePerfTest.xml";
    const auto log_path = cache_root / "UserInfoManagerCorePerfTest";
    const auto pid_path = cache_root / "UserInfoManagerCorePerfTest.pid";
    std::ofstream out(config_path);
    if(!out)
    {
      throw std::runtime_error("can't create config file: " + config_path.string());
    }

    out
      << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
      << "<cfg:AdConfiguration\n"
      << "  xmlns:cfg=\"http://www.adintelligence.net/xsd/AdServer/Configuration\"\n"
      << "  xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
      << "  xsi:schemaLocation=\"http://www.adintelligence.net/xsd/AdServer/Configuration "
      << USER_INFO_MANAGER_CONFIG_XSD << "\">\n"
      << "  <cfg:UserInfoManagerConfig host=\"localhost\" channels_update_period=\"3600\""
      << " colo_id=\"1\" session_timeout=\"1800\" max_base_profile_waiters=\"0\""
      << " max_temp_profile_waiters=\"0\" max_freqcap_profile_waiters=\"0\""
      << " repeat_trigger_timeout=\"0\" history_optimization_period=\"3600\""
      << " root_dir=\"" << cache_root.string() << "\""
      << " pid_file=\"" << pid_path.string() << "\" service_index=\"0\">\n"
      << "    <cfg:Logger filename=\"" << log_path.string() << "\" log_level=\"3\">\n"
      << "      <cfg:Suffix max_log_level=\"8\" name=\".log\"/>\n"
      << "    </cfg:Logger>\n"
      << "    <cfg:Storage common_chunks_number=\"" << chunk_count
      << "\" chunks_root=\"" << cache_root.string() << "/\">\n";

    write_chunks_config(out, "AddChunksConfig");
    write_chunks_config(out, "TempChunksConfig");
    write_chunks_config(out, "HistoryChunksConfig");
    write_chunks_config(out, "BaseChunksConfig");
    write_chunks_config(out, "FreqCapChunksConfig");

    out
      << "    </cfg:Storage>\n"
      << "    <cfg:CampaignServerCorbaRef name=\"CampaignServer\">\n"
      << "      <cfg:Ref ref=\"corbaloc:iiop:localhost:1/CampaignServer\"/>\n"
      << "    </cfg:CampaignServerCorbaRef>\n"
      << "    <cfg:FreqCaps confirm_timeout=\"1\"/>\n"
      << "  </cfg:UserInfoManagerConfig>\n"
      << "</cfg:AdConfiguration>\n";

    return config_path;
  }

  AdServer::UserInfoSvcs::UserInfoManagerCore::UserInfoManagerConfig
  read_config(const std::filesystem::path& config_path)
  {
    using namespace xsd::AdServer::Configuration;

    Config::ErrorHandler error_handler;
    std::unique_ptr<AdConfigurationType> ad_configuration =
      AdConfiguration(config_path.c_str(), error_handler);

    if(error_handler.has_errors())
    {
      std::string error_string;
      throw std::runtime_error(error_handler.text(error_string));
    }

    return ad_configuration->UserInfoManagerConfig();
  }

  void
  wait_storage_loaded(AdServer::UserInfoSvcs::UserInfoManagerCore* core)
  {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(60);

    while(std::chrono::steady_clock::now() < deadline)
    {
      auto accessor = core->user_info_container_->get_accessor();
      if(accessor.get().in())
      {
        return;
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    throw std::runtime_error("UserInfoManagerCore storage is not ready after 60 seconds");
  }

  class TestUserInfoConfigSource:
    public AdServer::UserInfoSvcs::UserInfoConfigSource
  {
  public:
    Config get_config() override
    {
      Config config;
      config.channels_config = new AdServer::UserInfoSvcs::ChannelDictionary();
      config.freq_cap_config = new AdServer::UserInfoSvcs::FreqCapConfig();
      config.freq_cap_config->confirm_timeout = Generics::Time(1);

      for(unsigned long channel_id = 1; channel_id <= 1000; ++channel_id)
      {
        AdServer::UserInfoSvcs::ChannelIntervalsPack_var intervals =
          new AdServer::UserInfoSvcs::ChannelIntervalsPack();
        intervals->contextual = true;
        intervals->zero_channel = true;
        intervals->weight = 1;

        if(channel_id <= 100)
        {
          intervals->today_long_intervals.insert(
            AdServer::UserInfoSvcs::ChannelInterval(
              Generics::Time::ZERO,
              Generics::Time(30 * 24 * 60 * 60),
              1,
              1));
        }

        config.channels_config->page_channels[channel_id] = intervals;
        config.channels_config->channel_features.insert(std::make_pair(
          channel_id,
          AdServer::UserInfoSvcs::ChannelFeatures(false, 0)));
      }

      return config;
    }
  };

  AdServer::Commons::UserId
  make_user_id(std::uint64_t index)
  {
    std::vector<unsigned char> bytes(AdServer::Commons::UserId::size());
    for(std::size_t i = 0; i < sizeof(index); ++i)
    {
      bytes[bytes.size() - 1 - i] =
        static_cast<unsigned char>((index >> (i * 8)) & 0xff);
    }

    return AdServer::Commons::UserId(bytes.begin(), bytes.end());
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
    const auto config_path = write_config(cache_root, options.chunk_count);
    const auto config = read_config(config_path);

    Logging::Logger_var logger(
      new Logging::OStream::Logger(Logging::OStream::Config(std::cerr, 3)));
    Generics::ActiveObjectCallback_var callback(
      new Logging::ActiveObjectCallbackImpl(
        logger,
        "UserInfoManagerCorePerfTest",
        "UserInfoManagerCorePerfTest"));

    auto core = std::make_shared<AdServer::UserInfoSvcs::UserInfoManagerCore>(
      callback,
      logger,
      config,
      std::make_shared<TestUserInfoConfigSource>());

    core->activate_object();
    wait_storage_loaded(core.get());
    core->update_config_();

    std::atomic<std::uint64_t> next{0};
    std::atomic<std::uint64_t> errors{0};
    std::atomic<std::uint64_t> matched{0};

    const auto started_at = std::chrono::steady_clock::now();
    const CpuTimes cpu_started = current_cpu_times();

    std::vector<std::thread> workers;
    workers.reserve(options.threads);

    for(std::size_t thread_index = 0; thread_index < options.threads; ++thread_index)
    {
      workers.emplace_back(
        [&, thread_index]()
        {
          while(true)
          {
            const auto operation_index = next.fetch_add(1, std::memory_order_relaxed);
            if(operation_index >= options.count)
            {
              break;
            }

            try
            {
              AdServer::UserInfoSvcs::UserInfoManagerCore::UserInfo user_info;
              user_info.user_id = make_user_id(operation_index + 1);
              user_info.time = Generics::Time::get_time_of_day();
              user_info.request_colo_id = 1;
              user_info.current_colo_id = 1;

              AdServer::UserInfoSvcs::UserInfoManagerCore::MatchParams match_params;
              match_params.no_result = false;
              match_params.page_channel_ids.reserve(100);
              for(unsigned long channel_id = 1; channel_id <= 100; ++channel_id)
              {
                AdServer::UserInfoSvcs::UserInfoManagerCore::ChannelTriggerMatch
                  channel_trigger_match;
                channel_trigger_match.channel_id = channel_id;
                match_params.page_channel_ids.push_back(channel_trigger_match);
              }

              AdServer::UserInfoSvcs::UserInfoManagerCore::MatchResult match_result;
              if(core->match(user_info, match_params, match_result))
              {
                matched.fetch_add(1, std::memory_order_relaxed);
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
        }
      );
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

    core->deactivate_object();
    core->wait_object();

    std::cout
      << "cache_root=" << cache_root.string() << '\n'
      << "chunk_count=" << options.chunk_count << '\n'
      << "threads=" << options.threads << '\n'
      << "operations=" << options.count << '\n'
      << "errors=" << errors.load(std::memory_order_relaxed) << '\n'
      << "matched=" << matched.load(std::memory_order_relaxed) << '\n'
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
