#ifndef _AD_SERVER_CAMPAIGN_SVCS_CAMPAIGNMANAGERLOGGER_HPP_
#define _AD_SERVER_CAMPAIGN_SVCS_CAMPAIGNMANAGERLOGGER_HPP_

#include <iomanip>
#include <sstream>
#include <optional>

#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <Generics/GnuHashTable.hpp>
#include <HTTP/UrlAddress.hpp>

#include <Commons/StringHolder.hpp>
#include <Commons/UserInfoManip.hpp>
#include <Commons/Containers.hpp>
#include <LogCommons/LogHolder.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <Commons/LogReferrerUtils.hpp>
#include "CampaignConfig.hpp"

namespace AdServer
{
  namespace CampaignSvcs
  {
    struct RequestBasicChannelsFlushTraits:
      public AdServer::LogProcessing::LogFlushTraits
    {
      unsigned long inventory_users_percentage;
      unsigned long distrib_count;
      bool dump_channel_triggers;
      bool adrequest_anonymize;
    };

    /** CampaignManagerLogger
     * facade that contains all logging logic
     */
    class CampaignManagerLogger:
      public virtual AdServer::LogProcessing::CompositeLogHolder
    {
    public:
      typedef std::set<unsigned long> CCGIdSet;

      struct Params
      {
        Params()
        : profiling_research_record_limit(5000),
          profiling_log_sampling(0)
        {}

        Commons::LogReferrer::Setting log_referrer_setting;

        AdServer::LogProcessing::LogFlushTraits channel_trigger_stat;
        AdServer::LogProcessing::LogFlushTraits channel_hit_stat;
        RequestBasicChannelsFlushTraits request_basic_channels;
        AdServer::LogProcessing::LogFlushTraits opt_out_stat;
        AdServer::LogProcessing::LogFlushTraits creative_stat;
        AdServer::LogProcessing::LogFlushTraits action_request;

        AdServer::LogProcessing::LogFlushTraits request;
        AdServer::LogProcessing::LogFlushTraits impression;
        AdServer::LogProcessing::LogFlushTraits click;
        AdServer::LogProcessing::LogFlushTraits advertiser_action;

        AdServer::LogProcessing::LogFlushTraits passback_impression;

        AdServer::LogProcessing::LogFlushTraits user_properties;

        AdServer::LogProcessing::LogFlushTraits tag_request;
        AdServer::LogProcessing::LogFlushTraits tag_position_stat;
        AdServer::LogProcessing::LogFlushTraits ccg_stat;
        AdServer::LogProcessing::LogFlushTraits cc_stat;

        AdServer::LogProcessing::LogFlushTraits search_term_stat;
        AdServer::LogProcessing::LogFlushTraits search_engine_stat;
        AdServer::LogProcessing::LogFlushTraits tag_auction_stat;
        AdServer::LogProcessing::LogFlushTraits passback_stat;
        AdServer::LogProcessing::LogFlushTraits user_agent_stat;

        AdServer::LogProcessing::LogFlushTraits prof_research;
        AdServer::LogProcessing::LogFlushTraits web_stat;
        AdServer::LogProcessing::LogFlushTraits research_web_stat;

        size_t profiling_research_record_limit;
        float  profiling_log_sampling;
      };

      struct CMPChannel
      {
        unsigned long channel_id;
        unsigned long channel_rate_id;
        RevenueDecimal imp_revenue;
        RevenueDecimal imp_sys_revenue;
        RevenueDecimal adv_imp_revenue;
        RevenueDecimal click_revenue;
        RevenueDecimal click_sys_revenue;
        RevenueDecimal adv_click_revenue;
      };

      typedef std::list<CMPChannel> CMPChannelList;

      struct TriggeredChannelsData
      {
        ChannelIdHashSet url_channels;
        ChannelIdHashSet page_channels;
        ChannelIdHashSet search_channels;
        ChannelIdHashSet url_keyword_channels;
        ChannelIdHashSet uid_channels;
      };

      struct TriggeredChannels:
        public TriggeredChannelsData,
        public ReferenceCounting::AtomicImpl
      {
      protected:
        virtual ~TriggeredChannels() noexcept
        {}
      };

      typedef ReferenceCounting::SmartPtr<TriggeredChannels>
        TriggeredChannels_var;

      typedef Generics::GnuHashTable<
        AdServer::Commons::StringHolderHashAdapter, TriggeredChannels_var>
        StringTriggerChannelMap;

      typedef std::map<unsigned long, unsigned long> TriggerChannelMap;

      typedef std::optional<unsigned long> UserIdHashMod;

      struct ActionInfo
      {
        ActionInfo(
          const Generics::Time& time_val,
          const AdServer::Commons::RequestId& request_id_val,
          const UserIdHashMod& user_id_hash_mod_val)
          : time(time_val),
            request_id(request_id_val),
            user_id_hash_mod(user_id_hash_mod_val)
        {}

        Generics::Time time;
        AdServer::Commons::RequestId request_id;
        UserIdHashMod user_id_hash_mod;
      };

      struct ImpressionInfo: public ActionInfo
      {
        ImpressionInfo(
          const Generics::Time& time_val,
          const AdServer::Commons::RequestId& request_id_val,
          const UserIdHashMod& user_id_hash_mod_val,
          RequestVerificationType verify_type_val,
          RevenueType pub_imp_revenue_type_val,
          const std::optional<RevenueDecimal>& pub_imp_revenue_val,
          const AdServer::Commons::UserId& user_id_val,
          const char* referrer_val,
          int viewability_val,
          const String::SubString& action_name_val)
          : ActionInfo(time_val, request_id_val, user_id_hash_mod_val),
            verify_type(verify_type_val),
            pub_imp_revenue_type(pub_imp_revenue_type_val),
            pub_imp_revenue(pub_imp_revenue_val),
            user_id(user_id_val),
            referrer(referrer_val),
            viewability(viewability_val),
            action_name(action_name_val.str())
        {}

        RequestVerificationType verify_type;
        RevenueType pub_imp_revenue_type;
        std::optional<RevenueDecimal> pub_imp_revenue;
        AdServer::Commons::UserId user_id;
        std::string referrer;
        int viewability;
        std::string action_name;
      };

      struct ClickInfo: public ActionInfo
      {
        ClickInfo(
          const Generics::Time& time_val,
          const AdServer::Commons::RequestId& request_id_val,
          const UserIdHashMod& user_id_hash_mod_val,
          const char* referrer_val)
          : ActionInfo(time_val, request_id_val, user_id_hash_mod_val),
            referer(referrer_val)
        {}

        std::string referer;
      };

      typedef std::list<unsigned long> CCGIdList;

      struct AdvActionInfo
      {
        Generics::Time time;
        AdServer::Commons::UserId action_request_id;
        UserStatus user_status;
        AdServer::Commons::UserId user_id;
        CCGIdList ccg_ids;
        std::optional<unsigned long> action_id;
        std::optional<unsigned long> device_channel_id;
        std::string order_id;
        RevenueDecimal action_value;
        std::string country;
        std::string referer;
        bool optout;
        bool log_as_test;

        unsigned long colo_id;
        std::string ip_hash;
      };

      struct AnonymousRequestInfo
      {
        Generics::Time time;
        Generics::Time isp_time;
        Generics::Time isp_time_offset;
        unsigned long colo_id;
        UserStatus user_status;
        bool log_as_test;

        unsigned long search_engine_id;
        Commons::StringHolder_var search_words;
        std::string client_app;
        std::string client_app_version;
        std::string full_platform;
        std::string web_browser;
        Commons::StringHolder_var user_agent;
        std::string search_engine_host;
        std::string country_code;
        bool page_keywords_present;

        ChannelIdHashSet platforms;
        ChannelIdSet platform_channels;
      };

      struct RequestInfo : public AnonymousRequestInfo
      {
        typedef std::set<std::string> KeywordSet;
        typedef std::vector<AdServer::Commons::StringHolder_var> KeywordVector;
        typedef std::list<unsigned long> KeywordIdList;
        typedef RevenueDecimal CommisionDecimal;

        bool is_ad_request;

        AdServer::Commons::RequestId request_id;
        AdServer::Commons::UserId household_id;
        AdServer::Commons::UserId user_id;
        AdServer::Commons::UserId merged_user_id;
        std::optional<AdServer::Commons::UserId> request_user_id;
        std::optional<AdServer::CampaignSvcs::UserStatus> request_user_status;

        bool fraud;
        bool disable_fraud_detection;
        bool track_passback;

        // geo_channels: ordered in accuracy desc (coord channels after non coord)
        ChannelIdList geo_channels;

        // Triggers from channels except discover and keyword channels
        TriggerChannelMap page_triggers;
        TriggerChannelMap search_triggers;
        TriggerChannelMap url_triggers;
        TriggerChannelMap url_keyword_triggers;
        // Same triggers as above, but got for discover and keyword channels only
        TriggerChannelMap discover_keyword_page_triggers;
        TriggerChannelMap discover_keyword_search_triggers;
        TriggerChannelMap discover_keyword_url_triggers;
        TriggerChannelMap discover_keyword_url_keyword_triggers;
        TriggeredChannelsData triggered_channels; // union of unique triggered channels

        ChannelIdArray hid_history_channels;

        // history channels excluding discover
        ChannelIdArray history_channels;
        unsigned long last_platform_channel_id;

        std::optional<unsigned long> full_referer_hash;
        std::optional<unsigned long> short_referer_hash;
        std::string referer;
        std::list<std::string> urls;
        bool profile_referer;

        KeywordIdList hit_keywords;

        std::string ip_hash;

        // required for research profiling logging, process_ad_request
        Commons::StringHolder_var page_keywords;
        Commons::StringHolder_var url_keywords;
        unsigned long random;
      };

      struct AdSelectionInfo
      {
        struct Revenue
        {
          Revenue()
            : rate_id(0),
              request(RevenueDecimal::ZERO),
              impression(RevenueDecimal::ZERO),
              click(RevenueDecimal::ZERO),
              action(RevenueDecimal::ZERO)
          {}

          Revenue& operator*=(const RevenueDecimal& op)
          {
            request = RevenueDecimal::mul(request, op, Generics::DMR_FLOOR);
            impression = RevenueDecimal::mul(impression, op, Generics::DMR_FLOOR);
            click = RevenueDecimal::mul(click, op, Generics::DMR_FLOOR);
            action = RevenueDecimal::mul(action, op, Generics::DMR_FLOOR);
            return *this;
          }

          Revenue& operator/=(const RevenueDecimal& divider)
          {
            request = RevenueDecimal::div(request, divider);
            impression = RevenueDecimal::div(impression, divider);
            click = RevenueDecimal::div(click, divider);
            action = RevenueDecimal::div(action, divider);
            return *this;
          }

          Revenue& div(const RevenueDecimal& divider, Revenue& reminder)
          {
            request = RevenueDecimal::div(request, divider, reminder.request);
            impression = RevenueDecimal::div(impression, divider, reminder.impression);
            click = RevenueDecimal::div(click, divider, reminder.click);
            action = RevenueDecimal::div(action, divider, reminder.action);
            return *this;
          }

          Revenue& operator+=(const Revenue& add)
          {
            request += add.request;
            impression += add.impression;
            click += add.click;
            action += add.action;
            return *this;
          }

          Revenue& operator-=(const Revenue& sub)
          {
            request -= sub.request;
            impression -= sub.impression;
            click -= sub.click;
            action -= sub.action;
            return *this;
          }

          Revenue& from_system_currency(const AdInstances::Currency& currency)
          {
            request = currency.from_system_currency(request);
            impression = currency.from_system_currency(impression);
            click = currency.from_system_currency(click);
            action = currency.from_system_currency(action);
            return  *this;
          }

          Revenue
          to_system_currency(const AdInstances::Currency& currency)
          {
            Revenue ret;
            ret.request = currency.to_system_currency(request);
            ret.impression = currency.to_system_currency(impression);
            ret.click = currency.to_system_currency(click);
            ret.action = currency.to_system_currency(action);
            return  *this;
          }

          Revenue
          convert_currency(
            const AdInstances::Currency& from_currency,
            const AdInstances::Currency& to_currency)
          {
            Revenue ret;
            ret.request = to_currency.convert(&from_currency, request);
            ret.impression = to_currency.convert(&from_currency, impression);
            ret.click = to_currency.convert(&from_currency, click);
            ret.action = to_currency.convert(&from_currency, action);
            return ret;
          }

          std::ostream& print(std::ostream& out) const noexcept
          {
            out << "request = " << request <<
              ", impression = " << impression <<
              ", click = " << click <<
              ", action = " << action;
            return out;
          }

          unsigned long rate_id;
          RevenueDecimal request;
          RevenueDecimal impression;
          RevenueDecimal click;
          RevenueDecimal action;
        };

        typedef std::list<RevenueDecimal> CTRList;

        Generics::Time adv_time;

        bool ad_selected;
        AdServer::Commons::RequestId request_id;

        bool log_as_test;

        unsigned long cc_id;
        unsigned long campaign_id;
        unsigned long ccg_id;
        unsigned long ctr_reset_id;
        unsigned long adv_account_id;
        unsigned long advertiser_id;
        unsigned long tag_delivery_threshold;
        bool has_custom_actions;

        unsigned long currency_exchange_id;

        RevenueDecimal ecpm;
        RevenueDecimal ecpm_bid;
        // bid that sent to publisher (RTB)
        RevenueDecimal external_ecpm_bid;

        unsigned long tag_ecpm;

        RevenueDecimal adv_currency_rate;
        Revenue adv_revenue;
        Revenue adv_payable_comm_revenue;
        Revenue adv_comm_revenue;

        RevenueDecimal pub_currency_rate;
        RevenueDecimal pub_commission;
        Revenue pub_revenue;
        Revenue pub_comm_revenue;

        RevenueDecimal isp_currency_rate;
        Revenue isp_revenue;
        RevenueDecimal isp_revenue_share;

        unsigned long num_shown;
        unsigned long position;

        ChannelIdList channels;
        std::string expression;

        bool text_campaign;
        unsigned long ccg_keyword_id;
        unsigned long keyword_channel_id;

        bool enabled_notice;
        bool enabled_impression_tracking;
        bool enabled_action_tracking;

        CMPChannelList cmp_channels;

        CCGIdSet keyword_ccg_ids;
        std::string ctr_algorithm_id;
        RevenueDecimal ctr;
        std::string conv_rate_algorithm_id;
        RevenueDecimal conv_rate;
        unsigned long campaign_imps;
        CTRList model_ctrs;
        RevenueDecimal adv_commission;
        RevenueDecimal self_service_commission;
        RevenueDecimal pub_cost_coef;
        unsigned long at_flags;
      };

      typedef std::list<AdSelectionInfo> AdSelectionInfoList;

      typedef std::list<unsigned long> CCIdList;

      struct AdRequestSelectionInfo
      {
        struct TimedId
        {
          TimedId(const Generics::Time& adv_time_val, unsigned long id_val)
            : adv_time(adv_time_val), id(id_val)
          {}

          unsigned long get_id() const noexcept { return id; }

          Generics::Time adv_time;
          unsigned long id;
        };


        typedef std::list<TimedId> TimedIdList;

        Generics::Time pub_time;
        unsigned long pub_account_id;
        unsigned long site_id;
        unsigned long request_tag_id; // original tag_id defined in request
        unsigned long tag_id;
        unsigned long size_id;
        unsigned long site_rate_id;
        std::string ext_tag_id;
        std::optional<unsigned long> page_load_id;
        std::optional<unsigned long> tag_visibility;
        std::optional<unsigned long> tag_top_offset;
        std::optional<unsigned long> tag_left_offset;
        unsigned long max_ads;
        std::string tag_size;
        std::set<std::string> tag_sizes;
        RevenueDecimal cpm_threshold; // publisher currency
        RevenueDecimal floor_cost;
        bool walled_garden;
        bool household_based;
        AuctionType auction_type;

        unsigned long min_no_adv_ecpm;
        unsigned long min_text_ecpm;
        bool text_campaigns;
        AdSelectionInfoList ad_selection_info_list;

        // adv date is key:
        TimedIdList lost_auction_ccgs;
        TimedIdList lost_auction_creatives;

        long tag_predicted_viewability;
      };

      struct PassbackInfo
      {
        PassbackInfo(
          const AdServer::Commons::RequestId& request_id_val,
          const Generics::Time& time_val,
          const UserIdHashMod& user_id_hash_mod_val)
          : request_id(request_id_val),
            time(time_val),
            user_id_hash_mod(user_id_hash_mod_val)
        {}

        AdServer::Commons::RequestId request_id;
        Generics::Time time;
        UserIdHashMod user_id_hash_mod;
      };

      struct PassbackTrackInfo
      {
        Generics::Time time;
        std::string country;
        bool log_as_test;
        unsigned long currency_exchange_id;
        unsigned long colo_id;
        unsigned long colo_rate_id;
        unsigned long tag_id;
        unsigned long site_rate_id;
        AdServer::CampaignSvcs::UserStatus user_status;
      };

      struct WebOperationInfo
      {
        enum WebFlags
        {
          LOG_HOUR = 0x01,
          LOG_TAG_ID = 0x02,
          LOG_CC_ID = 0x04,
          LOG_CT = 0x08,
          LOG_CURT = 0x10,
          LOG_BROWSER = 0x20,
          LOG_OS = 0x40
        };

        typedef std::list<AdServer::Commons::RequestId>
          RequestIdList;

        void init_by_flags(
          const Generics::Time& time_in,
          const char* ct_in,
          const char* curt_in,
          const char* browser_in,
          const char* os_in,
          unsigned int flags)
          noexcept;

        Generics::Time time;
        Generics::Time full_time;
        AdServer::Commons::RequestId global_request_id;
        RequestIdList request_ids;
        unsigned long colo_id;
        unsigned long tag_id;
        unsigned long cc_id;
        std::string ct;
        std::string curct;
        std::string browser;
        std::string os;
        unsigned long web_operation_id;
        std::string app;
        std::string source;
        std::string operation;
        std::string user_bind_src;
        char result;
        AdServer::CampaignSvcs::UserStatus user_status;
        bool test_request;
        std::string referer;
        std::string ip_address;
        std::string external_user_id;
        std::string user_agent;
      };

      struct CommonMatchRequestInfo
      {
        unsigned long colo_id;

        ChannelIdSet channels;
        ChannelIdHashSet triggered_page_channels;
        TriggerChannelMap page_triggers;

        ChannelIdSet hid_channels;
      };

      struct MatchRequestInfo
      {
        AdServer::Commons::UserId user_id;
        AdServer::Commons::UserId household_id;
        Generics::Time time;
        Generics::Time isp_offset;
        CommonMatchRequestInfo match_info;
      };

    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      CampaignManagerLogger(const Params& log_params, Logging::Logger* logger)
        /*throw(Exception)*/;

      void
      process_request(
        const RequestInfo& request_info,
        unsigned long profiling_type = PT_ALL)
        /*throw(Exception)*/;

      void process_ad_request(
        const RequestInfo& request_info,
        const AdRequestSelectionInfo& ad_request_selection_info)
        /*throw(Exception)*/;

      void process_impression(const ImpressionInfo& action_info)
        /*throw(Exception)*/;

      void process_click(const ClickInfo& action_info)
        /*throw(Exception)*/;

      void process_action(const AdvActionInfo& action_info)
        /*throw(Exception)*/;

      void process_oo_operation(
        unsigned long colo_id,
        const Generics::Time& time,
        const Generics::Time& time_offset,
        const AdServer::Commons::UserId& user_id,
        bool log_as_test,
        char operation)
        /*throw(Exception)*/;

      void process_web_operation(
        const WebOperationInfo& web_op_info)
        /*throw(Exception)*/;

      void process_passback(const PassbackInfo& passback_info)
        /*throw(Exception)*/;

      void process_passback_track(const PassbackTrackInfo& passback_track_info)
        /*throw(Exception)*/;

      void
      process_anon_request(
        const AnonymousRequestInfo& anon_request_info)
        /*throw(Exception)*/;

      void
      process_match_request(
        const MatchRequestInfo& match_request_info)
        /*throw(Exception)*/;

    public:
      class ChannelTriggerStatLogger;
      typedef
        ReferenceCounting::SmartPtr<ChannelTriggerStatLogger>
        ChannelTriggerStatLogger_var;

      class ChannelHitStatLogger;
      typedef
        ReferenceCounting::SmartPtr<ChannelHitStatLogger>
        ChannelHitStatLogger_var;

      class RequestBasicChannelsLogger;
      typedef
        ReferenceCounting::SmartPtr<RequestBasicChannelsLogger>
        RequestBasicChannelsLogger_var;

      class WebStatLogger;
      typedef ReferenceCounting::SmartPtr<WebStatLogger>
        WebStatLogger_var;

      class ResearchWebStatLogger;
      typedef ReferenceCounting::SmartPtr<ResearchWebStatLogger>
        ResearchWebStatLogger_var;

      class CreativeStatLogger;
      typedef
        ReferenceCounting::SmartPtr<CreativeStatLogger>
        CreativeStatLogger_var;

      class RequestLogger;
      typedef
        ReferenceCounting::SmartPtr<RequestLogger>
        RequestLogger_var;

      class ImpressionLogger;
      typedef
        ReferenceCounting::SmartPtr<ImpressionLogger>
        ImpressionLogger_var;

      class ClickLogger;
      typedef
        ReferenceCounting::SmartPtr<ClickLogger>
        ClickLogger_var;

      class AdvertiserActionLogger;
      typedef
        ReferenceCounting::SmartPtr<AdvertiserActionLogger>
        AdvertiserActionLogger_var;

      class ActionRequestLogger;
      typedef
        ReferenceCounting::SmartPtr<ActionRequestLogger>
        ActionRequestLogger_var;

      class PassbackImpressionLogger;
      typedef
        ReferenceCounting::SmartPtr<PassbackImpressionLogger>
        PassbackImpressionLogger_var;

      class UserPropertiesLogger;
      typedef ReferenceCounting::SmartPtr<UserPropertiesLogger>
        UserPropertiesLogger_var;

      class TagRequestLogger;
      typedef ReferenceCounting::SmartPtr<TagRequestLogger>
        TagRequestLogger_var;

      class TagPositionStatLogger;
      typedef ReferenceCounting::SmartPtr<TagPositionStatLogger>
        TagPositionStatLogger_var;

      class CcgStatLogger;
      typedef ReferenceCounting::SmartPtr<CcgStatLogger>
        CcgStatLogger_var;

      class CcStatLogger;
      typedef ReferenceCounting::SmartPtr<CcStatLogger>
        CcStatLogger_var;

      class SearchTermStatLogger;
      typedef ReferenceCounting::SmartPtr<SearchTermStatLogger>
        SearchTermStatLogger_var;

      class SearchEngineStatLogger;
      typedef ReferenceCounting::SmartPtr<SearchEngineStatLogger>
        SearchEngineStatLogger_var;

      class TagAuctionStatLogger;
      typedef ReferenceCounting::SmartPtr<TagAuctionStatLogger>
        TagAuctionStatLogger_var;

      class PassbackStatLogger;
      typedef ReferenceCounting::SmartPtr<PassbackStatLogger>
        PassbackStatLogger_var;

      class UserAgentStatLogger;
      typedef ReferenceCounting::SmartPtr<UserAgentStatLogger>
        UserAgentStatLogger_var;

      class ProfilingResearchLogger;
      typedef ReferenceCounting::SmartPtr<ProfilingResearchLogger>
        ProfilingResearchLogger_var;

    protected:
      virtual
      ~CampaignManagerLogger() noexcept;

    private:
      Logging::Logger_var logger_;
      ChannelTriggerStatLogger_var channel_trigger_stat_logger_;
      ChannelHitStatLogger_var channel_hit_stat_logger_;
      RequestBasicChannelsLogger_var request_basic_channels_logger_;
      WebStatLogger_var web_stat_logger_;
      ResearchWebStatLogger_var research_web_stat_logger_;

      CreativeStatLogger_var creative_stat_logger_; // only for requests counter logging

      ActionRequestLogger_var action_request_logger_;

      RequestLogger_var request_logger_;
      ImpressionLogger_var impression_logger_;
      ClickLogger_var click_logger_;
      AdvertiserActionLogger_var  advertiser_action_logger_;

      PassbackImpressionLogger_var passback_impression_logger_;
      //UserPropertiesLogger_var user_properties_logger_;

      TagRequestLogger_var tag_request_logger_;
      TagPositionStatLogger_var tag_position_stat_logger_;

      CcgStatLogger_var ccg_stat_logger_;
      CcStatLogger_var cc_stat_logger_;

      SearchTermStatLogger_var search_term_stat_logger_;
      SearchEngineStatLogger_var search_engine_stat_logger_;
      TagAuctionStatLogger_var tag_auction_stat_logger_;

      PassbackStatLogger_var passback_stat_logger_;
      UserAgentStatLogger_var user_agent_stat_logger_;
      ProfilingResearchLogger_var profiling_research_logger_;
    };

    typedef ReferenceCounting::SmartPtr<CampaignManagerLogger>
      CampaignManagerLogger_var;

  } // namespace CampaignSvcs
} // namespace AdServer

#endif /*_AD_SERVER_CAMPAIGN_SVCS_CAMPAIGNMANAGERLOGGER_HPP_*/

