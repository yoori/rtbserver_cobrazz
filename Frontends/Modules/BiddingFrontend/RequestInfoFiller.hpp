#pragma once

#include <optional>
#include <list>
#include <new>
#include <string>
#include <string_view>
#include <memory>

#include <boost/unordered/unordered_flat_map.hpp>

#include <GeoIP/IPMap.hpp>
#include <Logger/Logger.hpp>
#include <Generics/Time.hpp>
#include <Generics/Uuid.hpp>

#include <HTTP/Http.hpp>

#include <Commons/Containers.hpp>
#include <Commons/FastJsonParser.hpp>
#include <Commons/UserInfoManip.hpp>
#include <Frontends/FrontendCommons/RequestMatchers.hpp>
#include <Frontends/FrontendCommons/UserAgentMatcher.hpp>
#include <Frontends/FrontendCommons/RequestParamProcessor.hpp>
#include <Frontends/FrontendCommons/HttpResponse.hpp>

#include <Frontends/CommonModule/CommonModule.hpp>

#include <Generics/MonoAllocator.hpp>
#include "HashMaps.hpp"
#include "CampaignManagerTypes.hpp"
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>

#include <Frontends/Modules/BiddingFrontend/google-bidding.pb.h>
#include "JsonParamProcessor.hpp"

#include "AdXmlRequestInfoFiller.hpp"

namespace AdServer::Bidding
{
  struct SourceTraits
  {
    enum NoticeInstantiateType
    {
      NIT_NONE,
      NIT_NURL,
      NIT_BURL,
      NIT_NURL_AND_BURL
    };

    enum NativeAdsInstantiateType
    {
      NAIT_NONE,
      NAIT_ADM,
      NAIT_ADM_NATIVE,
      NAIT_EXT,
      NAIT_ESCAPE_SLASH_ADM,

      NAIT_NATIVE_AS_ELEMENT_1_2,
      NAIT_ADM_1_2,
      NAIT_ADM_NATIVE_1_2
    };

    enum ERIDReturnType
    {
      ERIDRT_SINGLE,
      ERIDRT_ARRAY,
      ERIDRT_EXT0,
      ERIDRT_EXT_BUZSAPE
    };

    std::optional<unsigned long> default_account_id;
    std::optional<AdServer::CampaignSvcs::AdRequestType> request_type;
    AdServer::CampaignSvcs::AdInstantiateType instantiate_type;
    NoticeInstantiateType notice_instantiate_type;
    std::string notice_url;

    AdServer::CampaignSvcs::AdInstantiateType vast_instantiate_type;
    NoticeInstantiateType vast_notice_instantiate_type;

    NoticeInstantiateType native_notice_instantiate_type;

    bool ipw_extension;
    bool truncate_domain;
    std::string seat;
    bool fill_adid;
    std::optional<Generics::Time> max_bid_time;
    NativeAdsInstantiateType native_ads_instantiate_type;
    std::optional<AdServer::CampaignSvcs::NativeAdsImpressionTrackerType>
      native_ads_impression_tracker_type;
    bool skip_ext_category;
    ERIDReturnType erid_return_type;
  };

  using DebugAdSlotSizeMap = Generics::MonoMap<unsigned long, Generics::MonoString>;

  struct GoogleAdSlotContext
  {
    GoogleAdSlotContext() :
      width(0),
      height(0),
      direct_deal_id(0),
      fixed_cpm_micros(std::numeric_limits<int64_t>::max())
    {}

    int width;
    int height;
    std::set<int64_t> billing_ids;
    int64_t direct_deal_id;
    int64_t fixed_cpm_micros;
  };

  using GoogleAdSlotContextArray = std::vector<GoogleAdSlotContext>;

  struct RequestInfo
  {
    using AccountIdArray = Generics::MonoVector<unsigned long>;
    static constexpr std::size_t ARENA_INITIAL_SIZE = 64 * 1024;

    struct AdditionalInfo
    {
      void
      clear() noexcept
      {
        tagid = {};
        ctr.reset();
        viewability.reset();
        vtr.reset();
      }

      std::string_view tagid;
      std::optional<float> ctr;
      std::optional<float> viewability;
      std::optional<float> vtr;
    };

