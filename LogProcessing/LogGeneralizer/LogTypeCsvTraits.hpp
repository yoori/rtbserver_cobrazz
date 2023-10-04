#ifndef AD_SERVER_LOG_PROCESSING_LOG_TYPE_CSV_TRAITS_HPP
#define AD_SERVER_LOG_PROCESSING_LOG_TYPE_CSV_TRAITS_HPP


#include <sstream>
#include <String/StringManip.hpp>

#include <LogCommons/LogCommons.hpp>
#include <LogCommons/ActionRequest.hpp>
#include <LogCommons/ActionStat.hpp>
#include <LogCommons/CampaignUserStat.hpp>
#include <LogCommons/CcUserStat.hpp>
#include <LogCommons/CcgKeywordStat.hpp>
#include <LogCommons/CcgSelectionFailureStat.hpp>
#include <LogCommons/CcgStat.hpp>
#include <LogCommons/CcgUserStat.hpp>
#include <LogCommons/ChannelCountStat.hpp>
#include <LogCommons/ChannelHitStat.hpp>
#include <LogCommons/ChannelImpInventory.hpp>
#include <LogCommons/ChannelInventory.hpp>
#include <LogCommons/ChannelInventoryEstimationStat.hpp>
#include <LogCommons/ChannelOverlapUserStat.hpp>
#include <LogCommons/ChannelPerformance.hpp>
#include <LogCommons/ChannelPriceRange.hpp>
#include <LogCommons/ChannelTriggerImpStat.hpp>
#include <LogCommons/ChannelTriggerStat.hpp>
#include <LogCommons/ColoUpdateStat.hpp>
#include <LogCommons/ColoUserStat.hpp>
#include <LogCommons/CmpStat.hpp>
#include <LogCommons/CreativeStat.hpp>
#include <LogCommons/RequestStatsHourlyExtStat.hpp>
#include <LogCommons/DeviceChannelCountStat.hpp>
#include <LogCommons/ExpressionPerformance.hpp>
#include <LogCommons/PageLoadsDailyStat.hpp>
#include <LogCommons/PassbackStat.hpp>
#include <LogCommons/SearchEngineStat.hpp>
#include <LogCommons/SearchTermStat.hpp>
#include <LogCommons/SiteReferrerStat.hpp>
#include <LogCommons/SiteUserStat.hpp>
#include <LogCommons/TagAuctionStat.hpp>
#include <LogCommons/TagPositionStat.hpp>
#include <LogCommons/UserAgentStat.hpp>
#include <LogCommons/UserProperties.hpp>
#include <LogCommons/WebStat.hpp>
#include <LogCommons/CampaignReferrerStat.hpp>
#include <LogCommons/CsvUtils.hpp>
#include <LogCommons/GenericLogCsvSaverImpl.hpp>

namespace AdServer
{
namespace LogProcessing
{
  struct ActionRequestCsvTraits: ActionRequestTraits
  {
    static const char* csv_base_name() { return "ActionRequests"; }

    static const char* csv_header()
    {
      return "action_date,colo_id,action_id,country_code,action_referrer_url,"
        "user_status,action_request_count,cur_value";
    }

    static std::ostream&
    write_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT& key,
      const BaseTraits::CollectorType::DataT::KeyT& inner_key,
      const BaseTraits::CollectorType::DataT::DataT& data)
    {
      write_key_as_csv(os, key) << ',';
      write_inner_key_as_csv(os, inner_key) << ',';
      return write_inner_data_as_csv(os, data);
    }

  private:
    static std::ostream&
    write_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT &key)
    {
      write_date_as_csv(os, key.sdate()) << ',';
      return os << key.colo_id();
    }

