#include <Generics/Time.hpp>
#include <PrivacyFilter/Filter.hpp>
#include <LogCommons/LogCommons.hpp>
#include <ProfilingCommons/ProfileMap/RocksDBProfileMap.hpp>

#include "Compatibility/RequestProfileAdapter.hpp"

#include "RequestInfoContainer.hpp"

/**
 * process request steps (See Request processing steps):
 *   request
 *   => impression track,
 *      cookie action marker (contains request id)
 **     one of two:
 **       1. keep uid -> request id link (privacy?)
 **       2. keep marker in cookies
 *   => click
 *   => action request (test that action marker exists)
 *
 * Need add Reason header
 * for action and click 'Bad Request' response.
 */

//#define DEBUG_OUTPUT_

namespace AdServer{
namespace RequestInfoSvcs {

  enum
  {
    ENABLED_NOTICE = 1,
    DISABLED_PUB_COST_CHECK = 2 // don't change by compatibility reasons
  };

  const Generics::Time
  RequestInfoContainer::DEFAULT_EXPIRE_TIME =
    Generics::Time::ONE_DAY * 180; // 180 days

  struct UuidToString
  {
    std::string
    operator()(const Generics::Uuid& uuid) const
    {
      return uuid.to_string();
    }
  };

  /** profile help methods */
  void
  convert_optional_uint(
    AdServer::Commons::Optional<unsigned long>& result,
    long source)
  {
    result = source >= 0 ?
      AdServer::Commons::Optional<unsigned long>(source) :
      AdServer::Commons::Optional<unsigned long>();
  }

  void
  fill_optional_uint(
    int32_t& result,
    const AdServer::Commons::Optional<unsigned long>& source)
  {
    result = source.present() ? static_cast<long>(*source) : -1;
  }

  void
  create_empty_revenue(RequestInfoRevenueWriter& revenue_writer)
  {
    revenue_writer.rate_id() = 0;
    revenue_writer.impression() = "0";
    revenue_writer.click() = "0";
    revenue_writer.action() = "0";
  }

  Generics::Time last_event_time(
    const RequestInfoProfileReader& profile_reader)
  {
    uint32_t res = profile_reader.time();

    if (profile_reader.click_time() > res)
    {
      res = profile_reader.click_time();
    }

    if (profile_reader.action_time() > res)
    {
      res = profile_reader.action_time();
    }

    if (profile_reader.impression_time() > res)
    {
      res = profile_reader.impression_time();
    }

    if (profile_reader.fraud_time() > res)
    {
      res = profile_reader.fraud_time();
    }

    return Generics::Time(res);
  }

  RevenueDecimal
  convert_rate(
    const RevenueDecimal& value,
    const RevenueDecimal& from_rate,
    const RevenueDecimal& to_rate)
  {
    if(from_rate == to_rate)
    {
      return value;
    }

    const RevenueDecimal sys_value = RevenueDecimal::div(
      value,
      from_rate,
      Generics::DDR_CEIL);

    return RevenueDecimal::mul(
      sys_value,
      to_rate,
      Generics::DMR_CEIL);
  }

  void
  convert_revenue_to_revenue_writer(
    const RequestInfo::Revenue& revenue,
    RequestInfoRevenueWriter& revenue_writer,
    bool override_zero_imp = true)
  {
    revenue_writer.rate_id() = revenue.rate_id;
    if(override_zero_imp ||
       revenue_writer.impression().empty() ||
       RevenueDecimal(revenue_writer.impression()) == RevenueDecimal::ZERO)
    {
      revenue_writer.impression() = revenue.impression.str();
    }
    revenue_writer.click() = revenue.click.str();
    revenue_writer.action() = revenue.action.str();
  }

  template<typename RevenueReaderType>
  void
  convert_revenue_reader_to_revenue(
    RequestInfo::Revenue& revenue,
    const RevenueReaderType& revenue_reader)
  {
    revenue.rate_id = revenue_reader.rate_id();
    revenue.impression = RevenueDecimal(String::SubString(revenue_reader.impression()));
    revenue.click = RevenueDecimal(String::SubString(revenue_reader.click()));
    revenue.action = RevenueDecimal(String::SubString(revenue_reader.action()));
  }

  void
  convert_request_reader_to_request_info(
    RequestInfo& request_info,
    const RequestInfoProfileReader& request_reader)
  {
    request_info.time = Generics::Time(request_reader.time());
    request_info.isp_time = Generics::Time(request_reader.isp_time());
    request_info.pub_time = Generics::Time(request_reader.pub_time());
    request_info.adv_time = Generics::Time(request_reader.adv_time());
    request_info.user_id = request_reader.user_id()[0] ?
      AdServer::Commons::UserId(request_reader.user_id()) :
      AdServer::Commons::UserId();
    request_info.household_id = request_reader.household_id()[0] ?
      AdServer::Commons::UserId(request_reader.household_id()) :
      AdServer::Commons::UserId();
    request_info.user_status = request_reader.user_status();
    request_info.test_request = request_reader.test_request();
    request_info.walled_garden = request_reader.walled_garden();
    request_info.hid_profile = request_reader.hid_profile();
    request_info.disable_fraud_detection = request_reader.disable_fraud_detection();
    request_info.colo_id = request_reader.colo_id();
    request_info.publisher_account_id = request_reader.publisher_account_id();
    request_info.site_id = request_reader.site_id();
    request_info.tag_id = request_reader.tag_id();
    request_info.size_id = request_reader.size_id();
    request_info.ext_tag_id = request_reader.ext_tag_id();
    request_info.adv_account_id = request_reader.adv_account_id();
    request_info.advertiser_id = request_reader.advertiser_id();
    request_info.campaign_id = request_reader.campaign_id();
    request_info.ccg_id = request_reader.ccg_id();
    request_info.ctr_reset_id = request_reader.ctr_reset_id();
    request_info.cc_id = request_reader.cc_id();
    request_info.has_custom_actions = request_reader.has_custom_actions();
    request_info.currency_exchange_id = request_reader.currency_exchange_id();
    request_info.tag_delivery_threshold = DeliveryThresholdDecimal(
      request_reader.tag_delivery_threshold());

    request_info.ccg_keyword_id = request_reader.ccg_keyword_id();
    request_info.keyword_id = request_reader.keyword_id();

    request_info.num_shown = request_reader.num_shown();
    request_info.position = request_reader.position();
    request_info.tag_size = request_reader.tag_size();
    convert_optional_uint(
      request_info.tag_visibility, request_reader.tag_visibility());
    convert_optional_uint(
      request_info.tag_top_offset, request_reader.tag_top_offset());
    convert_optional_uint(
      request_info.tag_left_offset, request_reader.tag_left_offset());

    request_info.text_campaign = request_reader.text_campaign();

    convert_revenue_reader_to_revenue(request_info.adv_revenue, request_reader.adv_revenue());
    request_info.adv_revenue.currency_rate = RevenueDecimal(request_reader.adv_currency_rate());
    convert_revenue_reader_to_revenue(
      request_info.adv_comm_revenue, request_reader.adv_comm_revenue());
    convert_revenue_reader_to_revenue(
      request_info.adv_payable_comm_amount, request_reader.adv_payable_comm_amount());

    convert_revenue_reader_to_revenue(request_info.pub_revenue, request_reader.pub_revenue());
    request_info.pub_revenue.currency_rate = RevenueDecimal(request_reader.pub_currency_rate());
    request_info.pub_bid_cost = RevenueDecimal(request_reader.pub_bid_cost());
    request_info.pub_floor_cost = RevenueDecimal(request_reader.pub_floor_cost());

    convert_revenue_reader_to_revenue(
      request_info.pub_comm_revenue, request_reader.pub_comm_revenue());
    request_info.pub_commission = RevenueDecimal(request_reader.pub_commission());

    convert_revenue_reader_to_revenue(request_info.isp_revenue, request_reader.isp_revenue());
    request_info.isp_revenue.currency_rate = RevenueDecimal(request_reader.isp_currency_rate());
    request_info.isp_revenue_share = RevenueDecimal(request_reader.isp_revenue_share());

    convert_revenue_reader_to_revenue(request_info.delta_adv_revenue, request_reader.delta_adv_revenue());

    request_info.expression = request_reader.expression();
    request_info.channels.assign(
      request_reader.channels().begin(), request_reader.channels().end());
    request_info.geo_channel_id = request_reader.geo_channel_id();
    request_info.device_channel_id = request_reader.device_channel_id();

    for(RequestInfoProfileReader::cmp_channels_Container::const_iterator
          ch_rate_it = request_reader.cmp_channels().begin();
        ch_rate_it != request_reader.cmp_channels().end(); ++ch_rate_it)
    {
      RequestInfo::ChannelRevenue ch_rev;
      ch_rev.channel_id = (*ch_rate_it).channel_id();
      ch_rev.channel_rate_id = (*ch_rate_it).channel_rate_id();
      ch_rev.impression = RevenueDecimal((*ch_rate_it).impression());
      ch_rev.sys_impression = RevenueDecimal((*ch_rate_it).sys_impression());
      ch_rev.adv_impression = RevenueDecimal((*ch_rate_it).adv_impression());
      ch_rev.click = RevenueDecimal((*ch_rate_it).click());
      ch_rev.sys_click = RevenueDecimal((*ch_rate_it).sys_click());
      ch_rev.adv_click = RevenueDecimal((*ch_rate_it).adv_click());
      request_info.cmp_channels.push_back(ch_rev);
    }

    request_info.client_app = request_reader.client_app();
    request_info.client_app_version = request_reader.client_app_version();
    request_info.browser_version = request_reader.browser_version();
    request_info.os_version = request_reader.os_version();
    request_info.country = request_reader.country();
    request_info.referer = request_reader.referer();

    request_info.enabled_notice =
      (request_reader.enabled_notice() & ENABLED_NOTICE);
    request_info.disabled_pub_cost_check =
      (request_reader.enabled_notice() & DISABLED_PUB_COST_CHECK);
    request_info.enabled_impression_tracking =
      request_reader.enabled_impression_tracking();
    request_info.enabled_action_tracking =
      request_reader.enabled_action_tracking();

    request_info.imp_time = Generics::Time(request_reader.impression_time());
    request_info.click_time = Generics::Time(request_reader.click_time());
    request_info.action_time = Generics::Time(request_reader.action_time());

    request_info.auction_type = CampaignSvcs::AuctionType(request_reader.auction_type());

    request_info.ctr = RevenueDecimal(request_reader.ctr());
    request_info.campaign_freq = request_reader.campaign_freq();
    request_info.referer_hash = request_reader.referer_hash();
    request_info.geo_channels.reserve(request_reader.geo_channels().size());
    std::copy(request_reader.geo_channels().begin(),
      request_reader.geo_channels().end(),
      std::back_inserter(request_info.geo_channels));
    request_info.user_channels.reserve(request_reader.user_channels().size());
    std::copy(request_reader.user_channels().begin(),
      request_reader.user_channels().end(),
      std::back_inserter(request_info.user_channels));
    request_info.url = request_reader.url();
    request_info.ip_address = request_reader.ip_address();
    request_info.ctr_algorithm_id = request_reader.ctr_algorithm_id();

    for(auto ctr_it = request_reader.model_ctrs().begin();
      ctr_it != request_reader.model_ctrs().end(); ++ctr_it)
    {
      request_info.model_ctrs.push_back(RevenueDecimal(*ctr_it));
    }

    request_info.conv_rate_algorithm_id = request_reader.conv_rate_algorithm_id();
    request_info.conv_rate = RevenueDecimal(request_reader.conv_rate());

    for(auto conv_it = request_reader.model_conv_rates().begin();
      conv_it != request_reader.model_conv_rates().end();
      ++conv_it)
    {
      request_info.model_conv_rates.push_back(RevenueDecimal(*conv_it));
    }

    request_info.viewability = request_reader.viewability(); 
    request_info.self_service_commission = RevenueDecimal(
      request_reader.self_service_commission());
    request_info.adv_commission = RevenueDecimal(
      request_reader.adv_commission());
    request_info.pub_cost_coef = RevenueDecimal(
      request_reader.pub_cost_coef());
    request_info.at_flags = request_reader.at_flags();
  }

  void
  convert_request_reader_to_impression_info(
    ImpressionInfo& impression_info,
    const RequestInfoProfileReader& request_reader,
    bool notice,
    const AdServer::Commons::RequestId& request_id)
  {
    AdServer::CampaignSvcs::RevenueType pub_revenue_type;

    impression_info.request_id = request_id;

    if(notice)
    {
      impression_info.time = Generics::Time(request_reader.time());
      pub_revenue_type =
        static_cast<AdServer::CampaignSvcs::RevenueType>(
          request_reader.notice_pub_revenue_type());
      if(request_reader.notice_imp_revenue()[0])
      {
        ImpressionInfo::PubRevenue pub_revenue;
        pub_revenue.revenue_type = pub_revenue_type;
        pub_revenue.impression = RevenueDecimal(request_reader.notice_imp_revenue());
        impression_info.pub_revenue = pub_revenue;
      }
      else
      {
        impression_info.pub_revenue = AdServer::Commons::Optional<ImpressionInfo::PubRevenue>();
      }
    }
    else
    {
      impression_info.time = Generics::Time(request_reader.impression_time());
      if(request_reader.impression_user_id()[0])
      {
        impression_info.user_id = AdServer::Commons::UserId(request_reader.impression_user_id());
      }
      impression_info.viewability = request_reader.viewability();
      pub_revenue_type =
        static_cast<AdServer::CampaignSvcs::RevenueType>(
          request_reader.impression_pub_revenue_type());

      if(request_reader.impression_imp_revenue()[0])
      {
        ImpressionInfo::PubRevenue pub_revenue;
        pub_revenue.revenue_type = pub_revenue_type;
        pub_revenue.impression = RevenueDecimal(request_reader.impression_imp_revenue());
        impression_info.pub_revenue = pub_revenue;
      }
      else
      {
        impression_info.pub_revenue = AdServer::Commons::Optional<ImpressionInfo::PubRevenue>();
      }

      if(request_reader.impression_user_id()[0])
      {
        impression_info.user_id = AdServer::Commons::UserId(request_reader.impression_user_id());
      }
    }

    /*
    // get impression pub revenue traits for
    // reverse impression/request processing case
    if(pub_revenue_type != AdServer::CampaignSvcs::RT_NONE)
    {
      ImpressionInfo::PubRevenue pub_revenue;
      pub_revenue.revenue_type = pub_revenue_type;
      pub_revenue.impression = RevenueDecimal(
        request_reader.pub_revenue().impression());
      impression_info.pub_revenue = pub_revenue;
    }
    */
  }

