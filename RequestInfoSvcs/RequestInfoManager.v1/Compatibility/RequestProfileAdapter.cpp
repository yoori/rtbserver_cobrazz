#include <Generics/Time.hpp>
#include <Commons/UserInfoManip.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>

#include <RequestInfoSvcs/RequestInfoCommons/RequestProfile.hpp>
#include <RequestInfoSvcs/RequestInfoManager/Compatibility/RequestProfile_v351.hpp>
#include <RequestInfoSvcs/RequestInfoManager/Compatibility/RequestProfile_v356.hpp>
#include <RequestInfoSvcs/RequestInfoManager/Compatibility/RequestProfile_v360.hpp>
#include <RequestInfoSvcs/RequestInfoManager/Compatibility/RequestProfile_v361.hpp>

#include "RequestProfileAdapter.hpp"

namespace AdServer
{
namespace RequestInfoSvcs
{
namespace
{
  const AdServer::Commons::UserId PROBE_USER_ID("PPPPPPPPPPPPPPPPPPPPPA..");
  const AdServer::Commons::UserId OPTOUT_USER_ID("OOOOOOOOOOOOOOOOOOOOOA..");

  using AdServer::CampaignSvcs::RevenueDecimal;

  namespace RequestInfoSvcs_v351
  {
    namespace RequestInfoSvcsTarget
    {
      using namespace ::AdServer::RequestInfoSvcs_v356;
    }

    void
    convert_revenue_(
      RequestInfoSvcsTarget::RequestInfoRevenueWriter& writer,
      const AdServer::RequestInfoSvcs_v351::RequestInfoRevenueReader& reader)
    {
      writer.rate_id() = reader.rate_id();
      writer.impression() = reader.impression();
      writer.click() = reader.click();
      writer.action() = reader.action();
    }

    void
    convert_cmp_channels_(
      RequestInfoSvcsTarget::RequestInfoProfileWriter::
        cmp_channels_Container& writer,
      const AdServer::RequestInfoSvcs_v351::RequestInfoProfileReader::
        cmp_channels_Container& reader)
    {
      for(AdServer::RequestInfoSvcs_v351::RequestInfoProfileReader::
            cmp_channels_Container::const_iterator it =
              reader.begin();
          it != reader.end(); ++it)
      {
        RequestInfoSvcsTarget::ChannelRevenueWriter rec_writer;
        rec_writer.channel_id() = (*it).channel_id();
        rec_writer.channel_rate_id() = (*it).channel_rate_id();
        rec_writer.impression() = (*it).impression();
        rec_writer.sys_impression() = (*it).sys_impression();
        rec_writer.adv_impression() = (*it).adv_impression();
        rec_writer.click() = (*it).click();
        rec_writer.sys_click() = (*it).sys_click();
        rec_writer.adv_click() = (*it).adv_click();
        writer.push_back(rec_writer);
      }
    }

