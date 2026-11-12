#pragma once

#include <cstdint>
#include <iomanip>
#include <memory>
#include <scoped_allocator>
#include <sstream>
#include <utility>
#include <vector>

#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/GnuHashTable.hpp>
#include <HTTP/UrlAddress.hpp>

#include <Commons/StringHolder.hpp>
#include <Commons/SharedMonoAllocator.hpp>
#include <Commons/UserInfoManip.hpp>
#include <Commons/Containers.hpp>
#include <Generics/MonoAllocator.hpp>
#include <LogCommons/LogHolder.hpp>
#include <LogCommons/RequestBasicChannels.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <Commons/LogReferrerUtils.hpp>
#include "CampaignManagerCore.hpp"
#include "CampaignConfig.hpp"

namespace AdServer::CampaignSvcs
{
  struct RequestBasicChannelsFlushTraits: public AdServer::LogProcessing::LogFlushTraits
  {
    double inventory_sampling;
    double user_navigation_sampling = 100.0;
    double user_trigger_match_sampling = 100.0;
    double channel_hits_sampling = 100.0;
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
      std::uint64_t queue_total = 0;
      std::uint64_t processing_requests = 0;
      std::uint64_t processing_requests_total = 0;
    };

    struct Params
    {
      Params();

      Commons::LogReferrer::Setting log_referrer_setting;

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
      TriggeredChannelsData();
      explicit TriggeredChannelsData(std::shared_ptr<Generics::MonoAllocatorArena> arena);
      TriggeredChannelsData(const TriggeredChannelsData& other);
      TriggeredChannelsData(
        const TriggeredChannelsData& other, std::shared_ptr<Generics::MonoAllocatorArena> arena);
      TriggeredChannelsData(TriggeredChannelsData&&) noexcept;
      TriggeredChannelsData& operator=(const TriggeredChannelsData& other);
      TriggeredChannelsData& operator=(TriggeredChannelsData&& other) noexcept;
      void swap(TriggeredChannelsData& other) noexcept;

    private:
      std::shared_ptr<Generics::MonoAllocatorArena> arena_;

    public:
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

    using TriggerChannelMap =
      AdServer::LogProcessing::RequestBasicChannelsInnerData::TriggerMatchArray;
    using ArenaTriggerChannelMap = Generics::MonoVector<TriggerChannelMap::value_type>;

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

    struct NavigationInfo
    {
      Generics::Time time;
      unsigned long colo_id = 0;
      AdServer::Commons::UserId user_id;
      std::string referer;
      std::string page_keywords;
      bool log_as_test = false;
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
        const String::SubString& action_name_val,
        const String::SubString& action_value_val = String::SubString());

      RequestVerificationType verify_type;
      RevenueType pub_imp_revenue_type;
      Commons::Optional<RevenueDecimal> pub_imp_revenue;
      AdServer::Commons::UserId user_id;
      std::string referrer;
      int viewability;
      std::string action_name;
      std::string action_value;
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
      AdServer::Commons::Optional<std::uint32_t> action_id;
      AdServer::Commons::Optional<std::uint32_t> device_channel_id;
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
      AnonymousRequestInfo();
      explicit AnonymousRequestInfo(std::shared_ptr<Generics::MonoAllocatorArena> arena);
      AnonymousRequestInfo(const AnonymousRequestInfo& other);
      AnonymousRequestInfo(
        const AnonymousRequestInfo& other, std::shared_ptr<Generics::MonoAllocatorArena> arena);
      AnonymousRequestInfo(AnonymousRequestInfo&&) noexcept;
      AnonymousRequestInfo& operator=(const AnonymousRequestInfo& other);
      AnonymousRequestInfo& operator=(AnonymousRequestInfo&& other) noexcept;
      void swap(AnonymousRequestInfo& other) noexcept;

      const std::shared_ptr<Generics::MonoAllocatorArena>& arena() const noexcept;

    private:
      std::shared_ptr<Generics::MonoAllocatorArena> arena_;

    public:
      Generics::Time time;
      Generics::Time isp_time;
      Generics::Time isp_time_offset;
      unsigned long colo_id = 0;
      UserStatus user_status = US_UNDEFINED;
      bool log_as_test = false;

      unsigned long search_engine_id = 0;
      Commons::StringHolder_var search_words;
      Generics::MonoString client_app;
      Generics::MonoString client_app_version;
      Generics::MonoString full_platform;
      Generics::MonoString web_browser;
      Commons::StringHolder_var user_agent;
      Generics::MonoString search_engine_host;
      Generics::MonoString country_code;
      bool page_keywords_present = false;