  void
  fill_auto_impression_info(
    ImpressionInfo& impression_info,
    const RequestInfoProfileReader& request_reader)
  {
    impression_info.time = Generics::Time(request_reader.time());
  }

  void
  clear_action_fields(
    RequestInfoProfileWriter& request_writer)
  {
    request_writer.version() = CURRENT_REQUEST_PROFILE_VERSION;
    request_writer.request_done() = 0;

    request_writer.notice_received() = 0;
    request_writer.notice_non_considered() = 0;
    request_writer.notice_pub_revenue_type() =
      AdServer::CampaignSvcs::RT_NONE;

    request_writer.impression_time() = 0;
    request_writer.impression_verified() = 0;
    request_writer.impression_non_considered() = 0;
    request_writer.impression_pub_revenue_type() =
      AdServer::CampaignSvcs::RT_NONE;

    request_writer.click_time() = 0;
    request_writer.click_done() = 0;
    request_writer.click_non_considered() = 0;

    request_writer.action_time() = 0;
    request_writer.actions_done() = 0;
    request_writer.actions_non_considered() = 0;

    request_writer.fraud_time() = 0;
    request_writer.fraud() = RequestInfo::RS_NORMAL;
  }

  void
  convert_request_info_to_request_writer(
    const RequestInfo& request_info,
    RequestInfoProfileWriter& request_writer)
  {
    request_writer.version() = CURRENT_REQUEST_PROFILE_VERSION;
    request_writer.request_done() = 1;

    request_writer.time() = request_info.time.tv_sec;
    request_writer.isp_time() = request_info.isp_time.tv_sec;
    request_writer.pub_time() = request_info.pub_time.tv_sec;
    request_writer.adv_time() = request_info.adv_time.tv_sec;
    request_writer.user_id() = request_info.user_id.to_string();
    if(!request_info.household_id.is_null())
    {
      request_writer.household_id() = request_info.household_id.to_string();
    }
    request_writer.user_status() = request_info.user_status;
    request_writer.test_request() = request_info.test_request;
    request_writer.walled_garden() = request_info.walled_garden;
    request_writer.hid_profile() = request_info.hid_profile;
    request_writer.disable_fraud_detection() = request_info.disable_fraud_detection;
    request_writer.colo_id() = request_info.colo_id;
    request_writer.publisher_account_id() = request_info.publisher_account_id;
    request_writer.site_id() = request_info.site_id;
    request_writer.tag_id() = request_info.tag_id;
    request_writer.size_id() = request_info.size_id;
    request_writer.ext_tag_id() = request_info.ext_tag_id;
    request_writer.adv_account_id() = request_info.adv_account_id;
    request_writer.advertiser_id() = request_info.advertiser_id;
    request_writer.campaign_id() = request_info.campaign_id;
    request_writer.ccg_id() = request_info.ccg_id;
    request_writer.ctr_reset_id() = request_info.ctr_reset_id;
    request_writer.cc_id() = request_info.cc_id;
    request_writer.has_custom_actions() = request_info.has_custom_actions;
    request_writer.currency_exchange_id() = request_info.currency_exchange_id;
    request_writer.tag_delivery_threshold() = request_info.tag_delivery_threshold.str();

    request_writer.ccg_keyword_id() = request_info.ccg_keyword_id;
    request_writer.keyword_id() = request_info.keyword_id;

    request_writer.num_shown() = request_info.num_shown;
    request_writer.position() = request_info.position;
    request_writer.tag_size() = request_info.tag_size;
    fill_optional_uint(
      request_writer.tag_visibility(), request_info.tag_visibility);
    fill_optional_uint(
      request_writer.tag_top_offset(), request_info.tag_top_offset);
    fill_optional_uint(
      request_writer.tag_left_offset(), request_info.tag_left_offset);

    request_writer.text_campaign() = request_info.text_campaign;

    convert_revenue_to_revenue_writer(
      request_info.adv_revenue, request_writer.adv_revenue());

    request_writer.adv_currency_rate() = request_info.adv_revenue.currency_rate.str();
    request_writer.pub_currency_rate() = request_info.pub_revenue.currency_rate.str();
    request_writer.isp_currency_rate() = request_info.isp_revenue.currency_rate.str();

    // fill orig revenues
    convert_revenue_to_revenue_writer(
      request_info.adv_revenue, request_writer.orig_adv_revenue());
    convert_revenue_to_revenue_writer(
      request_info.adv_comm_revenue, request_writer.orig_adv_comm_revenue());
    convert_revenue_to_revenue_writer(
      request_info.adv_payable_comm_amount, request_writer.orig_adv_payable_comm_amount());
    convert_revenue_to_revenue_writer(
      request_info.pub_revenue, request_writer.orig_pub_revenue());
    convert_revenue_to_revenue_writer(
      request_info.pub_comm_revenue, request_writer.orig_pub_comm_revenue());
    convert_revenue_to_revenue_writer(
      request_info.isp_revenue, request_writer.orig_isp_revenue());

    convert_revenue_to_revenue_writer(
      request_info.adv_comm_revenue, request_writer.adv_comm_revenue());
    convert_revenue_to_revenue_writer(
      request_info.adv_payable_comm_amount, request_writer.adv_payable_comm_amount());
    convert_revenue_to_revenue_writer(
      request_info.pub_revenue, request_writer.pub_revenue());
    request_writer.pub_bid_cost() = request_info.pub_bid_cost.str();
    request_writer.pub_floor_cost() = request_info.pub_floor_cost.str();
    convert_revenue_to_revenue_writer(
      request_info.pub_comm_revenue, request_writer.pub_comm_revenue());
    request_writer.pub_commission() = request_info.pub_commission.str();
    convert_revenue_to_revenue_writer(
      request_info.isp_revenue, request_writer.isp_revenue());
    request_writer.isp_revenue_share() = request_info.isp_revenue_share.str();

    convert_revenue_to_revenue_writer(
      request_info.delta_adv_revenue, request_writer.delta_adv_revenue());

    request_writer.expression() = request_info.expression;
    request_writer.channels().assign(
      request_info.channels.begin(), request_info.channels.end());
    request_writer.geo_channel_id() = request_info.geo_channel_id;
    request_writer.device_channel_id() = request_info.device_channel_id;

    for(RequestInfo::ChannelRevenueList::const_iterator ch_rate_it =
          request_info.cmp_channels.begin();
        ch_rate_it != request_info.cmp_channels.end(); ++ch_rate_it)
    {
      ChannelRevenueWriter ch_rev_writer;
      ch_rev_writer.channel_id() = ch_rate_it->channel_id;
      ch_rev_writer.channel_rate_id() = ch_rate_it->channel_rate_id;
      ch_rev_writer.impression() = ch_rate_it->impression.str();
      ch_rev_writer.sys_impression() = ch_rate_it->sys_impression.str();
      ch_rev_writer.adv_impression() = ch_rate_it->adv_impression.str();
      ch_rev_writer.click() = ch_rate_it->click.str();
      ch_rev_writer.sys_click() = ch_rate_it->sys_click.str();
      ch_rev_writer.adv_click() = ch_rate_it->adv_click.str();
      request_writer.cmp_channels().push_back(ch_rev_writer);
    }

    request_writer.client_app() = request_info.client_app;
    request_writer.client_app_version() = request_info.client_app_version;
    request_writer.browser_version() = request_info.browser_version;
    request_writer.os_version() = request_info.os_version;
    request_writer.country() = request_info.country;
    request_writer.referer() = request_info.referer;

    request_writer.enabled_notice() =
      (request_info.enabled_notice ? ENABLED_NOTICE : 0) |
      (request_info.disabled_pub_cost_check ? DISABLED_PUB_COST_CHECK : 0);
    request_writer.enabled_impression_tracking() =
      request_info.enabled_impression_tracking;
    request_writer.enabled_action_tracking() =
      request_info.enabled_action_tracking;

    request_writer.auction_type() = request_info.auction_type;

    request_writer.ctr() = request_info.ctr.str();
    request_writer.campaign_freq() = request_info.campaign_freq;
    request_writer.referer_hash() = request_info.referer_hash;
    request_writer.geo_channels().reserve(request_info.geo_channels.size());
    std::copy(request_info.geo_channels.begin(),
      request_info.geo_channels.end(),
      std::back_inserter(request_writer.geo_channels()));
    request_writer.user_channels().reserve(request_info.user_channels.size());
    std::copy(request_info.user_channels.begin(),
      request_info.user_channels.end(),
      std::back_inserter(request_writer.user_channels()));
    request_writer.url() = request_info.url;
    request_writer.ip_address() = request_info.ip_address;
    request_writer.ctr_algorithm_id() = request_info.ctr_algorithm_id;
    request_writer.model_ctrs().reserve(request_info.model_ctrs.size());
    for(auto ctr_it = request_info.model_ctrs.begin();
      ctr_it != request_info.model_ctrs.end(); ++ctr_it)
    {
      request_writer.model_ctrs().push_back(ctr_it->str());
    }
    request_writer.conv_rate_algorithm_id() = request_info.conv_rate_algorithm_id;
    request_writer.conv_rate() = request_info.conv_rate.str();
    request_writer.model_conv_rates().reserve(request_info.model_conv_rates.size());
    for(auto conv_it = request_info.model_conv_rates.begin();
      conv_it != request_info.model_conv_rates.end();
      ++conv_it)
    {
      request_writer.model_conv_rates().push_back(conv_it->str());
    }

    request_writer.viewability() = request_info.viewability;
    request_writer.self_service_commission() = request_info.self_service_commission.str();
    request_writer.adv_commission() = request_info.adv_commission.str();
    request_writer.pub_cost_coef() = request_info.pub_cost_coef.str();
    request_writer.at_flags() = request_info.at_flags;
  }

  void
  RequestInfoContainer::eval_revenues_on_impression(
    RequestInfoProfileWriter& request_writer,
    RequestInfo* request_info,
    const RevenueDecimal& pass_res_pub_imp_revenue,
    const RequestInfo::Revenue& orig_pub_revenue)
  {
    // apply cost_coef to imp_pub_revenue
    //RevenueDecimal b_res_pub_revenue = pass_res_pub_imp_revenue;

    const RevenueDecimal pub_cost_coef(request_writer.pub_cost_coef());
    RevenueDecimal res_pub_imp_revenue = RevenueDecimal::div(
      pass_res_pub_imp_revenue,
      AdServer::CampaignSvcs::REVENUE_ONE + pub_cost_coef,
      Generics::DDR_CEIL);

    /*
    std::cerr << "apply pub_cost_coef : " <<
      b_res_pub_revenue << " => " << res_pub_imp_revenue << std::endl;
    */
    request_writer.pub_revenue().impression() = res_pub_imp_revenue.str();

    RevenueDecimal pub_commission(request_writer.pub_commission());
    unsigned long acc_id = request_writer.publisher_account_id();
    RevenueDecimal res_pub_comm_revenue = div(
      RevenueDecimal::mul(
        res_pub_imp_revenue,
        pub_commission,
        Generics::DMR_CEIL),
      RevenueDecimal(1) - pub_commission,
      "Publisher commission is equal to ONE, setting commission revenue to ZERO!",
      acc_id);

    request_writer.pub_comm_revenue().impression() = res_pub_comm_revenue.str();

    // eval & save revenue values
    RequestInfo::Revenue res_adv_revenue;
    RequestInfo::Revenue add_adv_revenue;
    RequestInfo::Revenue res_adv_comm_revenue;
    RequestInfo::Revenue res_pub_revenue;
    RequestInfo::Revenue res_isp_revenue;

    {
      const RevenueDecimal self_service_commission =
        RevenueDecimal(request_writer.self_service_commission()) +
        RevenueDecimal(request_writer.isp_revenue_share());

      const RevenueDecimal adv_currency_rate(request_writer.adv_currency_rate());
      const RevenueDecimal pub_currency_rate(request_writer.pub_currency_rate());
      const RevenueDecimal isp_currency_rate(request_writer.isp_currency_rate());

      // reeval all revenue fields (imp, click, action) by schema
      RequestInfo::Revenue adv_revenue;
      convert_revenue_reader_to_revenue(adv_revenue, request_writer.adv_revenue());

      RequestInfo::Revenue prev_add_adv_revenue;
      convert_revenue_reader_to_revenue(prev_add_adv_revenue, request_writer.delta_adv_revenue());

      adv_revenue += prev_add_adv_revenue;

      RequestInfo::Revenue adv_comm_revenue;
      convert_revenue_reader_to_revenue(adv_comm_revenue, request_writer.adv_comm_revenue());

      RequestInfo::Revenue pub_revenue;
      convert_revenue_reader_to_revenue(pub_revenue, request_writer.orig_pub_revenue());

      RequestInfo::Revenue isp_revenue;
      convert_revenue_reader_to_revenue(isp_revenue, request_writer.isp_revenue());

      RequestInfo::Revenue orig_pub_revenue;
      convert_revenue_reader_to_revenue(orig_pub_revenue, request_writer.orig_pub_revenue());

      /*
      std::cout << "eval_revenues_on_impression(step 1): adv_revenue.impression = " << adv_revenue.impression <<
        ", adv_revenue.click = " << adv_revenue.click << std::endl <<
        "  adv_revenue:";
      adv_revenue.print(std::cout, "    ");
      std::cout << "  delta_adv_revenue:";
      prev_add_adv_revenue.print(std::cout, "    ");
      std::cout << "  adv_comm_revenue:";
      adv_comm_revenue.print(std::cout, "    ");
      std::cout << "  pub_revenue:";
      pub_revenue.print(std::cout, "    ");
      std::cout << "  isp_revenue:";
      isp_revenue.print(std::cout, "    ");
      std::cout << "  orig_pub_revenue:";
      orig_pub_revenue.print(std::cout, "    ");
      std::cout << std::endl;
      */

      res_adv_comm_revenue = adv_comm_revenue;
      res_pub_revenue = orig_pub_revenue;
      res_isp_revenue = isp_revenue;

      // fill res_pub_revenue
      {
        Commons::Optional<RevenueDecimal> override_pub_imp_revenue;
        if(!request_writer.notice_imp_revenue().empty())
        {
          override_pub_imp_revenue = RevenueDecimal(request_writer.notice_imp_revenue());
        }

        if(!request_writer.impression_imp_revenue().empty())
        {
          RevenueDecimal impression_imp_revenue = RevenueDecimal(request_writer.impression_imp_revenue());
          if(override_pub_imp_revenue.present())
          {
            override_pub_imp_revenue = std::min(*override_pub_imp_revenue, impression_imp_revenue);
          }
          else
          {
            override_pub_imp_revenue = impression_imp_revenue;
          }
        }

        if(override_pub_imp_revenue.present())
        {
          res_pub_revenue.impression = std::min(orig_pub_revenue.impression, *override_pub_imp_revenue);
        }
        else
        {
          res_pub_revenue = orig_pub_revenue;
        }
      }

      // apply cost_coef to res_pub_revenue
      res_pub_revenue /= AdServer::CampaignSvcs::REVENUE_ONE + pub_cost_coef;

      RequestInfo::Revenue delta_adv_revenue;
      RequestInfo::Revenue delta_adv_comm_revenue;
      //RequestInfo::Revenue delta_pub_revenue;
      RequestInfo::Revenue delta_isp_revenue;

      if((request_writer.at_flags() &
          AdServer::CampaignSvcs::AccountTypeFlags::AGENCY_PROFIT_BY_PUB_AMOUNT) == 0)
      {
        //std::cout << "eval_revenues_on_impression(step 1.5): schema #1, at_flags = " << request_writer.at_flags() << std::endl;
        // schema #1
        // delta_adv_revenue = ZERO
        delta_adv_comm_revenue =
          ((orig_pub_revenue - res_pub_revenue) * (
            self_service_commission + AdServer::CampaignSvcs::REVENUE_ONE)).convert_currency(
              pub_currency_rate,
              adv_currency_rate);

        //delta_pub_revenue = res_pub_revenue - orig_pub_revenue;

        delta_isp_revenue =
          ((res_pub_revenue - orig_pub_revenue) * self_service_commission).convert_currency(
            pub_currency_rate,
            isp_currency_rate);
      }
      else
      {
        //std::cout << "eval_revenues_on_impression(step 1.5): schema #2, at_flags = " << request_writer.at_flags() << std::endl;
        // schema #2
        const RevenueDecimal adv_commission = RevenueDecimal(request_writer.adv_commission());

        // calculate delta adv_revenue
        delta_adv_revenue = (
          res_pub_revenue *
          (self_service_commission + AdServer::CampaignSvcs::REVENUE_ONE) *
          (adv_commission + AdServer::CampaignSvcs::REVENUE_ONE)).convert_currency(
            pub_currency_rate,
            adv_currency_rate) - adv_revenue;

        // calculate delta adv_comm_revenue
        delta_adv_comm_revenue =
          ((res_pub_revenue - orig_pub_revenue) * (
            self_service_commission + AdServer::CampaignSvcs::REVENUE_ONE) *
           adv_commission).convert_currency(
             pub_currency_rate,
             adv_currency_rate);

        // calculate delta _revenue
        //delta_pub_revenue = res_pub_revenue - orig_pub_revenue;

        // calculate delta isp_revenue
        delta_isp_revenue =
          ((res_pub_revenue - orig_pub_revenue) * self_service_commission).convert_currency(
            pub_currency_rate,
            isp_currency_rate);
      }

      res_adv_revenue = adv_revenue + delta_adv_revenue;
      /*
      std::cout << "eval_revenues_on_impression(step 2): delta_adv_revenue.impression = " << delta_adv_revenue.impression <<
        ", delta_adv_revenue.click = " << delta_adv_revenue.click << std::endl;
      std::cout << "eval_revenues_on_impression(step 3): res_adv_revenue.impression = " << res_adv_revenue.impression <<
        ", res_adv_revenue.click = " << res_adv_revenue.click << std::endl;
      */
      convert_revenue_to_revenue_writer(
        res_adv_revenue,
        request_writer.adv_revenue());

      add_adv_revenue = RequestInfo::Revenue();
      add_adv_revenue -= delta_adv_revenue;

      convert_revenue_to_revenue_writer(
        add_adv_revenue,
        request_writer.delta_adv_revenue());

      res_adv_comm_revenue = adv_comm_revenue + delta_adv_comm_revenue;
      convert_revenue_to_revenue_writer(
        res_adv_comm_revenue,
        request_writer.adv_comm_revenue());

      convert_revenue_to_revenue_writer(
        res_pub_revenue,
        request_writer.pub_revenue());

      res_isp_revenue = isp_revenue + delta_isp_revenue;
      convert_revenue_to_revenue_writer(
        res_isp_revenue,
        request_writer.isp_revenue());
    }

    if(request_info)
    {
      request_info->adv_revenue = res_adv_revenue;
      request_info->delta_adv_revenue = add_adv_revenue;
      request_info->adv_comm_revenue = res_adv_comm_revenue;
      request_info->pub_revenue = res_pub_revenue;
      request_info->isp_revenue = res_isp_revenue;

      request_info->pub_comm_revenue.impression = res_pub_comm_revenue;
    }
  }

