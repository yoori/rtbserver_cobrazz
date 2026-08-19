#pragma once

#include <string>
#include <set>
#include <string_view>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>
#include <unordered_set>

#include <boost/unordered/unordered_flat_map.hpp>

#include <eh/Exception.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/PtrHolder.hpp>
#include <Logger/Logger.hpp>
#include <Generics/ActiveObject.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <String/StringManip.hpp>

#include <Commons/IPCrypter.hpp>
#include <Commons/Coro/Awaitable.hpp>
#include <Commons/SecToken.hpp>
#include <Commons/TextTemplateAsyncCache.hpp>
#include <LogCommons/AdRequestLogger.hpp>

#include <xsd/CampaignSvcs/CampaignManagerConfig.hpp>

#include "DomainParser.hpp"
#include "CampaignManagerDeclarations.hpp"
#include "CampaignConfig.hpp"
#include "CampaignIndex.hpp"
#include "CreativeTemplate.hpp"
#include "CampaignSelector.hpp"
#include "CampaignConfigSource.hpp"
#include "CreativeInstantiatorTypes.hpp"
#include "PassbackTemplate.hpp"
#include "BillingStateContainer.hpp"

#include "CTRProvider.hpp"
#include "CTRProviderImpl.hpp"
#include "BidCostProvider.hpp"

namespace AdServer::Commons
{
  class ExecutorPool;
}

namespace AdServer::CampaignSvcs
{
  using namespace AdServer::CampaignSvcs::AdInstances;

  class CampaignManagerLogger;

  namespace AdInstantiateRule
  {
    const String::SubString UNSECURE("unsecure"); // always using in the preview mode
    const String::SubString SECURE("secure");
  }