      ChannelIdHashSet platforms;
      Generics::MonoSet<unsigned long> platform_channels;
    };

    struct RequestInfo : public AnonymousRequestInfo
    {
      RequestInfo();
      explicit RequestInfo(std::shared_ptr<Generics::MonoAllocatorArena> arena);
      RequestInfo(const RequestInfo& other);
      RequestInfo(const RequestInfo& other, std::shared_ptr<Generics::MonoAllocatorArena> arena);
      RequestInfo(RequestInfo&&) noexcept;
      RequestInfo& operator=(const RequestInfo& other);
      RequestInfo& operator=(RequestInfo&& other) noexcept;
      void swap(RequestInfo& other) noexcept;

      using KeywordSet = std::set<std::string>;
      using KeywordVector = std::vector<AdServer::Commons::StringHolder_var>;
      using KeywordIdList = Generics::MonoList<unsigned long>;
      using CommisionDecimal = RevenueDecimal;

      bool is_ad_request = false;

      AdServer::Commons::RequestId request_id;
      AdServer::Commons::UserId household_id;
      AdServer::Commons::UserId user_id;
      AdServer::Commons::UserId merged_user_id;
      Generics::MonoString external_id;
      Commons::Optional<AdServer::Commons::UserId> request_user_id;
      Commons::Optional<AdServer::CampaignSvcs::UserStatus> request_user_status;

      bool fraud = false;
      bool disable_fraud_detection = false;
      bool track_passback = false;

      // geo_channels: ordered in accuracy desc (coord channels after non coord)
      Generics::MonoVector<unsigned long> geo_channels;

      // Triggers from matched channels
      ArenaTriggerChannelMap page_triggers;
      ArenaTriggerChannelMap search_triggers;
      ArenaTriggerChannelMap url_triggers;
      ArenaTriggerChannelMap url_keyword_triggers;
      TriggeredChannelsData triggered_channels; // union of unique triggered channels

      Generics::MonoVector<unsigned long> hid_history_channels;

      // history channels excluding discover
      Generics::MonoVector<unsigned long> history_channels;
      unsigned long last_platform_channel_id = 0;

      Commons::Optional<unsigned long> full_referer_hash;
      Commons::Optional<unsigned long> short_referer_hash;
      Generics::MonoString referer;
      CampaignManagerCore::ArenaStringArray urls;
      bool profile_referer = false;

      KeywordIdList hit_keywords;

      Generics::MonoString ip_hash;

      // required for research profiling logging, process_ad_request
      Commons::StringHolder_var page_keywords;
      Commons::StringHolder_var url_keywords;
      Generics::MonoString additional_info;
      unsigned long random = 0;
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

      AdSelectionInfo();
      explicit AdSelectionInfo(std::shared_ptr<Generics::MonoAllocatorArena> arena);
      AdSelectionInfo(const AdSelectionInfo& other);
      AdSelectionInfo(
        const AdSelectionInfo& other, std::shared_ptr<Generics::MonoAllocatorArena> arena);
      AdSelectionInfo(AdSelectionInfo&&) noexcept;
      AdSelectionInfo& operator=(const AdSelectionInfo& other);
      AdSelectionInfo& operator=(AdSelectionInfo&& other);
      void swap(AdSelectionInfo& other) noexcept;
      const std::shared_ptr<Generics::MonoAllocatorArena>& arena() const noexcept;

      using allocator_type = Commons::SharedMonoAllocator<char>;
      explicit AdSelectionInfo(const allocator_type& allocator);
      AdSelectionInfo(const AdSelectionInfo& other, const allocator_type& allocator);
      AdSelectionInfo(AdSelectionInfo&& other, const allocator_type& allocator);

    private:
      std::shared_ptr<Generics::MonoAllocatorArena> arena_;

    public:
      using CTRList = Generics::MonoList<RevenueDecimal>;

      Generics::Time adv_time;

      bool ad_selected = false;
      AdServer::Commons::RequestId request_id;

      bool log_as_test = false;

      unsigned long cc_id = 0;
      unsigned long campaign_id = 0;
      unsigned long ccg_id = 0;
      unsigned long ctr_reset_id = 0;
      unsigned long adv_account_id = 0;
      unsigned long advertiser_id = 0;
      unsigned long tag_delivery_threshold = 0;
      bool has_custom_actions = false;

