#include <array>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>

#include <sys/resource.h>

#include <Generics/AppUtils.hpp>
#include <Generics/MonoAllocator.hpp>

#include <CampaignSvcs/CampaignManager/CTRProviderImpl.hpp>

namespace
{
  using namespace AdServer::CampaignSvcs;
  using namespace AdServer::CampaignSvcs::CTR;

  enum class Mode
  {
    FULL,
    SWITCH_CAMPAIGNS
  };

  struct Options
  {
    std::string model;
    Mode mode = Mode::FULL;
    unsigned long count = 0;
    unsigned long campaign_id = 236995;
    std::optional<RevenueDecimal> expected_ctr;
  };

  struct CpuTimes
  {
    double user = 0.0;
    double sys = 0.0;
  };

  struct TestData
  {
    Account_var publisher;
    Site_var site;
    Tag_var tag;
    Size_var size;
    Tag::Size_var tag_size;
    Account_var isp;
    Colocation_var colocation;
    Account_var account;
    Account_var advertiser;
    std::array<Campaign_var, 2> campaigns;
    std::array<Creative_var, 2> creatives;
    Generics::MonoAllocatorArena arena;
    CampaignSelectParams_var request_params;
  };

  void
  print_usage()
  {
    std::cerr
      << "Usage: CTRModelPredictPerfTest --model <path> --mode <mode> --count <N>\n"
      << "Options:\n"
      << "  --model <path>  CampaignManager CTR model directory\n"
      << "  --mode <mode>   full or switch-campaigns\n"
      << "  --count <N>     sequential CTR calculations count\n"
      << "  --campaign-id <value>  DB campaign_id used by the candidate\n"
      << "  --expected-ctr <value>  fail if the resulting CTR differs\n";
  }

