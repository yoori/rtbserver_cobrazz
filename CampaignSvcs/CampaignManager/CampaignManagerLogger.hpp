#pragma once

#include <cstdint>
#include <iomanip>
#include <memory>
#include <sstream>

#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/GnuHashTable.hpp>
#include <HTTP/UrlAddress.hpp>

#include <Commons/StringHolder.hpp>
#include <Commons/UserInfoManip.hpp>
#include <Commons/Containers.hpp>
#include <LogCommons/LogHolder.hpp>
#include <LogCommons/RequestBasicChannels.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <Commons/LogReferrerUtils.hpp>
#include "CampaignConfig.hpp"

namespace AdServer::CampaignSvcs
{
  struct RequestBasicChannelsFlushTraits:
    public AdServer::LogProcessing::LogFlushTraits
  {
    double inventory_users_percentage;
    unsigned long distrib_count;
    bool dump_channel_triggers;
    bool adrequest_anonymize;
  };

  /** CampaignManagerLogger
   * facade that contains all logging logic
   */
  class CampaignManagerLogger:
    public Generics::RefCountableCompositeActiveObject,
    public virtual AdServer::LogProcessing::CompositeLogHolder
  {
  public:
    typedef std::set<unsigned long> CCGIdSet;

    struct Stats
    {
      std::uint64_t request_in_progress = 0;
      std::uint64_t queue_size = 0;
      std::uint64_t processing_requests = 0;
    };

    struct Params
    {
      Params();

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
      unsigned long threads;
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
      ChannelIdHashSet channels;
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
      virtual ~TriggeredChannels() noexcept;
    };

    typedef ReferenceCounting::SmartPtr<TriggeredChannels>
      TriggeredChannels_var;

    typedef Generics::GnuHashTable<
      AdServer::Commons::StringHolderHashAdapter, TriggeredChannels_var>
      StringTriggerChannelMap;

    typedef AdServer::LogProcessing::RequestBasicChannelsInnerData::
      TriggerMatchList TriggerChannelMap;

    typedef AdServer::Commons::Optional<unsigned long> UserIdHashMod;

    struct ActionInfo
    {
      ActionInfo(
        const Generics::Time& time_val,
        const AdServer::Commons::RequestId& request_id_val,
        const UserIdHashMod& user_id_hash_mod_val);

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
        const Commons::Optional<RevenueDecimal>& pub_imp_revenue_val,
        const AdServer::Commons::UserId& user_id_val,
        const char* referrer_val,
        int viewability_val,
        const String::SubString& action_name_val);

      RequestVerificationType verify_type;
      RevenueType pub_imp_revenue_type;
      Commons::Optional<RevenueDecimal> pub_imp_revenue;
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
        const char* referrer_val);

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
      AdServer::Commons::Optional<unsigned long> action_id;
      AdServer::Commons::Optional<unsigned long> device_channel_id;
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
      Commons::Optional<AdServer::Commons::UserId> request_user_id;
      Commons::Optional<AdServer::CampaignSvcs::UserStatus> request_user_status;

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

      Commons::Optional<unsigned long> full_referer_hash;
      Commons::Optional<unsigned long> short_referer_hash;
      std::string referer;
      std::list<std::string> urls;
      bool profile_referer;

      KeywordIdList hit_keywords;

      std::string ip_hash;

      // required for research profiling logging, process_ad_request
      Commons::StringHolder_var page_keywords;
      Commons::StringHolder_var url_keywords;
      std::string additional_info;
      unsigned long random;
    };

    struct AdSelectionInfo
    {
      struct Revenue
      {
        Revenue();

        Revenue&
        operator*=(const RevenueDecimal& op);

        Revenue&
        operator/=(const RevenueDecimal& divider);

        Revenue&
        div(const RevenueDecimal& divider, Revenue& reminder);

        Revenue&
        operator+=(const Revenue& add);

        Revenue&
        operator-=(const Revenue& sub);

        Revenue&
        from_system_currency(const AdInstances::Currency& currency);

        Revenue
        to_system_currency(const AdInstances::Currency& currency);

        Revenue
        convert_currency(
          const AdInstances::Currency& from_currency,
          const AdInstances::Currency& to_currency);

        std::ostream&
        print(std::ostream& out) const noexcept;

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
        TimedId(const Generics::Time& adv_time_val, unsigned long id_val);

        unsigned long
        get_id() const noexcept;

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
      Commons::Optional<unsigned long> page_load_id;
      Commons::Optional<unsigned long> tag_visibility;
      Commons::Optional<unsigned long> tag_top_offset;
      Commons::Optional<unsigned long> tag_left_offset;
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
        const UserIdHashMod& user_id_hash_mod_val);

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
      std::shared_ptr<const RequestInfo> request_info,
      unsigned long profiling_type = PT_ALL)
      /*throw(Exception)*/;

    void process_ad_request(
      std::shared_ptr<const RequestInfo> request_info,
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
    process_match_request(const MatchRequestInfo& match_request_info)
      /*throw(Exception)*/;

    void
    wait_processing();

    Stats
    get_stats() const noexcept;

  protected:
    virtual
    ~CampaignManagerLogger() noexcept;

  private:
    class Impl;

  private:
    std::shared_ptr<Impl> impl_;
  };

  using CampaignManagerLogger_var = ReferenceCounting::SmartPtr<CampaignManagerLogger>;

} // namespace AdServer::CampaignSvcs