  class CampaignManagerCore :
    public virtual ReferenceCounting::AtomicImpl,
    public virtual Generics::CompositeActiveObject,
    public virtual Generics::RefCountableActiveObject
  {
  public:
    static const unsigned long PARALLEL_TASKS_COUNT = 5;

    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(InvalidArgument, Exception);
    DECLARE_EXCEPTION(NotReady, Exception);
    DECLARE_EXCEPTION(CreativeInstantiateProblem, Exception);
    DECLARE_EXCEPTION(CreativeTemplateProblem, CreativeInstantiateProblem);
    DECLARE_EXCEPTION(CreativeOptionsProblem, CreativeInstantiateProblem);

  public:
    struct CreativeParams;

    using ByteArray = std::vector<unsigned char>;
    using IdArray = std::vector<unsigned long>;
    using StringArray = std::vector<std::string>;
    using RequestIdArray = std::vector<AdServer::Commons::RequestId>;

    struct TokenInfo
    {
      std::string name;
      std::string value;
    };

    using TokenArray = std::vector<TokenInfo>;

    struct TokenImageInfo
    {
      std::string name;
      std::string value;
      unsigned long width = 0;
      unsigned long height = 0;
    };

    using TokenImageArray = std::vector<TokenImageInfo>;

    struct NativeDataTokenInfo
    {
      std::string name;
      bool required = false;
    };

    struct NativeImageTokenInfo
    {
      std::string name;
      bool required = false;
      unsigned long width = 0;
      unsigned long height = 0;
    };

    struct GeoInfo
    {
      std::string country;
      std::string region;
      std::string city;
    };

    struct GeoCoordInfo
    {
      CoordDecimal longitude;
      CoordDecimal latitude;
      CoordDecimal accuracy;
    };

    struct ChannelSearchResult
    {
      unsigned long channel_id = 0;
      unsigned long use_count = 0;
      IdArray matched_simple_channels;
      IdArray ccg_ids;
      std::string discover_query;
      std::string language;
    };

    struct CategoryChannelNodeInfo
    {
      unsigned long channel_id = 0;
      std::string name;
      unsigned long flags = 0;
      std::vector<CategoryChannelNodeInfo> child_category_channels;
    };

    struct ColocationFlagsInfo
    {
      unsigned long colo_id = 0;
      unsigned long flags = 0;
      unsigned long hid_profile = 0;
    };

    enum class OptOperation
    {
      IN,
      OUT,
      STATUS,
      FORCED_IN
    };

    struct PassbackRequest
    {
      AdServer::Commons::RequestId request_id;
      Generics::Time time;
      AdServer::Commons::Optional<unsigned long> user_id_hash_mod;
    };

    struct PassbackTrackRequest
    {
      Generics::Time time;
      std::string country;
      unsigned long colo_id = 0;
      unsigned long tag_id = 0;
      unsigned long user_status = 0;
    };

    struct WebOperationRequest
    {
      Generics::Time time;
      unsigned long colo_id = 0;
      unsigned long tag_id = 0;
      unsigned long cc_id = 0;
      std::string ct;
      std::string curct;
      std::string browser;
      std::string os;
      std::string app;
      std::string source;
      std::string operation;
      std::string user_bind_src;
      char result = 0;
      unsigned long user_status = 0;
      bool test_request = false;
      RequestIdArray request_ids;
      AdServer::Commons::RequestId global_request_id;
      std::string referer;
      std::string ip_address;
      std::string external_user_id;
      std::string user_agent;
    };

    struct ActionRequest
    {
      Generics::Time time;
      bool test_request = false;
      bool log_as_test = false;
      std::optional<unsigned long> campaign_id;
      std::optional<unsigned long> action_id;
      std::string order_id;
      std::optional<RevenueDecimal> action_value;
      std::string referer;
      unsigned long user_status = 0;
      AdServer::Commons::UserId user_id;
      std::string ip_hash;
      IdArray platform_ids;
      std::string peer_ip;
      std::vector<GeoInfo> location;
    };

    struct ClickRequest
    {
      Generics::Time time;
      Generics::Time bid_time;
      unsigned long colo_id = 0;
      unsigned long tag_id = 0;
      unsigned long tag_size_id = 0;
      unsigned long ccid = 0;
      unsigned long ccg_keyword_id = 0;
      unsigned long creative_id = 0;
      AdServer::Commons::UserId match_user_id;
      AdServer::Commons::UserId cookie_user_id;
      AdServer::Commons::RequestId request_id;
      AdServer::Commons::Optional<unsigned long> user_id_hash_mod;
      std::string relocate;
      std::string referer;
      bool log_click = false;
      RevenueDecimal ctr = RevenueDecimal::ZERO;
      TokenValueMap tokens;
    };

    struct ClickResult
    {
      std::string url;
      unsigned long campaign_id = 0;
      unsigned long advertiser_id = 0;
    };

    struct TrackCreativeInfo
    {
      unsigned long ccid = 0;
      unsigned long ccg_keyword_id = 0;
      AdServer::Commons::RequestId request_id;
      unsigned long creative_id = 0;
      RevenueDecimal ctr = RevenueDecimal::ZERO;
    };

    using TrackCreativeArray = std::vector<TrackCreativeInfo>;

    struct VerifyImpressionRequest
    {
      Generics::Time time;
      Generics::Time bid_time;
      AdServer::Commons::Optional<unsigned long> user_id_hash_mod;
      TrackCreativeArray creatives;
      unsigned long pub_imp_revenue_type = 0;
      RevenueDecimal pub_imp_revenue = RevenueDecimal::ZERO;
      unsigned long request_type = 0;
      unsigned long verify_type = 0;
      AdServer::Commons::UserId user_id;
      std::string referer;
      long viewability = 0;
      std::string action_name;
    };

    struct TrackCreativeResultInfo
    {
      unsigned long campaign_id = 0;
      unsigned long advertiser_id = 0;
    };

    using VerifyImpressionResult = std::vector<TrackCreativeResultInfo>;

    struct ChannelTriggerMatchInfo
    {
      unsigned long channel_trigger_id = 0;
      unsigned long channel_id = 0;
    };

    using ChannelTriggerMatchArray = std::vector<ChannelTriggerMatchInfo>;

    struct TriggerMatchResultInfo
    {
      ChannelTriggerMatchArray url_channels;
      ChannelTriggerMatchArray pkw_channels;
      ChannelTriggerMatchArray skw_channels;
      ChannelTriggerMatchArray ukw_channels;
      IdArray uid_channels;
    };

    struct CCGKeywordInfo
    {
      unsigned long ccg_keyword_id = 0;
      unsigned long ccg_id = 0;
      unsigned long channel_id = 0;
      RevenueDecimal max_cpc = RevenueDecimal::ZERO;
      RevenueDecimal ctr = RevenueDecimal::ZERO;
      std::string click_url;
      std::string original_keyword;
    };

    using CCGKeywordArray = std::vector<CCGKeywordInfo>;

    struct CommonMatchRequestInfo
    {
      IdArray channels;
      ChannelTriggerMatchArray pkw_channels;
      IdArray hid_channels;
      unsigned long colo_id = 0;
      std::vector<GeoInfo> location;
      std::vector<GeoCoordInfo> coord_location;
      std::string full_referer;
    };

    struct MatchRequestInfo
    {
      AdServer::Commons::UserId user_id;
      AdServer::Commons::UserId household_id;
      Generics::Time request_time;
      CommonMatchRequestInfo match_info;
      std::string source;
    };

    struct PreviewCreativeParams
    {
      unsigned long ccid = 0;
      unsigned long tag_id = 0;
      std::string format;
      std::string original_url;
      std::string peer_ip;
    };

    struct CommonAdRequest
    {
      CommonAdRequest() = default;

      explicit CommonAdRequest(std::shared_ptr<Generics::MonoAllocatorArena> arena) noexcept
        : arena_(std::move(arena))
      {}

      CommonAdRequest(const CommonAdRequest&) = delete;
      CommonAdRequest& operator=(const CommonAdRequest&) = delete;
      CommonAdRequest(CommonAdRequest&&) noexcept = default;
      CommonAdRequest& operator=(CommonAdRequest&&) noexcept = default;

    private:
      std::shared_ptr<Generics::MonoAllocatorArena> arena_;

    public:
      Generics::Time time;
      AdServer::Commons::RequestId request_id;
      std::string creative_instantiate_type;
      unsigned long request_type = 0;
      unsigned long random = 0;
      bool test_request = false;
      bool log_as_test = false;
      unsigned long colo_id = 0;
      std::string external_user_id;
      std::string source_id;
      std::vector<GeoInfo> location;
      std::vector<GeoCoordInfo> coord_location;
      std::string full_referer;
      std::string referer;
      StringArray urls;
      std::string security_token;
      std::string pub_impr_track_url;
      std::string pub_param;
      std::string preclick_url;
      std::string click_prefix_url;
      std::string original_url;
      AdServer::Commons::UserId track_user_id;
      AdServer::Commons::UserId user_id;
      unsigned long user_status = 0;
      std::string signed_user_id;
      std::string peer_ip;
      std::string user_agent;
      std::string cohort;
      unsigned long hpos = 0;
      std::string ext_track_params;
      TokenArray tokens;
      bool set_cookie = false;
      std::string passback_type;
      std::string passback_url;
    };

    struct ContextAdRequest
    {
      ContextAdRequest() = default;

      explicit ContextAdRequest(std::shared_ptr<Generics::MonoAllocatorArena> arena)
        : arena_(arena ? std::move(arena) : std::make_shared<Generics::MonoAllocatorArena>()),
          client(Generics::mono_allocator<char>(arena_.get())),
          client_version(Generics::mono_allocator<char>(arena_.get())),
          platform_ids(Generics::mono_allocator<unsigned long>(arena_.get())),
          geo_channels(Generics::mono_allocator<unsigned long>(arena_.get())),
          platform(Generics::mono_allocator<char>(arena_.get())),
          full_platform(Generics::mono_allocator<char>(arena_.get())),
          web_browser(Generics::mono_allocator<char>(arena_.get())),
          ip_hash(Generics::mono_allocator<char>(arena_.get())),
          additional_info(Generics::mono_allocator<char>(arena_.get()))
      {}

      ContextAdRequest(const ContextAdRequest&) = delete;
      ContextAdRequest& operator=(const ContextAdRequest&) = delete;
      ContextAdRequest(ContextAdRequest&&) noexcept = default;
      ContextAdRequest& operator=(ContextAdRequest&&) noexcept = default;

    private:
      std::shared_ptr<Generics::MonoAllocatorArena> arena_;

    public:
      bool enabled_notice = false;
      Generics::MonoString client;
      Generics::MonoString client_version;
      Generics::MonoVector<unsigned long> platform_ids;
      Generics::MonoVector<unsigned long> geo_channels;
      Generics::MonoString platform;
      Generics::MonoString full_platform;
      Generics::MonoString web_browser;
      Generics::MonoString ip_hash;
      bool profile_referer = false;
      unsigned long page_load_id = 0;
      unsigned long full_referer_hash = 0;
      unsigned long short_referer_hash = 0;
      Generics::MonoString additional_info;
    };

    using CommonAdRequestPtr = std::shared_ptr<CommonAdRequest>;
    using ConstCommonAdRequestPtr = std::shared_ptr<const CommonAdRequest>;
    using ContextAdRequestPtr = std::shared_ptr<ContextAdRequest>;
    using ConstContextAdRequestPtr = std::shared_ptr<const ContextAdRequest>;
    using ContextAdRequestPtrArray = std::vector<ContextAdRequestPtr>;

    struct InstantiateAdRequest
    {
      CommonAdRequestPtr common_info;
      ContextAdRequestPtrArray context_info;
      TrackCreativeArray creatives;

      std::string format;
      unsigned long publisher_site_id = 0;
      unsigned long publisher_account_id = 0;
      unsigned long tag_id = 0;
      unsigned long tag_size_id = 0;
      AdServer::Commons::Optional<unsigned long> user_id_hash_mod;
      AdServer::Commons::UserId merged_user_id;
      IdArray pubpixel_accounts;
      std::string open_price;
      std::string openx_price;
      std::string liverail_price;
      std::string google_price;
      std::string ext_tag_id;
      unsigned long video_width = 0;
      unsigned long video_height = 0;
      bool consider_request = false;
      bool enabled_notice = false;
      bool emulate_click = false;
      std::optional<RevenueDecimal> pub_imp_revenue;
    };

    struct InstantiateAdResult
    {
      std::string creative_body;
      std::string mime_format;
      RequestIdArray request_ids;
    };

    struct AdSlotRequest
    {
      AdSlotRequest() = default;

      explicit AdSlotRequest(std::shared_ptr<Generics::MonoAllocatorArena> arena) noexcept
        : arena_(std::move(arena))
      {}

      AdSlotRequest(const AdSlotRequest&) = delete;
      AdSlotRequest& operator=(const AdSlotRequest&) = delete;
      AdSlotRequest(AdSlotRequest&&) noexcept = default;
      AdSlotRequest& operator=(AdSlotRequest&&) noexcept = default;

    private:
      std::shared_ptr<Generics::MonoAllocatorArena> arena_;

    public:
      unsigned long ad_slot_id = 0;
      std::string format;
      unsigned long tag_id = 0;
      StringArray sizes;
      std::string ext_tag_id;
      RevenueDecimal min_ecpm = RevenueDecimal::ZERO;
      std::string min_ecpm_currency_code;
      StringArray currency_codes;
      bool passback = false;
      StringArray exclude_categories;
      StringArray required_categories;
      long up_expand_space = -1;
      long right_expand_space = -1;
      long left_expand_space = -1;
      long down_expand_space = -1;
      long tag_visibility = -1;
      long tag_predicted_viewability = -1;
      unsigned long video_min_duration = 0;
      long video_max_duration = -1;
      long video_skippable_max_duration = -1;
      long video_allow_skippable = 1;
      long video_allow_unskippable = 1;
      unsigned long video_width = 0;
      unsigned long video_height = 0;
      unsigned long debug_ccg = 0;
      IdArray allowed_durations;
      std::vector<NativeDataTokenInfo> native_data_tokens;
      std::vector<NativeImageTokenInfo> native_image_tokens;
      unsigned long native_ads_impression_tracker_type = 0;
      bool fill_track_html = false;
      TokenArray tokens;
    };

    using AdSlotRequestPtr = std::shared_ptr<AdSlotRequest>;
    using ConstAdSlotRequestPtr = std::shared_ptr<const AdSlotRequest>;
    using AdSlotRequestPtrArray = std::vector<AdSlotRequestPtr>;

    struct ConfigRequestInfo
    {
      bool geo_channels = false;
    };

    struct SeqOrderInfo
    {
      unsigned long ccg_id = 0;
      unsigned long set_id = 0;
      unsigned long imps = 0;
    };

    struct CampaignFreqInfo
    {
      unsigned long campaign_id = 0;
      unsigned long imps = 0;
    };

    struct LogAdRequest
    {
      unsigned long profiling_type = 0;
      unsigned long search_engine_id = 0;
      bool page_keywords_present = false;
      bool disable_fraud_detection = false;
      bool fraud = false;
      TriggerMatchResultInfo trigger_match_result;
      std::string search_words;
      std::string page_keywords;
      std::string url_keywords;
      AdServer::Commons::UserId merged_user_id;
    };

    struct GetAdRequest
    {
      static constexpr std::size_t ARENA_INITIAL_SIZE = 64 * 1024;

      using SeqOrderArray = Generics::MonoVector<SeqOrderInfo>;
      using CampaignFreqArray = Generics::MonoVector<CampaignFreqInfo>;

      GetAdRequest()
        : GetAdRequest(
            std::make_shared<Generics::MonoAllocatorArena>(ARENA_INITIAL_SIZE))
      {}

      explicit GetAdRequest(std::shared_ptr<Generics::MonoAllocatorArena> arena)
        : arena_(arena ?
            std::move(arena) :
            std::make_shared<Generics::MonoAllocatorArena>(ARENA_INITIAL_SIZE)),
          ad_slots(
            Generics::mono_allocator<AdSlotRequestPtr>(arena_.get())),
          publisher_account_ids(
            Generics::mono_allocator<unsigned long>(arena_.get())),
          channels(
            Generics::mono_allocator<ChannelId>(arena_.get())),
          seq_orders(
            Generics::mono_allocator<SeqOrderInfo>(arena_.get())),
          campaign_freqs(
            Generics::mono_allocator<CampaignFreqInfo>(arena_.get())),
          full_freq_caps(
            Generics::mono_allocator<unsigned long>(arena_.get())),
          ccg_keywords(
            Generics::mono_allocator<CCGKeywordInfo>(arena_.get())),
          exclude_pubpixel_accounts(
            Generics::mono_allocator<unsigned long>(arena_.get()))
      {}

      GetAdRequest(const GetAdRequest&) = delete;
      GetAdRequest& operator=(const GetAdRequest&) = delete;

      GetAdRequest(GetAdRequest&&) noexcept = default;
      GetAdRequest& operator=(GetAdRequest&&) = delete;

      Generics::MonoAllocatorArena*
      resource() const noexcept
      {
        return arena_.get();
      }

      const std::shared_ptr<Generics::MonoAllocatorArena>&
      arena() const noexcept
      {
        return arena_;
      }

    private:
      std::shared_ptr<Generics::MonoAllocatorArena> arena_;

    public:
      CommonAdRequestPtr common_info;
      ContextAdRequestPtr context_info;
      Generics::MonoVector<AdSlotRequestPtr> ad_slots;

      bool need_debug_info = false;

      // select and filter ad parameters.
      unsigned long publisher_site_id = 0;
      Generics::MonoVector<unsigned long> publisher_account_ids;
      ChannelIdHashSet channels;
      SeqOrderArray seq_orders;
      CampaignFreqArray campaign_freqs;
      Generics::MonoVector<unsigned long> full_freq_caps;
      bool only_display_ad = false;
      bool profiling_available = false;
      unsigned long tag_delivery_factor = 0;
      unsigned long ccg_delivery_factor = 0;
      Generics::MonoVector<CCGKeywordInfo> ccg_keywords;
      Generics::Time client_create_time;
      Generics::Time session_start;

      // instantiate ad parameters.
      unsigned long ad_instantiate_type = 0;
      bool fill_track_pixel = false;
      bool fill_iurl = false;
      bool required_passback = false;
      Generics::MonoVector<unsigned long> exclude_pubpixel_accounts;
      unsigned long preview_ccid = 0;

      LogAdRequest log_request;
    };

    struct CreativeSelectDebugResult
    {
      RevenueDecimal imp_revenue = RevenueDecimal::ZERO;
      RevenueDecimal click_revenue = RevenueDecimal::ZERO;
      RevenueDecimal action_revenue = RevenueDecimal::ZERO;
      RevenueDecimal ecpm_bid = RevenueDecimal::ZERO;
      RevenueDecimal ctr = RevenueDecimal::ZERO;
      std::string ctr_algorithm_id;
      std::string html_url;
    };

    struct CreativeSelectResult
    {
      CreativeSelectResult() = default;
      CreativeSelectResult(const CreativeSelectResult&) = delete;
      CreativeSelectResult& operator=(const CreativeSelectResult&) = delete;
      CreativeSelectResult(CreativeSelectResult&&) noexcept = default;
      CreativeSelectResult& operator=(CreativeSelectResult&&) noexcept = default;

      AdServer::Commons::RequestId request_id;
      unsigned long ccid = 0;
      unsigned long cmp_id = 0;
      unsigned long campaign_group_id = 0;
      unsigned long order_set_id = 0;
      unsigned long advertiser_id = 0;
      std::string advertiser_name;
      std::string creative_size;
      RevenueDecimal revenue = RevenueDecimal::ZERO;
      RevenueDecimal ecpm = RevenueDecimal::ZERO;
      RevenueDecimal pub_ecpm = RevenueDecimal::ZERO;
      std::string click_url;
      std::string destination_url;
      std::string creative_version_id;
      unsigned long creative_id = 0;
      bool https_safe_flag = false;
      unsigned char expanding = 0;
    };

    struct AdSlotDebugResult
    {
      unsigned long tag_id = 0;
      unsigned long tag_size_id = 0;
      unsigned long site_id = 0;
      unsigned long site_rate_id = 0;
      unsigned long min_no_adv_ecpm = 0;
      unsigned long min_text_ecpm = 0;
      unsigned long auction_type = 0;
      std::string track_pixel_url;
      RevenueDecimal cpm_threshold = RevenueDecimal::ZERO;
      bool walled_garden = false;
      std::vector<CreativeSelectDebugResult> selected_creatives;
      std::string trace_ccg;
    };

    struct ContractInfo
    {
      ContractInfo() = default;
      ContractInfo(const ContractInfo&) = delete;
      ContractInfo& operator=(const ContractInfo&) = delete;
      ContractInfo(ContractInfo&&) noexcept = default;
      ContractInfo& operator=(ContractInfo&&) noexcept = default;

      unsigned long contract_id = 0;
      std::string number;
      std::string date;
      std::string type;
      bool vat_included = false;
      std::string ord_contract_id;
      std::string ord_ado_id;
      std::string subject_type;
      std::string action_type;
      bool agent_acting_for_publisher = false;
      unsigned long parent_contract_id = 0;
      std::string client_id;
      std::string client_name;
      std::string client_legal_form;
      std::string contractor_id;
      std::string contractor_name;
      std::string contractor_legal_form;
      Generics::Time timestamp;
    };

    struct ExtContractInfo
    {
      ContractInfo contract_info;
      std::string parent_contract_id;
    };

    struct GetAdSlotResult
    {
      unsigned long ad_slot_id = 0;
      AdServer::Commons::RequestId request_id;
      bool passback = false;
      std::string passback_url;
      std::string creative_body;
      std::string notice_url;
      StringArray track_pixel_urls;
      std::string yandex_track_params;
      std::string creative_url;
      std::string track_pixel_params;
      std::string click_params;
      std::string mime_format;
      std::string iurl;
      bool test_request = false;
      std::vector<CreativeSelectResult> selected_creatives;
      StringArray external_visual_categories;
      StringArray external_content_categories;
      std::string pub_currency_code;
      unsigned long overlay_width = 0;
      unsigned long overlay_height = 0;
      TokenArray tokens;
      TokenArray ext_tokens;
      bool track_impr = false;
      std::string tag_size;
      IdArray freq_caps;
      IdArray uc_freq_caps;
      AdSlotDebugResult debug_info;
      TokenArray native_data_tokens;
      TokenImageArray native_image_tokens;
      std::string track_html_body;
      std::string erid;
      std::vector<ExtContractInfo> contracts;
    };

    struct GetAdDebugResult
    {
      unsigned long colo_id = 0;
      IdArray geo_channels;
      IdArray platform_channels;
      unsigned long last_platform_channel_id = 0;
      unsigned long user_group_id = 0;
    };

    struct GetAdResult
    {
      std::string hostname;
      std::vector<GetAdSlotResult> ad_slots;
      Generics::Time process_time;
      GetAdDebugResult debug_info;
    };

  public:
    using CampaignManagerConfig = xsd::AdServer::Configuration::CampaignManagerType;

    CampaignManagerCore(
      const CampaignManagerConfig& configuration,
      const DomainParser::DomainConfig& domain_config,
      Generics::ActiveObjectCallback* callback,
      Logging::Logger* logger,
      std::shared_ptr<AdServer::Commons::ExecutorPool> executor_pool,
      CampaignManagerLogger* campaign_manager_logger,
      const CreativeInstantiatorTypes::CreativeInstantiate& creative_instantiate,
      const char* campaigns_types)
      /*throw(InvalidArgument, Exception, eh::Exception)*/;

    BillingStateContainer::Stats
    billing_server_stats() const noexcept;

    AdServer::Commons::Awaitable<GetAdResult>
    co_get_campaign_creative(GetAdRequest&& request_params);

    void process_match_request(const MatchRequestInfo& match_request_info)
      /*throw(Exception, NotReady)*/;

    AdServer::Commons::Awaitable<ByteArray>
    co_get_file(std::string file_name)
      /*throw(Exception)*/;

    AdServer::Commons::Awaitable<InstantiateAdResult>
    co_instantiate_ad(InstantiateAdRequest&& instantiate_ad_info)
      /*throw(Exception, NotReady)*/;

    std::string trace_campaign_selection_index()
      /*throw(Exception)*/;

    bool get_campaign_creative_by_ccid(
      const PreviewCreativeParams& params,
      std::string& creative_body)
      /*throw(Exception)*/;

    void consider_passback(const PassbackRequest& in)
      /*throw(Exception)*/;

    void consider_passback_track(const PassbackTrackRequest& in)
      /*throw(Exception, NotReady)*/;

    AdServer::Commons::Awaitable<bool>
    co_get_click_url(const ClickRequest& click_info, ClickResult& click_result_info)
      /*throw(Exception, NotReady)*/;

    AdServer::Commons::Awaitable<VerifyImpressionResult>
    co_verify_impression(const VerifyImpressionRequest& impression_info)
      /*throw(Exception, NotReady)*/;

    void action_taken(const ActionRequest& action_info)
      /*throw(Exception, NotReady)*/;

    std::vector<ChannelSearchResult>
    get_channel_links(const IdArray& channels, bool match)
      /*throw(Exception)*/;

    std::vector<CategoryChannelNodeInfo>
    get_category_channels(const std::string& language)
      /*throw(NotReady, Exception)*/;

    void
    verify_opt_operation(
      unsigned long time,
      long colo_id,
      const std::string& referer,
      OptOperation operation,
      unsigned long status,
      unsigned long user_status,
      bool log_as_test,
      const std::string& browser,
      const std::string& os,
      const std::string& ct,
      const std::string& curct,
      const AdServer::Commons::UserId& user_id)
      /*throw(NotReady)*/;

    void
    consider_web_operation(const WebOperationRequest& web_op_info)
      /*throw(InvalidArgument, NotReady)*/;

    StringArray
    get_pub_pixels(
      const std::string& country,
      unsigned long user_status,
      const IdArray& publisher_account_ids)
      /*throw(NotReady, Exception)*/;

    // config state
    bool ready() /*throw(eh::Exception)*/;

    ConstCampaignConfigPtr
    get_config(const ConfigRequestInfo& get_config_props)
      /*throw(Exception)*/;

    void
    progress_comment(std::string& res) /*throw(eh::Exception)*/;

    std::vector<ColocationFlagsInfo>
    get_colocation_flags()
      /*throw(NotReady, Exception)*/;

  private:
    virtual
    ~CampaignManagerCore() noexcept;

  public:
    struct ImageToken
    {
      std::string value;
      unsigned long width;
      unsigned long height;
    };

    struct RequestResultParams
    {
      RequestResultParams()
        : RequestResultParams(std::make_shared<Generics::MonoAllocatorArena>())
      {}

      explicit RequestResultParams(std::shared_ptr<Generics::MonoAllocatorArena> arena)
        : arena_(arena ? std::move(arena) : std::make_shared<Generics::MonoAllocatorArena>()),
          hit_keywords(Generics::mono_allocator<
            CampaignKeywordMap::value_type>(arena_.get())),
          overlay_width(0),
          overlay_height(0)
      {}

    private:
      std::shared_ptr<Generics::MonoAllocatorArena> arena_;

    public:
      Commons::RequestId request_id;
      CampaignKeywordMap hit_keywords;
      std::string track_pixel_url;
      std::string track_html_url;
      std::string track_html_body;
      StringList add_track_pixel_urls;
      std::string yandex_track_params;
      std::string mime_format;
      unsigned long overlay_width;
      unsigned long overlay_height;
      std::string notice_url;
      std::string track_pixel_params;
      std::string click_params;
      std::string iurl;
      TokenValueMap tokens;
      TokenValueMap ext_tokens;
      TokenValueMap native_data_tokens;
      StringFlatMap<ImageToken> native_image_tokens;
    };

    struct CreativeParams
    {
      std::string click_url;
    };

    using CreativeParamsList = std::list<CreativeParams>;

    struct AdSlotMinCpm
    {
      RevenueDecimal min_pub_ecpm;
      RevenueDecimal min_pub_ecpm_system;

      AdSlotMinCpm() noexcept
        : min_pub_ecpm(RevenueDecimal::ZERO),
          min_pub_ecpm_system(RevenueDecimal::ZERO)
      {}
    };

    struct AdSlotContext
    {
      static constexpr std::size_t ARENA_INITIAL_SIZE = 8 * 1024;

    private:
      std::shared_ptr<Generics::MonoAllocatorArena> arena_;

    public:
      explicit AdSlotContext(
        std::shared_ptr<Generics::MonoAllocatorArena> arena =
          nullptr)
        : arena_(arena ?
            std::move(arena) :
            std::make_shared<Generics::MonoAllocatorArena>(
              ARENA_INITIAL_SIZE)),
          test_request(false),
          full_freq_caps(arena_.get()),
          result_full_freq_caps(arena_.get()),
          request_blacklisted(false),
          publisher_account_id(0),
          request_tag_id(0),
          adsspace_system_cpm(RevenueDecimal(false, 100000, 0))
      {}

      bool test_request;
      std::string passback_url;
      FreqCapIdSet full_freq_caps;
      FreqCapIdSet result_full_freq_caps;
      bool request_blacklisted;
      Commons::Optional<RevenueDecimal> pub_imp_revenue;
      std::string tag_size;
      std::string tns_counter_device_type;
      unsigned long publisher_account_id;
      TokenValueMap tokens;
      unsigned long request_tag_id;
      RevenueDecimal adsspace_system_cpm;
    };

  private:
    /* Task implementations */

    class TaskMessage;
    using TaskMessage_var = ReferenceCounting::SmartPtr<TaskMessage>;

    class FlushLogsTaskMessage;
    class CheckConfigTaskMessage;
    class UpdateCTRProviderTask;
    class UpdateConvRateProviderTask;
    class UpdateBidCostProviderTask;

  public:
    struct InstantiateParams
    {
      InstantiateParams(
        const AdServer::Commons::Optional<unsigned long>&
          user_id_hash_mod_val)
        : user_id_hash_mod(user_id_hash_mod_val),
          generate_pubpixel_accounts(false),
          enabled_notice(false),
          video_width(0),
          video_height(0),
          publisher_site_id(0),
          publisher_account_id(0),
          init_source_macroses(true)
      {}

      InstantiateParams(
        const AdServer::Commons::UserId& user_id,
        bool enabled_notice_val)
        : generate_pubpixel_accounts(false),
          enabled_notice(enabled_notice_val),
          video_width(0),
          video_height(0),
          publisher_site_id(0),
          publisher_account_id(0),
          init_source_macroses(true)
      {
        if(!user_id.is_null())
        {
          user_id_hash_mod =
            AdServer::LogProcessing::user_id_distribution_hash(user_id);
        }
      }

      AdServer::Commons::Optional<unsigned long> user_id_hash_mod;
      std::string open_price;
      std::string openx_price;
      std::string liverail_price;
      std::string google_price;
      String::SubString ext_tag_id;

      bool generate_pubpixel_accounts;
      AccountIdList pubpixel_accounts;
      const bool enabled_notice;

      unsigned long video_width;
      unsigned long video_height;

      unsigned long publisher_site_id;
      unsigned long publisher_account_id;

      bool init_source_macroses;
    };

    struct ClickParams
    {
      std::string params;
      std::string mime_pub_preclick_url;

      std::string click_url;
      std::string click0_url; // click_url without pubpreclick
    };

    using CountryList = std::list<std::string>;

    struct ConfirmCreativeAmount
    {
      ConfirmCreativeAmount(
        unsigned long cc_id_val,
        const RevenueDecimal& ctr_val)
        noexcept;

      unsigned long cc_id;
      RevenueDecimal ctr;
    };

    using ConfirmCreativeAmountArray = std::vector<ConfirmCreativeAmount>;

    using TokenToParamMap = boost::unordered_flat_map<
      Generics::SubStringHashAdapter,
      std::string,
      SubStringHashAdapterHash,
      TransparentStringEqual>;

  private:
    AdServer::Commons::Awaitable<bool>
    select_adslot_campaign_creative_(
      AdSelectionResult& ad_selection_result,
      Tag::SizeMap& tag_sizes,
      GetAdSlotResult& ad_slot_result,
      AdSlotContext& ad_slot_context,
      const CampaignIndex& config_index,
      const CampaignConfig& const_config,
      const GetAdRequest& request_params,
      const AdSlotRequest& ad_slot,
      const AdSlotMinCpm& ad_slot_min_cpm,
      const Generics::Time& session_start,
      const Tag* tag,
      const Colocation* colocation,
      bool passback,
      GetAdDebugResult* ad_request_debug_info,
      AdSlotDebugResult* ad_slot_debug_info,
      const ChannelIdHashSet& matched_channels,
      Generics::MonoAllocatorArena* arena)
      /*throw(eh::Exception)*/;

    const Tag*
    resolve_tag(
      std::string* tag_size,
      unsigned long* selected_publisher_account_id,
      unsigned long request_type,
      unsigned long random,
      unsigned long publisher_site_id,
      const Generics::MonoVector<unsigned long>& publisher_account_ids,
      const CampaignConfig& campaign_config,
      unsigned long tag_id,
      const StringArray& sizes,
      const StringArray& currency_codes)
      const noexcept;

    AdServer::Commons::Awaitable<bool>
    co_get_adslot_campaign_creative_(
      const Tag*& log_tag,
      bool& log_request_without_tag,
      bool& log_ad_request,
      AdSelectionResult& ad_selection_result,
      AdSlotMinCpm& ad_slot_min_cpm,
      Tag::SizeMap& tag_sizes,
      GetAdSlotResult& ad_slot_result,
      AdSlotContext& ad_slot_context,
      const CampaignConfig& campaign_config,
      const CampaignIndex& config_index,
      const GetAdRequest& core_request_params,
      const Generics::Time& session_start,
      const Colocation* colocation,
      const AdSlotRequest& core_ad_slot,
      const CreativeInstantiateRule& creative_instantiate_rule,
      GetAdDebugResult* debug_info,
      const ChannelIdHashSet& matched_channels,
      Generics::MonoAllocatorArena* arena);

    bool
    get_campaign_creative_by_ccid_impl(
      const PreviewCreativeParams& params,
      std::string& creative_body)
      /*throw(eh::Exception)*/;

    AdServer::Commons::Awaitable<bool>
    co_get_site_creative_(
      const CampaignConfig& config,
      const CampaignIndex& config_index,
      const Colocation* colocation,
      const Tag* requested_tag,
      const Tag::SizeMap& tag_sizes,
      const GetAdRequest& request_params,
      const AdSlotRequest& ad_slot,
      AdSlotContext& ad_slot_context,
      const AdSlotMinCpm& ad_slot_min_cpm,
      const FreqCapIdSet& full_freq_caps,
      const SeqOrderMap& seq_orders,
      RequestResultParams& request_result_params,
      AdSelectionResult& select_result,
      std::string& creative_body,
      std::string& creative_url,
      GetAdDebugResult* ad_request_debug_info,
      AdSlotDebugResult* ad_slot_debug_info,
      Generics::MonoAllocatorArena* arena);

    void
    get_bid_costs_(
      RevenueDecimal& low_predicted_pub_ecpm_system,
      RevenueDecimal& top_predicted_pub_ecpm_system,
      const String::SubString& referer,
      const Tag* tag,
      const RevenueDecimal& min_pub_ecpm_system,
      const CampaignSelectionDataList& selected_campaigns);

    bool
    instantiate_text_creatives(
      const CampaignConfig* config,
      const Colocation* const colocation,
      const GetAdRequest& request_params,
      const AdSlotRequest& ad_slot,
      const CampaignSelector::WeightedCampaignKeywordList& campaign_keywords,
      AdSelectionResult& ad_selection_result,
      RequestResultParams& request_result_params,
      CreativeParamsList& creative_params_list,
      AdSlotDebugResult* ad_slot_debug_info,
      std::string& creative_body,
      std::string& creative_url,
      AdSlotContext& ad_slot_context,
      Generics::MonoAllocatorArena* arena)
      /*throw(eh::Exception)*/;

    bool
    instantiate_display_creative(
      const CampaignConfig* config,
      const Colocation* colocation,
      const GetAdRequest& request_params,
      const AdSlotRequest& ad_slot,
      const CampaignSelector::WeightedCampaign& weighted_campaign,
      AdSelectionResult& ad_selection_result,
      RequestResultParams& request_result_params,
      CreativeParams& creative_params,
      AdSlotDebugResult* debug_info,
      std::string& creative_body,
      std::string& creative_url,
      AdSlotContext& ad_slot_context,
      Generics::MonoAllocatorArena* arena)
      /*throw(eh::Exception)*/;

    bool check_request_constraints(
      const String::SubString& referer,
      const String::SubString& original_url,
      std::string& referer_str,
      std::string& original_url_str)
      /*throw(eh::Exception)*/;

    void log_incoming_request(
      unsigned long tag_id,
      const String::SubString& referer,
      const ChannelIdHashSet& channels,
      const Generics::MonoVector<unsigned long>& full_freq_caps)
      /*throw(eh::Exception)*/;

    ConstCampaignConfigPtr
    get_campaign_config(bool required = false) const
      /*throw(NotReady, eh::Exception)*/;

    ConstCampaignIndexPtr get_campaign_index() const /*throw(eh::Exception)*/;

    // task methods
    Generics::Time check_config() noexcept;

    Generics::Time update_ctr_provider() noexcept;

    Generics::Time update_conv_rate_provider() noexcept;

    Generics::Time update_bid_cost_provider() noexcept;

    void flush_logs() noexcept;

    static AdRequestType
    reduce_request_type_(unsigned long request_type)
      noexcept;

    void
    convert_ccg_keywords_(
      const CampaignConfig* campaign_config,
      const Tag* tag,
      CampaignKeywordMap& result_keywords,
      const Generics::MonoVector<CCGKeywordInfo>& keywords,
      bool profiling_available,
      const FreqCapIdSet& full_freq_caps)
      noexcept;

    std::string
    trace_campaign_selection_(
      const CampaignConfig& campaign_config,
      const CampaignIndex& campaign_index,
      unsigned long campaign_id,
      const GetAdRequest& request_params,
      const AdSlotRequest& ad_slot,
      const Colocation* colocation,
      AuctionType auction_type,
      bool test_request);

    void
    convert_external_categories_(
      MonoCreativeCategoryIdSet& exclude_categories,
      const CampaignConfig& config,
      unsigned long request_type,
      const StringArray& categories)
      noexcept;

    static void
    fill_triggered_channels_(
      ChannelWeightMap& res, const std::vector<ChannelWeight>& triggered_channels)
      noexcept;

    bool
    fill_category_channel_node_(
      CategoryChannelNodeInfo& res,
      const CategoryChannelNode* node,
      const std::string& language)
      noexcept;

    bool
    fill_ad_slot_min_cpm_(
      AdSlotMinCpm& ad_slot_min_cpm,
      const CampaignConfig* campaign_config,
      const Tag* tag,
      const String::SubString& min_ecpm_currency_code,
      const RevenueDecimal& min_ecpm)
      noexcept;

    bool
    preview_ccid_(
      const CampaignConfig* config,
      const Colocation* colocation,
      const Tag* tag,
      const GetAdRequest& request_params,
      const AdSlotRequest& ad_slot,
      AdSlotContext& ad_slot_context,
      RequestResultParams& request_result_params,
      AdSelectionResult& select_result,
      std::string& creative_body)
      /*throw(eh::Exception)*/;

    bool
    match_geo_channels_(
      const CampaignConfig& campaign_config,
      const std::vector<GeoInfo>& location,
      const std::vector<GeoCoordInfo>& coord_location,
      ChannelIdArray& geo_channels,
      ChannelIdSet& coord_channels)
      /*throw(NotReady, Exception)*/;

    static
    const Tag::Size*
    match_creative_by_size_(
      const Tag* tag,
      const Creative* creative)
      noexcept;

    void
    get_pub_pixel_account_ids_(
      AccountArray& result_account_ids,
      const CampaignConfig* campaign_config,
      const char* country,
      UserStatus user_status,
      const AccountIdSet& exclude_publisher_account_ids,
      unsigned long limit)
      noexcept;

    void
    get_inst_optin_pub_pixel_account_ids_(
      AccountIdList& result_account_ids,
      const CampaignConfig* campaign_config,
      const Tag* tag,
      const CommonAdRequest& request_params,
      const Generics::MonoVector<unsigned long>& exclude_pubpixel_accounts)
      noexcept;

    static bool
    size_blacklisted_(
      const CampaignConfig* campaign_config,
      const ChannelIdHashSet& matched_channels,
      unsigned long size_id)
      noexcept;

    PubPixelAccountMap::const_iterator
    find_pub_pixel_accounts_(
      const CampaignConfig* campaign_config,
      const char* country,
      UserStatus user_status) const
      noexcept;

    void
    precalculate_pub_pixel_accounts_(
      CampaignConfig* campaign_config)
      /*throw(eh::Exception)*/;

    template<typename PredProviderType>
    ReferenceCounting::SmartPtr<const PredProviderType>
    update_rate_provider_(
      const PredProviderType* old_ctr_provider,
      const String::SubString& capture_root,
      const String::SubString& res_root,
      const Generics::Time& expire_timeout);

    CTR::ConstCTRProviderImpl_var
    update_ctr_rate_provider_(
      const CTR::CTRProviderImpl* old_ctr_provider,
      const String::SubString& capture_root,
      const String::SubString& res_root,
      const Generics::Time& expire_timeout);

    bool
    apply_check_available_bid_result_(
      const Campaign* campaign, // change mutable
      const BillingStateContainer::BidCheckResult& check_result,
      const RevenueDecimal& ctr)
      noexcept;

    AdServer::Commons::Awaitable<bool>
    co_confirm_amounts_(
      const CampaignConfig* config,
      const Generics::Time& now,
      const ConfirmCreativeAmountArray& creatives,
      CCGRateType rate_type);

    static void
    fill_tns_counter_device_type_(
      std::string& tns_counter_device_type,
      const Generics::MonoUnorderedSet<std::string_view>& norm_platform_names)
      noexcept;

    // config manips
    static void
    fill_contract_(
      ::AdServer::CampaignSvcs::ContractInfo& contract_info,
      const Contract& contract)
      noexcept;

  protected:
    CampaignManagerConfig campaign_manager_config_;
    Generics::ActiveObjectCallback_var callback_;
    Logging::Logger_var logger_;

    DomainParser_var domain_parser_;
    ReferenceCounting::SmartPtr<CampaignManagerLogger> campaign_manager_logger_;

    CreativeInstantiatorTypes::CreativeInstantiate creative_instantiate_;
    TokenToParamMap token_to_parameters_;

    CampaignConfigSource_var campaign_config_source_;
    BillingStateContainer_var check_billing_state_container_;
    BillingStateContainer_var confirm_billing_state_container_;
    ReferenceCounting::PtrHolder<CTR::ConstCTRProviderImpl_var> ctr_provider_;
    ReferenceCounting::PtrHolder<CTR::ConstCTRProviderImpl_var> conv_rate_provider_;
    ReferenceCounting::PtrHolder<ConstBidCostProvider_var> bid_cost_provider_;

    mutable std::mutex lock_;

    ConstCampaignConfigPtr configuration_;
    IndexingProgress indexing_progress_;
    ConstCampaignIndexPtr configuration_index_;
    PassbackTemplateMap passback_templates_;

    Generics::TaskRunner_var task_runner_;
    Generics::TaskRunner_var update_task_runner_;
    Generics::Planner_var scheduler_;
    Commons::SecTokenGenerator_var sec_token_generator_;
    Commons::IPCrypter_var ip_crypter_;
    Generics::SignedUuidGenerator rid_signer_;
    CountryList country_whitelist_;
    Commons::FileCachePtr template_files_;
  };