    Generics::ConstSmartMemBuf_var
    convert_to_v356(const Generics::MemBuf& membuf)
    {
      try
      {
        RequestInfoSvcsTarget::RequestInfoProfileWriter request_writer;

        const AdServer::RequestInfoSvcs_v351::RequestInfoProfileReader
          old_request_reader(membuf.data(), membuf.size());

        request_writer.version() = 356;
        request_writer.request_done() = old_request_reader.request_done();
        request_writer.time() = old_request_reader.time();
        request_writer.isp_time() = old_request_reader.isp_time();
        request_writer.pub_time() = old_request_reader.pub_time();
        request_writer.adv_time() = old_request_reader.adv_time();
        request_writer.user_id() = old_request_reader.user_id();
        request_writer.household_id() = old_request_reader.household_id();
        request_writer.user_status() = old_request_reader.user_status();
        request_writer.test_request() = old_request_reader.test_request();
        request_writer.walled_garden() = old_request_reader.walled_garden();
        request_writer.hid_profile() = old_request_reader.hid_profile();
        request_writer.disable_fraud_detection() = old_request_reader.disable_fraud_detection();
        request_writer.adv_account_id() = old_request_reader.adv_account_id();
        request_writer.advertiser_id() = old_request_reader.advertiser_id();
        request_writer.campaign_id() = old_request_reader.campaign_id();
        request_writer.ccg_id() = old_request_reader.ccg_id();
        request_writer.cc_id() = old_request_reader.cc_id();
        request_writer.has_custom_actions() = old_request_reader.has_custom_actions();
        request_writer.publisher_account_id() = old_request_reader.publisher_account_id();
        request_writer.tag_id() = old_request_reader.tag_id();
        request_writer.size_id() = old_request_reader.size_id();
        request_writer.colo_id() = old_request_reader.colo_id();
        request_writer.currency_exchange_id() = old_request_reader.currency_exchange_id();
        request_writer.tag_delivery_threshold() = old_request_reader.tag_delivery_threshold();
        request_writer.ccg_keyword_id() = old_request_reader.ccg_keyword_id();
        request_writer.keyword_id() = old_request_reader.keyword_id();
        request_writer.num_shown() = old_request_reader.num_shown();
        request_writer.position() = old_request_reader.position();
        request_writer.text_campaign() = old_request_reader.text_campaign();
        request_writer.tag_size() = old_request_reader.tag_size();
        request_writer.tag_visibility() = old_request_reader.tag_visibility();
        request_writer.tag_top_offset() = old_request_reader.tag_top_offset();
        request_writer.tag_left_offset() = old_request_reader.tag_left_offset();
        request_writer.referer() = old_request_reader.referer();
        request_writer.ext_tag_id() = old_request_reader.ext_tag_id();

        convert_revenue_(request_writer.adv_revenue(), old_request_reader.adv_revenue());
        convert_revenue_(request_writer.adv_comm_revenue(), old_request_reader.adv_comm_revenue());
        convert_revenue_(request_writer.adv_payable_comm_amount(), old_request_reader.adv_payable_comm_amount());
        request_writer.impression_pub_revenue_type() = old_request_reader.impression_pub_revenue_type();
        convert_revenue_(request_writer.pub_revenue(), old_request_reader.pub_revenue());
        request_writer.pub_bid_cost() = old_request_reader.pub_bid_cost();
        request_writer.pub_floor_cost() = old_request_reader.pub_floor_cost();
        convert_revenue_(request_writer.pub_comm_revenue(), old_request_reader.pub_comm_revenue());
        request_writer.pub_commission() = old_request_reader.pub_commission();
        convert_revenue_(request_writer.isp_revenue(), old_request_reader.isp_revenue());
        request_writer.isp_revenue_share() = old_request_reader.isp_revenue_share();

        request_writer.adv_currency_rate() = old_request_reader.adv_currency_rate();
        request_writer.pub_currency_rate() = old_request_reader.pub_currency_rate();
        request_writer.isp_currency_rate() = old_request_reader.isp_currency_rate();

        std::copy(
          old_request_reader.channels().begin(),
          old_request_reader.channels().end(),
          std::back_inserter(request_writer.channels()));
        request_writer.geo_channel_id() = old_request_reader.geo_channel_id();
        request_writer.device_channel_id() = old_request_reader.device_channel_id();
        request_writer.expression() = old_request_reader.expression();
        request_writer.client_app() = old_request_reader.client_app();
        request_writer.client_app_version() = old_request_reader.client_app_version();
        request_writer.browser_version() = old_request_reader.browser_version();
        request_writer.os_version() = old_request_reader.os_version();
        request_writer.country() = old_request_reader.country();
        request_writer.enabled_notice() = old_request_reader.enabled_notice();
        request_writer.enabled_impression_tracking() = old_request_reader.enabled_impression_tracking();
        request_writer.enabled_action_tracking() = old_request_reader.enabled_action_tracking();
        request_writer.notice_received() = old_request_reader.notice_received();
        request_writer.notice_non_considered() = old_request_reader.notice_non_considered();
        request_writer.notice_pub_revenue_type() = old_request_reader.notice_pub_revenue_type();
        request_writer.impression_time() = old_request_reader.impression_time();
        request_writer.impression_verified() = old_request_reader.impression_verified();
        request_writer.impression_non_considered() = old_request_reader.impression_non_considered();
        request_writer.click_time() = old_request_reader.click_time();
        request_writer.click_done() = old_request_reader.click_done();
        request_writer.click_non_considered() = old_request_reader.click_non_considered();
        request_writer.action_time() = old_request_reader.action_time();
        request_writer.actions_done() = old_request_reader.actions_done();
        request_writer.actions_non_considered() = old_request_reader.actions_non_considered();
        request_writer.fraud_time() = old_request_reader.fraud_time();
        request_writer.fraud() = old_request_reader.fraud();
        request_writer.auction_type() = old_request_reader.auction_type();

        convert_cmp_channels_(request_writer.cmp_channels(), old_request_reader.cmp_channels());
        request_writer.ctr_reset_id() = old_request_reader.ctr_reset_id();
        request_writer.ctr() = "0.0";

        return Algs::save_to_membuf(request_writer);
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << "Can't adapt profile with version = 356: " << ex.what();
        throw RequestProfileAdapter::Exception(ostr);
      }
    }
  }

  namespace RequestInfoSvcs_v356
  {
    namespace RequestInfoSvcsTarget
    {
      using namespace ::AdServer::RequestInfoSvcs_v360;
    }

    void
    convert_revenue_(
      RequestInfoSvcsTarget::RequestInfoRevenueWriter& writer,
      const AdServer::RequestInfoSvcs_v356::RequestInfoRevenueReader& reader)
    {
      writer.rate_id() = reader.rate_id();
      writer.impression() = reader.impression();
      writer.click() = reader.click();
      writer.action() = reader.action();
    }

