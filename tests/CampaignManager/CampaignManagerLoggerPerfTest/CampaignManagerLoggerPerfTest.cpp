#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <sys/resource.h>

#include <Generics/AppUtils.hpp>
#include <Logger/Logger.hpp>

#include <CampaignSvcs/CampaignManager/CampaignManagerLogger.hpp>

namespace
{
  struct Options
  {
    unsigned long count = 0;
    unsigned long threads = 1;
    unsigned long profiling_type = AdServer::CampaignSvcs::PT_ALL;
    unsigned long channel_trigger_id_count = 1000000;
    std::filesystem::path log_root = "/tmp/CampaignManagerLoggerPerfTest";
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
      << "Usage: CampaignManagerLoggerPerfTest --count <N> [OPTIONS]\n"
      << "Options:\n"
      << "  --count <N>           process_request calls count\n"
      << "  --threads <N>         logger worker threads (default: 1)\n"
      << "  --profiling-type <N>  profiling mask (default: PT_ALL)\n"
      << "  --channel-trigger-id-count <N>\n"
      << "                        random channel_trigger_id range size (default: 1000000)\n"
      << "  --log-root <path>     output log root (default: /tmp/CampaignManagerLoggerPerfTest)\n";
  }

  Options
  parse_options(int argc, char** argv)
  {
    using namespace Generics::AppUtils;

    Option<unsigned long> opt_count(0);
    Option<unsigned long> opt_threads(1);
    Option<unsigned long> opt_profiling_type(AdServer::CampaignSvcs::PT_ALL);
    Option<unsigned long> opt_channel_trigger_id_count(1000000);
    StringOption opt_log_root("/tmp/CampaignManagerLoggerPerfTest");
    CheckOption opt_help;

    Args args(-1);
    args.add(equal_name("count"), opt_count);
    args.add(equal_name("threads"), opt_threads);
    args.add(equal_name("profiling-type"), opt_profiling_type);
    args.add(equal_name("channel-trigger-id-count"), opt_channel_trigger_id_count);
    args.add(equal_name("log-root"), opt_log_root);
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
    options.profiling_type = *opt_profiling_type;
    options.channel_trigger_id_count = *opt_channel_trigger_id_count;
    options.log_root = *opt_log_root;

    if (options.count == 0)
    {
      throw std::runtime_error("--count must be > 0");
    }

    if (options.threads == 0)
    {
      throw std::runtime_error("--threads must be > 0");
    }

    if (options.channel_trigger_id_count == 0)
    {
      throw std::runtime_error("--channel-trigger-id-count must be > 0");
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

  void
  init_flush_traits(
    AdServer::LogProcessing::LogFlushTraits& traits,
    const std::filesystem::path& root,
    const char* name)
  {
    traits.period = Generics::Time(24 * 60 * 60);
    traits.out_dir = (root / name).string();
    std::filesystem::create_directories(traits.out_dir);
  }

  AdServer::CampaignSvcs::CampaignManagerLogger::Params
  make_logger_params(const Options& options)
  {
    AdServer::CampaignSvcs::CampaignManagerLogger::Params params;

    init_flush_traits(params.request_basic_channels, options.log_root, "RequestBasicChannels");
    init_flush_traits(params.opt_out_stat, options.log_root, "OptOutStat");
    init_flush_traits(params.creative_stat, options.log_root, "CreativeStat");
    init_flush_traits(params.action_request, options.log_root, "ActionRequest");
    init_flush_traits(params.request, options.log_root, "Request");
    init_flush_traits(params.impression, options.log_root, "Impression");
    init_flush_traits(params.click, options.log_root, "Click");
    init_flush_traits(params.advertiser_action, options.log_root, "AdvertiserAction");
    init_flush_traits(params.passback_impression, options.log_root, "PassbackImpression");
    init_flush_traits(params.user_properties, options.log_root, "UserProperties");
    init_flush_traits(params.tag_request, options.log_root, "TagRequest");
    init_flush_traits(params.tag_position_stat, options.log_root, "TagPositionStat");
    init_flush_traits(params.ccg_stat, options.log_root, "CcgStat");
    init_flush_traits(params.cc_stat, options.log_root, "CcStat");
    init_flush_traits(params.search_term_stat, options.log_root, "SearchTermStat");
    init_flush_traits(params.search_engine_stat, options.log_root, "SearchEngineStat");
    init_flush_traits(params.tag_auction_stat, options.log_root, "TagAuctionStat");
    init_flush_traits(params.passback_stat, options.log_root, "PassbackStat");
    init_flush_traits(params.user_agent_stat, options.log_root, "UserAgentStat");
    init_flush_traits(params.prof_research, options.log_root, "ProfilingResearch");
    init_flush_traits(params.web_stat, options.log_root, "WebStat");
    init_flush_traits(params.research_web_stat, options.log_root, "ResearchWebStat");

    params.request_basic_channels.inventory_users_percentage = 100.0;
    params.request_basic_channels.distrib_count = 24;
    params.request_basic_channels.dump_channel_triggers = true;
    params.request_basic_channels.adrequest_anonymize = false;

    params.threads = options.threads;

    return params;
  }

  AdServer::Commons::StringHolder_var
  string_holder(const char* value)
  {
    return new AdServer::Commons::StringHolder(value);
  }

  void
  add_trigger(
    AdServer::CampaignSvcs::CampaignManagerLogger::RequestInfo& request_info,
    AdServer::CampaignSvcs::CampaignManagerLogger::TriggerChannelMap& triggers,
    AdServer::CampaignSvcs::ChannelIdHashSet& channel_set,
    unsigned long channel_id,
    unsigned long trigger_id)
  {
    triggers.emplace_back(channel_id, trigger_id);
    channel_set.insert(channel_id);
    request_info.triggered_channels.channels.insert(channel_id);
  }

  unsigned long
  next_random_channel_trigger_id(
    std::uint64_t& random_state,
    unsigned long channel_trigger_id_count)
  {
    random_state = random_state * 2862933555777941757ULL + 3037000493ULL;
    return 1 + random_state % channel_trigger_id_count;
  }

  void
  randomize_channel_trigger_ids(
    AdServer::CampaignSvcs::CampaignManagerLogger::RequestInfo& request_info,
    std::uint64_t& random_state,
    unsigned long channel_trigger_id_count)
  {
    auto randomize_triggers = [
      &random_state,
      channel_trigger_id_count
    ](AdServer::CampaignSvcs::CampaignManagerLogger::TriggerChannelMap& triggers)
    {
      for (auto& trigger : triggers)
      {
        trigger.channel_trigger_id = next_random_channel_trigger_id(
          random_state,
          channel_trigger_id_count);
      }
    };

    randomize_triggers(request_info.page_triggers);
    randomize_triggers(request_info.search_triggers);
    randomize_triggers(request_info.url_triggers);
    randomize_triggers(request_info.url_keyword_triggers);
  }

  std::shared_ptr<AdServer::CampaignSvcs::CampaignManagerLogger::RequestInfo>
  make_request_info()
  {
    using AdServer::CampaignSvcs::CampaignManagerLogger;

    auto request_info = std::make_shared<CampaignManagerLogger::RequestInfo>();

    request_info->time = Generics::Time(1760709051);
    request_info->isp_time = request_info->time;
    request_info->isp_time_offset = Generics::Time::ZERO;
    request_info->colo_id = 693;
    request_info->user_status = AdServer::CampaignSvcs::US_OPTIN;
    request_info->log_as_test = false;
    request_info->search_engine_id = 0;
    request_info->search_words = string_holder("");
    request_info->client_app = "msc";
    request_info->client_app_version = "";
    request_info->full_platform = "linux";
    request_info->web_browser = "firefox";
    request_info->user_agent = string_holder(
      "Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:73.0) "
      "Gecko/20100101 Firefox/73.0");
    request_info->search_engine_host = "";
    request_info->country_code = "ru";
    request_info->page_keywords_present = true;
    request_info->platforms.insert(1156234);
    request_info->platforms.insert(1891019);
    request_info->platform_channels.insert(1156234);
    request_info->platform_channels.insert(1891019);

    request_info->is_ad_request = true;
    request_info->request_id = AdServer::Commons::RequestId("pqE7jNSUQoaV_UsdtySkIQ..");
    request_info->user_id = AdServer::Commons::UserId("19GyC8_FSLafQfNj06PZzA..");
    request_info->merged_user_id = request_info->user_id;
    request_info->fraud = false;
    request_info->disable_fraud_detection = false;
    request_info->track_passback = false;

    request_info->geo_channels.push_back(617257);
    request_info->geo_channels.push_back(428122);
    request_info->geo_channels.push_back(3612040);

    add_trigger(
      *request_info,
      request_info->page_triggers,
      request_info->triggered_channels.page_channels,
      3752930,
      281349425);
    add_trigger(
      *request_info,
      request_info->page_triggers,
      request_info->triggered_channels.page_channels,
      3752931,
      281349424);
    add_trigger(
      *request_info,
      request_info->page_triggers,
      request_info->triggered_channels.page_channels,
      3772324,
      281666558);
    add_trigger(
      *request_info,
      request_info->page_triggers,
      request_info->triggered_channels.page_channels,
      3816395,
      284370848);

    add_trigger(
      *request_info,
      request_info->url_triggers,
      request_info->triggered_channels.url_channels,
      1895387,
      89518002);
    add_trigger(
      *request_info,
      request_info->url_triggers,
      request_info->triggered_channels.url_channels,
      3675065,
      216046518);
    add_trigger(
      *request_info,
      request_info->url_triggers,
      request_info->triggered_channels.url_channels,
      4634631,
      287724140);

    request_info->history_channels = {
      1895387,
      3675065,
      3752930,
      3752931,
      3772324,
      3772325,
      3772326,
      3816395,
      3816396,
      4634631
    };
    request_info->last_platform_channel_id = 1891019;
    request_info->referer = "http://rutube.ru/";
    request_info->urls.push_back("http://rutube.ru/");
    request_info->profile_referer = true;
    request_info->ip_hash = "213.33.171.240";
    request_info->page_keywords = string_holder(
      "rtbreq rtbmscreq rtbip213x33x171 rtbip213x33x171x240 "
      "rtbmscip213x33x171 rtbmscip213x33x171x240 "
      "rtbplatformfirefoxx8x rtbplatformlinux");
    request_info->url_keywords = string_holder("http rutube ru");
    request_info->additional_info = "";
    request_info->random = 6476436;

    return request_info;
  }
}

int
main(int argc, char** argv)
{
  try
  {
    const auto options = parse_options(argc, argv);
    auto params = make_logger_params(options);

    Logging::Logger_var logger(new Logging::Null::Logger);
    AdServer::CampaignSvcs::CampaignManagerLogger_var campaign_logger(
      new AdServer::CampaignSvcs::CampaignManagerLogger(params, logger));

    campaign_logger->activate_object();

    static constexpr unsigned long REQUEST_INFO_POOL_SIZE = 8192;
    std::vector<
      std::shared_ptr<AdServer::CampaignSvcs::CampaignManagerLogger::RequestInfo>> request_infos;
    request_infos.reserve(
      options.count < REQUEST_INFO_POOL_SIZE ? options.count : REQUEST_INFO_POOL_SIZE);
    while (request_infos.size() < request_infos.capacity())
    {
      request_infos.emplace_back(make_request_info());
    }

    std::uint64_t channel_trigger_random_state = 1;
    const auto start_cpu = current_cpu_times();
    for (unsigned long i = 0; i < options.count; ++i)
    {
      const auto request_info_i = i % request_infos.size();
      if (request_info_i == 0 && i != 0)
      {
        campaign_logger->wait_processing();
      }

      auto& request_info = request_infos[request_info_i];
      randomize_channel_trigger_ids(
        *request_info,
        channel_trigger_random_state,
        options.channel_trigger_id_count);
      campaign_logger->process_request(request_info, options.profiling_type);
    }
    campaign_logger->wait_processing();
    const auto process_finish_cpu = current_cpu_times();

    campaign_logger->flush_if_required(Generics::Time::get_time_of_day());
    const auto flush_finish_cpu = current_cpu_times();

    campaign_logger->deactivate_object();
    campaign_logger->wait_object();

    const double process_user_cpu = process_finish_cpu.user - start_cpu.user;
    const double process_sys_cpu = process_finish_cpu.sys - start_cpu.sys;
    const double flush_user_cpu = flush_finish_cpu.user - process_finish_cpu.user;
    const double flush_sys_cpu = flush_finish_cpu.sys - process_finish_cpu.sys;
    std::cout
      << "completed: " << options.count
      << ", threads: " << options.threads
      << ", profiling_type: " << options.profiling_type
      << ", channel_trigger_id_count: " << options.channel_trigger_id_count
      << ", process_cpu_time: " <<
        format_float(process_user_cpu + process_sys_cpu) << "s"
      << ", process_user_cpu_time: " <<
        format_float(process_user_cpu) << "s"
      << ", process_sys_cpu_time: " <<
        format_float(process_sys_cpu) << "s"
      << ", flush_cpu_time: " <<
        format_float(flush_user_cpu + flush_sys_cpu) << "s"
      << ", flush_user_cpu_time: " <<
        format_float(flush_user_cpu) << "s"
      << ", flush_sys_cpu_time: " <<
        format_float(flush_sys_cpu) << "s" << std::endl;

    return 0;
  }
  catch(const std::exception& ex)
  {
    std::cerr << "Caught std::exception: " << ex.what() << std::endl;
  }

  return 1;
}