    struct NativeDataToken
    {
      std::string name;
      bool required = false;
    };

    using NativeDataTokens = Generics::MonoVector<NativeDataToken>;

    struct NativeImageToken
    {
      std::string name;
      bool required = false;
      unsigned long width = 0;
      unsigned long height = 0;
    };

    using NativeImageTokens = Generics::MonoVector<NativeImageToken>;

    struct TokenInfo
    {
      std::string name;
      std::string value;
    };

    using TokenSeq = Generics::MonoVector<TokenInfo>;

    struct GeoInfo
    {
      std::string country;
      std::string region;
      std::string city;
    };

    using GeoInfoSeq = Generics::MonoVector<GeoInfo>;

    struct GeoCoordInfo
    {
      CampaignSvcs::CoordDecimal longitude = CampaignSvcs::CoordDecimal::ZERO;
      CampaignSvcs::CoordDecimal latitude = CampaignSvcs::CoordDecimal::ZERO;
      CampaignSvcs::AccuracyDecimal accuracy = CampaignSvcs::AccuracyDecimal::ZERO;
    };

    using GeoCoordInfoSeq = Generics::MonoVector<GeoCoordInfo>;

    struct AdSlotInfo
    {
      explicit AdSlotInfo(std::shared_ptr<Generics::MonoAllocatorArena> arena = nullptr)
        : arena_(arena ? std::move(arena) : std::make_shared<Generics::MonoAllocatorArena>()),
          string_holders_(arena_.get()),
          sizes(arena_.get()),
          currency_codes(arena_.get()),
          exclude_categories(arena_.get()),
          required_categories(arena_.get()),
          allowed_durations(arena_.get()),
          native_data_tokens(arena_.get()),
          native_image_tokens(arena_.get()),
          tokens(arena_.get())
      {}

      AdSlotInfo(AdSlotInfo&&) noexcept = default;
      AdSlotInfo& operator=(AdSlotInfo&&) noexcept = default;

      Generics::MonoAllocatorArena*
      resource() const noexcept
      {
        return arena_.get();
      }

      Generics::MonoString&
      hold_string(Generics::MonoString&& value)
      {
        Generics::MonoString& held_value = string_holders_.emplace_back(std::move(value));
        return held_value;
      }

    private:
      std::shared_ptr<Generics::MonoAllocatorArena> arena_;
      Generics::MonoList<Generics::MonoString> string_holders_;

    public:
      unsigned long ad_slot_id = 0;
      std::string_view format;
      unsigned long tag_id = 0;
      Generics::MonoVector<std::string_view> sizes;
      std::string ext_tag_id;
      CampaignSvcs::RevenueDecimal min_ecpm = CampaignSvcs::RevenueDecimal::ZERO;
      std::string_view min_ecpm_currency_code;
      Generics::MonoVector<std::string_view> currency_codes;
      bool passback = false;
      long up_expand_space = -1;
      long right_expand_space = -1;
      long left_expand_space = -1;
      long tag_visibility = -1;
      long tag_predicted_viewability = -1;
      long down_expand_space = -1;
      unsigned long video_min_duration = 0;
      long video_max_duration = -1;
      long video_skippable_max_duration = -1;
      long video_allow_skippable = 1;
      long video_allow_unskippable = 1;
      unsigned long video_width = 0;
      unsigned long video_height = 0;
      Generics::MonoVector<std::string_view> exclude_categories;
      Generics::MonoVector<std::string_view> required_categories;
      unsigned long debug_ccg = 0;
      Generics::MonoVector<unsigned long> allowed_durations;
      NativeDataTokens native_data_tokens;
      NativeImageTokens native_image_tokens;
      unsigned long native_ads_impression_tracker_type = 0;
      bool fill_track_html = false;
      TokenSeq tokens;

    };

    using AdSlotArray = Generics::MonoVector<AdSlotInfo>;

  private:
    std::shared_ptr<Generics::MonoAllocatorArena> request_arena_;

    static std::shared_ptr<Generics::MonoAllocatorArena>
    make_arena_(std::shared_ptr<Generics::MonoAllocatorArena> arena)
    {
      return arena ?
        std::move(arena) :
        std::make_shared<Generics::MonoAllocatorArena>(ARENA_INITIAL_SIZE);
    }