    void
    convert_cmp_channels_(
      RequestInfoSvcsTarget::RequestInfoProfileWriter::
        cmp_channels_Container& writer,
      const AdServer::RequestInfoSvcs_v356::RequestInfoProfileReader::
        cmp_channels_Container& reader)
    {
      for(AdServer::RequestInfoSvcs_v356::RequestInfoProfileReader::
            cmp_channels_Container::const_iterator it =
              reader.begin();
          it != reader.end(); ++it)
      {
        RequestInfoSvcsTarget::ChannelRevenueWriter rec_writer;
        rec_writer.channel_id() = (*it).channel_id();
        rec_writer.channel_rate_id() = (*it).channel_rate_id();
        rec_writer.impression() = (*it).impression();
        rec_writer.sys_impression() = (*it).sys_impression();
        rec_writer.adv_impression() = (*it).adv_impression();
        rec_writer.click() = (*it).click();
        rec_writer.sys_click() = (*it).sys_click();
        rec_writer.adv_click() = (*it).adv_click();
        writer.push_back(rec_writer);
      }
    }

    Generics::ConstSmartMemBuf_var
    convert_to_v360(const Generics::MemBuf& membuf)
    {
      try
      {
        RequestInfoSvcsTarget::RequestInfoProfileWriter request_writer;

        const AdServer::RequestInfoSvcs_v356::RequestInfoProfileReader
          old_request_reader(membuf.data(), membuf.size());

        request_writer.version() = 360;
        request_writer.request_done() = old_request_reader.request_done();
        request_writer.time() = old_request_reader.time();
        request_writer.isp_time() = old_request_reader.isp_time();
        request_writer.pub_time() = old_request_reader.pub_time();
        request_writer.adv_time() = old_request_reader.adv_time();
        request_writer.user_id() = old_request_reader.user_id();
        request_writer.household_id() = old_request_reader.household_id();
        request_writer.user_status() = old_request_reader.user_status();
        request_writer.test_request() = old_request_reader.test_request();
        request_writer.walled_garden() = old_request_reader.walled_garden();
        request_writer.hid_profile() = old_request_reader.hid_profile();
        request_writer.disable_fraud_detection() = old_request_reader.disable_fraud_detection();
        request_writer.adv_account_id() = old_request_reader.adv_account_id();
        request_writer.advertiser_id() = old_request_reader.advertiser_id();
        request_writer.campaign_id() = old_request_reader.campaign_id();
        request_writer.ccg_id() = old_request_reader.ccg_id();
        request_writer.cc_id() = old_request_reader.cc_id();
        request_writer.has_custom_actions() = old_request_reader.has_custom_actions();
        request_writer.publisher_account_id() = old_request_reader.publisher_account_id();
        request_writer.tag_id() = old_request_reader.tag_id();
        request_writer.site_id() = old_request_reader.site_id();
        request_writer.size_id() = old_request_reader.size_id();
        request_writer.colo_id() = old_request_reader.colo_id();
        request_writer.currency_exchange_id() = old_request_reader.currency_exchange_id();
        request_writer.tag_delivery_threshold() = old_request_reader.tag_delivery_threshold();
        request_writer.ccg_keyword_id() = old_request_reader.ccg_keyword_id();
        request_writer.keyword_id() = old_request_reader.keyword_id();
        request_writer.num_shown() = old_request_reader.num_shown();
        request_writer.position() = old_request_reader.position();
        request_writer.text_campaign() = old_request_reader.text_campaign();
        request_writer.tag_size() = old_request_reader.tag_size();
        request_writer.tag_visibility() = old_request_reader.tag_visibility();
        request_writer.tag_top_offset() = old_request_reader.tag_top_offset();
        request_writer.tag_left_offset() = old_request_reader.tag_left_offset();
        request_writer.referer() = old_request_reader.referer();
        request_writer.ext_tag_id() = old_request_reader.ext_tag_id();

        convert_revenue_(request_writer.adv_revenue(), old_request_reader.adv_revenue());
        convert_revenue_(request_writer.adv_comm_revenue(), old_request_reader.adv_comm_revenue());
        convert_revenue_(request_writer.adv_payable_comm_amount(), old_request_reader.adv_payable_comm_amount());
        request_writer.impression_pub_revenue_type() = old_request_reader.impression_pub_revenue_type();
        convert_revenue_(request_writer.pub_revenue(), old_request_reader.pub_revenue());
        request_writer.pub_bid_cost() = old_request_reader.pub_bid_cost();
        request_writer.pub_floor_cost() = old_request_reader.pub_floor_cost();
        convert_revenue_(request_writer.pub_comm_revenue(), old_request_reader.pub_comm_revenue());
        request_writer.pub_commission() = old_request_reader.pub_commission();
        convert_revenue_(request_writer.isp_revenue(), old_request_reader.isp_revenue());
        request_writer.isp_revenue_share() = old_request_reader.isp_revenue_share();

        request_writer.adv_currency_rate() = old_request_reader.adv_currency_rate();
        request_writer.pub_currency_rate() = old_request_reader.pub_currency_rate();
        request_writer.isp_currency_rate() = old_request_reader.isp_currency_rate();

        std::copy(
          old_request_reader.channels().begin(),
          old_request_reader.channels().end(),
          std::back_inserter(request_writer.channels()));
        request_writer.geo_channel_id() = old_request_reader.geo_channel_id();
        request_writer.device_channel_id() = old_request_reader.device_channel_id();
        request_writer.expression() = old_request_reader.expression();
        request_writer.client_app() = old_request_reader.client_app();
        request_writer.client_app_version() = old_request_reader.client_app_version();
        request_writer.browser_version() = old_request_reader.browser_version();
        request_writer.os_version() = old_request_reader.os_version();
        request_writer.country() = old_request_reader.country();
        request_writer.enabled_notice() = old_request_reader.enabled_notice();
        request_writer.enabled_impression_tracking() = old_request_reader.enabled_impression_tracking();
        request_writer.enabled_action_tracking() = old_request_reader.enabled_action_tracking();
        request_writer.notice_received() = old_request_reader.notice_received();
        request_writer.notice_non_considered() = old_request_reader.notice_non_considered();
        request_writer.notice_pub_revenue_type() = old_request_reader.notice_pub_revenue_type();
        request_writer.impression_time() = old_request_reader.impression_time();
        request_writer.impression_verified() = old_request_reader.impression_verified();
        request_writer.impression_non_considered() = old_request_reader.impression_non_considered();
        request_writer.click_time() = old_request_reader.click_time();
        request_writer.click_done() = old_request_reader.click_done();
        request_writer.click_non_considered() = old_request_reader.click_non_considered();
        request_writer.action_time() = old_request_reader.action_time();
        request_writer.actions_done() = old_request_reader.actions_done();
        request_writer.actions_non_considered() = old_request_reader.actions_non_considered();
        request_writer.fraud_time() = old_request_reader.fraud_time();
        request_writer.fraud() = old_request_reader.fraud();
        request_writer.auction_type() = old_request_reader.auction_type();
        request_writer.viewability() = -1;

        convert_cmp_channels_(request_writer.cmp_channels(), old_request_reader.cmp_channels());
        request_writer.ctr_reset_id() = old_request_reader.ctr_reset_id();
        request_writer.ctr() = old_request_reader.ctr();
        std::copy(old_request_reader.post_impression_actions().begin(),
          old_request_reader.post_impression_actions().end(),
          std::back_inserter(request_writer.post_impression_actions()));        
        request_writer.campaign_freq() = old_request_reader.campaign_freq();
        request_writer.referer_hash() = old_request_reader.url_hash();
        std::copy(old_request_reader.geo_channels().begin(),
          old_request_reader.geo_channels().end(),
          std::back_inserter(request_writer.geo_channels()));
        std::copy(old_request_reader.user_channels().begin(),
          old_request_reader.user_channels().end(),
          std::back_inserter(request_writer.user_channels()));
        request_writer.url() = old_request_reader.url();
        request_writer.ip_address() = old_request_reader.ip_address();
        request_writer.ctr_algorithm_id() = old_request_reader.ctr_algorithm_id();
        std::copy(old_request_reader.model_ctrs().begin(),
          old_request_reader.model_ctrs().end(),
          std::back_inserter(request_writer.model_ctrs()));
        request_writer.conv_rate_algorithm_id() = old_request_reader.conv_rate_algorithm_id();
        request_writer.conv_rate() = old_request_reader.conv_rate();
        std::copy(old_request_reader.model_conv_rates().begin(),
          old_request_reader.model_conv_rates().end(),
          std::back_inserter(request_writer.model_conv_rates()));

        return Algs::save_to_membuf(request_writer);
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << "Can't adapt profile with version = 356: " << ex.what();
        throw RequestProfileAdapter::Exception(ostr);
      }
    }
  }