  void
  RequestInfoContainer::eval_revenues_on_impression_fin(
    RequestInfoProfileWriter& request_writer,
    RequestInfo* request_info // fill revenues if passed
    )
  {
    // eval final pub amount
    Commons::Optional<RevenueDecimal> prev_pub_imp_revenue;
    if(!request_writer.notice_imp_revenue().empty())
    {
      prev_pub_imp_revenue = RevenueDecimal(request_writer.notice_imp_revenue());
    }

    if(!request_writer.impression_imp_revenue().empty())
    {
      RevenueDecimal impression_imp_revenue = RevenueDecimal(request_writer.impression_imp_revenue());
      if(prev_pub_imp_revenue.present())
      {
        prev_pub_imp_revenue = std::min(*prev_pub_imp_revenue, impression_imp_revenue);
      }
      else
      {
        prev_pub_imp_revenue = impression_imp_revenue;
      }
    }

    // make this step:
    // on notice if cost present
    // on impression always for apply VIVAKI scheme
    {
      RequestInfo::Revenue orig_pub_revenue;
      convert_revenue_reader_to_revenue(orig_pub_revenue, request_writer.orig_pub_revenue());

      RevenueDecimal res_imp_pub_revenue = orig_pub_revenue.impression;

      if(request_writer.impression_pub_revenue_type() == AdServer::CampaignSvcs::RT_SHARE)
      {
        // imp_pub_revenue is share for SHARE case
        const RevenueDecimal& pub_share = *prev_pub_imp_revenue;

        res_imp_pub_revenue = RevenueDecimal::mul(
          orig_pub_revenue.impression,
          pub_share,
          Generics::DMR_CEIL);
      }
      else
      {
        if(prev_pub_imp_revenue.present())
        {
          res_imp_pub_revenue = std::min(res_imp_pub_revenue, *prev_pub_imp_revenue);
        }
      }

      request_writer.pub_revenue().impression() = res_imp_pub_revenue.str();

      eval_revenues_on_impression(
        request_writer,
        request_info,
        res_imp_pub_revenue,
        orig_pub_revenue);
    } // if (impression_info.pub_revenue.present())
  }

  void
  RequestInfoContainer::convert_impression_info_to_request_writer(
    const ImpressionInfo& impression_info,
    RequestInfoProfileWriter& request_writer,
    RequestInfo* request_info,
    bool notice)
  {
    //std::cerr << "RequestInfoContainer::convert_impression_info_to_request_writer()" << std::endl;

    /*
    if(!impression_info.user_id.is_null())
    {
      request_writer.user_id() = impression_info.user_id.to_string();
      request_writer.user_status() = 'I';
    }
    */

    if(notice)
    {
      request_writer.notice_pub_revenue_type() = impression_info.pub_revenue->revenue_type;
      
      if(impression_info.pub_revenue.present())
      {
        request_writer.notice_imp_revenue() = impression_info.pub_revenue->impression.str();
      }
      else
      {
        request_writer.notice_imp_revenue().clear();
      }
    }
    else
    {
      if(!impression_info.user_id.is_null())
      {
        request_writer.impression_user_id() = impression_info.user_id.to_string();
      }

      request_writer.impression_pub_revenue_type() = impression_info.pub_revenue->revenue_type;

      if(impression_info.pub_revenue.present())
      {
        request_writer.impression_imp_revenue() = impression_info.pub_revenue->impression.str();
      }
      else
      {
        request_writer.impression_imp_revenue().clear();
      }
    }

    /*
    // calculate pub_revenue
    if(request_writer.request_done())
    {
      Commons::Optional<RevenueDecimal> prev_pub_imp_revenue;
      if(!request_writer.notice_imp_revenue().empty())
      {
        prev_pub_imp_revenue = RevenueDecimal(request_writer.notice_imp_revenue());
      }

      if(!request_writer.impression_imp_revenue().empty())
      {
        RevenueDecimal impression_imp_revenue = RevenueDecimal(request_writer.impression_imp_revenue());
        if(prev_pub_imp_revenue.present())
        {
          prev_pub_imp_revenue = std::min(*prev_pub_imp_revenue, impression_imp_revenue);
        }
        else
        {
          prev_pub_imp_revenue = impression_imp_revenue;
        }
      }

      // make this step:
      // on notice if cost present
      // on impression always for apply VIVAKI scheme
      if(prev_pub_imp_revenue.present() && !notice && !request_writer.impression_verified())
      {
        // override publisher revenue values
        // reset pub_revenue_type for exclude double applying
        if(notice)
        {
          request_writer.notice_pub_revenue_type() = AdServer::CampaignSvcs::RT_NONE;
        }
        else
        {
          request_writer.impression_pub_revenue_type() = AdServer::CampaignSvcs::RT_NONE;
        }

        RequestInfo::Revenue orig_pub_revenue;
        convert_revenue_reader_to_revenue(orig_pub_revenue, request_writer.pub_revenue());

        RevenueDecimal imp_pub_revenue = impression_info.pub_revenue.present() ?
          impression_info.pub_revenue->impression :
          orig_pub_revenue.impression;

        RevenueDecimal res_imp_pub_revenue;

        if(!impression_info.pub_revenue.present() ||
          impression_info.pub_revenue->revenue_type == AdServer::CampaignSvcs::RT_ABSOLUTE)
        {
          res_imp_pub_revenue = imp_pub_revenue;
        }
        else // RT_SHARE
        {
          // imp_pub_revenue is share for SHARE case
          const RevenueDecimal& pub_share = imp_pub_revenue;

          res_imp_pub_revenue = RevenueDecimal::mul(
            orig_pub_revenue.impression,
            pub_share,
            Generics::DMR_CEIL);
        }

        res_imp_pub_revenue = std::min(res_imp_pub_revenue, orig_pub_revenue.impression);
        request_writer.pub_revenue().impression() = res_imp_pub_revenue.str();

        // make all reevaluations only on impression (it can be applied only once)
        //if(!notice && (!request_writer.enabled_notice() || request_writer.notice_received()))
        if(!notice)
        {
          eval_revenues_on_impression(
            request_writer,
            request_info,
            res_imp_pub_revenue,
            orig_pub_revenue);
        }
      } // if (impression_info.pub_revenue.present())
    }
    else // !request_writer.request_done() (impression came before profile)
    {
      if(impression_info.pub_revenue.present())
      {
        assert(impression_info.pub_revenue->revenue_type != AdServer::CampaignSvcs::RT_NONE);

        if(notice)
        {
          request_writer.notice_pub_revenue_type() =
            impression_info.pub_revenue->revenue_type;
        }
        else
        {
          request_writer.impression_pub_revenue_type() =
            impression_info.pub_revenue->revenue_type;
        }

        request_writer.pub_revenue().impression() = impression_info.pub_revenue->impression.str();
      }
    }

    if(notice)
    {
      request_writer.notice_pub_revenue_type() = AdServer::CampaignSvcs::RT_NONE;
    }
    else
    {
      request_writer.impression_pub_revenue_type() = AdServer::CampaignSvcs::RT_NONE;
    }
    */

    if(!notice)
    {
      request_writer.impression_time() = impression_info.time.tv_sec;
      request_writer.viewability() = impression_info.viewability;
    }
  }

  void
  create_empty_stub(
    RequestInfoProfileWriter& request_writer)
  {
    request_writer.version() = CURRENT_REQUEST_PROFILE_VERSION;
    request_writer.request_done() = 0;
    request_writer.user_id() = "";
    request_writer.household_id() = "";
    request_writer.user_status() = '-';
    request_writer.time() = 0;
    request_writer.isp_time() = 0;
    request_writer.pub_time() = 0;
    request_writer.adv_time() = 0;
    request_writer.test_request() = 0;
    request_writer.walled_garden() = 0;
    request_writer.hid_profile() = 0;
    request_writer.disable_fraud_detection() = 0;
    request_writer.colo_id() = 0;
    request_writer.publisher_account_id() = 0;
    request_writer.site_id() = 0;
    request_writer.tag_id() = 0;
    request_writer.size_id() = 0;
    request_writer.adv_account_id() = 0;
    request_writer.advertiser_id() = 0;
    request_writer.campaign_id() = 0;
    request_writer.ccg_id() = 0;
    request_writer.ctr_reset_id() = 0;
    request_writer.cc_id() = 0;
    request_writer.has_custom_actions() = 0;
    request_writer.currency_exchange_id() = 0;
    request_writer.tag_delivery_threshold() = "1";

    request_writer.ccg_keyword_id() = 0;
    request_writer.keyword_id() = 0;
    request_writer.num_shown() = 0;
    request_writer.position() = 0;
    request_writer.tag_visibility() = -1;
    request_writer.tag_top_offset() = -1;
    request_writer.tag_left_offset() = -1;

    request_writer.text_campaign() = 0;

    create_empty_revenue(request_writer.adv_revenue());
    create_empty_revenue(request_writer.adv_comm_revenue());
    create_empty_revenue(request_writer.adv_payable_comm_amount());
    request_writer.pub_bid_cost() = "0";
    request_writer.pub_floor_cost() = "0";
    create_empty_revenue(request_writer.pub_revenue());
    create_empty_revenue(request_writer.pub_comm_revenue());
    request_writer.pub_commission() = "0";
    create_empty_revenue(request_writer.isp_revenue());
    request_writer.isp_revenue_share() = "0";
    request_writer.adv_currency_rate() = "0";
    request_writer.pub_currency_rate() = "0";
    request_writer.isp_currency_rate() = "0";
    create_empty_revenue(request_writer.delta_adv_revenue());

    request_writer.geo_channel_id() = 0;
    request_writer.device_channel_id() = 0;
    request_writer.expression() = "";

    request_writer.client_app() = "";
    request_writer.client_app_version() = "";
    request_writer.browser_version() = "";
    request_writer.os_version() = "";
    request_writer.country() = "";
    request_writer.referer() = "";

    request_writer.enabled_notice() = 0;
    request_writer.enabled_impression_tracking() = 0;
    request_writer.enabled_action_tracking() = 0;

    request_writer.auction_type() = 0;
    request_writer.campaign_freq() = 0;
    request_writer.referer_hash() = 0;
    request_writer.ctr() = "0";
    request_writer.conv_rate() = "0";
    request_writer.viewability() = -1;
    request_writer.self_service_commission() = "0";
    request_writer.adv_commission() = "0";
    request_writer.pub_cost_coef() = "0";
    request_writer.at_flags() = 0;

    clear_action_fields(request_writer);
  }

