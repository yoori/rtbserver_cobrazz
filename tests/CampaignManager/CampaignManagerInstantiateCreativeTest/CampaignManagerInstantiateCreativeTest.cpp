#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <sys/resource.h>

#include <Generics/AppUtils.hpp>
#include <Logger/ActiveObjectCallback.hpp>
#include <Logger/Logger.hpp>

#include <CampaignSvcs/CampaignManager/CreativeTextGenerator.hpp>
#include <CampaignSvcs/CampaignManager/CampaignManagerCore.hpp>
#include <CampaignSvcs/CampaignManager/CreativeInstantiator.hpp>

namespace
{
  using namespace AdServer::CampaignSvcs;
  using namespace AdServer::CampaignSvcs::AdInstances;

  struct Options
  {
    unsigned long count = 0;
    unsigned long threads = 1;
    std::string rid_private_key =
      "/home/jurij_kuznecov/projects/run/etc-ssv400/adserver/"
      "adcluster/build00/rid_private_key.der";
    std::string template_root = "/tmp/CampaignManagerInstantiateCreativeTest";
  };

  struct CpuTimes
  {
    double user = 0.0;
    double sys = 0.0;
  };

  struct Fixture
  {
    ReferenceCounting::SmartPtr<CampaignConfig> campaign_config;
    Colocation_var colocation;
    Campaign_var campaign;
    Creative_var creative;
    Tag_var tag;
    Tag::Size_var tag_size;
    CampaignManagerCore::CommonAdRequest request_info;
    CampaignManagerCore::InstantiateParams instantiate_params;
    CampaignManagerCore::AdSlotContext ad_slot_context;
    CampaignManagerCore::IdArray exclude_pubpixel_accounts;

    Fixture()
      : instantiate_params(AdServer::Commons::Optional<unsigned long>(12345))
    {}
  };

  void
  print_usage()
  {
    std::cerr
      << "Usage: CampaignManagerInstantiateCreativeTest --count <N> [OPTIONS]\n"
      << "Options:\n"
      << "  --count <N>                    instantiate_creative calls count\n"
      << "  --threads <N>                  worker threads (default: 1)\n"
      << "  --rid-private-key <p>          RID private key path\n"
      << "  --template-root <p>            temp template directory\n";
  }