      unsigned long currency_exchange_id = 0;

      RevenueDecimal ecpm = RevenueDecimal::ZERO;
      RevenueDecimal ecpm_bid = RevenueDecimal::ZERO;
      // bid that sent to publisher (RTB)
      RevenueDecimal external_ecpm_bid = RevenueDecimal::ZERO;

      unsigned long tag_ecpm = 0;

      RevenueDecimal adv_currency_rate = RevenueDecimal::ZERO;
      Revenue adv_revenue;
      Revenue adv_payable_comm_revenue;
      Revenue adv_comm_revenue;

      RevenueDecimal pub_currency_rate = RevenueDecimal::ZERO;
      RevenueDecimal pub_commission = RevenueDecimal::ZERO;
      Revenue pub_revenue;
      Revenue pub_comm_revenue;

      RevenueDecimal isp_currency_rate = RevenueDecimal::ZERO;
      Revenue isp_revenue;
      RevenueDecimal isp_revenue_share = RevenueDecimal::ZERO;

      unsigned long num_shown = 0;
      unsigned long position = 0;

      Generics::MonoList<unsigned long> channels;
      Generics::MonoString expression;

      bool text_campaign = false;
      unsigned long ccg_keyword_id = 0;
      unsigned long keyword_channel_id = 0;

      bool enabled_notice = false;
      bool enabled_impression_tracking = false;
      bool enabled_action_tracking = false;

      Generics::MonoList<CMPChannel> cmp_channels;

      Generics::MonoSet<unsigned long> keyword_ccg_ids;
      Generics::MonoString ctr_algorithm_id;
      RevenueDecimal ctr = RevenueDecimal::ZERO;
      Generics::MonoString conv_rate_algorithm_id;
      RevenueDecimal conv_rate = RevenueDecimal::ZERO;
      std::shared_ptr<const std::vector<std::string>> expected_post_actions =
        empty_expected_post_actions();
      unsigned long campaign_imps = 0;
      CTRList model_ctrs;
      RevenueDecimal adv_commission = RevenueDecimal::ZERO;
      RevenueDecimal self_service_commission = RevenueDecimal::ZERO;
      RevenueDecimal pub_cost_coef = RevenueDecimal::ZERO;
      unsigned long at_flags = 0;
    };

    using AdSelectionInfoList = std::list<AdSelectionInfo,
      std::scoped_allocator_adaptor<Commons::SharedMonoAllocator<AdSelectionInfo>>>;
    using ArenaStringSet = std::set<Generics::MonoString, std::less<>,
      std::scoped_allocator_adaptor<Generics::MonoAllocator<Generics::MonoString>>>;

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


      AdRequestSelectionInfo();
      explicit AdRequestSelectionInfo(std::shared_ptr<Generics::MonoAllocatorArena> arena);
      AdRequestSelectionInfo(const AdRequestSelectionInfo& other);
      AdRequestSelectionInfo(
        const AdRequestSelectionInfo& other, std::shared_ptr<Generics::MonoAllocatorArena> arena);
      AdRequestSelectionInfo(AdRequestSelectionInfo&&) noexcept;
      AdRequestSelectionInfo& operator=(const AdRequestSelectionInfo& other);
      AdRequestSelectionInfo& operator=(AdRequestSelectionInfo&& other) noexcept;
      void swap(AdRequestSelectionInfo& other) noexcept;
      const std::shared_ptr<Generics::MonoAllocatorArena>& arena() const noexcept;

    private:
      std::shared_ptr<Generics::MonoAllocatorArena> arena_;

    public:
      using TimedIdList = Generics::MonoList<TimedId>;

      Generics::Time pub_time;
      unsigned long pub_account_id = 0;
      unsigned long site_id = 0;
      unsigned long request_tag_id = 0; // original tag_id defined in request
      unsigned long tag_id = 0;
      unsigned long size_id = 0;
      unsigned long site_rate_id = 0;
      Generics::MonoString ext_tag_id;
      Commons::Optional<unsigned long> page_load_id;
      Commons::Optional<unsigned long> tag_visibility;
      Commons::Optional<unsigned long> tag_top_offset;
      Commons::Optional<unsigned long> tag_left_offset;
      unsigned long max_ads = 0;
      Generics::MonoString tag_size;
      ArenaStringSet tag_sizes;
      RevenueDecimal cpm_threshold = RevenueDecimal::ZERO; // publisher currency
      RevenueDecimal floor_cost = RevenueDecimal::ZERO;
      bool walled_garden = false;
      bool household_based = false;
      AuctionType auction_type = AT_MAX_ECPM;