  public:
    RequestInfo()
      : RequestInfo(std::make_shared<Generics::MonoAllocatorArena>(ARENA_INITIAL_SIZE))
    {}

    explicit RequestInfo(std::shared_ptr<Generics::MonoAllocatorArena> arena)
      : request_arena_(make_arena_(std::move(arena))),
        external_user_id(request_arena_.get()),
        geo_location(request_arena_.get()),
        coord_location(request_arena_.get()),
        ssp_geo_country(request_arena_.get()),
        ssp_geo_region(request_arena_.get()),
        ssp_geo_city(request_arena_.get()),
        ssp_geo_type(request_arena_.get()),
        full_referer(request_arena_.get()),
        referer(request_arena_.get()),
        urls(request_arena_.get()),
        security_token(request_arena_.get()),
        pub_impr_track_url(request_arena_.get()),
        pub_param(request_arena_.get()),
        preclick_url(request_arena_.get()),
        click_prefix_url(request_arena_.get()),
        original_url(request_arena_.get()),
        cohort(request_arena_.get()),
        ext_track_params(request_arena_.get()),
        tokens(request_arena_.get()),
        passback_type(request_arena_.get()),
        passback_url(request_arena_.get()),
        platform_ids(request_arena_.get()),
        geo_channels(request_arena_.get()),
        platform(request_arena_.get()),
        full_platform(request_arena_.get()),
        web_browser(request_arena_.get()),
        ip_hash(request_arena_.get()),
        current_time(),
        source_id(request_arena_.get()),
        debug_ccg(0),
        publisher_account_ids(request_arena_.get()),
        publisher_site_id(0),
        random(CampaignSvcs::RANDOM_PARAM_MAX),
        flag(0),
        filter_request(false),
        skip_ccg_keywords(false),
        search_words(request_arena_.get()),
        truncate_domain(false),
        ipw_extension(false),
        format(request_arena_.get()),
        default_debug_size(request_arena_.get()),
        debug_sizes(request_arena_.get()),
        user_create_time(Generics::Time::ZERO),
        location(),
        is_app(false),
        application_id(request_arena_.get()),
        advertising_id(request_arena_.get()),
        idfa(request_arena_.get()),
        notice_instantiate_type(SourceTraits::NIT_NONE),
        vast_notice_instantiate_type(SourceTraits::NIT_NONE),
        native_notice_instantiate_type(SourceTraits::NIT_NONE),
        platform_names(std::less<>(), request_arena_.get()),
        native_ads_instantiate_type(SourceTraits::NAIT_NONE),
        native_ads_impression_tracker_type(AdServer::CampaignSvcs::NAITT_IMP),
        erid_return_type(SourceTraits::ERIDRT_EXT_BUZSAPE), // by default fill buz sape nroa
        skip_ext_category(false),
        require_debug_info(request_arena_.get()),
        bid_request_id(),
        bid_site_id(),
        bid_publisher_id(),
        ext_user_ids(request_arena_.get()),
        additional_info(),
        keywords(request_arena_.get()),
        ad_slots(request_arena_.get()),
        page_keywords(request_arena_.get()),
        url_keywords(request_arena_.get()),
        campaign_additional_info(request_arena_.get()),
        moved_string_holders_(request_arena_.get()),
        string_holders_(request_arena_.get())
    {}

    RequestInfo(const RequestInfo&) = delete;
    RequestInfo& operator=(const RequestInfo&) = delete;

    RequestInfo(RequestInfo&&) noexcept = default;
    RequestInfo& operator=(RequestInfo&&) = delete;

    void
    reset() noexcept
    {
      auto arena = std::move(request_arena_);
      this->~RequestInfo();
      arena->release();
      new(this) RequestInfo(std::move(arena));
    }

    const std::shared_ptr<Generics::MonoAllocatorArena>&
    arena() const noexcept
    {
      return request_arena_;
    }

    Generics::MonoAllocatorArena*
    resource() const noexcept
    {
      return request_arena_.get();
    }

    std::string_view
    hold_string(std::string&& value)
    {
      std::string& held_value = moved_string_holders_.emplace_back(std::move(value));
      return std::string_view(held_value.data(), held_value.size());
    }