  namespace RequestInfoSvcs_v360
  {
    namespace RequestInfoSvcsTarget
    {
      using namespace ::AdServer::RequestInfoSvcs_v361;
    }

    void
    convert_revenue_(
      RequestInfoSvcsTarget::RequestInfoRevenueWriter& writer,
      const AdServer::RequestInfoSvcs_v360::RequestInfoRevenueReader& reader)
    {
      writer.rate_id() = reader.rate_id();
      writer.impression() = reader.impression();
      writer.click() = reader.click();
      writer.action() = reader.action();
    }

    void
    convert_cmp_channels_(
      RequestInfoSvcsTarget::RequestInfoProfileWriter::
        cmp_channels_Container& writer,
      const AdServer::RequestInfoSvcs_v360::RequestInfoProfileReader::
        cmp_channels_Container& reader)
    {
      for(AdServer::RequestInfoSvcs_v360::RequestInfoProfileReader::
            cmp_channels_Container::const_iterator it =
              reader.begin();
          it != reader.end(); ++it)
      {
        RequestInfoSvcsTarget::ChannelRevenueWriter rec_writer;
        rec_writer.channel_id() = (*it).channel_id();
        rec_writer.channel_rate_id() = (*it).channel_rate_id();
        rec_writer.impression() = (*it).impression();
        rec_writer.sys_impression() = (*it).sys_impression();
        rec_writer.adv_impression() = (*it).adv_impression();
        rec_writer.click() = (*it).click();
        rec_writer.sys_click() = (*it).sys_click();
        rec_writer.adv_click() = (*it).adv_click();
        writer.push_back(rec_writer);
      }
    }

