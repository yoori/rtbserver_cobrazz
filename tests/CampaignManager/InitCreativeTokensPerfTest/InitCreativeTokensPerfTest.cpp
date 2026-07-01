#include <atomic>
#include <chrono>
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

#include <CampaignSvcs/CampaignManager/CreativeTextGenerator.hpp>
#include <CampaignSvcs/CampaignManager/CreativeTemplateArgs.hpp>

namespace
{
  struct Options
  {
    unsigned long count = 0;
    unsigned long threads = 1;
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
      << "Usage: InitCreativeTokensPerfTest --count <N> [OPTIONS]\n"
      << "Options:\n"
      << "  --count <N>    init_creative_tokens calls count\n"
      << "  --threads <N>  worker threads (default: 1)\n";
  }

  Options
  parse_options(int argc, char** argv)
  {
    using namespace Generics::AppUtils;

    Option<unsigned long> opt_count(0);
    Option<unsigned long> opt_threads(1);
    CheckOption opt_help;

    Args args(-1);
    args.add(equal_name("count"), opt_count);
    args.add(equal_name("threads"), opt_threads);
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

    if(options.count == 0)
    {
      throw std::runtime_error("--count must be > 0");
    }

    if(options.threads == 0)
    {
      throw std::runtime_error("--threads must be > 0");
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
  fill_token(
    AdServer::CampaignSvcs::TokenValueMap& values,
    const std::string& key,
    const std::string& value)
  {
    values.set_value(key, value);
  }

  AdServer::CampaignSvcs::CreativeInstantiateRule
  make_rule()
  {
    using namespace AdServer::CampaignSvcs;

    CreativeInstantiateRule rule;
    rule.use_empty_values_ = false;
    rule.url_prefix = "https://ad.example/";
    rule.image_url = "https://ad.example/img/";
    rule.publ_url = "https://pub.example/";
    rule.click_url = "https://click.example/";
    rule.ad_server = "https://ad.example/";
    rule.ad_image_server = "https://img.example/";
    rule.track_pixel_url = "https://track.example/";
    rule.notice_url = "https://notice.example/";
    rule.action_pixel_url = "https://action.example/";
    rule.local_passback_prefix = "https://passback.example/";
    rule.dynamic_creative_prefix = "https://dynamic.example/";
    rule.passback_template_path_prefix = "/tmp/";
    rule.passback_pixel_url = "https://passback-pixel.example/";
    rule.user_bind_url = "https://bind.example/";
    rule.pub_pixels_optin = "https://pub.example/optin";
    rule.pub_pixels_optout = "https://pub.example/optout";
    rule.script_instantiate_url = "https://script.example/";
    rule.iframe_instantiate_url = "https://iframe.example/";
    rule.direct_instantiate_url = "https://direct.example/";
    rule.nonsecure_direct_instantiate_url = "http://direct.example/";
    rule.video_instantiate_url = "https://video.example/";
    rule.nonsecure_video_instantiate_url = "http://video.example/";
    return rule;
  }

  AdServer::CampaignSvcs::TokenValueMap
  make_request_args()
  {
    using namespace AdServer::CampaignSvcs;

    TokenValueMap request_args;
    fill_token(request_args, CreativeTokens::REQUEST_ID, "pqE7jNSUQoaV_UsdtySkIQ..");
    fill_token(request_args, CreativeTokens::GREQUESTID, "b7495c64-edfc-48b7-5c3b-99fd7e401730");
    fill_token(request_args, CreativeTokens::RANDOM, "6476436");
    fill_token(request_args, CreativeTokens::SOURCE_ID, "msc");
    fill_token(request_args, CreativeTokens::EXTERNAL_USER_ID, "msc/TEST30");
    fill_token(request_args, CreativeTokens::REFERER, "http://rutube.ru/");
    fill_token(request_args, CreativeTokens::REFERER_DOMAIN, "rutube.ru");
    fill_token(request_args, CreativeTokens::UA,
      "Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:73.0) Gecko/20100101 Firefox/73.0");
    fill_token(request_args, CreativeTokens::IP, "213.33.171.240");
    fill_token(request_args, CreativeTokens::TAGID, "11");
    fill_token(request_args, CreativeTokens::SITE_ID, "11");
    fill_token(request_args, CreativeTokens::PUBLISHER_ID, "1");
    fill_token(request_args, CreativeTokens::USER_STATUS, "optin");
    fill_token(request_args, CreativeTokens::CREATIVE_SIZE, "300x250");
    fill_token(request_args, CreativeTokens::TAGSIZE, "300x250");
    fill_token(request_args, CreativeTokens::COLOCATION, "693");
    fill_token(request_args, CreativeTokens::TRACKPIXEL,
      "https://track.example/pixel?req=pqE7jNSUQoaV_UsdtySkIQ..&src=msc");
    fill_token(request_args, CreativeTokens::TAG_TRACK_PIXEL,
      "https://pub.example/tag?req=pqE7jNSUQoaV_UsdtySkIQ..");
    return request_args;
  }

  AdServer::CampaignSvcs::OptionTokenValueMap
  make_creative_args()
  {
    using namespace AdServer::CampaignSvcs;

    OptionTokenValueMap creative_args;
    creative_args[CreativeTokens::REQUEST_ID] =
      AdServer::CampaignSvcs::OptionValue(0, "pqE7jNSUQoaV_UsdtySkIQ..");
    creative_args[CreativeTokens::CCID] =
      AdServer::CampaignSvcs::OptionValue(0, "2558626");
    creative_args[CreativeTokens::ADVERTISER_ID] =
      AdServer::CampaignSvcs::OptionValue(0, "12028");
    creative_args[CreativeTokens::CGID] =
      AdServer::CampaignSvcs::OptionValue(0, "21875");
    creative_args[CreativeTokens::CID] =
      AdServer::CampaignSvcs::OptionValue(0, "9803");
    creative_args[CreativeTokens::CREATIVE_SIZE] =
      AdServer::CampaignSvcs::OptionValue(0, "300x250");
    creative_args[CreativeTokens::TEMPLATE_FORMAT] =
      AdServer::CampaignSvcs::OptionValue(0, "text/html");
    creative_args[CreativeTokens::ACTIONPIXEL] =
      AdServer::CampaignSvcs::OptionValue(0, "https://action.example/pixel");
    creative_args[CreativeTokens::DISPLAY_URL] =
      AdServer::CampaignSvcs::OptionValue(0, "http://rutube.ru/");
    creative_args[CreativeTokens::DESTURL] =
      AdServer::CampaignSvcs::OptionValue(0, "https://click.example/pqE7jNSUQoaV_UsdtySkIQ..");
    creative_args[CreativeTokens::ADV_CLICK_URL] =
      AdServer::CampaignSvcs::OptionValue(0, "https://click.example/pqE7jNSUQoaV_UsdtySkIQ..");
    return creative_args;
  }
}

int
main(int argc, char** argv)
{
  try
  {
    const auto options = parse_options(argc, argv);

    const auto rule = make_rule();
    const auto request_args = make_request_args();
    const auto creative_args = make_creative_args();
    const AdServer::CampaignSvcs::CreativeInstantiateArgs creative_instantiate_args;
    const AdServer::CampaignSvcs::TokenProcessorMap token_processors;

    std::atomic<unsigned long> checksum{0};

    const auto start_cpu = current_cpu_times();

    std::vector<std::thread> workers;
    workers.reserve(options.threads);

    const unsigned long per_thread = options.count / options.threads;
    const unsigned long remainder = options.count % options.threads;

    for(unsigned long thread_index = 0; thread_index < options.threads; ++thread_index)
    {
      const unsigned long items =
        per_thread + (thread_index < remainder ? 1 : 0);

      workers.emplace_back(
        [
          &rule,
          &creative_instantiate_args,
          &token_processors,
          &request_args,
          &creative_args,
          items,
          &checksum
        ]()
        {
          AdServer::CampaignSvcs::TokenValueMap result;

          for(unsigned long i = 0; i < items; ++i)
          {
            result.clear();
            AdServer::CampaignSvcs::CreativeTextGenerator::init_creative_tokens(
              rule,
              creative_instantiate_args,
              token_processors,
              request_args,
              creative_args,
              result);

            checksum.fetch_add(result.size(), std::memory_order_relaxed);
          }
        });
    }

    for(auto& worker : workers)
    {
      worker.join();
    }

    const auto finish_cpu = current_cpu_times();

    const double user_cpu = finish_cpu.user - start_cpu.user;
    const double sys_cpu = finish_cpu.sys - start_cpu.sys;

    std::cout
      << "completed: " << options.count
      << ", threads: " << options.threads
      << ", checksum: " << checksum.load()
      << ", cpu_time: " << format_float(user_cpu + sys_cpu) << "s"
      << ", user_cpu_time: " << format_float(user_cpu) << "s"
      << ", sys_cpu_time: " << format_float(sys_cpu) << "s"
      << std::endl;

    return 0;
  }
  catch(const std::exception& ex)
  {
    std::cerr << "Caught std::exception: " << ex.what() << std::endl;
  }

  return 1;
}