    std::string_view
    hold_string(Generics::MonoString&& value)
    {
      Generics::MonoString& held_value = string_holders_.emplace_back(std::move(value));
      return std::string_view(held_value.data(), held_value.size());
    }

    Generics::Time time;
    AdServer::Commons::RequestId request_id;
    std::string_view creative_instantiate_type;
    unsigned long request_type = 0;
    bool test_request = false;
    bool log_as_test = false;
    unsigned long colo_id = 0;
    Generics::MonoString external_user_id;
    GeoInfoSeq geo_location;
    GeoCoordInfoSeq coord_location;
    Generics::MonoString ssp_geo_country;
    Generics::MonoString ssp_geo_region;
    Generics::MonoString ssp_geo_city;
    Generics::MonoString ssp_geo_type;
    std::optional<AdServer::CampaignSvcs::CoordDecimal> ssp_latitude;
    std::optional<AdServer::CampaignSvcs::CoordDecimal> ssp_longitude;
    Generics::MonoString full_referer;
    Generics::MonoString referer;
    Generics::MonoVector<std::string> urls;
    Generics::MonoString security_token;
    Generics::MonoString pub_impr_track_url;
    Generics::MonoString pub_param;
    Generics::MonoString preclick_url;
    Generics::MonoString click_prefix_url;
    Generics::MonoString original_url;
    std::optional<AdServer::Commons::UserId> track_user_id;
    std::optional<AdServer::Commons::UserId> user_id;
    unsigned long user_status = 0;
    std::string_view peer_ip;
    std::string_view user_agent;
    Generics::MonoString cohort;
    unsigned long hpos = 0;
    Generics::MonoString ext_track_params;
    TokenSeq tokens;
    bool set_cookie = false;
    Generics::MonoString passback_type;
    Generics::MonoString passback_url;

    bool enabled_notice = false;
    std::string_view client;
    std::string_view client_version;
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

    Generics::Time current_time;
    Generics::MonoString source_id;
    unsigned long debug_ccg;
    AccountIdArray publisher_account_ids;
    unsigned long publisher_site_id;
    unsigned long random;
    unsigned long flag;
    bool filter_request;
    bool skip_ccg_keywords;
    Generics::MonoString search_words;
    std::string_view seat;
    bool truncate_domain;
    bool ipw_extension;
    Generics::MonoString format;
    Generics::MonoString default_debug_size;
    DebugAdSlotSizeMap debug_sizes;
    Generics::Time user_create_time;
    FrontendCommons::Location_var location;

    bool is_app;
    Generics::MonoString application_id;
    Generics::MonoString advertising_id; // ADVERTISING_ID
    Generics::MonoString idfa;
    std::string_view ssp_devicetype_str;
    std::string_view ssp_video_placementtype_str;

    SourceTraits::NoticeInstantiateType notice_instantiate_type;
    SourceTraits::NoticeInstantiateType vast_notice_instantiate_type;
    SourceTraits::NoticeInstantiateType native_notice_instantiate_type;

    FrontendCommons::PlatformMatcher::PlatformNameSet platform_names;
    SourceTraits::NativeAdsInstantiateType native_ads_instantiate_type;
    AdServer::CampaignSvcs::NativeAdsImpressionTrackerType native_ads_impression_tracker_type;
    SourceTraits::ERIDReturnType erid_return_type;

    bool skip_ext_category;
    std::string_view notice_url;
    Generics::MonoString require_debug_info;

    std::string_view bid_request_id;
    std::string_view bid_site_id;
    std::string_view bid_publisher_id;
    Generics::MonoVector<Generics::MonoString> ext_user_ids;
    AdditionalInfo additional_info;
    Generics::MonoString keywords;

    bool fill_track_pixel = false;
    bool fill_iurl = false;
    unsigned long ad_instantiate_type = 0;
    bool only_display_ad = false;
    unsigned long search_engine_id = 0;
    bool page_keywords_present = false;
    unsigned long preview_ccid = 0;
    AdSlotArray ad_slots;
    bool need_debug_info = false;
    Generics::MonoString page_keywords;
    Generics::MonoString url_keywords;
    Generics::MonoString campaign_additional_info;