    Generics::ConstSmartMemBuf_var
    convert_to_v361(const Generics::MemBuf& membuf)
    {
      try
      {
        RequestInfoSvcsTarget::RequestInfoProfileWriter request_writer;

        const AdServer::RequestInfoSvcs_v360::RequestInfoProfileReader
          old_request_reader(membuf.data(), membuf.size());

        request_writer.version() = 361;
        request_writer.request_done() = old_request_reader.request_done();
        request_writer.time() = old_request_reader.time();
        request_writer.isp_time() = old_request_reader.isp_time();
        request_writer.pub_time() = old_request_reader.pub_time();
        request_writer.adv_time() = old_request_reader.adv_time();
        request_writer.user_id() = old_request_reader.user_id();
        request_writer.household_id() = old_request_reader.household_id();
        request_writer.user_status() = old_request_reader.user_status();
        request_writer.test_request() = old_request_reader.test_request();
        request_writer.walled_garden() = old_request_reader.walled_garden();
        request_writer.hid_profile() = old_request_reader.hid_profile();
        request_writer.disable_fraud_detection() = old_request_reader.disable_fraud_detection();
        request_writer.adv_account_id() = old_request_reader.adv_account_id();
        request_writer.advertiser_id() = old_request_reader.advertiser_id();
        request_writer.campaign_id() = old_request_reader.campaign_id();
        request_writer.ccg_id() = old_request_reader.ccg_id();
        request_writer.cc_id() = old_request_reader.cc_id();
        request_writer.has_custom_actions() = old_request_reader.has_custom_actions();
        request_writer.publisher_account_id() = old_request_reader.publisher_account_id();
        request_writer.tag_id() = old_request_reader.tag_id();
        request_writer.site_id() = old_request_reader.site_id();
        request_writer.size_id() = old_request_reader.size_id();
        request_writer.colo_id() = old_request_reader.colo_id();
        request_writer.currency_exchange_id() = old_request_reader.currency_exchange_id();
        request_writer.tag_delivery_threshold() = old_request_reader.tag_delivery_threshold();
        request_writer.ccg_keyword_id() = old_request_reader.ccg_keyword_id();
        request_writer.keyword_id() = old_request_reader.keyword_id();
        request_writer.num_shown() = old_request_reader.num_shown();
        request_writer.position() = old_request_reader.position();
        request_writer.text_campaign() = old_request_reader.text_campaign();
        request_writer.tag_size() = old_request_reader.tag_size();
        request_writer.tag_visibility() = old_request_reader.tag_visibility();
        request_writer.tag_top_offset() = old_request_reader.tag_top_offset();
        request_writer.tag_left_offset() = old_request_reader.tag_left_offset();
        request_writer.referer() = old_request_reader.referer();
        request_writer.ext_tag_id() = old_request_reader.ext_tag_id();

        convert_revenue_(request_writer.adv_revenue(), old_request_reader.adv_revenue());
        convert_revenue_(request_writer.adv_comm_revenue(), old_request_reader.adv_comm_revenue());
        convert_revenue_(request_writer.adv_payable_comm_amount(), old_request_reader.adv_payable_comm_amount());
        request_writer.impression_pub_revenue_type() = old_request_reader.impression_pub_revenue_type();
        convert_revenue_(request_writer.pub_revenue(), old_request_reader.pub_revenue());
        request_writer.pub_bid_cost() = old_request_reader.pub_bid_cost();
        request_writer.pub_floor_cost() = old_request_reader.pub_floor_cost();
        convert_revenue_(request_writer.pub_comm_revenue(), old_request_reader.pub_comm_revenue());
        request_writer.pub_commission() = old_request_reader.pub_commission();
        convert_revenue_(request_writer.isp_revenue(), old_request_reader.isp_revenue());
        request_writer.isp_revenue_share() = old_request_reader.isp_revenue_share();

        request_writer.adv_currency_rate() = old_request_reader.adv_currency_rate();
        request_writer.pub_currency_rate() = old_request_reader.pub_currency_rate();
        request_writer.isp_currency_rate() = old_request_reader.isp_currency_rate();

        std::copy(
          old_request_reader.channels().begin(),
          old_request_reader.channels().end(),
          std::back_inserter(request_writer.channels()));
        request_writer.geo_channel_id() = old_request_reader.geo_channel_id();
        request_writer.device_channel_id() = old_request_reader.device_channel_id();
        request_writer.expression() = old_request_reader.expression();
        request_writer.client_app() = old_request_reader.client_app();
        request_writer.client_app_version() = old_request_reader.client_app_version();
        request_writer.browser_version() = old_request_reader.browser_version();
        request_writer.os_version() = old_request_reader.os_version();
        request_writer.country() = old_request_reader.country();
        request_writer.enabled_notice() = old_request_reader.enabled_notice();
        request_writer.enabled_impression_tracking() = old_request_reader.enabled_impression_tracking();
        request_writer.enabled_action_tracking() = old_request_reader.enabled_action_tracking();
        request_writer.notice_received() = old_request_reader.notice_received();
        request_writer.notice_non_considered() = old_request_reader.notice_non_considered();
        request_writer.notice_pub_revenue_type() = old_request_reader.notice_pub_revenue_type();
        request_writer.impression_time() = old_request_reader.impression_time();
        request_writer.impression_verified() = old_request_reader.impression_verified();
        request_writer.impression_non_considered() = old_request_reader.impression_non_considered();
        request_writer.click_time() = old_request_reader.click_time();
        request_writer.click_done() = old_request_reader.click_done();
        request_writer.click_non_considered() = old_request_reader.click_non_considered();
        request_writer.action_time() = old_request_reader.action_time();
        request_writer.actions_done() = old_request_reader.actions_done();
        request_writer.actions_non_considered() = old_request_reader.actions_non_considered();
        request_writer.fraud_time() = old_request_reader.fraud_time();
        request_writer.fraud() = old_request_reader.fraud();
        request_writer.auction_type() = old_request_reader.auction_type();
        request_writer.viewability() = -1;

        convert_cmp_channels_(request_writer.cmp_channels(), old_request_reader.cmp_channels());
        request_writer.ctr_reset_id() = old_request_reader.ctr_reset_id();
        request_writer.ctr() = old_request_reader.ctr();
        std::copy(old_request_reader.post_impression_actions().begin(),
          old_request_reader.post_impression_actions().end(),
          std::back_inserter(request_writer.post_impression_actions()));        
        request_writer.campaign_freq() = old_request_reader.campaign_freq();
        request_writer.referer_hash() = old_request_reader.referer_hash();
        std::copy(old_request_reader.geo_channels().begin(),
          old_request_reader.geo_channels().end(),
          std::back_inserter(request_writer.geo_channels()));
        std::copy(old_request_reader.user_channels().begin(),
          old_request_reader.user_channels().end(),
          std::back_inserter(request_writer.user_channels()));
        request_writer.url() = old_request_reader.url();
        request_writer.ip_address() = old_request_reader.ip_address();
        request_writer.ctr_algorithm_id() = old_request_reader.ctr_algorithm_id();
        std::copy(old_request_reader.model_ctrs().begin(),
          old_request_reader.model_ctrs().end(),
          std::back_inserter(request_writer.model_ctrs()));
        request_writer.conv_rate_algorithm_id() = old_request_reader.conv_rate_algorithm_id();
        request_writer.conv_rate() = old_request_reader.conv_rate();
        std::copy(old_request_reader.model_conv_rates().begin(),
          old_request_reader.model_conv_rates().end(),
          std::back_inserter(request_writer.model_conv_rates()));

        request_writer.delta_adv_revenue().rate_id() = 0;
        request_writer.delta_adv_revenue().impression() = "0";
        request_writer.delta_adv_revenue().click() = "0";
        request_writer.delta_adv_revenue().action() = "0";

        request_writer.self_service_commission() = "0";
        request_writer.adv_commission() = "0";
        request_writer.at_flags() = 0;
        request_writer.pub_cost_coef() = "0";

        return Algs::save_to_membuf(request_writer);
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << "Can't adapt profile with version = 360: " << ex.what();
        throw RequestProfileAdapter::Exception(ostr);
      }
    }
  }