  using CampaignManagerCore_var = ReferenceCounting::SmartPtr<CampaignManagerCore>;
}

namespace AdServer::CampaignSvcs
{
  inline
  ConstCampaignConfigPtr
  CampaignManagerCore::get_campaign_config(bool required) const
    /*throw(NotReady, eh::Exception)*/
  {
    ConstCampaignConfigPtr res;

    {
      std::lock_guard<std::mutex> guard(lock_);
      res = configuration_;
    }

    if(required && !res)
    {
      throw CampaignManagerCore::NotReady("Campaign configuration isn't loaded");
    }

    return res;
  }

  inline
  ConstCampaignIndexPtr
  CampaignManagerCore::get_campaign_index() const /*throw(eh::Exception)*/
  {
    std::lock_guard<std::mutex> guard(lock_);
    return configuration_index_;
  }

  inline
  void
  CampaignManagerCore::progress_comment(std::string& res)
    /*throw(eh::Exception)*/
  {
    IndexingProgress::SyncPolicy::ReadGuard guard(indexing_progress_.lock);
    std::ostringstream ostr;
    if(!indexing_progress_.common_campaign_count)
    {
      ostr << "waiting configuration, ";
    }
    ostr << "campaigns " <<
      indexing_progress_.loaded_campaign_count << "/" <<
      indexing_progress_.common_campaign_count;
    res = ostr.str();
  }

  inline
  void
  CampaignManagerCore::fill_triggered_channels_(
    ChannelWeightMap& res, const std::vector<ChannelWeight>& triggered_channels)
    noexcept
  {
    for(const auto& triggered_channel : triggered_channels)
    {
      res.insert(std::make_pair(
        triggered_channel.channel_id, triggered_channel.weight));
    }
  }

  // CampaignManagerCore::ConfirmCreativeAmount
  inline
  CampaignManagerCore::ConfirmCreativeAmount::ConfirmCreativeAmount(
    unsigned long cc_id_val,
    const RevenueDecimal& ctr_val)
    noexcept
    : cc_id(cc_id_val),
      ctr(ctr_val)
  {}
}
