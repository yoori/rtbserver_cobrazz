#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
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

  struct Options
  {
    std::string model;
    unsigned long count = 0;
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
    Campaign_var campaign;
    Creative_var creative;
    Generics::MonoAllocatorArena arena;
    CampaignSelectParams_var request_params;
  };

  void
  print_usage()
  {
    std::cerr
      << "Usage: CTRModelPredictPerfTest --model <path> --count <N>\n"
      << "Options:\n"
      << "  --model <path>  CampaignManager CTR model directory\n"
      << "  --count <N>     sequential CTR calculations count\n";
  }

  Options
  parse_options(int argc, char** argv)
  {
    using namespace Generics::AppUtils;

    StringOption opt_model;
    Option<unsigned long> opt_count(0);
    CheckOption opt_help;

    Args args(-1);
    args.add(equal_name("model"), opt_model);
    args.add(equal_name("count"), opt_count);
    args.add(equal_name("help") || short_name("h"), opt_help);
    args.parse(argc - 1, argv + 1);

    if(opt_help.enabled())
    {
      print_usage();
      std::exit(0);
    }

    Options options;
    options.model = *opt_model;
    options.count = *opt_count;

    if(options.model.empty())
    {
      throw std::runtime_error("--model must be specified");
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
  init_test_data(TestData& data)
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

    data.campaign = new Campaign();
    data.campaign->campaign_id = 21251;
    data.campaign->campaign_group_id = 236995;
    data.campaign->account = data.account;
    data.campaign->advertiser = data.advertiser;
    data.campaign->ctr = RevenueDecimal(0.001);

    data.creative = new Creative(
      data.campaign,
      2527297,
      857450,
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

int
main(int argc, char** argv)
{
  try
  {
    const Options options = parse_options(argc, argv);
    TestData data;
    init_test_data(data);

    CTRProvider_var provider(
      new CTRProviderImpl(options.model, Generics::Time::ZERO, nullptr));
    CTRProvider::Calculation_var calculation =
      provider->create_calculation(data.request_params);
    CTRProvider::CalculationContext_var context =
      calculation->create_context(data.tag_size);

    const std::string algorithm_id = calculation->algorithm_id(data.creative);
    if(algorithm_id.empty())
    {
      throw std::runtime_error("the model selected the default CTR algorithm");
    }

    const CpuTimes cpu_started = current_cpu_times();
    const auto started = std::chrono::steady_clock::now();

    RevenueDecimal ctr;
    for(unsigned long i = 0; i < options.count; ++i)
    {
      ctr = context->get_ctr(data.creative);
    }

    const auto finished = std::chrono::steady_clock::now();
    const CpuTimes cpu_finished = current_cpu_times();

    const double elapsed = std::chrono::duration<double>(finished - started).count();
    const double user_cpu = cpu_finished.user - cpu_started.user;
    const double sys_cpu = cpu_finished.sys - cpu_started.sys;

    std::cout
      << "model=" << options.model << '\n'
      << "algorithm_id=" << algorithm_id << '\n'
      << "count=" << options.count << '\n'
      << "ctr=" << ctr << '\n'
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