  namespace RequestInfoSvcs_v361
  {
    namespace RequestInfoSvcsTarget
    {
      using namespace ::AdServer::RequestInfoSvcs;
    }

    void
    convert_revenue_(
      RequestInfoSvcsTarget::RequestInfoRevenueWriter& writer,
      const AdServer::RequestInfoSvcs_v361::RequestInfoRevenueReader& reader)
    {
      writer.rate_id() = reader.rate_id();
      writer.impression() = reader.impression();
      writer.click() = reader.click();
      writer.action() = reader.action();
    }

    void
    convert_cmp_channels_(
      RequestInfoSvcsTarget::RequestInfoProfileWriter::
        cmp_channels_Container& writer,
      const AdServer::RequestInfoSvcs_v361::RequestInfoProfileReader::
        cmp_channels_Container& reader)
    {
      for(AdServer::RequestInfoSvcs_v361::RequestInfoProfileReader::
            cmp_channels_Container::const_iterator it =
              reader.begin();
          it != reader.end(); ++it)
      {
        RequestInfoSvcsTarget::ChannelRevenueWriter rec_writer;
        rec_writer.channel_id() = (*it).channel_id();
        rec_writer.channel_rate_id() = (*it).channel_rate_id();
        rec_writer.impression() = (*it).impression();
        rec_writer.sys_impression() = (*it).sys_impression();
        rec_writer.adv_impression() = (*it).adv_impression();
        rec_writer.click() = (*it).click();
        rec_writer.sys_click() = (*it).sys_click();
        rec_writer.adv_click() = (*it).adv_click();
        writer.push_back(rec_writer);
      }
    }