  Options
  parse_options(int argc, char** argv)
  {
    using namespace Generics::AppUtils;

    Option<unsigned long> opt_count(0);
    Option<unsigned long> opt_threads(1);
    StringOption opt_rid_private_key(
      "/home/jurij_kuznecov/projects/run/etc-ssv400/adserver/"
      "adcluster/build00/rid_private_key.der");
    StringOption opt_template_root("/tmp/CampaignManagerInstantiateCreativeTest");
    CheckOption opt_help;

    Args args(-1);
    args.add(equal_name("count"), opt_count);
    args.add(equal_name("threads"), opt_threads);
    args.add(equal_name("rid-private-key"), opt_rid_private_key);
    args.add(equal_name("template-root"), opt_template_root);
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
    options.rid_private_key = *opt_rid_private_key;
    options.template_root = *opt_template_root;

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

  CreativeInstantiator::CreativeInstantiate
  make_creative_instantiate()
  {
    CreativeInstantiator::CreativeInstantiate creative_instantiate;

    CreativeInstantiateRule rule;
    rule.url_prefix = "https://ad.example/";
    rule.image_url = "https://img.example/";
    rule.publ_url = "https://pub.example/";
    rule.click_url = "https://click.example/click";
    rule.ad_server = "https://ad.example";
    rule.ad_image_server = "https://img.example";
    rule.track_pixel_url = "https://track.example/pixel";
    rule.notice_url = "https://notice.example";
    rule.action_pixel_url = "https://action.example/action";
    rule.local_passback_prefix = "https://passback.example/";
    rule.dynamic_creative_prefix = "https://dynamic.example/";
    rule.passback_template_path_prefix = "/tmp/";
    rule.passback_pixel_url = "https://passback.example/pixel";
    rule.user_bind_url = "https://bind.example";
    rule.pub_pixels_optin = "https://pub.example/optin";
    rule.pub_pixels_optout = "https://pub.example/optout";
    rule.script_instantiate_url = "https://script.example";
    rule.iframe_instantiate_url = "https://iframe.example";
    rule.direct_instantiate_url = "https://direct.example";
    rule.nonsecure_direct_instantiate_url = "http://direct.example";
    rule.video_instantiate_url = "https://video.example";
    rule.nonsecure_video_instantiate_url = "http://video.example";

    creative_instantiate.creative_rules["secure"] = rule;

    return creative_instantiate;
  }

  void
  write_template_file(const std::filesystem::path& path)
  {
    std::filesystem::create_directories(path.parent_path());

    std::ofstream out(path);
    if(!out)
    {
      throw std::runtime_error("can't create template file: " + path.string());
    }

    out
      << "request=##REQUESTID##\n"
      //<< "global=##GREQUESTID##\n"
      << "ccid=##CCID##\n"
      << "cid=##CID##\n"
      << "tag=##TAGID##\n"
      << "click=##CLICK##\n";
  }

  RevenueDecimal
  revenue(unsigned long value)
  {
    return RevenueDecimal(false, value, 0);
  }

  Account_var
  make_account(unsigned long account_id, const Currency_var& currency)
  {
    Account_var account(new AccountDef());
    account->account_id = account_id;
    account->internal_account_id = account_id;
    account->flags = 0;
    account->at_flags = AccountTypeFlags::USE_SELF_BUDGET;
    account->text_adserving = 'A';
    account->currency = currency;
    account->country = "RU";
    account->commision = RevenueDecimal::ZERO;
    account->media_handling_fee = RevenueDecimal::ZERO;
    account->budget = revenue(1000000);
    account->paid_amount = RevenueDecimal::ZERO;
    account->status = 'A';
    account->eval_status = 'A';
    account->auction_rate = AR_GROSS;
    account->use_pub_pixels = false;
    account->self_service_commission = RevenueDecimal::ZERO;
    account->set_available(true);
    return account;
  }

  Fixture
  make_fixture(const std::filesystem::path& template_path)
  {
    Fixture fixture;

    fixture.campaign_config = new CampaignConfig();
    fixture.campaign_config->default_click_token_processor =
      BaseTokenProcessor::default_token_processor(
        CreativeTokens::ADV_CLICK_URL.c_str());

    Currency_var currency(new Currency());
    currency->currency_id = 1;
    currency->currency_exchange_id = 1;
    currency->effective_date = 0;
    currency->fraction = 1;
    currency->currency_code = "RUB";
    currency->rate = RevenueDecimal::div(revenue(1), revenue(1));
    fixture.campaign_config->currencies[currency->currency_id] = currency;
    fixture.campaign_config->currency_codes.emplace(
      Generics::StringHashAdapter(currency->currency_code),
      currency);

    Account_var publisher = make_account(9803, currency);
    Account_var account = make_account(11081, currency);
    Account_var advertiser = make_account(12028, currency);
    fixture.campaign_config->accounts[publisher->account_id] = publisher;
    fixture.campaign_config->accounts[account->account_id] = account;
    fixture.campaign_config->accounts[advertiser->account_id] = advertiser;

    Site_var site(new Site());
    site->site_id = 11;
    site->freq_cap_id = 0;
    site->noads_timeout = 0;
    site->flags = 0;
    site->account = publisher;
    site->status = 'A';
    fixture.campaign_config->sites[site->site_id] = site;

    Size_var size(new Size());
    size->size_id = 9;
    size->protocol_name = "300x250";
    size->size_type_id = 1;
    size->width = 300;
    size->height = 250;
    fixture.campaign_config->sizes[size->size_id] = size;

    fixture.tag = new Tag();
    fixture.tag->tag_id = 15037;
    fixture.tag->site = site;
    fixture.tag->flags = 0;
    fixture.tag->marketplace = 'O';
    fixture.tag->adjustment = RevenueDecimal::ZERO;
    fixture.tag->allow_expandable = false;
    fixture.tag->min_visibility = 0;
    fixture.tag->auction_max_ecpm_share = RevenueDecimal::ZERO;
    fixture.tag->auction_prop_probability_share = RevenueDecimal::ZERO;
    fixture.tag->auction_random_share = RevenueDecimal::ZERO;
    fixture.tag->pub_max_random_cpm = RevenueDecimal::ZERO;
    fixture.tag->max_random_cpm = RevenueDecimal::ZERO;
    fixture.tag->cost_coef = revenue(1);
    fixture.tag->skip_min_ecpm = true;

    fixture.tag_size = new Tag::Size();
    fixture.tag_size->size = size;
    fixture.tag_size->max_text_creatives = 0;
    fixture.tag->sizes[size->size_id] = fixture.tag_size;
    fixture.campaign_config->tags[fixture.tag->tag_id] = fixture.tag;

    fixture.colocation = new Colocation();
    fixture.colocation->colo_id = 693;
    fixture.colocation->colo_rate_id = 1;
    fixture.colocation->at_flags = 0;
    fixture.colocation->account = account;
    fixture.colocation->revenue_share = RevenueDecimal::ZERO;
    fixture.colocation->ad_serving = CS_ALL;
    fixture.colocation->hid_profile = false;
    fixture.campaign_config->colocations[fixture.colocation->colo_id] =
      fixture.colocation;

    fixture.campaign = new Campaign();
    fixture.campaign->campaign_id = 236995;
    fixture.campaign->campaign_group_id = 236995;
    fixture.campaign->account = account;
    fixture.campaign->advertiser = advertiser;
    fixture.campaign->fc_id = 0;
    fixture.campaign->group_fc_id = 0;
    fixture.campaign->ccg_rate_id = 1;
    fixture.campaign->ccg_rate_type = CR_CPM;
    fixture.campaign->flags = 0;
    fixture.campaign->marketplace = 'O';
    fixture.campaign->status = 'A';
    fixture.campaign->eval_status = 'A';
    fixture.campaign->country = "RU";
    fixture.campaign->imp_revenue = revenue(1);
    fixture.campaign->click_revenue = RevenueDecimal::ZERO;
    fixture.campaign->click_sys_revenue = RevenueDecimal::ZERO;
    fixture.campaign->action_revenue = RevenueDecimal::ZERO;
    fixture.campaign->commision = RevenueDecimal::ZERO;
    fixture.campaign->ccg_type = CT_DISPLAY;
    fixture.campaign->targeting_type = 'C';
    fixture.campaign->start_user_group_id = 0;
    fixture.campaign->end_user_group_id = 99;
    fixture.campaign->ctr_reset_id = 0;
    fixture.campaign->mode = CM_NON_RANDOM;
    fixture.campaign->min_uid_age = Generics::Time::ZERO;
    fixture.campaign->seq_set_rotate_imps = 0;
    fixture.campaign->delivery_coef = TAG_DELIVERY_MAX;
    fixture.campaign->max_pub_share = RevenueDecimal::ZERO;
    fixture.campaign->bid_strategy = BS_MAX_REACH;

    Creative::CategorySet categories;
    fixture.creative = new Creative(
      fixture.campaign,
      2527264,
      857412,
      0,
      1,
      "html",
      "1",
      OptionValue(1, "https://advertiser.example/landing"),
      "advertiser.example",
      "advertiser.example",
      categories);
    fixture.creative->https_safe_flag = true;

    Creative::Size creative_size;
    creative_size.size = size;
    creative_size.up_expand_space = 0;
    creative_size.right_expand_space = 0;
    creative_size.down_expand_space = 0;
    creative_size.left_expand_space = 0;
    creative_size.expandable = false;
    creative_size.available_appformats.insert("html");
    fixture.creative->sizes[size->size_id] = creative_size;

    fixture.campaign->add_creative(fixture.creative);
    fixture.campaign_config->campaigns[fixture.campaign->campaign_id] =
      fixture.campaign;
    fixture.campaign_config->creatives[fixture.creative->creative_id] =
      fixture.creative;
    fixture.campaign_config->campaign_creatives.emplace(
      Generics::NumericHashAdapter<unsigned long>(fixture.creative->ccid),
      fixture.creative);

    ReferenceCounting::SmartPtr<RCOptionTokenValueMap> template_tokens(
      new RCOptionTokenValueMap());
    ReferenceCounting::SmartPtr<RCOptionTokenValueMap> template_hidden_tokens(
      new RCOptionTokenValueMap());

    CreativeTemplate creative_template(
      template_path.c_str(),
      CreativeTemplateFactory::Handler::CTT_TEXT,
      "text/html;charset=utf-8",
      false,
      template_tokens,
      template_hidden_tokens,
      Generics::Time::get_time_of_day());
    fixture.campaign_config->creative_templates.insert(
      CreativeTemplateKey("html", "300x250", "html"),
      creative_template);

    fixture.request_info.time = Generics::Time::get_time_of_day();
    fixture.request_info.request_id =
      AdServer::Commons::RequestId::create_random_based();
    fixture.request_info.creative_instantiate_type = "secure";
    fixture.request_info.request_type = AR_OPENRTB_WITH_CLICKURL;
    fixture.request_info.random = 3311422;
    fixture.request_info.colo_id = 693;
    fixture.request_info.external_user_id = "msc/test5";
    fixture.request_info.source_id = "msc";
    fixture.request_info.referer = "http://uralsib.ru";
    fixture.request_info.peer_ip = "213.33.171.240";
    fixture.request_info.user_agent =
      "Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:73.0) "
      "Gecko/20100101 Firefox/73.0";
    fixture.request_info.user_status = US_OPTIN;
    fixture.request_info.tokens.push_back(
      CampaignManagerCore::TokenInfo{CreativeTokens::TAGID, "bad-tag"});

    fixture.instantiate_params.generate_pubpixel_accounts = true;
    fixture.instantiate_params.publisher_account_id = 9803;
    fixture.instantiate_params.init_source_macroses = true;

    fixture.ad_slot_context.test_request = false;
    fixture.ad_slot_context.request_blacklisted = false;
    fixture.ad_slot_context.publisher_account_id = 9803;
    fixture.ad_slot_context.tag_size = "300x250";

    return fixture;
  }

  void
  instantiate_once(
    CreativeInstantiator& creative_instantiator,
    Fixture& fixture,
    std::atomic<unsigned long>& checksum)
  {
    AdSelectionResult ad_selection_result;
    ad_selection_result.tag = fixture.tag;
    ad_selection_result.tag_size = fixture.tag_size;
    ad_selection_result.auction_type = AT_MAX_ECPM;

    CampaignSelectionData selection;
    selection.request_id = fixture.request_info.request_id;
    selection.campaign = fixture.campaign;
    selection.creative = fixture.creative;
    selection.ecpm = revenue(1);
    selection.ecpm_bid = revenue(1);
    selection.actual_cpc = RevenueDecimal::ZERO;
    selection.ctr = RevenueDecimal::ZERO;
    selection.conv_rate = RevenueDecimal::ZERO;
    selection.campaign_imps = 0;
    selection.count_impression = true;
    selection.track_impr = true;
    selection.selection_done = true;
    ad_selection_result.selected_campaigns.push_back(selection);

    CampaignManagerCore::RequestResultParams request_result_params;
    CampaignManagerCore::CreativeParamsList creative_params_list;
    std::string creative_body;

    creative_instantiator.instantiate_creative(
      fixture.request_info,
      fixture.campaign_config,
      fixture.colocation,
      fixture.instantiate_params,
      "html",
      ad_selection_result,
      request_result_params,
      creative_params_list,
      creative_body,
      fixture.ad_slot_context,
      &fixture.exclude_pubpixel_accounts);

    const std::string expected_tag_token =
      std::string("tag=") + String::StringManip::IntToStr(
        fixture.tag->tag_id).str().str() + "\n";
    if(creative_body.find(expected_tag_token) == std::string::npos)
    {
      throw std::runtime_error(
        "system TAGID token was overridden by request token");
    }

    checksum.fetch_add(
      creative_body.size() + request_result_params.mime_format.size(),
      std::memory_order_relaxed);
  }

  CreativeInstantiator::Config
  make_creative_instantiator_config(const std::filesystem::path& template_path)
  {
    CreativeInstantiator::Config result;
    result.service_index = "1_1";
    result.post_instantiate_script_mime_format = "text/html";
    result.post_instantiate_iframe_mime_format = "text/html";
    result.post_instantiate_script_template_file = template_path.string();
    result.post_instantiate_iframe_template_file = template_path.string();
    result.instantiate_track_html_file = template_path.string();
    return result;
  }
}

namespace AdServer::CampaignSvcs::CTR
{
  CTRProvider::CTRProvider(
    const String::SubString&,
    const Generics::Time& config_timestamp,
    Generics::TaskRunner*)
    : config_timestamp_(config_timestamp),
      remove_config_files_at_destroy_(false)
  {}