  void
  save_request_writer(
    Generics::ConstSmartMemBuf_var& mem_buf_ptr,
    const RequestInfoProfileWriter& request_writer)
  {
    unsigned long sz = request_writer.size();

    Generics::SmartMemBuf_var mem_buf =
      new Generics::SmartMemBuf(sz);
    request_writer.save(mem_buf->membuf().data(), sz);
    mem_buf_ptr = Generics::transfer_membuf(mem_buf);

    assert(mem_buf_ptr->membuf().size() >= 4 &&
      *reinterpret_cast<const uint32_t*>(
        mem_buf_ptr->membuf().data()) == CURRENT_REQUEST_PROFILE_VERSION);
  }

  class RequestInfoContainer::ProxyImpl:
    public virtual ReferenceCounting::AtomicImpl,
    public virtual RequestContainerProcessor
  {
  public:
    ProxyImpl(RequestInfoContainer* owner) noexcept;

    virtual void
    process_request(const RequestInfo& request_info)
      /*throw(RequestContainerProcessor::Exception)*/;

    virtual void
    process_impression(const ImpressionInfo& impression_info)
      /*throw(RequestContainerProcessor::Exception)*/;

    virtual void
    process_action(
      ActionType action_type,
      const Generics::Time& time,
      const AdServer::Commons::RequestId& request_id)
      /*throw(RequestContainerProcessor::Exception)*/;

    virtual void
    process_custom_action(
      const AdServer::Commons::RequestId& request_id,
      const AdvCustomActionInfo& adv_custom_action_info)
      /*throw(RequestContainerProcessor::Exception)*/;

    virtual void
    process_impression_post_action(
      const AdServer::Commons::RequestId& request_id,
      const RequestPostActionInfo& request_post_action_info)
      /*throw(RequestContainerProcessor::Exception)*/;

    void detach() noexcept;

  protected:
    virtual
    ~ProxyImpl() noexcept = default;

  private:
    typedef Sync::Policy::PosixThread SyncPolicy;

  private:
    RequestInfoContainer_var lock_owner_() const noexcept;

  private:
    mutable SyncPolicy::Mutex lock_;
    RequestInfoContainer* owner_;
  };

  class RequestInfoContainer::RequestOperationProxy:
    public virtual ReferenceCounting::AtomicImpl,
    public virtual RequestOperationProcessor
  {
  public:
    // proxy with disabling operation move
    // disable of moving allow to solve case
    // when moved action received earlier then exchange over RS_MOVED profile
    RequestOperationProxy(RequestInfoContainer* owner) noexcept;

    virtual void
    process_impression(const ImpressionInfo& impression_info)
      /*throw(RequestOperationProcessor::Exception)*/;

    virtual void
    process_action(
      const AdServer::Commons::UserId& new_user_id,
      ActionType action_type,
      const Generics::Time& time,
      const AdServer::Commons::RequestId& request_id)
      /*throw(RequestOperationProcessor::Exception)*/;

    virtual void
    process_impression_post_action(
      const AdServer::Commons::UserId& new_user_id,
      const AdServer::Commons::RequestId& request_id,
      const RequestPostActionInfo& request_post_action_info)
      /*throw(RequestOperationProcessor::Exception)*/;

    virtual void
    change_request_user_id(
      const AdServer::Commons::UserId& new_user_id,
      const AdServer::Commons::RequestId& request_id,
      const Generics::ConstSmartMemBuf* request_profile)
      /*throw(RequestOperationProcessor::Exception)*/;

    void detach() noexcept;

  protected:
    virtual
    ~RequestOperationProxy() noexcept = default;

  private:
    typedef Sync::Policy::PosixThread SyncPolicy;

  private:
    RequestInfoContainer_var lock_owner_() const noexcept;

  private:
    mutable SyncPolicy::Mutex lock_;
    RequestInfoContainer* owner_;
  };

  // RequestInfoContainer::ProxyImpl
  RequestInfoContainer::ProxyImpl::ProxyImpl(RequestInfoContainer* owner)
    noexcept
    : owner_(owner)
  {}

  void
  RequestInfoContainer::ProxyImpl::detach() noexcept
  {
    SyncPolicy::WriteGuard lock(lock_);
    owner_ = 0;
  }

  RequestInfoContainer_var
  RequestInfoContainer::ProxyImpl::lock_owner_() const noexcept
  {
    SyncPolicy::ReadGuard lock(lock_);
    return ReferenceCounting::add_ref(owner_);
  }

  void
  RequestInfoContainer::ProxyImpl::process_request(
    const RequestInfo& request_info)
    /*throw(RequestContainerProcessor::Exception)*/
  {
    RequestInfoContainer_var owner = lock_owner_();

    if(owner.in())
    {
      owner->process_request(request_info);
    }
  }

  void
  RequestInfoContainer::ProxyImpl::process_impression(
    const ImpressionInfo& impression_info)
    /*throw(RequestContainerProcessor::Exception)*/
  {
    RequestInfoContainer_var owner = lock_owner_();

    if(owner.in())
    {
      owner->process_impression(impression_info);
    }
  }

  void
  RequestInfoContainer::ProxyImpl::process_action(
    ActionType action_type,
    const Generics::Time& time,
    const AdServer::Commons::RequestId& request_id)
    /*throw(RequestContainerProcessor::Exception)*/
  {
    RequestInfoContainer_var owner = lock_owner_();

    if(owner.in())
    {
      owner->process_action(action_type, time, request_id);
    }
  }

  void
  RequestInfoContainer::ProxyImpl::process_custom_action(
    const AdServer::Commons::RequestId& request_id,
    const AdvCustomActionInfo& adv_custom_action_info)
    /*throw(RequestContainerProcessor::Exception)*/
  {
    RequestInfoContainer_var owner = lock_owner_();

    if(owner.in())
    {
      owner->process_custom_action(
        request_id,
        adv_custom_action_info);
    }
  }

  void
  RequestInfoContainer::ProxyImpl::process_impression_post_action(
    const AdServer::Commons::RequestId& request_id,
    const RequestPostActionInfo& request_post_action_info)
    /*throw(RequestContainerProcessor::Exception)*/
  {
    RequestInfoContainer_var owner = lock_owner_();

    if(owner.in())
    {
      owner->process_impression_post_action(
        request_id,
        request_post_action_info);
    }
  }

  // RequestInfoContainer::RequestOperationProxy
  RequestInfoContainer::RequestOperationProxy::RequestOperationProxy(
    RequestInfoContainer* owner)
    noexcept
    : owner_(owner)
  {}

  void
  RequestInfoContainer::RequestOperationProxy::detach() noexcept
  {
    SyncPolicy::WriteGuard lock(lock_);
    owner_ = 0;
  }

  RequestInfoContainer_var
  RequestInfoContainer::RequestOperationProxy::lock_owner_() const noexcept
  {
    SyncPolicy::ReadGuard lock(lock_);
    return ReferenceCounting::add_ref(owner_);
  }

  void
  RequestInfoContainer::RequestOperationProxy::process_impression(
    const ImpressionInfo& impression_info)
    /*throw(RequestOperationProcessor::Exception)*/
  {
    RequestInfoContainer_var owner = lock_owner_();

    if(owner.in())
    {
      try
      {
        owner->process_impression_(
          impression_info,
          false // second move impossible
          );
      }
      catch(const eh::Exception& ex)
      {
        throw RequestOperationProcessor::Exception(ex.what());
      }
    }
  }

  void
  RequestInfoContainer::RequestOperationProxy::process_action(
    const AdServer::Commons::UserId& /*new_user_id*/,
    ActionType action_type,
    const Generics::Time& time,
    const AdServer::Commons::RequestId& request_id)
    /*throw(RequestOperationProcessor::Exception)*/
  {
    RequestInfoContainer_var owner = lock_owner_();

    if(owner.in())
    {
      try
      {
        // already moved operation will disable second moving
        owner->process_action_(
          action_type, time, request_id, false);
      }
      catch(const eh::Exception& ex)
      {
        throw RequestOperationProcessor::Exception(ex.what());
      }
    }
  }

  void
  RequestInfoContainer::RequestOperationProxy::process_impression_post_action(
    const AdServer::Commons::UserId&, // new_user_id
    const AdServer::Commons::RequestId& request_id,
    const RequestPostActionInfo& request_post_action_info)
    /*throw(RequestOperationProcessor::Exception)*/
  {
    RequestInfoContainer_var owner = lock_owner_();

    if(owner.in())
    {
      try
      {
        // already moved operation will disable second moving
        owner->process_impression_post_action_(
          request_id,
          request_post_action_info,
          false);
      }
      catch(const eh::Exception& ex)
      {
        throw RequestOperationProcessor::Exception(ex.what());
      }
    }
  }

  void
  RequestInfoContainer::RequestOperationProxy::change_request_user_id(
    const AdServer::Commons::UserId& new_user_id,
    const AdServer::Commons::RequestId& request_id,
    const Generics::ConstSmartMemBuf* request_profile)
    /*throw(RequestOperationProcessor::Exception)*/
  {
    RequestInfoContainer_var owner = lock_owner_();

    if(owner.in())
    {
      try
      {
        owner->change_request_user_id_(
          new_user_id,
          request_id,
          request_profile);
      }
      catch(const eh::Exception& ex)
      {
        throw RequestOperationProcessor::Exception(ex.what());
      }
    }
  }

  // RequestInfoContainer::Transaction
  RequestInfoContainer::Transaction::Transaction(
    BidProfileMap::Transaction* transaction,
    RequestInfoMap::Transaction* old_transaction)
    : transaction_(ReferenceCounting::add_ref(transaction)),
      old_transaction_(ReferenceCounting::add_ref(old_transaction))
  {}

  Generics::ConstSmartMemBuf_var
  RequestInfoContainer::Transaction::get_profile(
    Generics::Time* last_access_time)
  {
    Generics::ConstSmartMemBuf_var new_profile;

    if(transaction_)
    {
      new_profile = transaction_->get_profile(last_access_time);
    }

    if(new_profile)
    {
      return new_profile;
    }

    if(old_transaction_)
    {
      return old_transaction_->get_profile(last_access_time);
    }

    return Generics::ConstSmartMemBuf_var();
  }

  void
  RequestInfoContainer::Transaction::save_profile(
    const Generics::ConstSmartMemBuf* mem_buf,
    const Generics::Time& now)
  {
    if(transaction_)
    {
      transaction_->save_profile(mem_buf, now);
    }
    else if(old_transaction_)
    {
      old_transaction_->save_profile(mem_buf, now);
    }
    else
    {
      assert(false);
    }
  }