    Generics::ConstSmartMemBuf_var
    convert_to_v362(const Generics::MemBuf& membuf)
    {
      try
      {
        RequestInfoSvcsTarget::RequestInfoProfileWriter request_writer;

        const AdServer::RequestInfoSvcs_v361::RequestInfoProfileReader
          old_request_reader(membuf.data(), membuf.size());

        request_writer.version() = 362;
        request_writer.request_done() = old_request_reader.request_done();
        request_writer.time() = old_request_reader.time();
        request_writer.isp_time() = old_request_reader.isp_time();
        request_writer.pub_time() = old_request_reader.pub_time();
        request_writer.adv_time() = old_request_reader.adv_time();
        request_writer.user_id() = old_request_reader.user_id();
        request_writer.household_id() = old_request_reader.household_id();
        request_writer.user_status() = old_request_reader.user_status();
        request_writer.test_request() = old_request_reader.test_request();
        request_writer.walled_garden() = old_request_reader.walled_garden();
        request_writer.hid_profile() = old_request_reader.hid_profile();
        request_writer.disable_fraud_detection() = old_request_reader.disable_fraud_detection();
        request_writer.adv_account_id() = old_request_reader.adv_account_id();
        request_writer.advertiser_id() = old_request_reader.advertiser_id();
        request_writer.campaign_id() = old_request_reader.campaign_id();
        request_writer.ccg_id() = old_request_reader.ccg_id();
        request_writer.cc_id() = old_request_reader.cc_id();
        request_writer.has_custom_actions() = old_request_reader.has_custom_actions();
        request_writer.publisher_account_id() = old_request_reader.publisher_account_id();
        request_writer.tag_id() = old_request_reader.tag_id();
        request_writer.site_id() = old_request_reader.site_id();
        request_writer.size_id() = old_request_reader.size_id();
        request_writer.colo_id() = old_request_reader.colo_id();
        request_writer.currency_exchange_id() = old_request_reader.currency_exchange_id();
        request_writer.tag_delivery_threshold() = old_request_reader.tag_delivery_threshold();
        request_writer.ccg_keyword_id() = old_request_reader.ccg_keyword_id();
        request_writer.keyword_id() = old_request_reader.keyword_id();
        request_writer.num_shown() = old_request_reader.num_shown();
        request_writer.position() = old_request_reader.position();
        request_writer.text_campaign() = old_request_reader.text_campaign();
        request_writer.tag_size() = old_request_reader.tag_size();
        request_writer.tag_visibility() = old_request_reader.tag_visibility();
        request_writer.tag_top_offset() = old_request_reader.tag_top_offset();
        request_writer.tag_left_offset() = old_request_reader.tag_left_offset();
        request_writer.referer() = old_request_reader.referer();
        request_writer.ext_tag_id() = old_request_reader.ext_tag_id();

        convert_revenue_(request_writer.adv_revenue(), old_request_reader.adv_revenue());
        convert_revenue_(request_writer.adv_comm_revenue(), old_request_reader.adv_comm_revenue());
        convert_revenue_(request_writer.adv_payable_comm_amount(), old_request_reader.adv_payable_comm_amount());
        request_writer.impression_pub_revenue_type() = old_request_reader.impression_pub_revenue_type();
        convert_revenue_(request_writer.pub_revenue(), old_request_reader.pub_revenue());
        request_writer.pub_bid_cost() = old_request_reader.pub_bid_cost();
        request_writer.pub_floor_cost() = old_request_reader.pub_floor_cost();
        convert_revenue_(request_writer.pub_comm_revenue(), old_request_reader.pub_comm_revenue());
        request_writer.pub_commission() = old_request_reader.pub_commission();
        convert_revenue_(request_writer.isp_revenue(), old_request_reader.isp_revenue());
        request_writer.isp_revenue_share() = old_request_reader.isp_revenue_share();

        request_writer.adv_currency_rate() = old_request_reader.adv_currency_rate();
        request_writer.pub_currency_rate() = old_request_reader.pub_currency_rate();
        request_writer.isp_currency_rate() = old_request_reader.isp_currency_rate();

        std::copy(
          old_request_reader.channels().begin(),
          old_request_reader.channels().end(),
          std::back_inserter(request_writer.channels()));
        request_writer.geo_channel_id() = old_request_reader.geo_channel_id();
        request_writer.device_channel_id() = old_request_reader.device_channel_id();
        request_writer.expression() = old_request_reader.expression();
        request_writer.client_app() = old_request_reader.client_app();
        request_writer.client_app_version() = old_request_reader.client_app_version();
        request_writer.browser_version() = old_request_reader.browser_version();
        request_writer.os_version() = old_request_reader.os_version();
        request_writer.country() = old_request_reader.country();
        request_writer.enabled_notice() = old_request_reader.enabled_notice();
        request_writer.enabled_impression_tracking() = old_request_reader.enabled_impression_tracking();
        request_writer.enabled_action_tracking() = old_request_reader.enabled_action_tracking();
        request_writer.notice_received() = old_request_reader.notice_received();
        request_writer.notice_non_considered() = old_request_reader.notice_non_considered();
        request_writer.notice_pub_revenue_type() = old_request_reader.notice_pub_revenue_type();
        request_writer.impression_time() = old_request_reader.impression_time();
        request_writer.impression_verified() = old_request_reader.impression_verified();
        request_writer.impression_non_considered() = old_request_reader.impression_non_considered();
        request_writer.click_time() = old_request_reader.click_time();
        request_writer.click_done() = old_request_reader.click_done();
        request_writer.click_non_considered() = old_request_reader.click_non_considered();
        request_writer.action_time() = old_request_reader.action_time();
        request_writer.actions_done() = old_request_reader.actions_done();
        request_writer.actions_non_considered() = old_request_reader.actions_non_considered();
        request_writer.fraud_time() = old_request_reader.fraud_time();
        request_writer.fraud() = old_request_reader.fraud();
        request_writer.auction_type() = old_request_reader.auction_type();
        request_writer.viewability() = -1;

        convert_cmp_channels_(request_writer.cmp_channels(), old_request_reader.cmp_channels());
        request_writer.ctr_reset_id() = old_request_reader.ctr_reset_id();
        request_writer.ctr() = old_request_reader.ctr();
        std::copy(old_request_reader.post_impression_actions().begin(),
          old_request_reader.post_impression_actions().end(),
          std::back_inserter(request_writer.post_impression_actions()));        
        request_writer.campaign_freq() = old_request_reader.campaign_freq();
        request_writer.referer_hash() = old_request_reader.referer_hash();
        std::copy(old_request_reader.geo_channels().begin(),
          old_request_reader.geo_channels().end(),
          std::back_inserter(request_writer.geo_channels()));
        std::copy(old_request_reader.user_channels().begin(),
          old_request_reader.user_channels().end(),
          std::back_inserter(request_writer.user_channels()));
        request_writer.url() = old_request_reader.url();
        request_writer.ip_address() = old_request_reader.ip_address();
        request_writer.ctr_algorithm_id() = old_request_reader.ctr_algorithm_id();
        std::copy(old_request_reader.model_ctrs().begin(),
          old_request_reader.model_ctrs().end(),
          std::back_inserter(request_writer.model_ctrs()));
        request_writer.conv_rate_algorithm_id() = old_request_reader.conv_rate_algorithm_id();
        request_writer.conv_rate() = old_request_reader.conv_rate();
        std::copy(old_request_reader.model_conv_rates().begin(),
          old_request_reader.model_conv_rates().end(),
          std::back_inserter(request_writer.model_conv_rates()));

        request_writer.delta_adv_revenue().rate_id() = 0;
        request_writer.delta_adv_revenue().impression() = "0";
        request_writer.delta_adv_revenue().click() = "0";
        request_writer.delta_adv_revenue().action() = "0";

        request_writer.self_service_commission() = old_request_reader.self_service_commission();
        request_writer.adv_commission() = old_request_reader.adv_commission();
        request_writer.at_flags() = old_request_reader.at_flags();
        request_writer.pub_cost_coef() = old_request_reader.pub_cost_coef();

        // request_writer.notice_imp_revenue
        // request_writer.impression_imp_revenue
        // request_writer.impression_user_id

        return Algs::save_to_membuf(request_writer);
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << "Can't adapt profile with version = 360: " << ex.what();
        throw RequestProfileAdapter::Exception(ostr);
      }
    }
  }
}
}
}