  Options
  parse_options(int argc, char** argv)
  {
    using namespace Generics::AppUtils;

    StringOption opt_model;
    StringOption opt_mode;
    StringOption opt_expected_ctr;
    Option<unsigned long> opt_count(0);
    Option<unsigned long> opt_campaign_id(236995);
    CheckOption opt_help;

    Args args(-1);
    args.add(equal_name("model"), opt_model);
    args.add(equal_name("mode"), opt_mode);
    args.add(equal_name("count"), opt_count);
    args.add(equal_name("campaign-id"), opt_campaign_id);
    args.add(equal_name("expected-ctr"), opt_expected_ctr);
    args.add(equal_name("help") || short_name("h"), opt_help);
    args.parse(argc - 1, argv + 1);

    if(opt_help.enabled())
    {
      print_usage();
      std::exit(0);
    }

    Options options;
    options.model = *opt_model;
    if(*opt_mode == "full")
    {
      options.mode = Mode::FULL;
    }
    else if(*opt_mode == "switch-campaigns")
    {
      options.mode = Mode::SWITCH_CAMPAIGNS;
    }
    else
    {
      throw std::runtime_error(
        "--mode must be full or switch-campaigns");
    }
    options.count = *opt_count;
    options.campaign_id = *opt_campaign_id;
    if(!opt_expected_ctr->empty())
    {
      options.expected_ctr.emplace(String::SubString(*opt_expected_ctr));
    }

    if(options.model.empty())
    {
      throw std::runtime_error("--model must be specified");
    }
    if(options.count == 0)
    {
      throw std::runtime_error("--count must be > 0");
    }
    if(options.mode == Mode::SWITCH_CAMPAIGNS &&
      options.campaign_id == std::numeric_limits<unsigned long>::max())
    {
      throw std::runtime_error(
        "--campaign-id is too large for switch-campaigns mode");
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
  init_test_data(TestData& data, unsigned long campaign_id)
  {
    data.publisher = new AccountDef();
    data.publisher->account_id = 9803;

    data.site = new Site();
    data.site->site_id = 7207;
    data.site->account = data.publisher;

    data.tag = new Tag();
    data.tag->tag_id = 15037;
    data.tag->site = data.site;
    data.tag->adjustment = RevenueDecimal(false, 1, 0);

    data.size = new Size();
    data.size->size_id = 722;
    data.size->protocol_name = "970x250";

    data.tag_size = new Tag::Size();
    data.tag_size->size = data.size;
    data.tag_size->max_text_creatives = 1;
    data.tag->sizes.emplace(data.size->size_id, data.tag_size);

    data.isp = new AccountDef();
    data.isp->account_id = 9387;

    data.colocation = new Colocation();
    data.colocation->colo_id = 693;
    data.colocation->account = data.isp;

    data.request_params = new CampaignSelectParams(
      true,
      FreqCapIdSet(&data.arena),
      SeqOrderMap(&data.arena),
      data.colocation,
      data.tag,
      data.tag->sizes,
      false,
      -1,
      -1,
      &data.arena);

    data.request_params->country_code = "ru";
    data.request_params->referer_hostname = "uralsib.ru";
    data.request_params->time = Generics::Time(1787430315);
    data.request_params->time_hour = 20;
    data.request_params->time_week_day = 6;
    data.request_params->last_platform_channel_id = 1891019;
    data.request_params->geo_channels.insert(1156234);
    for(const unsigned long channel_id : {
      3781076UL, 3752930UL, 3772324UL, 3772325UL, 3816396UL,
      2633255UL, 3731826UL, 3732085UL, 3819790UL, 3752931UL,
      3752932UL, 3772326UL, 3816395UL, 1895326UL, 3609715UL,
      3731990UL, 3732226UL})
    {
      data.request_params->channels.insert(channel_id);
    }

    data.account = new AccountDef();
    data.account->account_id = 9387;

    data.advertiser = new AccountDef();
    data.advertiser->account_id = 12210;

    for(std::size_t index = 0; index < data.campaigns.size(); ++index)
    {
      data.campaigns[index] = new Campaign();
      data.campaigns[index]->campaign_id = 21251 + index;
      // CampaignManager campaign_group_id is DB campaign_id.
      data.campaigns[index]->campaign_group_id = campaign_id + index;
      data.campaigns[index]->account = data.account;
      data.campaigns[index]->advertiser = data.advertiser;
      data.campaigns[index]->ctr = RevenueDecimal(0.001);

      data.creatives[index] = new Creative(
        data.campaigns[index],
        2527297 + index,
        857450 + index,
        0,
        0,
        "",
        "",
        OptionValue(),
        "",
        "",
        Creative::CategorySet());
    }

  }
}

int
main(int argc, char** argv)
{
  try
  {
    const Options options = parse_options(argc, argv);
    TestData data;
    init_test_data(data, options.campaign_id);

    CTRProvider_var provider(
      new CTRProviderImpl(options.model, Generics::Time::ZERO, nullptr));
    CTRProvider::Calculation_var calculation =
      provider->create_calculation(data.request_params);
    CTRProvider::CalculationContext_var context =
      calculation->create_context(data.tag_size);

    const std::string algorithm_id = calculation->algorithm_id(data.creatives[0]);
    if(algorithm_id.empty())
    {
      throw std::runtime_error("the model selected the default CTR algorithm");
    }
    if(options.mode == Mode::SWITCH_CAMPAIGNS &&
      calculation->algorithm_id(data.creatives[1]).empty())
    {
      throw std::runtime_error(
        "the model selected the default CTR algorithm for the second campaign");
    }

    const CpuTimes cpu_started = current_cpu_times();
    const auto started = std::chrono::steady_clock::now();

    RevenueDecimal ctr;
    std::array<std::optional<RevenueDecimal>, 2> campaign_ctrs;
    if(options.mode == Mode::FULL)
    {
      for(unsigned long i = 0; i < options.count; ++i)
      {
        ctr = context->get_ctr(data.creatives[0]);
      }
      campaign_ctrs[0] = ctr;
    }
    else
    {
      for(unsigned long i = 0; i < options.count; ++i)
      {
        const std::size_t campaign_index = i % data.creatives.size();
        ctr = context->get_ctr(data.creatives[campaign_index]);
        campaign_ctrs[campaign_index] = ctr;
      }
    }

    if(options.expected_ctr && ctr != *options.expected_ctr)
    {
      std::ostringstream error;
      error
        << "CTR mismatch: expected=" << *options.expected_ctr
        << ", actual=" << ctr;
      throw std::runtime_error(error.str());
    }

    const auto finished = std::chrono::steady_clock::now();
    const CpuTimes cpu_finished = current_cpu_times();

    const double elapsed = std::chrono::duration<double>(finished - started).count();
    const double user_cpu = cpu_finished.user - cpu_started.user;
    const double sys_cpu = cpu_finished.sys - cpu_started.sys;

    std::cout
      << "model=" << options.model << '\n'
      << "mode=" << (
        options.mode == Mode::FULL ? "full" : "switch-campaigns") << '\n'
      << "algorithm_id=" << algorithm_id << '\n'
      << "count=" << options.count << '\n'
      << "ctr=" << ctr << '\n';
    for(std::size_t index = 0; index < campaign_ctrs.size(); ++index)
    {
      if(campaign_ctrs[index])
      {
        std::cout
          << "campaign_" << index << "_ctr=" << *campaign_ctrs[index] << '\n';
      }
    }
    std::cout
      << "elapsed_sec=" << format_float(elapsed) << '\n'
      << "rps=" << format_float(options.count / elapsed) << '\n'
      << "cpu_sec=" << format_float(user_cpu + sys_cpu) << '\n'
      << "user_cpu_sec=" << format_float(user_cpu) << '\n'
      << "sys_cpu_sec=" << format_float(sys_cpu) << std::endl;
  }
  catch(const std::exception& ex)
  {
    std::cerr << "CTRModelPredictPerfTest failed: " << ex.what() << std::endl;
    return 1;
  }

  return 0;
}