  /** RequestInfoContainer */
  RequestInfoContainer::RequestInfoContainer(
    Logging::Logger* logger,
    RequestActionProcessor* request_processor,
    RequestOperationProcessor* request_operation_processor,
    const char* requestfile_base_path,
    const char* requestfile_prefix,
    const String::SubString& bidfile_base_path,
    const String::SubString& /*bidfile_prefix*/,
    ProfilingCommons::ProfileMapFactory::Cache* cache,
    const Generics::Time& expire_time,
    const Generics::Time& extend_time_period)
    /*throw(Exception)*/
    : logger_(ReferenceCounting::add_ref(logger)),
      expire_time_(expire_time),
      proxy_(new ProxyImpl(this)),
      request_operation_proxy_(new RequestOperationProxy(this)),
      request_processor_(ReferenceCounting::add_ref(request_processor)),
      request_operation_processor_(ReferenceCounting::add_ref(request_operation_processor))
  {
    static const char* FUN = "RequestInfoContainer::RequestInfoContainer()";

    Generics::Time extend_time_period_val(extend_time_period);

    if(extend_time_period_val.tv_sec == 0)
    {
      extend_time_period_val = expire_time / 2;
    }

    try
    {
      request_map_ = ProfilingCommons::ProfileMapFactory::
        open_transaction_packed_expire_map<
          ProfilingCommons::RequestIdPackHashAdapter,
          ProfilingCommons::RequestIdAccessor,
          RequestProfileAdapter>(
          requestfile_base_path,
          requestfile_prefix,
          extend_time_period_val,
          cache);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't init RequestInfoMap. "
        "Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }

    if(!bidfile_base_path.empty())
    {
      try
      {
        ReferenceCounting::SmartPtr<
          AdServer::ProfilingCommons::RocksDBProfileMap<
          AdServer::Commons::RequestId, UuidToString> >
          bid_map_impl(
            new AdServer::ProfilingCommons::RocksDBProfileMap<
            AdServer::Commons::RequestId, UuidToString>(
              bidfile_base_path,
              expire_time_));

        bid_profile_map_ = new AdServer::ProfilingCommons::TransactionProfileMap<
          AdServer::Commons::RequestId>(bid_map_impl);
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": Can't init RequestInfoMap(rocksdb). "
          "Caught eh::Exception: " << ex.what();
        throw Exception(ostr);
      }
    }
  }

  RequestInfoContainer::~RequestInfoContainer() noexcept
  {
    proxy_->detach();
  }

  RequestContainerProcessor_var
  RequestInfoContainer::proxy() noexcept
  {
    return proxy_;
  }

  RequestOperationProcessor_var
  RequestInfoContainer::request_operation_proxy() noexcept
  {
    return request_operation_proxy_;
  }

  Generics::ConstSmartMemBuf_var
  RequestInfoContainer::get_profile(
    const AdServer::Commons::RequestId& request_id)
    /*throw(Exception)*/
  {
    static const char* FUN = "RequestInfoContainer::get_profile()";

    try
    {
      return get_profile_(request_id);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't get profile for id = " << request_id <<
        ". Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void RequestInfoContainer::process_request(
    const RequestInfo& request_info)
    /*throw(RequestContainerProcessor::Exception)*/
  {
    static const char* FUN = "RequestInfoContainer::process_request()";

    RequestProcessDelegate request_process_delegate;

    try
    {
      Transaction_var transaction =
        get_transaction_(request_info.request_id);
      Generics::ConstSmartMemBuf_var mem_buf =
        get_profile_(transaction);

      Generics::Time last_event_time;

      if(process_request_buf_(
           mem_buf,
           request_process_delegate,
           &last_event_time,
           request_info,
           request_operation_processor_))
      {
        save_profile_(
          transaction,
          mem_buf,
          std::max(request_info.time, last_event_time));
      }
    }
    catch(const eh::Exception& ex)
    {
      throw_request_processing_exception_(FUN, request_info.request_id, ex.what());
    }

    delegate_processing_(request_process_delegate);
  }

  void
  RequestInfoContainer::process_impression(
    const ImpressionInfo& impression_info)
    /*throw(RequestContainerProcessor::Exception)*/
  {
    process_impression_(
      impression_info,
      request_operation_processor_ // move enabled if processor present
      );
  }

  void
  RequestInfoContainer::process_action(
    ActionType action_type,
    const Generics::Time& time,
    const AdServer::Commons::RequestId& request_id)
    /*throw(RequestContainerProcessor::Exception)*/
  {
    process_action_(
      action_type,
      time,
      request_id,
      request_operation_processor_ // move enabled if processor present
      );
  }

  void
  RequestInfoContainer::process_custom_action(
    const AdServer::Commons::RequestId& request_id,
    const AdvCustomActionInfo& adv_custom_action_info)
    /*throw(RequestContainerProcessor::Exception)*/
  {
    static const char* FUN = "RequestInfoContainer::process_custom_action()";

    RequestProcessDelegate request_process_delegate;

    try
    {
      Generics::ConstSmartMemBuf_var mem_buf = get_profile_(request_id);

      process_custom_action_buf_(
        mem_buf,
        request_process_delegate,
        request_id,
        adv_custom_action_info);
    }
    catch(const eh::Exception& ex)
    {
      throw_request_processing_exception_(FUN, request_id, ex.what());
    }

    delegate_processing_(request_process_delegate);
  }

  void
  RequestInfoContainer::process_impression_post_action(
    const AdServer::Commons::RequestId& request_id,
    const RequestPostActionInfo& request_post_action_info)
    /*throw(RequestContainerProcessor::Exception)*/
  {
    process_impression_post_action_(
      request_id,
      request_post_action_info,
      request_operation_processor_ // move enabled if processor present
      );
  }

  void
  RequestInfoContainer::process_impression_(
    const ImpressionInfo& impression_info,
    bool move_enabled)
    /*throw(RequestContainerProcessor::Exception)*/
  {
    static const char* FUN = "RequestInfoContainer::process_impression_()";

    RequestProcessDelegate request_process_delegate;

    try
    {
      Transaction_var transaction =
        get_transaction_(impression_info.request_id);

      Generics::ConstSmartMemBuf_var mem_buf = get_profile_(transaction);

      Generics::Time last_event_time;

      if(impression_info.verify_impression)
      {
        if(process_impression_buf_(
             mem_buf,
             request_process_delegate,
             &last_event_time,
             impression_info,
             move_enabled))
        {
          save_profile_(
            transaction,
            mem_buf,
            std::max(impression_info.time, last_event_time));
        }
      }
      else
      {
        if(process_notice_buf_(
             mem_buf,
             request_process_delegate,
             &last_event_time,
             impression_info,
             move_enabled))
        {
          save_profile_(
            transaction,
            mem_buf,
            std::max(impression_info.time, last_event_time));
        }
      }
    }
    catch(const eh::Exception& ex)
    {
      throw_request_processing_exception_(FUN, impression_info.request_id, ex.what());
    }

    try
    {
    }
    catch(const eh::Exception& ex)
    {
      throw_request_processing_exception_(FUN, impression_info.request_id, ex.what());
    }

    delegate_processing_(request_process_delegate);
  }

  void
  RequestInfoContainer::process_action_(
    ActionType action_type,
    const Generics::Time& time,
    const AdServer::Commons::RequestId& request_id,
    bool move_enabled)
    /*throw(RequestContainerProcessor::Exception)*/
  {
    static const char* FUN = "RequestInfoContainer::process_action_()";

    RequestProcessDelegate request_process_delegate;

    try
    {
      Transaction_var transaction = get_transaction_(request_id);

      Generics::ConstSmartMemBuf_var mem_buf =
        get_profile_(transaction);

      bool save_profile = false;
      Generics::Time last_event_time;

      if(action_type == AT_CLICK)
      {
        save_profile = process_click_buf_(
          mem_buf,
          request_process_delegate,
          &last_event_time,
          request_id,
          time,
          move_enabled);
      }
      else if(action_type == AT_ACTION)
      {
        save_profile = process_action_buf_(
          mem_buf,
          request_process_delegate,
          &last_event_time,
          request_id,
          time,
          move_enabled);
      }
      else if(action_type == AT_FRAUD_ROLLBACK)
      {
        save_profile = process_fraud_rollback_buf_(
          mem_buf,
          request_process_delegate,
          &last_event_time,
          request_id,
          time);
      }

      if(save_profile)
      {
        save_profile_(transaction, mem_buf, std::max(time, last_event_time));
      }
    }
    catch(const eh::Exception& ex)
    {
      throw_request_processing_exception_(FUN, request_id, ex.what());
    }

    delegate_processing_(request_process_delegate);
  }

  void
  RequestInfoContainer::process_impression_post_action_(
    const AdServer::Commons::RequestId& request_id,
    const RequestPostActionInfo& request_post_action_info,
    bool move_enabled)
    /*throw(Exception)*/
  {
    static const char* FUN = "RequestInfoContainer::process_impression_post_action_()";

    RequestProcessDelegate request_process_delegate;
    bool save_profile = false;
    Generics::Time last_event_time;

    try
    {
      Transaction_var transaction = get_transaction_(request_id);
      Generics::ConstSmartMemBuf_var mem_buf =
        get_profile_(transaction);

      save_profile = process_impression_post_action_buf_(
        mem_buf,
        request_process_delegate,
        &last_event_time,
        request_id,
        request_post_action_info,
        move_enabled);

      if(save_profile)
      {
        save_profile_(
          transaction,
          mem_buf,
          std::max(request_post_action_info.time, last_event_time));
      }
    }
    catch(const eh::Exception& ex)
    {
      throw_request_processing_exception_(FUN, request_id, ex.what());
    }

    delegate_processing_(request_process_delegate);
  }

  void
  RequestInfoContainer::change_request_user_id_(
    const AdServer::Commons::UserId& /*user_id*/,
    const AdServer::Commons::RequestId& request_id,
    const Generics::ConstSmartMemBuf* external_request_profile)
    /*throw(Exception)*/
  {
    static const char* FUN = "RequestInfoContainer::change_request_user_id_()";

    RequestProcessDelegate request_process_delegate;

    try
    {
      RequestProfileAdapter request_profile_adapter;
      Generics::ConstSmartMemBuf_var request_profile =
        request_profile_adapter(
          external_request_profile);

      ImpressionInfo notice_info;

      Transaction_var transaction = get_transaction_(request_id);

      /*
      unsigned long notice_non_considered = 0;
      unsigned long impression_non_considered = 0;
      unsigned long click_non_considered = 0;
      */
      bool request_processed_on_sender = false;
      RequestInfoProfileWriter::post_impression_actions_Container post_impression_actions;

      {
        RequestInfoProfileReader new_request_reader(
          request_profile->membuf().data(),
          request_profile->membuf().size());

        RequestInfoProfileWriter request_writer(
          request_profile->membuf().data(),
          request_profile->membuf().size());

        request_processed_on_sender =
          new_request_reader.fraud() != RequestInfo::RS_NORMAL;

        // restart sub actions (notice/impression/click) processing by moving it to non considered state
        // processing will be activated in process_request_buf_
        request_writer.request_done() = 0;
        request_writer.fraud() = RequestInfo::RS_NORMAL;

        if(request_writer.notice_received())
        {
          request_writer.notice_non_considered() += request_writer.notice_received();
          request_writer.notice_received() = 0;

          convert_request_reader_to_impression_info(
            notice_info,
            new_request_reader,
            true,
            request_id);
        }

        if(request_writer.click_done())
        {
          request_writer.click_non_considered() += request_writer.click_done();
          request_writer.click_done() = 0;
        }

        post_impression_actions.swap(request_writer.post_impression_actions());

#       ifdef DEBUG_OUTPUT_
        std::cerr << "RequestInfoContainer::change_request_user_id_(): "
          "request_writer.click_non_considered = " << request_writer.click_non_considered() <<
          ", request_writer.impression_non_considered = " << request_writer.impression_non_considered() <<
          ", request_writer.notice_received = " << request_writer.notice_received() <<
          ", request_writer.notice_non_considered = " << request_writer.notice_non_considered() <<
          std::endl;
#       endif

        if(request_writer.actions_done())
        {
          request_writer.actions_non_considered() +=
            request_writer.actions_done();
          request_writer.actions_done() = 0;
        }

        // this block required for process case when
        // other operation already moved
        // and overwrite RS_MOVED (local moving) or
        // created stub (services exchange)
        Generics::ConstSmartMemBuf_var old_mem_buf = get_profile_(transaction);

#       ifdef DEBUG_OUTPUT_
        std::cerr << "RequestInfoContainer::change_request_user_id_(): "
          "new_request_reader.impression_non_considered() = " <<
          new_request_reader.impression_non_considered() << std::endl;
#       endif

        // move localy got and unprocessed actions (non considered) for process on git request buf
        if(old_mem_buf.in())
        {
          RequestInfoProfileReader old_request_reader(
            old_mem_buf->membuf().data(),
            old_mem_buf->membuf().size());

#         ifdef DEBUG_OUTPUT_
          std::cerr << "RequestInfoContainer::change_request_user_id_(): "
            "old_request_reader.impression_non_considered() = " <<
            old_request_reader.impression_non_considered() <<
            ", new_request_reader.impression_non_considered() = " <<
            new_request_reader.impression_non_considered() << std::endl;
#         endif

          if(old_request_reader.fraud() != RequestInfo::RS_MOVED)
          {
            request_writer.notice_non_considered() += old_request_reader.notice_non_considered();
            request_writer.impression_non_considered() += old_request_reader.impression_non_considered();
            request_writer.click_non_considered() += old_request_reader.click_non_considered();
            request_writer.actions_non_considered() += request_writer.actions_non_considered();
          }
        }

        save_request_writer(request_profile, request_writer);
      }

      RequestInfoProfileReader request_reader(
        request_profile->membuf().data(),
        request_profile->membuf().size());

      RequestInfo request_info;
      request_info.request_id = request_id;

      convert_request_reader_to_request_info(request_info, request_reader);

      /*
      ImpressionInfo impression_info;
      impression_info.request_id = request_id;

      convert_request_reader_to_impression_info(
        impression_info,
        request_reader,
        false,
        request_id);
      */

      Generics::ConstSmartMemBuf_var mem_buf = Generics::transfer_membuf(
        Algs::copy_membuf(request_profile));

      bool save_profile = process_request_buf_(
        mem_buf,
        request_process_delegate,
        0,
        request_info,
        false // second move impossible
        );

      if(request_processed_on_sender &&
        request_process_delegate.process_request.present())
      {
        request_process_delegate.process_request = RequestInfo::RS_RESAVE;
      }

      assert(mem_buf->membuf().size() >= 4 &&
        *reinterpret_cast<const uint32_t*>(
          mem_buf->membuf().data()) == CURRENT_REQUEST_PROFILE_VERSION);

      bool click_done = false;

      // reconstruct impression_non_considered for duplicate process
      /*
      if(impression_non_considered > 0 || click_non_considered > 0)
      {
        RequestInfoProfileWriter request_writer(mem_buf->membuf().data(), mem_buf->membuf().size());
        request_writer.impression_non_considered() = impression_non_considered > 0 ?
          impression_non_considered - 1 : 0;
        if(click_non_considered > 0)
        {
          request_writer.click_non_considered() = click_non_considered - 1;
          click_done = true;
        }
        save_request_writer(mem_buf, request_writer);
      }

      //std::cerr << "P1, click_non_considered = " << click_non_considered << std::endl;
      save_profile |= process_impression_buf_(
        mem_buf,
        request_process_delegate,
        0, // last_event_time
        impression_info,
        false);

      if(request_reader.notice_non_considered())
      {
        notice_info.request_id = request_id;

        save_profile |= process_notice_buf_(
          mem_buf,
          request_process_delegate,
          0, // last_event_time
          notice_info,
          false);
      }

      if(click_non_considered > 0)
      {
        save_profile |= process_click_buf_(
          mem_buf,
          request_process_delegate,
          0, // last_event_time
          request_id,
          Generics::Time(request_reader.click_time()),
          false);
      }

      if(!post_impression_actions.empty())
      {
        for(auto it = post_impression_actions.begin();
          it != post_impression_actions.end(); ++it)
        {
          save_profile |= process_impression_post_action_buf_(
            mem_buf,
            request_process_delegate,
            0, // last_event_time
            request_id,
            RequestPostActionInfo(
              Generics::Time(request_reader.time()),
              *it),
            false);
        }
      }

      for(unsigned long act_i = 0;
          act_i < request_reader.actions_non_considered(); ++act_i)
      {
        save_profile |= process_action_buf_(
          mem_buf,
          request_process_delegate,
          0, // last_event_time
          request_id,
          Generics::Time(request_reader.action_time()),
          false);
      }
      */

      if(save_profile)
      {
        save_profile_(
          transaction,
          mem_buf,
          Generics::Time(request_reader.time()));
      }
    }
    catch(const eh::Exception& ex)
    {
      throw_request_processing_exception_(FUN, request_id, ex.what());
    }

    delegate_processing_(request_process_delegate);
  }

  void
  RequestInfoContainer::clear_expired_requests()
    /*throw(Exception)*/
  {
    Generics::Time now = Generics::Time::get_time_of_day();
    request_map_->clear_expired(now - expire_time_);
  }

  void
  RequestInfoContainer::throw_request_processing_exception_(
    const char* fun,
    const AdServer::Commons::RequestId& request_id,
    const char* message)
    /*throw(Exception)*/
  {
    Stream::Error ostr;
    ostr << fun << ": Caught eh::Exception on processing request_id='" <<
      request_id << "': " << message;
    throw Exception(ostr);
  }

  void
  RequestInfoContainer::delegate_processing_(
    const RequestProcessDelegate& request_process_delegate)
    /*throw(Exception)*/
  {
    static const char* FUN = "RequestInfoContainer::delegate_processing_()";

    if(logger_->log_level() >= Logging::Logger::TRACE &&
       (request_process_delegate.request_info.present() ||
        request_process_delegate.rollback_request_info.present()) &&
       (request_process_delegate.process_request.present() ||
        request_process_delegate.process_impression ||
        request_process_delegate.process_click ||
        request_process_delegate.process_actions ||
        request_process_delegate.process_fraud_request.present() ||
        !request_process_delegate.process_rollback_impressions.empty() ||
        !request_process_delegate.process_rollback_clicks.empty()))
    {
      Stream::Error ostr;
      ostr << FUN << ": delegate processing for" <<
        (request_process_delegate.process_request.present() ? " request" : "") <<
        (request_process_delegate.process_impression ? " impression" : "") <<
        (request_process_delegate.process_click ? " click" : "") <<
        (request_process_delegate.process_actions ? " actions" : "") <<
        (request_process_delegate.process_fraud_request.present() ?
         (std::string(" ") + RequestInfo::request_state_string(*request_process_delegate.process_fraud_request)) :
         std::string())
        ;
      for(auto rollback_imp_it = request_process_delegate.process_rollback_impressions.begin();
        rollback_imp_it != request_process_delegate.process_rollback_impressions.end();
        ++rollback_imp_it)
      {
        ostr << " imp: " << RequestInfo::request_state_string(*rollback_imp_it);
      }

      for(auto rollback_click_it = request_process_delegate.process_rollback_clicks.begin();
        rollback_click_it != request_process_delegate.process_rollback_clicks.end();
        ++rollback_click_it)
      {
        ostr << " click: " << RequestInfo::request_state_string(*rollback_click_it);
      }

      if(request_process_delegate.request_info.present())
      {
        ostr << "Request Info:" << std::endl;
        request_process_delegate.request_info->print(ostr, "  ");
      }

      if(request_process_delegate.rollback_request_info.present())
      {
        ostr << "Rollback Request Info:" << std::endl;
        request_process_delegate.rollback_request_info->print(ostr, "  ");
      }

      logger_->log(
        ostr.str(),
        Logging::Logger::TRACE,
        Aspect::REQUEST_INFO_CONTAINER);
    }

    try
    {
      if(request_process_delegate.process_request.present())
      {
        assert(request_process_delegate.request_info.present());

        request_processor_->process_request(
          *request_process_delegate.request_info,
          RequestActionProcessor::ProcessingState(
            *request_process_delegate.process_request));
      }

      if(request_process_delegate.process_impression)
      {
        assert(request_process_delegate.request_info.present());

        request_processor_->process_impression(
          *request_process_delegate.request_info,
          *request_process_delegate.impression_info,
          RequestActionProcessor::ProcessingState(
            request_process_delegate.request_info->fraud));
      }

      if(request_process_delegate.process_click)
      {
        assert(request_process_delegate.request_info.present());

        request_processor_->process_click(
          *request_process_delegate.request_info,
          RequestActionProcessor::ProcessingState(
            request_process_delegate.request_info->fraud));
      }

      for(unsigned long i = 0; i < request_process_delegate.process_actions; ++i)
      {
        assert(request_process_delegate.request_info.present());

        request_processor_->process_action(
          *request_process_delegate.request_info);
      }

      if(request_process_delegate.process_fraud_request.present() ||
         !request_process_delegate.process_rollback_impressions.empty() ||
         !request_process_delegate.process_rollback_clicks.empty())
      {
        assert(!request_process_delegate.process_fraud_request.present() ||
            *request_process_delegate.process_fraud_request != RequestInfo::RS_MOVED ||
          (request_process_delegate.process_rollback_impressions.empty() &&
            request_process_delegate.process_rollback_clicks.empty()));

        RequestInfo fraud_request_info(
          request_process_delegate.rollback_request_info.present() ?
          *request_process_delegate.rollback_request_info :
          *request_process_delegate.request_info);

        if(request_process_delegate.process_fraud_request.present())
        {
          fraud_request_info.fraud = *request_process_delegate.process_fraud_request; // REVIEW

          request_processor_->process_request(
            fraud_request_info,
            RequestActionProcessor::ProcessingState(
              *request_process_delegate.process_fraud_request));
        }

        for(auto rollback_imp_it = request_process_delegate.process_rollback_impressions.begin();
          rollback_imp_it != request_process_delegate.process_rollback_impressions.end();
          ++rollback_imp_it)
        {
          request_processor_->process_impression(
            fraud_request_info,
            *request_process_delegate.impression_info,
            RequestActionProcessor::ProcessingState(*rollback_imp_it));
        }

        for(auto rollback_click_it = request_process_delegate.process_rollback_clicks.begin();
          rollback_click_it != request_process_delegate.process_rollback_clicks.end();
          ++rollback_click_it)
        {
          request_processor_->process_click(
            fraud_request_info,
            RequestActionProcessor::ProcessingState(*rollback_click_it));
        }
      }

      for(AdvCustomActionInfoList::const_iterator adv_action_it =
            request_process_delegate.custom_actions.begin();
          adv_action_it != request_process_delegate.custom_actions.end();
          ++adv_action_it)
      {
        assert(request_process_delegate.request_info.present());

        request_processor_->process_custom_action(
          *request_process_delegate.request_info,
          *adv_action_it);
      }

      for(auto request_post_act_it =
            request_process_delegate.process_post_impression_actions.begin();
        request_post_act_it != request_process_delegate.process_post_impression_actions.end();
        ++request_post_act_it)
      {
        assert(request_process_delegate.request_info.present());

        request_processor_->process_request_post_action(
          *request_process_delegate.request_info,
          *request_post_act_it);
      }

      // move operations
      if(request_process_delegate.move_request_profile.in())
      {
        assert(request_operation_processor_);

        request_operation_processor_->change_request_user_id(
          request_process_delegate.move_request_user_id,
          request_process_delegate.move_request_id,
          request_process_delegate.move_request_profile);
      }

      if(request_operation_processor_.in())
      {
        if(request_process_delegate.move_notice_info.present())
        {
          ImpressionInfo imp_info(*request_process_delegate.move_notice_info);
          assert(!imp_info.verify_impression); // notice
          imp_info.user_id = request_process_delegate.move_request_user_id;
          request_operation_processor_->process_impression(imp_info);
        }
        
        if(request_process_delegate.move_impression_info.present())
        {
          ImpressionInfo imp_info(*request_process_delegate.move_impression_info);
          imp_info.user_id = request_process_delegate.move_request_user_id;
          request_operation_processor_->process_impression(imp_info);
        }

        for(auto it = request_process_delegate.move_actions.begin();
            it != request_process_delegate.move_actions.end(); ++it)
        {
          request_operation_processor_->process_action(
            request_process_delegate.move_request_user_id,
            it->action_type,
            it->time,
            it->request_id);
        }

        for(auto it = request_process_delegate.move_impression_post_actions.begin();
          it != request_process_delegate.move_impression_post_actions.end();
          ++it)
        {
          request_operation_processor_->process_impression_post_action(
            request_process_delegate.move_request_user_id,
            it->request_id,
            *it);
        }
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  Generics::ConstSmartMemBuf_var
  RequestInfoContainer::get_profile_(
    const AdServer::Commons::RequestId& request_id)
  {
    Generics::ConstSmartMemBuf_var res;

    if(bid_profile_map_)
    {
      res = bid_profile_map_->get_profile(request_id);
    }

    if(!res && request_map_)
    {
      res = request_map_->get_profile(request_id);
    }

    if(res.in())
    {
      RequestProfileAdapter request_profile_adapter;
      return request_profile_adapter(res);
    }

    return res;
  }

  Generics::ConstSmartMemBuf_var
  RequestInfoContainer::get_profile_(
    Transaction* transaction)
  {
    Generics::ConstSmartMemBuf_var mem_buf = transaction->get_profile();
    RequestProfileAdapter request_profile_adapter;
    if(mem_buf)
    {
      mem_buf = request_profile_adapter(mem_buf);
    }

    assert(!mem_buf.in() || (
      mem_buf->membuf().size() >= 4 &&
      *reinterpret_cast<const uint32_t*>(
        mem_buf->membuf().data()) == CURRENT_REQUEST_PROFILE_VERSION));

    return mem_buf;
  }

  void
  RequestInfoContainer::save_profile_(
    Transaction* transaction,
    const Generics::ConstSmartMemBuf* mem_buf,
    const Generics::Time& time)
  {
    assert(
      mem_buf->membuf().size() >= 4 &&
      *reinterpret_cast<const uint32_t*>(
        mem_buf->membuf().data()) == CURRENT_REQUEST_PROFILE_VERSION);
    transaction->save_profile(mem_buf, time);
  }

  RequestInfoContainer::Transaction_var
  RequestInfoContainer::get_transaction_(
    const AdServer::Commons::RequestId& request_id)
  {
    BidProfileMap::Transaction_var new_transaction;

    if(bid_profile_map_)
    {
      new_transaction = bid_profile_map_->get_transaction(request_id);
    }

    RequestInfoMap::Transaction_var old_transaction;

    if(request_map_)
    {
      old_transaction = request_map_->get_transaction(request_id);
    }

    return new Transaction(new_transaction, old_transaction);
  }

  bool
  RequestInfoContainer::process_request_buf_(
    Generics::ConstSmartMemBuf_var& mem_buf,
    RequestProcessDelegate& request_process_delegate,
    Generics::Time* last_event,
    const RequestInfo& request_info,
    bool move_enabled)
    /*throw(Exception)*/
  {
    static const char* FUN = "RequestInfoContainer::process_request_buf_()";

    bool save_profile = false;

    bool delegate_process_notice = false;
    bool notice_done = false;
    ImpressionInfo notice_info;

    //bool impression_done = false;
    //ImpressionInfo impression_info;

    //bool move_request = false;

#   ifdef DEBUG_OUTPUT_
    std::cerr << "RequestInfoContainer::process_request_buf_(" <<
      request_info.request_id << "): this = " << this <<
      ", mem_buf = " << (mem_buf.in () ? mem_buf->membuf().data() : nullptr) <<
      ", mem_buf.size = " << (mem_buf.in () ? mem_buf->membuf().size() : 0) <<
      std::endl;
#   endif

    try
    {
      RequestInfoProfileWriter request_writer;
      bool process_request = false;
      //AdServer::Commons::UserId move_request_user_id;

      if(!mem_buf.in())
      {
        // simple fill profile by request info
        clear_action_fields(request_writer);
        request_writer.version() = CURRENT_REQUEST_PROFILE_VERSION;
        process_request = true;
      }
      else
      {
        // exist stub - check that notice done but not considered
        RequestInfoProfileReader request_reader(
          mem_buf->membuf().data(),
          mem_buf->membuf().size());

#       ifdef DEBUG_OUTPUT_
        std::cerr << "RequestInfoContainer::process_request_buf_(" <<
          request_info.request_id << "): exists stub, this = " << this <<
          ", request_reader.impression_non_considered() = " << request_reader.impression_non_considered() <<
          ", request_reader.click_non_considered() = " << request_reader.click_non_considered() <<
          ", mem_buf = " << (mem_buf.in () ? mem_buf->membuf().data() : nullptr) <<
          std::endl;
#       endif

        if (last_event)
        {
          *last_event = last_event_time(request_reader);
        }

        if(request_reader.request_done() == 0 &&
          request_reader.fraud() != RequestInfo::RS_MOVED)
        {
          request_writer.init(mem_buf->membuf().data(), mem_buf->membuf().size());

          /*
          AdServer::Commons::UserId request_reader_user_id;
          if(request_reader.user_id()[0])
          {
            request_reader_user_id = AdServer::Commons::UserId(request_reader.user_id());
          }

          if(move_enabled &&
             !request_reader_user_id.is_null() &&
             !(request_info.user_id ==
               AdServer::Commons::UserId(request_reader.user_id())))
          {
            move_request_user_id = AdServer::Commons::UserId(
              request_reader.user_id());
            move_request = true;
          }
          */

          if(request_reader.notice_non_considered() != 0)
          {
            // notice already done - read cost values
            notice_done = true;
            convert_request_reader_to_impression_info(
              notice_info,
              request_reader,
              true,
              request_info.request_id);

            delegate_process_notice = true;
            request_writer.notice_non_considered() = 0;

            /*
            if(!move_request)
            {
              delegate_process_notice = true;
              request_writer.notice_non_considered() = 0;
            }
            */
          }

          /*
          if(request_reader.impression_non_considered() != 0)
          {
            // impression already done - read cost values
            impression_done = true;

            convert_request_reader_to_impression_info( // REVIEW
              impression_info,
              request_reader,
              false,
              request_info.request_id);
          }
          */

          process_request = true;
        }
      }

      if(process_request)
      {
        RequestInfo delegate_request_info = request_info;
        convert_request_info_to_request_writer(request_info, request_writer);

        //if(!move_request && !(request_writer.enabled_notice() & ENABLED_NOTICE))
        if(!(request_writer.enabled_notice() & ENABLED_NOTICE))
        {
          delegate_process_notice = true;
          notice_info.time = Generics::Time(request_writer.time());
        }

        // apply cost changes received on notice or impression
        /*
        if(notice_done)
        {
          convert_impression_info_to_request_writer(
            notice_info,
            request_writer,
            &delegate_request_info,
            true);
        }
        */

        /*
        // apply impression cost changes here, we need apply reconstructed impression_info to request buf
        // and impression_info can't be delegated here
        if(impression_done)
        {
          convert_impression_info_to_request_writer(
            impression_info,
            request_writer,
            &delegate_request_info,
            false);

          if(move_request)
          {
            impression_done = false;
          }
          //else
          //{
          //  assert(request_writer.impression_non_considered() > 0);
          //  request_writer.impression_non_considered() -= 1;
          //}

#         ifdef DEBUG_OUTPUT_
          std::cerr << "RequestInfoContainer::process_request_buf_(" <<
            request_info.request_id << "): this = " << this <<
            ", impression_done = " << impression_done <<
            ", move_request = " << move_request <<
            ", request_writer.impression_non_considered() = " << request_writer.impression_non_considered() <<
            std::endl;
#         endif
        }
        */

        /*
        if(move_request)
        {
          Generics::ConstSmartMemBuf_var move_mem_buf;
          request_writer.user_id() = move_request_user_id.to_string();
          // RS_NORMAL in move_mem_buf used for indicate that request not processed localy
          request_writer.fraud() = RequestInfo::RS_NORMAL;
          save_request_writer(move_mem_buf, request_writer);
          request_process_delegate.move_request_user_id = move_request_user_id;
          request_process_delegate.move_request_profile = move_mem_buf;

          request_writer.request_done() = 1;
          request_writer.notice_pub_revenue_type() = AdServer::CampaignSvcs::RT_NONE;
          request_writer.impression_pub_revenue_type() = AdServer::CampaignSvcs::RT_NONE;
          // mark local profile as moved
          request_writer.fraud() = RequestInfo::RS_MOVED;
        }
        else
        */
        {
          request_process_delegate.request_info = delegate_request_info;
          request_process_delegate.process_request = RequestInfo::RS_NORMAL;
        }

        save_request_writer(mem_buf, request_writer);
        save_profile = true;
      }

#     ifdef DEBUG_OUTPUT_
      std::cerr << "RequestInfoContainer::process_request_buf_(" <<
        request_info.request_id << "): finish, this = " << this <<
        ", request_writer.impression_non_considered() = " << request_writer.impression_non_considered() <<
        ", request_writer.click_non_considered() = " << request_writer.click_non_considered() <<
        ", mem_buf = " << (mem_buf.in () ? mem_buf->membuf().data() : nullptr) <<
        std::endl;
#     endif
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }

    if(delegate_process_notice)
    {
      notice_info.request_id = request_info.request_id;

      save_profile |= process_notice_buf_(
        mem_buf,
        request_process_delegate,
        0, // last_event
        notice_info,
        move_enabled);
    }

    /*
    assert(
      !request_process_delegate.process_impression || (
        request_process_delegate.process_impression && !move_request));
    */
    return save_profile;
  }

  bool
  RequestInfoContainer::process_notice_buf_(
    Generics::ConstSmartMemBuf_var& mem_buf,
    RequestProcessDelegate& request_process_delegate,
    Generics::Time* last_event,
    const ImpressionInfo& notice_info,
    bool move_enabled)
    /*throw(Exception)*/
  {
    static const char* FUN = "RequestInfoContainer::process_notice_buf_()";

    bool save_profile = false;
    bool delegate_process_impression = false;
    ImpressionInfo impression_info;

#   ifdef DEBUG_OUTPUT_
    std::cerr << "RequestInfoContainer::process_notice_buf_(" <<
      notice_info.request_id << "): this = " << this << std::endl;
#   endif

    try
    {
      RequestInfoProfileWriter request_writer;
      bool init_profile = true;

      if(mem_buf.in())
      {
        RequestInfoProfileReader request_reader(
          mem_buf->membuf().data(), mem_buf->membuf().size());

        if (last_event)
        {
          *last_event = last_event_time(request_reader);
        }

#       ifdef DEBUG_OUTPUT_
        std::cerr << "RequestInfoContainer::process_notice_buf_(" <<
          impression_info.request_id << "), start : this = " << this <<
          ", rs_moved = " << (request_reader.fraud() == RequestInfo::RS_MOVED ? 1 : 0) <<
          ", request_done = " << request_reader.request_done() <<
          std::endl;
#       endif

        if(request_reader.fraud() == RequestInfo::RS_MOVED)
        {
          if(move_enabled)
          {
            init_profile = false;

            request_process_delegate.move_request_user_id =
              AdServer::Commons::UserId(request_reader.user_id());
            request_process_delegate.move_notice_info = notice_info;
          }
          // if move disabled (operation already moved over moved profile)
          // skip old profile
        }
        else
        {
          init_profile = false;

          request_writer.init(
            mem_buf->membuf().data(), mem_buf->membuf().size());

#         ifdef DEBUG_OUTPUT_
          std::cerr << "RequestInfoContainer::process_notice_buf_(" <<
            impression_info.request_id << "), start #2 : this = " << this <<
            ", rs_moved = " << (request_writer.fraud() == RequestInfo::RS_MOVED ? 1 : 0) <<
            ", request_done = " << request_writer.request_done() <<
            ", request_writer.notice_non_considered = " << request_writer.notice_non_considered() <<
            std::endl;
#         endif

          if(request_reader.request_done())
          {
            if(!request_reader.notice_received())
            {
              if(!request_process_delegate.request_info.present())
              {
                convert_request_reader_to_request_info(
                  request_process_delegate.request_info.fill(),
                  request_reader);

                request_process_delegate.request_info->request_id =
                  notice_info.request_id;
              }

              if(request_reader.impression_non_considered() > 0)
              {
                delegate_process_impression = true;

                convert_request_reader_to_impression_info(
                  impression_info,
                  request_reader,
                  false,
                  notice_info.request_id);

                request_writer.impression_non_considered() -= 1;

#               ifdef DEBUG_OUTPUT_
                std::cerr << "RequestInfoContainer::process_notice_buf_(" <<
                  impression_info.request_id << "), imp: this = " << this <<
                  ", request_writer.impression_non_considered = " <<
                  request_writer.impression_non_considered() << std::endl;
#               endif
              }
              else if(request_reader.enabled_impression_tracking() == 0)
              {
                delegate_process_impression = true;

                fill_auto_impression_info(impression_info, request_reader);
              }

              convert_impression_info_to_request_writer(
                notice_info,
                request_writer,
                &*request_process_delegate.request_info,
                true);

              request_writer.notice_received() = 1;

              save_profile = true;
            }
          }
          else // !request_reader.request_done()
          {
            convert_impression_info_to_request_writer(
              notice_info,
              request_writer,
              0, // request_info
              true);

            request_writer.notice_non_considered() = 1;
            save_profile = true;
          }

#         ifdef DEBUG_OUTPUT_
          std::cerr << "RequestInfoContainer::process_notice_buf_(" <<
            impression_info.request_id << "), finish #2 : this = " << this <<
            ", rs_moved = " << (request_writer.fraud() == RequestInfo::RS_MOVED ? 1 : 0) <<
            ", request_done = " << request_writer.request_done() <<
            ", request_writer.notice_received = " << request_writer.notice_received() <<
            ", request_writer.notice_non_considered = " << request_writer.notice_non_considered() <<
            std::endl;
#         endif
        } // request_reader.fraud() != RS_MOVED
      }

      if(init_profile) // profile don't exists or must be skipped
      {
        create_empty_stub(request_writer);

        convert_impression_info_to_request_writer(
          notice_info,
          request_writer,
          0, // request_info
          true);

        request_writer.notice_non_considered() = 1;
        save_profile = true;
      }

      if(save_profile)
      {
        save_request_writer(mem_buf, request_writer);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }

    if(delegate_process_impression)
    {
      impression_info.request_id = notice_info.request_id;

      save_profile |= process_impression_buf_(
        mem_buf,
        request_process_delegate,
        0, // last_event
        impression_info,
        move_enabled);
    }

    return save_profile;
  }

  bool
  RequestInfoContainer::process_impression_buf_(
    Generics::ConstSmartMemBuf_var& mem_buf,
    RequestProcessDelegate& request_process_delegate,
    Generics::Time* last_event,
    const ImpressionInfo& impression_info,
    bool move_enabled)
    /*throw(Exception)*/
  {
    static const char* FUN = "RequestInfoContainer::process_impression_buf_()";

    bool save_profile = false;
    bool delegate_process_click = false;
    unsigned long delegate_duplicate_impressions = 0;
    RequestPostActionInfoList delegate_process_post_impression_actions;
    bool move_request = false;

    assert(!impression_info.request_id.is_null());

#   ifdef DEBUG_OUTPUT_
    std::cerr << "RequestInfoContainer::process_impression_buf_(" <<
      impression_info.request_id << "): this = " << this <<
      ", membuf = " << mem_buf.in() << std::endl;
#   endif

    try
    {
      RequestInfoProfileWriter request_writer;

      if(mem_buf.in())
      {
        RequestInfoProfileReader request_reader(
          mem_buf->membuf().data(), mem_buf->membuf().size());

#       ifdef DEBUG_OUTPUT_
        std::cerr << "RequestInfoContainer::process_impression_buf_(" <<
          impression_info.request_id << "): this = " << this <<
          ", request_reader.impression_non_considered = " << request_reader.impression_non_considered() <<
          ", request_reader.impression_verified = " << request_reader.impression_verified() <<
          ", request_reader.click_non_considered = " << request_reader.click_non_considered() <<
          ", request_reader.notice_received = " << request_reader.notice_received() <<
          ", rs_moved = " << (request_reader.fraud() != RequestInfo::RS_MOVED ? 1 : 0) <<
          ", request_reader.user_id = " << request_reader.user_id() <<
          ", impression_info.user_id = " << impression_info.user_id <<
          std::endl;
#       endif

        if (last_event)
        {
          *last_event = last_event_time(request_reader);
        }

        if(request_reader.fraud() != RequestInfo::RS_MOVED &&
          !request_reader.impression_verified())
        {
#         ifdef DEBUG_OUTPUT_
          std::cerr << "RequestInfoContainer::process_impression_buf_(" <<
            impression_info.request_id << "): this = " << this <<
            ", point #1, notice_received = " << request_reader.notice_received() <<
            std::endl;
#         endif
          request_writer.init(
            mem_buf->membuf().data(), mem_buf->membuf().size());

          if(move_enabled &&
             request_reader.request_done() &&
             !impression_info.user_id.is_null() &&
             !(impression_info.user_id ==
               AdServer::Commons::UserId(request_reader.user_id())))
          {
            /*
            // this event can't be part of chain
            // we can't process user_id change if process_impression_buf_
            // called from other processing method,
            // because user_id is equal for this case
            assert(!request_process_delegate.request_info.present() &&
              !request_process_delegate.process_request.present() &&
              !request_process_delegate.process_impression &&
              !request_process_delegate.process_click &&
              request_process_delegate.process_actions == 0 &&
              !request_process_delegate.process_fraud_request.present() &&
              request_process_delegate.process_rollback_impressions.empty() &&
              request_process_delegate.process_rollback_clicks.empty());
            */

            request_process_delegate.request_info->request_id =
              impression_info.request_id;
            request_process_delegate.request_info->fraud =
              RequestInfo::RS_RESAVE;

            // change profile
            request_writer.fraud() = RequestInfo::RS_MOVED;
            request_writer.user_id() = impression_info.user_id.to_string();
            convert_impression_info_to_request_writer(
              impression_info,
              request_writer,
              0,
              false);
            request_writer.request_done() = 0;
            /*
            // disable notice/impression_pub_revenue_type - it already applied
            request_writer.notice_pub_revenue_type() = AdServer::CampaignSvcs::RT_NONE;
            request_writer.impression_pub_revenue_type() = AdServer::CampaignSvcs::RT_NONE;
            */
            save_profile = true;

            // fill delegate operations
            convert_request_reader_to_request_info(
              request_process_delegate.request_info.fill(),
              request_reader);
            request_process_delegate.request_info->request_id = impression_info.request_id;
            request_process_delegate.request_info->fraud = RequestInfo::RS_RESAVE;
            // rollback previously processed request
            request_process_delegate.process_fraud_request = RequestInfo::RS_MOVED;
            move_request = true;

            request_process_delegate.move_request_user_id = impression_info.user_id;
            request_process_delegate.move_impression_info = impression_info;
          }
          else if(request_reader.notice_received())
          {
            // fill delegates
            if(!request_process_delegate.request_info.present())
            {
              convert_request_reader_to_request_info(
                request_process_delegate.request_info.fill(),
                request_reader);

              request_process_delegate.request_info->request_id =
                impression_info.request_id;
            }

            request_process_delegate.request_info->imp_time = impression_info.time;
            request_process_delegate.process_impression = true;
            delegate_process_click = request_reader.click_non_considered() > 0;
            delegate_duplicate_impressions = request_writer.impression_non_considered();

            for(auto act_it = request_writer.post_impression_actions().begin();
              act_it != request_writer.post_impression_actions().end(); ++act_it)
            {
              delegate_process_post_impression_actions.push_back(
                RequestPostActionInfo(
                  impression_info.time,
                  *act_it));
            }

            request_writer.post_impression_actions().clear();

            if(request_reader.click_non_considered() > 0)
            {
              request_writer.click_non_considered() -= 1;
            }

            // fill profile
            if(request_writer.impression_non_considered() == 0)
            {
              convert_impression_info_to_request_writer(
                impression_info,
                request_writer,
                &*request_process_delegate.request_info,
                false);
            }

            eval_revenues_on_impression_fin(
              request_writer,
              &*request_process_delegate.request_info);

            // eval costs
            request_writer.impression_verified() = 1;
            request_writer.impression_non_considered() = 0;
            save_profile = true;
          }
          else // !request_reader.notice_received()
          {
            if(request_writer.impression_non_considered() == 0)
            {
              convert_impression_info_to_request_writer(
                impression_info,
                request_writer,
                0, // request_info
                false);
            }

            request_writer.impression_non_considered() += 1;
            save_profile = true;
          }
        }
        else
          // double impression:
          //   RS_MOVED ||
          //   request_reader.impression_verified() ||
          //   request_writer.impression_non_considered() > 0
        {
          bool duplicate_processed = false;

          if(request_reader.impression_verified() || request_reader.fraud() == RequestInfo::RS_MOVED)
            // impression already confirmed and changed viewability
            // greatest viewability override previously saved
          {
            // this event can't be part of chain
            // we can't process viewablity change if process_impression_buf_
            // called from other processing method,
            // because viewability is equal for this case
            assert(!request_process_delegate.process_request.present() &&
              !request_process_delegate.process_impression &&
              !request_process_delegate.process_click &&
              request_process_delegate.process_actions == 0 &&
              !request_process_delegate.process_fraud_request.present() &&
              request_process_delegate.process_rollback_impressions.empty() &&
              request_process_delegate.process_rollback_clicks.empty());

            if(request_reader.fraud() == RequestInfo::RS_MOVED) // for RS_MOVED impression already done
            {
              if(move_enabled)
              {
                // move impression with viewability
                request_process_delegate.move_request_user_id =
                  AdServer::Commons::UserId(request_reader.user_id());
                request_process_delegate.move_impression_info = impression_info;
                duplicate_processed = true;
              }
            }
          }

          if(!duplicate_processed)
          {
            if(request_reader.impression_verified() || request_reader.fraud() == RequestInfo::RS_MOVED)
            {
              // if duplicate not processed (rollbacked or something else)
              // process as trivial duplicate
              delegate_duplicate_impressions = 1;
            }
            else // increase impression non considered
            {
              request_writer.init(
                mem_buf->membuf().data(), mem_buf->membuf().size());
              request_writer.impression_non_considered() += 1;
              save_profile = true;
            }
          } // !duplicate_processed
        }
      }
      else // profile don't exists
      {
        create_empty_stub(request_writer);

        convert_impression_info_to_request_writer(
          impression_info,
          request_writer,
          0,
          false);

        request_writer.impression_non_considered() = 1;
        save_profile = true;
      }

      if(save_profile)
      {
        save_request_writer(mem_buf, request_writer);
      }

#     ifdef DEBUG_OUTPUT_
      std::cerr << "RequestInfoContainer::process_impression_buf_(" <<
        impression_info.request_id << "): finish, this = " << this <<
        ", request_writer.version() = " << request_writer.version() <<
        ", request_writer.impression_verified() = " << request_writer.impression_verified() <<
        ", request_writer.impression_non_considered() = " << request_writer.impression_non_considered() <<
        ", request_writer.notice_received() = " << request_writer.notice_received() <<
        ", request_writer.click_non_considered() = " << request_writer.click_non_considered() <<
        ", mem_buf = " << (mem_buf.in () ? mem_buf->membuf().data() : nullptr) <<
        ", mem_buf.size = " << (mem_buf.in () ? mem_buf->membuf().size() : 0) <<
        std::endl;
#     endif

      if(move_request)
      {
        request_process_delegate.move_request_id = impression_info.request_id;
        request_process_delegate.move_request_profile = mem_buf;
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }

    // process duplicate impressions
    if(delegate_duplicate_impressions > 0)
    {
      if(!request_process_delegate.rollback_request_info.present())
      {
        // use final profile state
        RequestInfoProfileReader request_reader(
          mem_buf->membuf().data(), mem_buf->membuf().size());
        convert_request_reader_to_request_info(
          request_process_delegate.rollback_request_info.fill(),
          request_reader);
        request_process_delegate.rollback_request_info->request_id =
          impression_info.request_id;
      }

      for(unsigned long i = 0; i < delegate_duplicate_impressions; ++i)
      {
        request_process_delegate.process_rollback_impressions.push_back(
          RequestInfo::RS_DUPLICATE);
      }
    }

    // process impression
    if(request_process_delegate.process_impression)
    {
      request_process_delegate.impression_info = impression_info;
    }

    // process click
    if(delegate_process_click)
    {
      save_profile |= process_click_buf_(
        mem_buf,
        request_process_delegate,
        0, // last event
        impression_info.request_id,
        impression_info.time,
        move_enabled);
    }

    if(!delegate_process_post_impression_actions.empty())
    {
      for(auto it = delegate_process_post_impression_actions.begin();
        it != delegate_process_post_impression_actions.end(); ++it)
      {
        save_profile |= process_impression_post_action_buf_(
          mem_buf,
          request_process_delegate,
          0, // last event
          impression_info.request_id,
          *it,
          move_enabled);
      }
    }

    return save_profile;
  }

  bool
  RequestInfoContainer::process_click_buf_(
    Generics::ConstSmartMemBuf_var& mem_buf,
    RequestProcessDelegate& request_process_delegate,
    Generics::Time* last_event,
    const AdServer::Commons::RequestId& request_id,
    const Generics::Time& click_time,
    bool move_enabled)
    /*throw(Exception)*/
  {
    static const char* FUN = "RequestInfoContainer::process_click_buf_()";

    bool save_profile = false;
    unsigned long delegate_duplicate_clicks = 0;
    unsigned long delegate_process_action = 0;

#   ifdef DEBUG_OUTPUT_
    std::cerr << "RequestInfoContainer::process_click_buf_(" <<
      request_id << ")" << std::endl;
#   endif

    try
    {
      bool click_non_consider = false;
      RequestInfoProfileWriter request_writer;
      bool init_profile = true;

      if(mem_buf.in())
      {
        RequestInfoProfileReader request_reader(
          mem_buf->membuf().data(), mem_buf->membuf().size());

        if (last_event)
        {
          *last_event = last_event_time(request_reader);
        }

        if(request_reader.fraud() == RequestInfo::RS_MOVED)
        {
          if(move_enabled)
          {
            init_profile = false;
            request_process_delegate.move_request_user_id =
              AdServer::Commons::UserId(request_reader.user_id());
            request_process_delegate.move_actions.push_back(
              MoveActionInfo(AT_CLICK, click_time, request_id));
          }
          // if move disabled (operation already moved over moved profile)
          // skip old profile
        }
        else
        {
          init_profile = false;

          request_writer.init(
            mem_buf->membuf().data(), mem_buf->membuf().size());

          if(request_reader.impression_verified())
          {
            if(!request_process_delegate.request_info.present())
            {
              convert_request_reader_to_request_info(
                request_process_delegate.request_info.fill(),
                request_reader);

              request_process_delegate.request_info->request_id = request_id;
            }

            request_process_delegate.request_info->click_time = click_time;

            if(!request_reader.click_done())
            {
              request_process_delegate.process_click = true;

              if(request_reader.fraud() == RequestInfo::RS_FRAUD)
              {
                request_process_delegate.process_rollback_clicks.push_back(RequestInfo::RS_FRAUD);
              }
              delegate_duplicate_clicks = request_writer.click_non_considered();
              delegate_process_action = request_reader.actions_non_considered();

              request_writer.click_time() = click_time.tv_sec;
              request_writer.click_done() = 1;

              request_writer.click_non_considered() = 0;
              request_writer.actions_non_considered() = 0;

              save_profile = true;
            }
            else // double click
            {
              delegate_duplicate_clicks = 1;
            }
          }
          else
          {
            click_non_consider = true;
          }
        } // request_reader.fraud() != RS_MOVED
      }

      if(init_profile) // profile don't exists or must be skipped
      {
        create_empty_stub(request_writer);
        click_non_consider = true;
      }

      if(click_non_consider)
      {
        // create click stub - if impression will be done process it
        request_writer.click_non_considered() += 1;
        save_profile = true;
      }

      if(save_profile)
      {
        request_writer.click_time() = click_time.tv_sec;
        save_request_writer(mem_buf, request_writer);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }

    // process duplicate clicks
    if(delegate_duplicate_clicks > 0)
    {
      if(!request_process_delegate.rollback_request_info.present())
      {
        // use final profile state
        RequestInfoProfileReader request_reader(
          mem_buf->membuf().data(), mem_buf->membuf().size());
        convert_request_reader_to_request_info(
          request_process_delegate.rollback_request_info.fill(),
          request_reader);
        request_process_delegate.rollback_request_info->request_id = request_id;
      }

      for(unsigned long i = 0; i < delegate_duplicate_clicks; ++i)
      {
        request_process_delegate.process_rollback_clicks.push_back(
          RequestInfo::RS_DUPLICATE);
      }
    }

    assert(delegate_process_action == 0 ||
      request_process_delegate.request_info.present());

    for(unsigned long i = 0; i < delegate_process_action; ++i)
    {
      save_profile |= process_action_buf_(
        mem_buf,
        request_process_delegate,
        0, // last_event
        request_id,
        request_process_delegate.request_info->action_time,
        move_enabled);
    }

    return save_profile;
  }

  bool
  RequestInfoContainer::process_action_buf_(
    Generics::ConstSmartMemBuf_var& mem_buf,
    RequestProcessDelegate& request_process_delegate,
    Generics::Time* last_event,
    const AdServer::Commons::RequestId& request_id,
    const Generics::Time& action_time,
    bool move_enabled)
    /*throw(Exception)*/
  {
    static const char* FUN = "RequestInfoContainer::process_action_buf_()";

    bool save_profile = true;

    try
    {
      bool action_non_consider = false;
      RequestInfoProfileWriter request_writer;
      bool init_profile = true;

      if(mem_buf.in())
      {
        RequestInfoProfileReader request_reader(
          mem_buf->membuf().data(), mem_buf->membuf().size());

        if (last_event)
        {
          *last_event = last_event_time(request_reader);
        }

        if(request_reader.fraud() == RequestInfo::RS_MOVED)
        {
          if(move_enabled)
          {
            init_profile = false;
            save_profile = false;

            request_process_delegate.move_request_user_id =
              AdServer::Commons::UserId(request_reader.user_id());
            request_process_delegate.move_actions.push_back(
              MoveActionInfo(AT_ACTION, action_time, request_id));
          }
          // if move disabled (operation already moved over moved profile)
          // skip old profile
        }
        else
        {
          init_profile = false;

          request_writer.init(
            mem_buf->membuf().data(), mem_buf->membuf().size());

          if(request_reader.click_done())
          {
            if(!request_process_delegate.request_info.present())
            {
              convert_request_reader_to_request_info(
                request_process_delegate.request_info.fill(),
                request_reader);

              request_process_delegate.request_info->request_id =
                request_id;
            }

            if(request_reader.enabled_action_tracking())
            {
              request_process_delegate.process_actions += 1;
              ++request_writer.actions_done();
            }
          }
          else
          {
            action_non_consider = true;
          }
        } // request_reader.fraud() != RS_MOVED
      }

      if(init_profile) // profile don't exists or must be skipped
      {
        // create action stub - if request and impression will be received log it
        create_empty_stub(request_writer);
        action_non_consider = true;
      }

      if(save_profile)
      {
        if(action_non_consider)
        {
          ++request_writer.actions_non_considered();
        }

        request_writer.action_time() = action_time.tv_sec;

        save_request_writer(mem_buf, request_writer);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }

    return save_profile;
  }

  void
  RequestInfoContainer::process_custom_action_buf_(
    const Generics::ConstSmartMemBuf* mem_buf,
    RequestProcessDelegate& request_process_delegate,
    const AdServer::Commons::RequestId& request_id,
    const AdvCustomActionInfo& adv_custom_action_info)
    /*throw(Exception)*/
  {
    static const char* FUN = "RequestInfoContainer::process_custom_action_buf_()";

    try
    {
      if(mem_buf)
      {
        RequestInfoProfileReader request_reader(
          mem_buf->membuf().data(), mem_buf->membuf().size());

        if(request_reader.request_done())
        {
          if(!request_process_delegate.request_info.present())
          {
            convert_request_reader_to_request_info(
              request_process_delegate.request_info.fill(),
              request_reader);

            request_process_delegate.request_info->request_id =
              request_id;
          }

          request_process_delegate.custom_actions.push_back(
            adv_custom_action_info);
        }
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  bool
  RequestInfoContainer::process_impression_post_action_buf_(
    Generics::ConstSmartMemBuf_var& mem_buf,
    RequestProcessDelegate& request_process_delegate,
    Generics::Time* last_event,
    const AdServer::Commons::RequestId& request_id,
    const RequestPostActionInfo& request_post_action_info,
    bool move_enabled)
    /*throw(Exception)*/
  {
    static const char* FUN = "RequestInfoContainer::process_impression_post_action_buf_()";

    bool save_profile = true;

    try
    {
      RequestInfoProfileWriter request_writer;
      bool init_profile = true;

      if(mem_buf.in())
      {
        RequestInfoProfileReader request_reader(
          mem_buf->membuf().data(), mem_buf->membuf().size());

        if(last_event)
        {
          *last_event = last_event_time(request_reader);
        }

        if(request_reader.fraud() == RequestInfo::RS_MOVED)
        {
          if(move_enabled)
          {
            init_profile = false;
            save_profile = false;
            request_process_delegate.move_request_user_id =
              AdServer::Commons::UserId(request_reader.user_id());
            request_process_delegate.move_impression_post_actions.push_back(
              MoveRequestPostActionInfo(request_id, request_post_action_info));
          }
          // if move disabled (operation already moved over moved profile)
          // skip old profile
        }
        else
        {
          init_profile = false;

          request_writer.init(
            mem_buf->membuf().data(), mem_buf->membuf().size());

          auto ins_it = std::lower_bound(
            request_writer.post_impression_actions().begin(),
            request_writer.post_impression_actions().end(),
            request_post_action_info.action_name);

          bool ignore_action = (
            ins_it != request_writer.post_impression_actions().end() &&
            *ins_it == request_post_action_info.action_name);

          if(!ignore_action)
          {
            save_profile = true;

            request_writer.post_impression_actions().insert(
              ins_it,
              request_post_action_info.action_name);

            if(request_reader.impression_verified())
            {
              if(!request_process_delegate.request_info.present())
              {
                convert_request_reader_to_request_info(
                  request_process_delegate.request_info.fill(),
                  request_reader);

                request_process_delegate.request_info->request_id =
                  request_id;
              }

              request_process_delegate.process_post_impression_actions.push_back(
                request_post_action_info);
            }
          }
          else
          {
            save_profile = false;
          }
        } // request_reader.fraud() != RS_MOVED
      }

      if(init_profile) // profile don't exists or must be skipped
      {
        // create action stub - if request and impression will be received log it
        create_empty_stub(request_writer);

        request_writer.post_impression_actions().push_back(
          request_post_action_info.action_name);
      }

      if(save_profile)
      {
        save_request_writer(mem_buf, request_writer);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }

    return save_profile;
  }

  bool
  RequestInfoContainer::process_fraud_rollback_buf_(
    Generics::ConstSmartMemBuf_var& mem_buf,
    RequestProcessDelegate& request_process_delegate,
    Generics::Time* last_event,
    const AdServer::Commons::RequestId& request_id,
    const Generics::Time& rollback_time)
    /*throw(Exception)*/
  {
    static const char* FUN = "RequestInfoContainer::process_fraud_rollback_buf_()";

    bool save_profile = false;

    try
    {
      if(mem_buf.in())
      {
        RequestInfoProfileReader request_reader(
          mem_buf->membuf().data(), mem_buf->membuf().size());

        if (last_event)
        {
          *last_event = last_event_time(request_reader);
        }

        if(request_reader.fraud() == RequestInfo::RS_NORMAL) // check RS_MOVED
        {
          RequestInfoProfileWriter request_writer(
            mem_buf->membuf().data(), mem_buf->membuf().size());

          if(request_reader.request_done())
          {
            if(!request_process_delegate.request_info.present())
            {
              convert_request_reader_to_request_info(
                request_process_delegate.request_info.fill(),
                request_reader);

              request_process_delegate.request_info->request_id =
                request_id;
            }

            request_process_delegate.process_fraud_request = RequestInfo::RS_FRAUD;

            if(request_reader.impression_verified())
            {
              request_process_delegate.process_rollback_impressions.push_back(
                RequestInfo::RS_FRAUD);
            }

            if(request_reader.click_done())
            {
              request_process_delegate.process_rollback_clicks.push_back(RequestInfo::RS_FRAUD);
            }
          }

          request_writer.fraud() = RequestInfo::RS_FRAUD;
          request_writer.fraud_time() = rollback_time.tv_sec;

          save_request_writer(mem_buf, request_writer);
          save_profile = true;
        }
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }

    return save_profile;
  }
} /* namespace RequestInfoSvcs */
} /* namespace AdServer */