namespace AdServer
{
namespace RequestInfoSvcs
{
  Generics::ConstSmartMemBuf_var
  RequestProfileAdapter::operator()(
    const Generics::ConstSmartMemBuf* mem_buf) /*throw(Exception)*/
  {
    static const char* FUN = "RequestProfileAdapter::operator()";

    const unsigned long version_head_size =
      RequestInfoProfileVersionReader::FIXED_SIZE;

    if(mem_buf->membuf().size() < version_head_size)
    {
      throw Exception("Corrupt header");
    }

    Generics::ConstSmartMemBuf_var result_mem_buf =
      ReferenceCounting::add_ref(mem_buf);

    try
    {
      const RequestInfoProfileVersionReader version_reader(
        mem_buf->membuf().data(), mem_buf->membuf().size());

      if(version_reader.version() != AdServer::RequestInfoSvcs::CURRENT_REQUEST_PROFILE_VERSION)
      {
        unsigned long current_version = version_reader.version();

        if(current_version == 351)
        {
          result_mem_buf = RequestInfoSvcs_v351::convert_to_v356(result_mem_buf->membuf());
          current_version = 356;
        }

        if(current_version == 356)
        {
          result_mem_buf = RequestInfoSvcs_v356::convert_to_v360(result_mem_buf->membuf());
          current_version = 360;
        }
 
        if(current_version == 360)
        {
          result_mem_buf = RequestInfoSvcs_v360::convert_to_v361(result_mem_buf->membuf());
          current_version = 361;
        }

        if(current_version == 361)
        {
          result_mem_buf = RequestInfoSvcs_v361::convert_to_v362(result_mem_buf->membuf());
          current_version = 362;
        }

        if(current_version != AdServer::RequestInfoSvcs::CURRENT_REQUEST_PROFILE_VERSION)
        {
          Stream::Error ostr;
          ostr << FUN << ": incorrect version after adaptation = " <<
            current_version;
          throw Exception(ostr);
        }
      }

      return result_mem_buf;
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }
}
}