      unsigned long min_no_adv_ecpm = 0;
      unsigned long min_text_ecpm = 0;
      bool text_campaigns = false;
      AdSelectionInfoList ad_selection_info_list;

      // adv date is key:
      TimedIdList lost_auction_ccgs;
      TimedIdList lost_auction_creatives;

      long tag_predicted_viewability = 0;
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
      CommonMatchRequestInfo()
        : arena_(std::make_shared<Generics::MonoAllocatorArena>()),
          triggered_page_channels(arena_.get())
      {}

    private:
      std::shared_ptr<Generics::MonoAllocatorArena> arena_;

    public:
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
    set_campaign_config(AdInstances::ConstCampaignConfigPtr campaign_config);

    void
    process_request(
      std::shared_ptr<const RequestInfo> request_info,
      unsigned long profiling_type = PT_ALL)
      /*throw(Exception)*/;

    void
    process_request(
      const Colocation* colocation,
      std::shared_ptr<const CampaignManagerCore::GetAdRequest> request_params,
      CampaignManagerCore::AdSlotContext&& ad_slot_context,
      unsigned long profiling_type = PT_ALL,
      std::shared_ptr<const ChannelIdArray> geo_channels = {},
      bool reset_request_user = false)
      /*throw(Exception)*/;

    void
    process_request(
      const Colocation* colocation,
      CampaignManagerCore::CommonAdRequest&& common_info,
      CampaignManagerCore::ContextAdRequest&& context_info,
      std::shared_ptr<const CampaignManagerCore::GetAdRequest> request_params,
      CampaignManagerCore::AdSlotContext&& ad_slot_context,
      unsigned long profiling_type = PT_ALL,
      std::shared_ptr<const ChannelIdArray> geo_channels = {},
      bool reset_request_user = false)
      /*throw(Exception)*/;

    struct AdRequestSlotLog
    {
      ReferenceCounting::ConstPtr<Tag> tag;
      Tag::SizeMap tag_sizes;
      CampaignManagerCore::AdSlotContext ad_slot_context;
      CampaignManagerCore::AdSlotMinCpm ad_slot_min_cpm;
      bool disable_impression_tracking = false;
      std::shared_ptr<const CampaignManagerCore::AdSlotRequest> ad_slot;
      AdSelectionResult ad_selection_result;
    };

    using AdRequestSlotLogArray = std::vector<AdRequestSlotLog>;

    void process_ad_request(
      const Colocation* colocation,
      std::shared_ptr<const CampaignManagerCore::GetAdRequest> request_params,
      AdRequestSlotLogArray&& ad_slots,
      std::shared_ptr<const ChannelIdArray> geo_channels = {},
      bool reset_request_user = false)
      /*throw(Exception)*/;

    void process_impression(const ImpressionInfo& action_info)
      /*throw(Exception)*/;

    void process_click(const ClickInfo& action_info)
      /*throw(Exception)*/;

    void process_action(const AdvActionInfo& action_info)
      /*throw(Exception)*/;

    void process_navigation(const NavigationInfo& navigation_info)
      /*throw(Exception)*/;

    void process_oo_operation(
      unsigned long colo_id,
      const Generics::Time& time,
      const Generics::Time& time_offset,
      const AdServer::Commons::UserId& user_id,
      bool log_as_test,
      char operation)
      /*throw(Exception)*/;

    void process_web_operation(const WebOperationInfo& web_op_info)
      /*throw(Exception)*/;

    void process_passback(const PassbackInfo& passback_info)
      /*throw(Exception)*/;

    void process_passback_track(const PassbackTrackInfo& passback_track_info)
      /*throw(Exception)*/;

    void
    process_anon_request(const AnonymousRequestInfo& anon_request_info)
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
    void process_ad_request_(
      std::shared_ptr<const RequestInfo> request_info,
      const AdRequestSelectionInfo& ad_request_selection_info)
      /*throw(Exception)*/;

    class Impl;

  private:
    std::shared_ptr<Impl> impl_;
  };

  using CampaignManagerLogger_var = ReferenceCounting::SmartPtr<CampaignManagerLogger>;

} // namespace AdServer::CampaignSvcs