  private:
    Generics::MonoList<std::string> moved_string_holders_;
    Generics::MonoList<Generics::MonoString> string_holders_;
  };

  class RequestInfoFiller: public FrontendCommons::HTTPExceptions
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    using SourceMap = StringFlatMap<SourceTraits>;

    using ExternalUserIdSet = std::set<std::string>;

    struct EncryptionKeys: public ReferenceCounting::AtomicImpl
    {
      EncryptionKeys()
        : google_encryption_key_size(0),
          google_integrity_key_size(0)
      {}

      Generics::ArrayAutoPtr<unsigned char> google_encryption_key;
      unsigned long google_encryption_key_size;
      Generics::ArrayAutoPtr<unsigned char> google_integrity_key;
      unsigned long google_integrity_key_size;

    protected:
      virtual ~EncryptionKeys() noexcept {}
    };

    struct AccountTraits: public EncryptionKeys
    {
      AccountTraits()
        : display_billing_id(0),
          video_billing_id(0)
      {}

      std::optional<CampaignSvcs::RevenueDecimal> max_cpm;
      unsigned long display_billing_id;
      unsigned long video_billing_id;

    protected:
      virtual ~AccountTraits() noexcept {}
    };

    typedef ReferenceCounting::SmartPtr<AccountTraits> AccountTraits_var;
    using AccountTraitsById = boost::unordered_flat_map<unsigned long, AccountTraits_var>;
    using OpenRtbEnumNameMap = boost::unordered_flat_map<unsigned int, std::string>;

  public:
    RequestInfoFiller(
      Logging::Logger* logger,
      unsigned long colo_id,
      CommonModule* common_module,
      std::shared_ptr<GeoIPMapping::IPMapCity2> ip_map,
      const char* user_agent_filter_path,
      const ExternalUserIdSet& skip_external_ids,
      bool ip_logging_enabled,
      const char* ip_salt,
      const SourceMap& sources,
      bool enable_profile_referer,
      const AccountTraitsById& account_traits)
      /*throw(eh::Exception)*/;

    AdXmlRequestInfoFiller*
    adxml_request_info_filler() noexcept;

    void
    fill(
      RequestInfo& request_info,
      const FCGI::HttpRequest& request,
      const Generics::Time& now) const
      noexcept;

    void
    fill_by_google_request(
      RequestInfo& request_info,
      GoogleAdSlotContextArray& as_slots_context,
      const Google::BidRequest& bid_request) const
      /*throw(InvalidParamException, Exception)*/;

    // OpenRTB, Yandex
    void
    fill_by_openrtb_request(
      RequestInfo& request_info,
      JsonProcessingContext& context,
      std::string&& bid_request) const
      /*throw(InvalidParamException, Exception)*/;

    bool
    fill_adid(std::string_view source_id) const noexcept;

    void
    init_request_param(RequestInfo& request_info) const
      noexcept;

    static void
    init_adslot(RequestInfo::AdSlotInfo& adslot_info)
      noexcept;

    template<typename StringType>
    void
    fill_by_referer(
      RequestInfo& request_info,
      StringType& search_words,
      const HTTP::HTTPAddress& referer,
      bool fill_search_words = true,
      bool fill_instantiate_type = true)
      const
      noexcept;

    void
    fill_by_user_agent(
      RequestInfo& request_info,
      std::string_view user_agent,
      bool filter_request,
      bool application = false)
      const
      noexcept;

    void
    fill_by_ip(RequestInfo& request_info, std::string_view ip)
      const
      noexcept;

  protected:
    typedef Sync::Policy::PosixThread SyncPolicy;

    typedef std::shared_ptr<GeoIPMapping::IPMapCity2> IPMapPtr;

    using SourceNameMap = SubStringHashAdapterFlatMap<std::string>;

    typedef FrontendCommons::RequestParamProcessor<RequestInfo>
      RequestInfoParamProcessor;
    typedef ReferenceCounting::SmartPtr<RequestInfoParamProcessor>
      RequestInfoParamProcessor_var;
    using ParamProcessorMap = SubStringHashAdapterFlatMap<RequestInfoParamProcessor_var>;