  CTRProvider::~CTRProvider() noexcept = default;

  CTRProvider::Calculation_var
  CTRProvider::create_calculation(const CampaignSelectParams*) const noexcept
  {
    return nullptr;
  }

  Generics::Time
  CTRProvider::check_config_appearance(
    std::string&,
    const String::SubString&)
  {
    return Generics::Time::ZERO;
  }

  void
  CTRProvider::remove_config_files_at_destroy(bool val) const noexcept
  {
    remove_config_files_at_destroy_ = val;
  }

  CTRProvider::Calculation::~Calculation() noexcept = default;

  CTRProvider::CalculationContext_var
  CTRProvider::Calculation::create_context(const Tag::Size*) const noexcept
  {
    return nullptr;
  }

  std::string
  CTRProvider::Calculation::algorithm_id(const Creative*) const noexcept
  {
    return std::string();
  }

  CTRProvider::CalculationContext::~CalculationContext() noexcept = default;

  RevenueDecimal
  CTRProvider::CalculationContext::get_ctr(const Creative*) const
  {
    return RevenueDecimal::ZERO;
  }

  bool
  CTRProvider::CalculationContext::check_rate(
    const Creative*,
    RevenueDecimal* rate,
    bool* creative_dependent) const
  {
    if(rate)
    {
      *rate = RevenueDecimal::ZERO;
    }

    if(creative_dependent)
    {
      *creative_dependent = false;
    }

    return true;
  }