    static std::ostream&
    write_inner_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::KeyT &key)
    {
      os << key.action_id() << ',';
      write_string_as_csv(os, ToUpper()(key.country_code())) << ',';
      write_string_as_csv(os, key.action_referrer_url()) << ',';
      return os << key.user_status();
    }

    static std::ostream&
    write_inner_data_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::DataT &data)
    {
      return os << data.action_request_count() << ',' <<
        data.cur_value();
    }
  };

  struct ActionStatCsvTraits: ActionStatTraits
  {
    static const char* csv_base_name() { return "ActionStats"; }

    static const char* csv_header()
    {
      return "action_date,colo_id,action_request_id,request_id,cc_id,action_id,"
        "tag_id,order_id,country_code,action_referrer_url,"
        "imp_date,click_date,cur_value";
    }

    static std::ostream&
    write_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT& key,
      const BaseTraits::CollectorType::DataT::KeyT& inner_key,
      const BaseTraits::CollectorType::DataT::DataT& data)
    {
      write_key_as_csv(os, key) << ',';
      write_inner_key_as_csv(os, inner_key) << ',';
      return write_inner_data_as_csv(os, data);
    }

  private:
    static std::ostream&
    write_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT &key)
    {
      write_date_as_csv(os, key.sdate()) << ',';
      return os << key.colo_id();
    }

    static std::ostream&
    write_inner_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::KeyT &key)
    {
      return os << key.action_request_id() << ',' <<
        key.request_id() << ',' <<
        key.cc_id();
    }

    static std::ostream&
    write_inner_data_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::DataT &data)
    {
      os << data.action_id() << ',' <<
        data.tag_id() << ',';
      write_string_as_csv(os, MimeCoder<UTF8Encoder>()(data.order_id(), 100)) << ',';
      write_string_as_csv(os, ToUpper()(data.country_code().get()), "-") << ',';
      write_string_as_csv(os, data.referrer(), "-") << ',';
      write_date_as_csv(os, data.imp_date()) << ',';
      write_optional_date_as_csv(os, data.click_date()) << ',';
      os << data.cur_value();
      return os;
    }
  };

  struct AdvertiserUserStatCsvTraits: AdvertiserUserStatVersionsTraits
  {
    static const char* csv_base_name() { return "AdvertiserUserStats"; }

    static const char* csv_header()
    {
      return "adv_sdate,adv_account_id,last_appearance_date,unique_users,"
        "text_unique_users,display_unique_users";
    }

    static std::ostream&
    write_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT& key,
      const BaseTraits::CollectorType::DataT::KeyT& inner_key,
      const BaseTraits::CollectorType::DataT::DataT& data)
    {
      write_key_as_csv(os, key) << ',';
      write_inner_key_as_csv(os, inner_key) << ',';
      return write_inner_data_as_csv(os, data);
    }

  private:
    static std::ostream&
    write_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT &key)
    {
      return write_date_as_csv(os, key);
    }

    static std::ostream&
    write_inner_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::KeyT &key)
    {
      os << key.adv_account_id() << ',';
      write_optional_or_null_date_as_csv_2(os, key.last_appearance_date());
      return os;
    }

    static std::ostream&
    write_inner_data_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::DataT &data)
    {
      return os << data.unique_users() << ',' << data.text_unique_users() << ',' <<
        data.display_unique_users();
    }
  };

  struct CampaignUserStatCsvTraits: CampaignUserStatTraits
  {
    static const char* csv_base_name() { return "CampaignUserStats"; }

    static const char* csv_header()
    {
      return "adv_sdate,colo_id,campaign_id,last_appearance_date,unique_users";
    }

    static std::ostream&
    write_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT& key,
      const BaseTraits::CollectorType::DataT::KeyT& inner_key,
      const BaseTraits::CollectorType::DataT::DataT& data)
    {
      write_key_as_csv(os, key) << ',';
      write_inner_key_as_csv(os, inner_key) << ',';
      return write_inner_data_as_csv(os, data);
    }

  private:
    static std::ostream&
    write_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT &key)
    {
      write_date_as_csv(os, key.adv_sdate()) << ',';
      return os << key.colo_id();
    }

    static std::ostream&
    write_inner_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::KeyT &key)
    {
      os << key.cmp_id() << ',';
      write_optional_or_null_date_as_csv_2(os, key.last_appearance_date());
      return os;
    }

    static std::ostream&
    write_inner_data_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::DataT &data)
    {
      return os << data.unique_users();
    }
  };

  struct CcgUserStatCsvTraits: CcgUserStatTraits
  {
    static const char* csv_base_name() { return "CcgUserStats"; }

    static const char* csv_header()
    {
      return "adv_sdate,colo_id,ccg_id,last_appearance_date,unique_users";
    }

    static std::ostream&
    write_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT& key,
      const BaseTraits::CollectorType::DataT::KeyT& inner_key,
      const BaseTraits::CollectorType::DataT::DataT& data)
    {
      write_key_as_csv(os, key) << ',';
      write_inner_key_as_csv(os, inner_key) << ',';
      return write_inner_data_as_csv(os, data);
    }

  private:
    static std::ostream&
    write_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT &key)
    {
      write_date_as_csv(os, key.adv_sdate()) << ',';
      return os << key.colo_id();
    }

    static std::ostream&
    write_inner_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::KeyT &key)
    {
      os << key.ccg_id() << ',';
      write_optional_or_null_date_as_csv_2(os, key.last_appearance_date());
      return os;
    }

    static std::ostream&
    write_inner_data_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::DataT &data
                            )
    {
      return os << data.unique_users();
    }
  };

  struct CcStatCsvTraits: CcStatTraits
  {
    static const char*
    csv_base_name() noexcept
    {
      return "CcStatsDaily";
    }

    static const char*
    csv_header() noexcept
    {
      return "adv_sdate,colo_id,cc_id,auctions_lost";
    }

    static std::ostream&
    write_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT& key,
      const BaseTraits::CollectorType::DataT::KeyT& inner_key,
      const BaseTraits::CollectorType::DataT::DataT& data)
    {
      write_key_as_csv(os, key) << ',';
      write_inner_key_as_csv(os, inner_key) << ',';
      return write_inner_data_as_csv(os, data);
    }

  private:
    static std::ostream&
    write_key_as_csv(
      std::ostream& os,
      const BaseTraits::CollectorType::KeyT& key)
      /*throw(eh::Exception)*/
    {
      write_date_as_csv(os, key.sdate()) << ',';
      return os << key.colo_id();
    }

    static std::ostream&
    write_inner_key_as_csv(
      std::ostream& os,
      const BaseTraits::CollectorType::DataT::KeyT& key)
      /*throw(eh::Exception)*/
    {
      return os << key.cc_id();
    }

    static std::ostream&
    write_inner_data_as_csv(
      std::ostream& os,
      const BaseTraits::CollectorType::DataT::DataT& data)
      /*throw(eh::Exception)*/
    {
      return os << data.auctions_lost();
    }
  };

  struct CcgStatCsvTraits: CcgStatTraits
  {
    static const char*
    csv_base_name() noexcept
    {
      return "CcgStatsDaily";
    }

    static const char*
    csv_header() noexcept
    {
      return "adv_sdate,colo_id,ccg_id,auctions_lost";
    }

    static std::ostream&
    write_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT& key,
      const BaseTraits::CollectorType::DataT::KeyT& inner_key,
      const BaseTraits::CollectorType::DataT::DataT& data)
    {
      write_key_as_csv(os, key) << ',';
      write_inner_key_as_csv(os, inner_key) << ',';
      return write_inner_data_as_csv(os, data);
    }

  private:
    static std::ostream&
    write_key_as_csv(
      std::ostream& os,
      const BaseTraits::CollectorType::KeyT& key)
      /*throw(eh::Exception)*/
    {
      write_date_as_csv(os, key.sdate()) << ',';
      return os << key.colo_id();
    }

    static std::ostream&
    write_inner_key_as_csv(
      std::ostream& os,
      const BaseTraits::CollectorType::DataT::KeyT& key)
      /*throw(eh::Exception)*/
    {
      return os << key.ccg_id();
    }

    static std::ostream&
    write_inner_data_as_csv(
      std::ostream& os,
      const BaseTraits::CollectorType::DataT::DataT& data)
      /*throw(eh::Exception)*/
    {
      return os << data.auctions_lost();
    }
  };

  struct CcUserStatCsvTraits: CcUserStatTraits
  {
    static const char* csv_base_name() { return "CcUserStats"; }

    static const char* csv_header()
    {
      return "adv_sdate,colo_id,cc_id,last_appearance_date,unique_users";
    }

    static std::ostream&
    write_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT& key,
      const BaseTraits::CollectorType::DataT::KeyT& inner_key,
      const BaseTraits::CollectorType::DataT::DataT& data)
    {
      write_key_as_csv(os, key) << ',';
      write_inner_key_as_csv(os, inner_key) << ',';
      return write_inner_data_as_csv(os, data);
    }

  private:
    static std::ostream&
    write_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT &key)
    {
      write_date_as_csv(os, key.adv_sdate()) << ',';
      return os << key.colo_id();
    }

    static std::ostream&
    write_inner_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::KeyT &key)
    {
      os << key.cc_id() << ',';
      write_optional_or_null_date_as_csv_2(os, key.last_appearance_date());
      return os;
    }

    static std::ostream&
    write_inner_data_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::DataT &data)
    {
      return os << data.unique_users();
    }
  };

  struct CcgKeywordStatCsvTraits: CcgKeywordStatTraits
  {
    static const char* csv_base_name() { return "CcgKeywordStats"; }

    static const char* csv_header()
    {
      return "sdate,colo_id,ccg_keyword_id,currency_exchange_id,cc_id,"
        "imps,clicks,adv_amount,adv_comm_amount,pub_amount_adv";
    }

    static std::ostream&
    write_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT& key,
      const BaseTraits::CollectorType::DataT::KeyT& inner_key,
      const BaseTraits::CollectorType::DataT::DataT& data)
    {
      write_key_as_csv(os, key) << ',';
      write_inner_key_as_csv(os, inner_key) << ',';
      return write_inner_data_as_csv(os, data);
    }

  private:
    static std::ostream&
    write_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT &key)
    {
      write_date_as_csv(os, key.sdate()) << ',';
      write_optional_value_as_csv(os, key.colo_id(), "0");
      return os;
    }

    static std::ostream&
    write_inner_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::KeyT &key)
    {
      return os << key.ccg_keyword_id() << ',' <<
        key.currency_exchange_id() << ',' <<
        key.cc_id();
    }

    static std::ostream&
    write_inner_data_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::DataT &data)
    {
      return os << data.imps() << ',' <<
        data.clicks() << ',' <<
        data.adv_revenue() << ',' <<
        data.adv_comm_revenue() << ',' <<
        data.pub_advcurrency_amount();
    }
  };

  struct CcgSelectionFailureStatCsvTraits: CcgSelectionFailureStatTraits
  {
    static const char* csv_base_name() { return "CcgSelectionFailure"; }

    static const char* csv_header()
    {
      return "adv_sdate,ccg_id,combination_mask,requests";
    }

    static std::ostream&
    write_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT& key,
      const BaseTraits::CollectorType::DataT::KeyT& inner_key,
      const BaseTraits::CollectorType::DataT::DataT& data)
    {
      write_key_as_csv(os, key) << ',';
      write_inner_key_as_csv(os, inner_key) << ',';
      return write_inner_data_as_csv(os, data);
    }

  private:
    static std::ostream&
    write_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT &key)
    {
      return write_date_as_csv(os, key);
    }

    static std::ostream&
    write_inner_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::KeyT &key)
    {
      return os << key.ccg_id() << ',' << key.combination_mask();
    }

    static std::ostream&
    write_inner_data_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::DataT &data)
    {
      return os << data.requests();
    }
  };

  struct ChannelCountStatCsvTraits: ChannelCountStatTraits
  {
    static const char* csv_base_name() { return "ChannelCountStats"; }

    static const char* csv_header()
    {
      return "isp_sdate,colo_id,"
        "channel_type,channel_count,"
        "users_count";
    }

    static std::ostream&
    write_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT& key,
      const BaseTraits::CollectorType::DataT::KeyT& inner_key,
      const BaseTraits::CollectorType::DataT::DataT& data)
    {
      write_key_as_csv(os, key) << ',';
      write_inner_key_as_csv(os, inner_key) << ',';
      return write_inner_data_as_csv(os, data);
    }

  private:
    static std::ostream&
    write_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT &key)
    {
      write_date_as_csv(os, key.isp_sdate()) << ',';
      return os << key.colo_id();
    }

    static std::ostream&
    write_inner_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::KeyT &key)
    {
      return os << key.channel_type() << ',' << key.channel_count();
    }

    static std::ostream&
    write_inner_data_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::DataT &data)
    {
      return os << data.users_count();
    }
  };

  struct RequestStatsHourlyExtStatCsvTraits: RequestStatsHourlyExtStatTraits
  {
    static const char* csv_base_name() { return "RequestStatsHourlyExt"; }

    static const char* csv_header()
    {
      return "sdate,adv_sdate,colo_id,publisher_account_id,tag_id,size_id,"
        "country_code,adv_account_id,campaign_id,ccg_id,cc_id,ccg_rate_id,"
        "colo_rate_id,site_rate_id,currency_exchange_id,delivery_threshold,"
	"num_shown,position,test,fraud,walled_garden,user_status,"
	"geo_channel_id,device_channel_id,ctr_reset_id,hid_profile,"
	"viewability,unverified_imps,imps,clicks,actions,adv_amount,"
	"pub_amount,isp_amount,adv_comm_amount,pub_comm_amount,"
	"adv_payable_comm_amount,pub_advcurrency_amount,"
	"isp_advcurrency_amount,undup_imps,undup_clicks,ym_confirmed_clicks,"
	"ym_bounced_clicks,ym_robots_clicks,ym_session_time";
    }

    static std::ostream&
    write_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT& key,
      const BaseTraits::CollectorType::DataT::KeyT& inner_key,
      const BaseTraits::CollectorType::DataT::DataT& data)
    {
      write_key_as_csv(os, key) << ',';
      write_inner_key_as_csv(os, inner_key) << ',';
      return write_inner_data_as_csv(os, data);
    }

  private:
    static std::ostream&
    write_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT &key)
    {
      write_date_as_csv(os, key.sdate()) << ',';
      return write_date_as_csv(os, key.adv_sdate());
    }

    static std::ostream&
    write_inner_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::KeyT &key)
    {
      os << key.colo_id() << ',';
      os << key.publisher_account_id() << ',';
      os << key.tag_id() << ',';
      write_optional_value_as_csv(os, key.size_id()) << ',';
      write_string_as_csv(os, ToUpper()(key.country_code())) << ',';
      os << key.adv_account_id() << ',';
      os << key.campaign_id() << ',';
      os << key.ccg_id() << ',';
      os << key.cc_id() << ',';
      os << key.ccg_rate_id() << ',';
      os << key.colo_rate_id() << ',';
      os << key.site_rate_id() << ',';
      os << key.currency_exchange_id() << ',';
      os << key.delivery_threshold() << ',';
      os << key.num_shown() << ',';
      os << key.position() << ',';
      os << key.test() << ',';
      os << key.fraud() << ',';
      os << key.walled_garden() << ',';
      os << key.user_status() << ',';
      write_optional_value_as_csv(os, key.geo_channel_id()) << ',';
      write_optional_value_as_csv(os, key.device_channel_id()) << ',';
      os << key.ctr_reset_id() << ',';
      os << key.hid_profile() << ',';
      os << key.viewability();
      return os;
    }

    static std::ostream&
    write_inner_data_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::DataT &data)
    {
      return os <<
        data.unverified_imps() << ',' <<
        data.imps() << ',' <<
        data.clicks() << ',' <<
        data.actions() << ',' <<
        data.adv_amount() << ',' <<
        data.pub_amount() << ',' <<
        data.isp_amount() << ',' <<
        data.adv_comm_amount() << ',' <<
        data.pub_comm_amount() << ',' <<
        data.adv_payable_comm_amount() << ',' <<
        data.pub_advcurrency_amount() << ',' <<
        data.isp_advcurrency_amount() << ',' <<
        data.undup_imps() << ',' <<
        data.undup_clicks() << ',' <<
        data.ym_confirmed_clicks() << ',' <<
        data.ym_bounced_clicks() << ',' <<
        data.ym_robots_clicks() << ',' <<
        data.ym_session_time();
    }
  };

  struct ChannelHitStatCsvTraits: ChannelHitStatTraits
  {
    static const char* csv_base_name() { return "ChannelInventory"; }

    static const char* csv_header()
    {
      return "sdate,colo_id,channel_id,sum_ecpm,active_user_count,"
        "total_user_count,hits,hits_urls,hits_kws,hits_search_kws,"
        "hits_url_kws";
    }

    static std::ostream&
    write_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT& key,
      const BaseTraits::CollectorType::DataT::KeyT& inner_key,
      const BaseTraits::CollectorType::DataT::DataT& data)
    {
      write_key_as_csv(os, key) << ',';
      write_inner_key_as_csv(os, inner_key) << ',';
      return write_inner_data_as_csv(os, data);
    }

  private:
    static std::ostream&
    write_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT &key)
    {
      write_date_as_csv(os, key.sdate()) << ',';
      return os << key.colo_id();
    }

    static std::ostream&
    write_inner_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::KeyT &key)
    {
      return os << key.channel_id();
    }

    static std::ostream&
    write_inner_data_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::DataT &data)
    {
      return os << "0,0,0," <<
        data.hits() << ',' <<
        data.hits_urls() << ',' <<
        data.hits_kws() << ',' <<
        data.hits_search_kws() << ',' <<
        data.hits_url_kws();
    }
  };

  struct ChannelImpInventoryCsvTraits: ChannelImpInventoryTraits
  {
    static const char* csv_base_name() { return "ChannelImpInventory"; }

    static const char* csv_header()
    {
      return "sdate,colo_id,channel_id,ccg_type,"
        "clicks,actions,revenue,impops_user_count,"
        "imps,"
        "imps_user_count,"
        "imps_value,"
        "imps_other,"
        "imps_other_user_count,"
        "imps_other_value,"
        "impops_no_imp,"
        "impops_no_imp_user_count,"
        "impops_no_imp_value";
    }

    static std::ostream&
    write_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT& key,
      const BaseTraits::CollectorType::DataT::KeyT& inner_key,
      const BaseTraits::CollectorType::DataT::DataT& data)
    {
      write_key_as_csv(os, key) << ',';
      write_inner_key_as_csv(os, inner_key) << ',';
      return write_inner_data_as_csv(os, data);
    }

  private:
    static std::ostream&
    write_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT &key)
    {
      write_date_as_csv(os, key.sdate()) << ',';
      return os << key.colo_id();
    }

    static std::ostream&
    write_inner_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::KeyT &key)
    {
      return os << key.channel_id() << ',' << key.ccg_type();
    }

    static std::ostream&
    write_inner_data_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::DataT &data)
    {
      return os <<
        data.clicks() << ',' <<
        data.actions() << ',' <<
        data.revenue() << ',' <<
        data.impops_user_count() << ',' <<

        data.imp_count().imps << ',' <<
        data.imp_count().user_count << ',' <<
        data.imp_count().value << ',' <<

        data.imp_other_count().imps << ',' <<
        data.imp_other_count().user_count << ',' <<
        data.imp_other_count().value << ',' <<

        data.no_impops_count().imps << ',' <<
        data.no_impops_count().user_count << ',' <<
        data.no_impops_count().value;
    }
  };

  struct ChannelInventoryCsvTraits: ChannelInventoryTraits
  {
    static const char* csv_base_name() { return "ChannelInventory"; }

    static const char* csv_header()
    {
      return "sdate,colo_id,channel_id,sum_ecpm,active_user_count,"
        "total_user_count,hits,hits_urls,hits_kws,hits_search_kws,"
        "hits_url_kws";
    }

    static std::ostream&
    write_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT& key,
      const BaseTraits::CollectorType::DataT::KeyT& inner_key,
      const BaseTraits::CollectorType::DataT::DataT& data)
    {
      write_key_as_csv(os, key) << ',';
      write_inner_key_as_csv(os, inner_key) << ',';
      return write_inner_data_as_csv(os, data);
    }

  private:
    static std::ostream&
    write_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT &key)
    {
      write_date_as_csv(os, key.sdate()) << ',';
      return os << key.colo_id();
    }

    static std::ostream&
    write_inner_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::KeyT &key)
    {
      return os << key.channel_id();
    }

    static std::ostream&
    write_inner_data_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::DataT &data)
    {
      return os << data.sum_user_ecpm() << ',' <<
        data.active_user_count() << ',' <<
        data.total_user_count() << ",0,0,0,0,0";
    }
  };

  struct ChannelInventoryEstimationStatCsvTraits: ChannelInventoryEstimationStatTraits
  {
    static const char* csv_base_name() { return "ChannelInventoryEstimStats"; }

    static const char* csv_header()
    {
      return "sdate,colo_id,channel_id,match_level,users_regular,users_from_now";
    }

    static std::ostream&
    write_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT& key,
      const BaseTraits::CollectorType::DataT::KeyT& inner_key,
      const BaseTraits::CollectorType::DataT::DataT& data)
    {
      write_key_as_csv(os, key) << ',';
      write_inner_key_as_csv(os, inner_key) << ',';
      return write_inner_data_as_csv(os, data);
    }

  private:
    static std::ostream&
    write_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT &key)
    {
      write_date_as_csv(os, key.sdate()) << ',';
      return os << key.colo_id();
    }

    static std::ostream&
    write_inner_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::KeyT &key)
    {
      return os << key.channel_id() << ',' << key.level();
    }

    static std::ostream&
    write_inner_data_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::DataT &data)
    {
      return os << data.users_regular() << ',' << data.users_from_now();
    }
  };

  struct ChannelOverlapUserStatCsvTraits: ChannelOverlapUserStatTraits
  {
    static const char* csv_base_name() { return "ChannelOverlapUserStats"; }

    static const char* csv_header()
    {
      return "sdate,channel1_id,channel2_id,unique_users";
    }

    static std::ostream&
    write_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT& key,
      const BaseTraits::CollectorType::DataT::KeyT& inner_key,
      const BaseTraits::CollectorType::DataT::DataT& data)
    {
      write_key_as_csv(os, key) << ',';
      write_inner_key_as_csv(os, inner_key) << ',';
      return write_inner_data_as_csv(os, data);
    }

  private:
    static std::ostream&
    write_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT &key)
    {
      return write_date_as_csv(os, key);
    }

    static std::ostream&
    write_inner_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::KeyT &key)
    {
      return os << key.channel1_id() << ',' << key.channel2_id();
    }

    static std::ostream&
    write_inner_data_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::DataT &data)
    {
      return os << data.unique_users();
    }
  };

  struct ChannelPerformanceCsvTraits: ChannelPerformanceTraits
  {
    static const char* csv_base_name() { return "ChannelPerformance"; }

    static const char* csv_header()
    {
      return "sdate,colo_id,channel_id,ccg_id,tag_size,requests,imps,clicks,"
        "actions,revenue";
    }

    static std::ostream&
    write_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT& key,
      const BaseTraits::CollectorType::DataT::KeyT& inner_key,
      const BaseTraits::CollectorType::DataT::DataT& data)
    {
      write_key_as_csv(os, key) << ',';
      write_inner_key_as_csv(os, inner_key) << ',';
      return write_inner_data_as_csv(os, data);
    }

  private:
    static std::ostream&
    write_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT &key)
    {
      write_date_as_csv(os, key.sdate()) << ',';
      return os << key.colo_id();
    }

    static std::ostream&
    write_inner_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::KeyT &key)
    {
      os << key.channel_id() << ',';
      os << key.ccg_id() << ',';
      write_string_as_csv(os, key.tag_size());
      return os;
    }

    static std::ostream&
    write_inner_data_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::DataT &data)
    {
      return os << data.requests() << ',' <<
        data.imps() << ',' <<
        data.clicks() << ',' <<
        data.actions() << ',' <<
        data.revenue();
    }
  };

  struct ChannelPriceRangeCsvTraits: ChannelPriceRangeTraits
  {
    static const char* csv_base_name() { return "ChannelInventoryByCpm"; }

    static const char* csv_header()
    {
      return "sdate,creative_size,country_code,channel_id,ecpm,colo_id,"
        "user_count,impops";
    }

    static std::ostream&
    write_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT& key,
      const BaseTraits::CollectorType::DataT::KeyT& inner_key,
      const BaseTraits::CollectorType::DataT::DataT& data)
    {
      write_key_as_csv(os, key) << ',';
      write_inner_key_as_csv(os, inner_key) << ',';
      return write_inner_data_as_csv(os, data);
    }

  private:
    static std::ostream&
    write_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT &key)
    {
      return write_date_as_csv(os, key);
    }

    static std::ostream&
    write_inner_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::KeyT &key)
    {
      write_string_as_csv(os, key.creative_size()) << ',';
      write_string_as_csv(os, ToUpper()(key.country_code()), "-") << ',';
      return os << key.channel_id() << ',' << key.ecpm() << ',' << key.colo_id();
    }

    static std::ostream&
    write_inner_data_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::DataT &data)
    {
      return os << data.unique_users_count() << ',' << data.impops();
    }
  };

  struct ChannelTriggerImpStatCsvTraits: ChannelTriggerImpStatTraits
  {
    static const char* csv_base_name() { return "ChannelTriggerImpStats"; }

    static const char* csv_header()
    {
      return "sdate,colo_id,channel_trigger_id,trigger_type,"
        "approximated_imps,approximated_clicks,channel_id";
    }

    static std::ostream&
    write_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT& key,
      const BaseTraits::CollectorType::DataT::KeyT& inner_key,
      const BaseTraits::CollectorType::DataT::DataT& data)
    {
      write_date_as_csv(os, key.sdate()) << ',';
      os << key.colo_id() << ',' <<
        inner_key.channel_trigger_id() << ',' <<
        inner_key.type() << ',' <<
        data.approximated_imps() << ',' <<
        data.approximated_clicks() << ',' <<
        inner_key.channel_id();
      return os;
    }
  };

  struct ChannelTriggerStatCsvTraits: ChannelTriggerStatTraits
  {
    static const char* csv_base_name() { return "ChannelTriggerStats-2"; }

    static const char* csv_header()
    {
      return "sdate,colo_id,channel_trigger_id,trigger_type,hits,channel_id";
    }

    static std::ostream&
    write_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT& key,
      const BaseTraits::CollectorType::DataT::KeyT& inner_key,
      const BaseTraits::CollectorType::DataT::DataT& data)
    {
      write_date_as_csv(os, key.sdate()) << ',';
      os << key.colo_id() << ',' <<
        inner_key.channel_trigger_id() << ',' <<
        inner_key.type() << ',' <<
        data.hits() << ',' <<
        inner_key.channel_id();

      return os;
    }
  };

  struct CmpStatCsvTraits: CmpStatTraits
  {
    static const char* csv_base_name() { return "CmpStats"; }

    static const char* csv_header()
    {
      return "sdate,colo_id,cc_id,channel_rate_id,channel_id,"
        "currency_exchange_id,country_code,walled_garden,fraud_correction,"
        "tag_id,size_id,delivery_threshold,imps,clicks,cmp_amount,"
        "adv_amount_cmp,cmp_amount_global";
    }

    static std::ostream&
    write_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT& key,
      const BaseTraits::CollectorType::DataT::KeyT& inner_key,
      const BaseTraits::CollectorType::DataT::DataT& data)
    {
      write_key_as_csv(os, key) << ',';
      write_inner_key_as_csv(os, inner_key) << ',';
      return write_inner_data_as_csv(os, data);
    }

  private:
    static std::ostream&
    write_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT &key)
    {
      write_date_as_csv(os, key.sdate()) << ',';
      return os << key.colo_id();
    }

    static std::ostream&
    write_inner_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::KeyT &key)
    {
      os << key.cc_id() << ',' <<
        key.channel_rate_id() << ',' <<
        key.channel_id() << ',' <<
        key.currency_exchange_id() << ',';
      write_optional_string_upper_as_csv(os, key.country_code(), "-") << ',';
      os << bool_to_char(key.walled_garden()) << ',' <<
        bool_to_char(key.fraud()) << ',' <<
        key.tag_id() << ',';
      write_optional_value_as_csv(os, key.size_id()) << ',';
      os << key.delivery_threshold();
      return os;
    }

    static std::ostream&
    write_inner_data_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::DataT &data)
    {
      return os << data.imps() << ',' <<
        data.clicks() << ',' <<
        data.cmp_amount() << ',' <<
        data.adv_amount_cmp() << ',' <<
        data.cmp_sys_amount();
    }
  };

  struct CreativeStatCsvTraits: CreativeStatTraits
  {
    static const char* csv_base_name() { return "RequestStatsHourly"; }

    static const char* csv_header()
    {
      return "sdate,colo_id,cc_id,tag_id,size_id,colo_rate_id,ccg_rate_id,"
        "site_rate_id,currency_exchange_id,country_code,walled_garden,"
        "fraud_correction,num_shown,position,test,user_status,"
        "delivery_threshold,ctr_reset_id,hid_profile,geo_channel_id,"
        "device_channel_id,imp,imp_unverified,clicks,actions,adv_amount,"
        "pub_amount,isp_amount,adv_comm_amount,pub_comm_amount,"
        "adv_invoice_comm_amount,pub_amount_adv,isp_amount_adv";
    }

    static std::ostream&
    write_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT& key,
      const BaseTraits::CollectorType::DataT::KeyT& inner_key,
      const BaseTraits::CollectorType::DataT::DataT& data)
    {
      write_date_as_csv(os, key.sdate()) << ',';

      os << inner_key.colo_id() << ',' <<
        inner_key.cc_id() << ',' <<
        inner_key.tag_id() << ',';
      write_optional_value_as_csv(os, inner_key.size_id()) << ',';
      os << inner_key.colo_rate_id() << ',' <<
        inner_key.ccg_rate_id() << ',' <<
        inner_key.site_rate_id() << ',' <<
        inner_key.currency_exchange_id() << ',';
      write_string_as_csv(os, ToUpper()(inner_key.country_code()), "-") << ',';
      os << bool_to_char(inner_key.walled_garden()) << ',' <<
        bool_to_char(inner_key.fraud()) << ',' <<
        inner_key.num_shown() << ',' <<
        inner_key.position() << ',' <<
        bool_to_char(inner_key.test()) << ',' <<
        inner_key.user_status() << ',' <<
        inner_key.delivery_threshold() << ',' <<
        inner_key.ctr_reset_id() << ',' <<
        bool_to_char(inner_key.hid_profile()) << ',';
      write_optional_value_as_csv(os, inner_key.geo_channel_id()) << ',';
      write_optional_value_as_csv(os, inner_key.device_channel_id()) << ',';

      os << data.imps() << ',' <<
        data.unverified_imps() << ',' <<
        data.clicks() << ',' <<
        data.actions() << ',' <<
        data.adv_amount() << ',' <<
        data.pub_amount() << ',' <<
        data.isp_amount() << ',' <<
        data.adv_comm_amount() << ',' <<
        data.pub_comm_amount() << ',' <<
        data.adv_payable_comm_amount() << ',' <<
        data.pub_advcurrency_amount() << ',' <<
        data.isp_advcurrency_amount();

      return os;
    }
  };

  struct ColoUpdateStatCsvTraits: ColoUpdateStatTraits
  {
    static const char* csv_base_name() { return "ColoStats"; }

    static const char* csv_header()
    {
      return "colo_id,last_channel_update,last_campaign_update,software_version";
    }

    static std::ostream&
    write_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT& key,
      const BaseTraits::CollectorType::DataT& data)
    {
      write_key_as_csv(os, key) << ',';
      return write_data_as_csv(os, data);
    }

  private:
    static std::ostream&
    write_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT &key)
    {
      return os << key.colo_id();
    }

    static std::ostream&
    write_data_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT &data)
    {
      write_optional_date_as_csv(os, data.last_channels_update()) << ',';
      write_optional_date_as_csv(os, data.last_campaigns_update()) << ',';
      write_optional_value_as_csv(os, data.version());
      return os;
    }
  };

  struct ColoUserStatCsvTraits: ColoUserStatTraits
  {
    static const char* csv_base_name() { return "ColoUserStats"; }

    static const char* csv_header()
    {
      return "isp_sdate,colo_id,create_date,last_appearance_date,unique_users,"
        "network_unique_users,profiling_unique_users,unique_hids";
    }

    static std::ostream&
    write_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT& key,
      const BaseTraits::CollectorType::DataT::KeyT& inner_key,
      const BaseTraits::CollectorType::DataT::DataT& data)
    {
      write_key_as_csv(os, key) << ',';
      write_inner_key_as_csv(os, inner_key) << ',';
      return write_inner_data_as_csv(os, data);
    }

  private:
    static std::ostream&
    write_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT &key)
    {
      write_date_as_csv(os, key.sdate()) << ',';
      return os << key.colo_id();
    }

    static std::ostream&
    write_inner_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::KeyT &key)
    {
      write_date_as_csv(os, key.create_date()) << ',';
      write_optional_or_null_date_as_csv_2(os, key.last_appearance_date());
      return os;
    }

    static std::ostream&
    write_inner_data_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::DataT &data
                            )
    {
      return os << data.unique_users() << ',' <<
        data.network_unique_users() << ',' <<
        data.profiling_unique_users() << ',' <<
        data.unique_hids();
    }
  };

  struct DeviceChannelCountStatCsvTraits: DeviceChannelCountStatTraits
  {
    static const char* csv_base_name() { return "DeviceChannelCountStats"; }

    static const char* csv_header()
    {
      return "isp_sdate,colo_id,device_channel_id,channel_type,channel_count,"
        "users_count";
    }

    static std::ostream&
    write_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT& key,
      const BaseTraits::CollectorType::DataT::KeyT& inner_key,
      const BaseTraits::CollectorType::DataT::DataT& data)
    {
      write_key_as_csv(os, key) << ',';
      write_inner_key_as_csv(os, inner_key) << ',';
      return write_inner_data_as_csv(os, data);
    }

  private:
    static std::ostream&
    write_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT &key)
    {
      write_date_as_csv(os, key.isp_sdate()) << ',';
      return os << key.colo_id();
    }

    static std::ostream&
    write_inner_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::KeyT &key)
    {
      return os << key.device_channel_id() << ',' << key.channel_type() << ',' <<
        key.channel_count();
    }

    static std::ostream&
    write_inner_data_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::DataT &data
                            )
    {
      return os << data.users_count();
    }
  };

  struct ExpressionPerformanceCsvTraits: ExpressionPerformanceTraits
  {
    static const char* csv_base_name() { return "ExpressionPerformance"; }

    static const char* csv_header()
    {
      return "sdate,colo_id,cc_id,expression,imps_verified,clicks,actions";
    }

    static std::ostream&
    write_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT& key,
      const BaseTraits::CollectorType::DataT::KeyT& inner_key,
      const BaseTraits::CollectorType::DataT::DataT& data)
    {
      write_key_as_csv(os, key) << ',';
      write_inner_key_as_csv(os, inner_key) << ',';
      return write_inner_data_as_csv(os, data);
    }

  private:
    static std::ostream&
    write_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT &key)
    {
      write_date_as_csv(os, key.sdate()) << ',';
      return os << key.colo_id();
    }

    static std::ostream&
    write_inner_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::KeyT &key)
    {
      os << key.cc_id() << ',';
      write_not_empty_string_as_csv(os, key.expression());
      return os;
    }

    static std::ostream&
    write_inner_data_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::DataT &data)
    {
      return os << data.imps_verified() << ',' <<
        data.clicks() << ',' <<
        data.actions();
    }
  };

  struct GlobalColoUserStatCsvTraits: GlobalColoUserStatTraits
  {
    static const char* csv_base_name() { return "GlobalColoUserStats"; }

    static const char* csv_header()
    {
      return "global_sdate,colo_id,create_date,last_appearance_date,"
        "unique_users,network_unique_users,profiling_unique_users,"
        "unique_hids";
    }

    static std::ostream&
    write_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT& key,
      const BaseTraits::CollectorType::DataT::KeyT& inner_key,
      const BaseTraits::CollectorType::DataT::DataT& data)
    {
      write_key_as_csv(os, key) << ',';
      write_inner_key_as_csv(os, inner_key) << ',';
      return write_inner_data_as_csv(os, data);
    }

  private:
    static std::ostream&
    write_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT &key)
    {
      write_date_as_csv(os, key.sdate()) << ',';
      return os << key.colo_id();
    }

    static std::ostream&
    write_inner_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::KeyT &key)
    {
      write_date_as_csv(os, key.create_date()) << ',';
      write_optional_or_null_date_as_csv_2(os, key.last_appearance_date());
      return os;
    }

    static std::ostream&
    write_inner_data_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::DataT &data)
    {
      return os << data.unique_users() << ',' <<
        data.network_unique_users() << ',' <<
        data.profiling_unique_users() << ',' <<
        data.unique_hids();
    }
  };

  struct PageLoadsDailyStatCsvTraits: PageLoadsDailyStatTraits
  {
    static const char*
    csv_base_name() noexcept
    {
      return "PageLoadsDaily";
    }

    static const char*
    csv_header() noexcept
    {
      return "sdate,colo_id,"
        "site_id,country_code,tag_group,"
        "page_loads,utilized_page_loads";
    }

    static std::ostream&
    write_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT& key,
      const BaseTraits::CollectorType::DataT::KeyT& inner_key,
      const BaseTraits::CollectorType::DataT::DataT& data)
    {
      write_key_as_csv(os, key) << ',';
      write_inner_key_as_csv(os, inner_key) << ',';
      return write_inner_data_as_csv(os, data);
    }

  private:
    static std::ostream&
    write_key_as_csv(
      std::ostream& os,
      const BaseTraits::CollectorType::KeyT& key)
    {
      write_date_as_csv(os, key.sdate()) << ',';
      return os << key.colo_id();
    }

    static std::ostream&
    write_inner_key_as_csv(
      std::ostream& os,
      const BaseTraits::CollectorType::DataT::KeyT& key)
    {
      os << key.site_id() << ',';
      write_string_as_csv(os, ToUpper()(key.country()), "-") << ',';
      return write_list_as_csv(os, key.tags());
    }

    static std::ostream&
    write_inner_data_as_csv(
      std::ostream& os,
      const BaseTraits::CollectorType::DataT::DataT& data)
    {
      return os << data.page_loads() << ',' << data.utilized_page_loads();
    }
  };

  struct PassbackStatCsvTraits: PassbackStatTraits
  {
    static const char* csv_base_name() { return "PassbackStats"; }

    static const char* csv_header()
    {
      return "sdate,colo_id,"
        "user_status,country_code,tag_id,size_id,"
        "requests";
    }

    static std::ostream&
    write_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT& key,
      const BaseTraits::CollectorType::DataT::KeyT& inner_key,
      const BaseTraits::CollectorType::DataT::DataT& data)
    {
      write_key_as_csv(os, key) << ',';
      write_inner_key_as_csv(os, inner_key) << ',';
      return write_inner_data_as_csv(os, data);
    }

  private:
    static std::ostream&
    write_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT &key)
    {
      write_date_as_csv(os, key.sdate()) << ',';
      return os << key.colo_id();
    }

    static std::ostream&
    write_inner_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::KeyT &key)
    {
      os << key.user_status() << ',';
      write_string_as_csv(os, ToUpper()(key.country_code()), "-") << ',';
      os << key.tag_id() << ',';
      write_optional_value_as_csv(os, key.size_id());
      return os;
    }

    static std::ostream&
    write_inner_data_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::DataT &data)
    {
      return os << data.requests();
    }
  };

  struct SearchEngineStatCsvTraits: SearchEngineStatTraits
  {
    static const char* csv_base_name() { return "SearchEngineStatsDaily"; }

    static const char* csv_header()
    {
      return "sdate,colo_id,search_engine_id,host_name,hits,hits_empty_page";
    }

    static std::ostream&
    write_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT& key,
      const BaseTraits::CollectorType::DataT::KeyT& inner_key,
      const BaseTraits::CollectorType::DataT::DataT& data)
    {
      write_key_as_csv(os, key) << ',';
      write_inner_key_as_csv(os, inner_key) << ',';
      return write_inner_data_as_csv(os, data);
    }

  private:
    static std::ostream&
    write_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT &key)
    {
      write_date_as_csv(os, key.sdate()) << ',';
      return os << key.colo_id();
    }

    static std::ostream&
    write_inner_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::KeyT &key)
    {
      os << key.search_engine_id() << ',';
      write_not_empty_string_as_csv(os, key.host_name());
      return os;
    }

    static std::ostream&
    write_inner_data_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::DataT &data)
    {
      return os << data.hits() << ',' << data.hits_empty_page();
    }
  };

  struct SearchTermStatCsvTraits: SearchTermStatTraits
  {
    static const char* csv_base_name() { return "SearchTermStatsDaily"; }

    static const char* csv_header()
    {
      return "sdate,colo_id,search_term,hits";
    }

    static std::ostream&
    write_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT& key,
      const BaseTraits::CollectorType::DataT::KeyT& inner_key,
      const BaseTraits::CollectorType::DataT::DataT& data)
    {
      write_key_as_csv(os, key) << ',';
      write_inner_key_as_csv(os, inner_key) << ',';
      return write_inner_data_as_csv(os, data);
    }

  private:
    static std::ostream&
    write_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT &key)
    {
      write_date_as_csv(os, key.sdate()) << ',';
      return os << key.colo_id();
    }

    static std::ostream&
    write_inner_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::KeyT &key)
    {
      write_not_empty_string_as_csv(os, key.search_term());
      return os;
    }

    static std::ostream&
    write_inner_data_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::DataT &data
                            )
    {
      return os << data.hits();
    }
  };

  struct SiteReferrerStatCsvTraits: SiteReferrerStatTraits
  {
    static const char* csv_base_name() { return "SiteReferrerStats"; }

    static const char* csv_header()
    {
      return "sdate,colo_id,"
        "tag_id,ext_tag_id,url,user_status,"
        "requests,imps,clicks,passbacks,"
        "bids_won_count,bids_lost_count,no_bid_count,floor_won_cost,"
        "floor_lost_cost,floor_no_bid_cost,bid_won_amount,bid_lost_amount,cost";
    }

    static std::ostream&
    write_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT& key,
      const BaseTraits::CollectorType::DataT::KeyT& inner_key,
      const BaseTraits::CollectorType::DataT::DataT& data)
    {
      write_key_as_csv(os, key) << ',';
      write_inner_key_as_csv(os, inner_key) << ',';
      return write_inner_data_as_csv(os, data);
    }

  private:
    static std::ostream&
    write_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT &key)
    {
      write_date_as_csv(os, key.sdate()) << ',';
      return os << key.colo_id();
    }

    static std::ostream&
    write_inner_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::KeyT &key)
    {
      os << key.tag_id() << ',';
      write_string_as_csv(os, MimeCoder<>()(key.ext_tag_id(), 50)) << ',';
      write_string_as_csv(os, key.url()) << ',';
      return os << key.user_status();

    }

    static std::ostream&
    write_inner_data_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::DataT &data)
    {
      return os << data.requests() << ',' <<
        data.imps() << ',' <<
        data.clicks() << ',' <<
        data.passbacks() << ',' <<
        data.bids_won_count() << ',' <<
        data.bids_lost_count() << ',' <<
        data.no_bid_count() << ',' <<
        data.floor_won_cost() << ',' <<
        data.floor_lost_cost() << ',' <<
        data.floor_no_bid_cost() << ',' <<
        data.bid_won_amount() << ',' <<
        data.bid_lost_amount() << ',' <<
        data.cost();
    }
  };

  struct SiteUserStatCsvTraits: SiteUserStatTraits
  {
    static const char* csv_base_name() { return "SiteUserStats"; }

    static const char* csv_header()
    {
      return "isp_sdate,colo_id,site_id,last_appearance_date,unique_users";
    }

    static std::ostream&
    write_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT& key,
      const BaseTraits::CollectorType::DataT::KeyT& inner_key,
      const BaseTraits::CollectorType::DataT::DataT& data)
    {
      write_key_as_csv(os, key) << ',';
      write_inner_key_as_csv(os, inner_key) << ',';
      return write_inner_data_as_csv(os, data);
    }

  private:
    static std::ostream&
    write_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT &key)
    {
      write_date_as_csv(os, key.isp_sdate()) << ',';
      return os << key.colo_id();
    }

    static std::ostream&
    write_inner_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::KeyT &key)
    {
      os << key.site_id() << ',';
      write_date_as_csv_2(os, key.last_appearance_date());
      return os;
    }

    static std::ostream&
    write_inner_data_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::DataT &data)
    {
      return os << data.unique_users();
    }
  };

  struct TagAuctionStatCsvTraits: TagAuctionStatTraits
  {
    static const char*
    csv_base_name() noexcept
    {
      return "TagAuctionStats";
    }

    static const char*
    csv_header() noexcept
    {
      return "pub_sdate,colo_id,"
        "tag_id,auction_ccg_count,"
        "requests";
    }

    static std::ostream&
    write_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT& key,
      const BaseTraits::CollectorType::DataT::KeyT& inner_key,
      const BaseTraits::CollectorType::DataT::DataT& data)
    {
      write_key_as_csv(os, key) << ',';
      write_inner_key_as_csv(os, inner_key) << ',';
      return write_inner_data_as_csv(os, data);
    }

  private:
    static std::ostream&
    write_key_as_csv(
      std::ostream& os,
      const BaseTraits::CollectorType::KeyT& key)
    {
      write_date_as_csv(os, key.pub_sdate()) << ',';
      return os << key.colo_id();
    }

    static std::ostream&
    write_inner_key_as_csv(
      std::ostream& os,
      const BaseTraits::CollectorType::DataT::KeyT& key)
    {
      return os << key.tag_id() << ',' << key.auction_ccg_count();
    }

    static std::ostream&
    write_inner_data_as_csv(
      std::ostream& os,
      const BaseTraits::CollectorType::DataT::DataT& data)
    {
      return os << data.requests();
    }
  };

  struct TagPositionStatCsvTraits: TagPositionStatTraits
  {
    static const char* csv_base_name() { return "TagPositionStats"; }

    static const char* csv_header()
    {
      return "pub_sdate,colo_id,tag_id,top_offset,left_offset,visibility,test,"
        "requests,imps,clicks";
    }

    static std::ostream&
    write_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT& key,
      const BaseTraits::CollectorType::DataT::KeyT& inner_key,
      const BaseTraits::CollectorType::DataT::DataT& data)
    {
      write_key_as_csv(os, key) << ',';
      write_inner_key_as_csv(os, inner_key) << ',';
      return write_inner_data_as_csv(os, data);
    }

  private:
    static std::ostream&
    write_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT &key)
    {
      write_date_as_csv(os, key.sdate()) << ',';
      return os << key.colo_id();
    }

    static std::ostream&
    write_inner_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::KeyT &key)
    {
      os << key.tag_id() << ',';
      write_optional_value_as_csv(os, key.top_offset()) << ',';
      write_optional_value_as_csv(os, key.left_offset()) << ',';
      write_optional_value_as_csv(os, key.visibility()) << ',';
      os << bool_to_char(key.test());
      return os;
    }

    static std::ostream&
    write_inner_data_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::DataT &data)
    {
      return os << data.requests() << ',' << data.imps() << ',' <<
        data.clicks();
    }
  };

  struct UserAgentStatCsvTraits: UserAgentStatTraits
  {
    static const char* csv_base_name() { return "UserAgentStats"; }

    static const char* csv_header()
    {
      return "sdate,user_agent,requests,channels,platforms";
    }

    static std::ostream&
    write_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT& key,
      const BaseTraits::CollectorType::DataT::KeyT& inner_key,
      const BaseTraits::CollectorType::DataT::DataT& data)
    {
      write_key_as_csv(os, key) << ',';
      write_inner_key_as_csv(os, inner_key) << ',';
      return write_inner_data_as_csv(os, data);
    }

  private:
    static std::ostream&
    write_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT &key)
    {
      return write_date_as_csv(os, key);
    }

    static std::ostream&
    write_inner_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::KeyT &key)
    {
      std::string user_agent;
      undisplayable_mime_encode(user_agent, key.user_agent());
      const std::size_t max_len = key.max_user_agent_length();
      return write_not_empty_string_as_csv(os, trim(user_agent, max_len));
    }

    static std::ostream&
    write_inner_data_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT::DataT &data
                            )
    {
      os << data.requests() << ',';
      write_list_as_csv(os, data.channels()) << ',';
      write_list_as_csv(os, data.platforms());
      return os;
    }
  };

  struct UserPropertiesCsvTraits: UserPropertiesTraits
  {
    static const char* csv_base_name() { return "UserPropertyStats"; }

    static const char* csv_header()
    {
      return "sdate,colo_id,property_name,property_value,isp_sdate,user_status,"
        "requests,imps,clicks,actions,imps_unverified,profiling_requests";
    }

    static std::ostream&
    write_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT& key,
      const BaseTraits::CollectorType::DataT& data)
    {
      write_key_as_csv(os, key) << ',';
      return write_data_as_csv(os, data);
    }

  private:
    static std::ostream&
    write_key_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT &key
                     )
    {
      write_date_as_csv(os, key.sdate()) << ',';
      os << key.colo_id() << ',';
      write_not_empty_string_as_csv(os, key.property_name()) << ',';
      write_string_as_csv(os, MimeCoder<>()(key.property_value(), 1024)) << ',';
      write_date_as_csv(os, key.isp_sdate()) << ',';
      return os << key.user_status();
    }

    static std::ostream&
    write_data_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::DataT &data)
    {
      os << data.requests() << ',' <<
        data.imps_verified() << ',' <<
        data.clicks() << ',' <<
        data.actions() << ',' <<
        data.imps_unverified() << ',' <<
        data.profiling_requests();
      return os;
    }
  };

  struct WebStatCsvTraits: WebStatTraits
  {
    static const char* csv_base_name() { return "WebStats-2"; }

    static const char* csv_header()
    {
      return "sdate,colo_id,ct,curct,browser,os,web_operation_id,result,"
        "user_status,test,tag_id,cc_id,count,source";
    }

    static std::ostream&
    write_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT& key,
      const BaseTraits::CollectorType::DataT::KeyT& inner_key,
      const BaseTraits::CollectorType::DataT::DataT& data)
    {
      write_date_as_csv(os, key.sdate()) << ',';
      os << key.colo_id() << ',';

      std::string encoded_str;
      undisplayable_mime_encode(encoded_str, inner_key.ct());
      write_string_as_csv(os, encoded_str) << ',';

      undisplayable_mime_encode(encoded_str, inner_key.curct());
      write_string_as_csv(os, encoded_str) << ',';

      write_string_as_csv(os, inner_key.browser()) << ',';
      write_string_as_csv(os, inner_key.os()) << ',';
      os << inner_key.web_operation_id() << ','
         << inner_key.result() << ','
         << inner_key.user_status() << ','
         << bool_to_char(inner_key.test()) << ',';
      write_optional_value_as_csv(os, inner_key.tag_id()) << ',';
      write_optional_value_as_csv(os, inner_key.cc_id()) << ',';

      os << data.count() << ',';
      write_string_as_csv(os, inner_key.source());

      return os;
    }
  };

  struct CampaignReferrerStatCsvTraits: CampaignReferrerStatTraits
  {
    static const char* csv_base_name() { return "CcgSiteReferrerStats"; }

    static const char* csv_header()
    {
      return "adv_sdate,ccg_id,cc_id,"
        "site_id,ext_tag_id,domain,"
        "imps,clicks,"
        "video_start,video_view,video_q1,video_mid,video_q3,video_complete,video_skip,video_pause,"
        "video_mute,video_unmute,video_resume,video_fullscreen,video_error,"
        "adv_amount,adv_comm_amount,pub_amount_adv,pub_comm_amount_adv,adv_payable_comm_amount,isp_amount_adv";
    }

    static std::ostream&
    write_as_csv(
      std::ostream &os,
      const BaseTraits::CollectorType::KeyT& key,
      const BaseTraits::CollectorType::DataT::KeyT& inner_key,
      const BaseTraits::CollectorType::DataT::DataT& data)
    {
      write_date_as_csv(os, key.adv_sdate()) << ',' <<
        inner_key.ccg_id() << ',' <<
        inner_key.cc_id() << ',' <<
        inner_key.site_id() << ',';
      write_string_as_csv(os, MimeCoder<>()(inner_key.ext_tag_id(), 50)) << ',';
      write_string_as_csv(os, inner_key.referer()) << ',';
      os << data.imps() << ',' <<
        data.clicks() << ',' <<
        data.video_start() << ',' <<
        data.video_view() << ',' <<
        data.video_q1() << ',' <<
        data.video_mid() << ',' <<
        data.video_q3() << ',' <<
        data.video_complete() << ',' <<
        data.video_skip() << ',' <<
        data.video_pause() << ',' <<
        data.video_mute() << ',' <<
        data.video_unmute() << ',' <<
        data.video_resume() << ',' <<
        data.video_fullscreen() << ',' <<
        data.video_error() << ',' <<
        data.adv_amount() << ',' <<
        data.adv_comm_amount() << ',' <<
        data.pub_amount_adv() << ',' <<
        data.pub_comm_amount_adv() << ',' <<
        data.adv_payable_comm_amount() << ',' <<
        data.isp_amount_adv();

      return os;
    }
  };
} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_LOG_TYPE_CSV_TRAITS_HPP */