  protected:
    bool
    parse_debug_size_param_(
      DebugAdSlotSizeMap& debug_sizes,
      std::string_view name,
      std::string_view value) const
      noexcept;

    template<typename StringType>
    void
    fill_additional_url_(
      RequestInfo& request_info,
      StringType& search_words,
      const HTTP::HTTPAddress& add_url)
      const
      noexcept;

    template<typename StringType>
    void
    fill_search_words_(
      RequestInfo& request_info,
      StringType& search_words,
      const HTTP::HTTPAddress& url)
      const
      noexcept;

    void
    fill_request_type_(RequestInfo& request_info, std::string_view source_id)
      const
      noexcept;

    void
    fill_vast_instantiate_type_(RequestInfo& request_info, std::string_view source_id) const
      noexcept;

    void
    fill_native_instantiate_type_(RequestInfo& request_info, std::string_view source_id) const
      noexcept;

    void
    add_special_keywords_(
      std::string& keywords,
      RequestInfo& request_info,
      const JsonProcessingContext* context = 0,
      std::string_view alt_app_id = std::string_view())
      const
      noexcept;

    void
    add_special_keywords_(
      Generics::MonoString& keywords,
      RequestInfo& request_info,
      const JsonProcessingContext* context = 0,
      std::string_view alt_app_id = std::string_view())
      const
      noexcept;

    template<typename StringType>
    void
    add_special_keywords_impl_(
      StringType& keywords,
      RequestInfo& request_info,
      const JsonProcessingContext* context,
      std::string_view alt_app_id)
      const
      noexcept;

    void
    verify_user_id_(
      std::string_view signed_user_id,
      std::string_view source_id,
      RequestInfo& request_info)
      const
      noexcept;

    bool
    use_external_user_id_(std::string_view external_user_id)
      const noexcept;

    void
    select_referer_(
      RequestInfo& request_info,
      const JsonProcessingContext& context,
      HTTP::HTTPAddress& referer) const
      /*throw(eh::Exception)*/;

    std::string
    first_significant_domain_part_(std::string_view host) const
      /*throw(eh::Exception)*/;

    static
    std::string
    openrtb_ext_tag_id(
      std::string_view publisher_id,
      std::string_view id,
      std::string_view publisher_name,
      std::string_view name);

    static
    std::string
    normalize_ext_tag_id_(std::string_view src)
      noexcept;

    void
    add_param_processor_(std::string_view name, RequestInfoParamProcessor* processor)
      noexcept;

    void
    init_param_processors_() noexcept;

    void
    init_fast_json_processors_();

    void
    parse_openrtb_request_(
      RequestInfo& request_info,
      JsonProcessingContext& context,
      std::string_view bid_request) const;

    static std::string_view
    make_ssp_uid_by_device_(const JsonProcessingContext& ctx)
      /*throw(eh::Exception)*/;

    static std::string
    adapt_app_store_url_(std::string_view store_url)
      /*throw(eh::Exception)*/;

    static std::string_view
    norm_keyword_ext_(RequestInfo& request_info, std::string_view kw) noexcept;

    std::string_view
    openrtb_devicetype_to_string_(unsigned int devicetype) const;

    std::string_view
    openrtb_video_placement_to_string_(unsigned int video_placement_type) const;

  private:
    Logging::Logger_var logger_;
    unsigned long colo_id_;
    const ExternalUserIdSet skip_external_ids_;
    const CommonModule_var common_module_;
    const bool ip_logging_enabled_;
    const std::string ip_salt_;

    IPMapPtr ip_map_;
    FrontendCommons::UserAgentMatcher user_agent_matcher_;
    SourceNameMap source_mapping_;

    ParamProcessorMap param_processors_;
    std::unique_ptr<AdServer::Commons::FastJsonParser<Generics::MonoString>> fast_json_parser_;
    const SourceMap sources_;
    const bool enable_profile_referer_;
    const AccountTraitsById account_traits_;

    std::unique_ptr<AdXmlRequestInfoFiller> adxml_request_info_filler_;

    const OpenRtbEnumNameMap openrtb_devicetype_mapping_;
    const OpenRtbEnumNameMap openrtb_video_placement_mapping_;
  };
}

namespace Request::Context
{
  extern const std::string_view SOURCE_ID;
}