  void
  CTRProvider::CalculationContext::get_ctr_details(
    CTRList& ctrs,
    const Creative*) const
  {
    ctrs.clear();
  }

}

int
main(int argc, char** argv)
{
  try
  {
    const Options options = parse_options(argc, argv);
    const std::filesystem::path template_path =
      std::filesystem::path(options.template_root) / "instantiate-template.txt";
    write_template_file(template_path);

    CreativeInstantiator::CreativeInstantiate creative_instantiate =
      make_creative_instantiate();

    Logging::Logger_var logger(new Logging::Null::Logger);
    CampaignManagerCore::CountryList country_whitelist;
    PassbackTemplateMap passback_templates;
    CampaignManagerCore::TokenToParamMap token_to_parameters;
    AdServer::Commons::IPCrypter_var ip_crypter;
    Generics::SignedUuidGenerator rid_signer(options.rid_private_key.c_str());
    CreativeInstantiator creative_instantiator(
      make_creative_instantiator_config(template_path),
      creative_instantiate,
      passback_templates,
      token_to_parameters,
      ip_crypter,
      rid_signer,
      logger,
      country_whitelist);

    Fixture fixture = make_fixture(template_path);
    std::atomic<unsigned long> checksum{0};

    const CpuTimes start_cpu = current_cpu_times();

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
          &creative_instantiator,
          &fixture,
          items,
          &checksum
        ]()
        {
          for(unsigned long i = 0; i < items; ++i)
          {
            instantiate_once(creative_instantiator, fixture, checksum);
          }
        });
    }

    for(auto& worker : workers)
    {
      worker.join();
    }

    const CpuTimes finish_cpu = current_cpu_times();
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
  catch(const eh::Exception& ex)
  {
    std::cerr << "Caught eh::Exception: " << ex.what() << std::endl;
  }

  return 1;
}
