#ifndef BIDDINGFRONTENDGRPC_REQUESTINFOFILLER_HPP
#define BIDDINGFRONTENDGRPC_REQUESTINFOFILLER_HPP

// STD
#include <string>
#include <optional>

// UNIXCOMMONS
#include <Generics/GnuHashTable.hpp>
#include <Generics/Uuid.hpp>
#include <Generics/Time.hpp>
#include <GeoIP/IPMap.hpp>
#include <HTTP/Http.hpp>
#include <Logger/Logger.hpp>

// THIS
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <Commons/Containers.hpp>
#include <Commons/UserInfoManip.hpp>
#include <Frontends/CommonModule/CommonModule.hpp>
#include <Frontends/FrontendCommons/RequestMatchers.hpp>
#include <Frontends/FrontendCommons/RequestParamProcessor.hpp>
#include <Frontends/FrontendCommons/UserAgentMatcher.hpp>
#include <Frontends/Modules/BiddingFrontend/google-bidding.pb.h>
#include <Frontends/Modules/BiddingFrontendGrpc/JsonParamProcessor.hpp>
#include <Frontends/Modules/BiddingFrontendGrpc/AdXmlRequestInfoFiller.hpp>

namespace AdServer::Bidding::Grpc
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
    std::optional<unsigned long> appnexus_member_id;
    bool fill_adid;
    std::optional<Generics::Time> max_bid_time;
    NativeAdsInstantiateType native_ads_instantiate_type;
    std::optional<AdServer::CampaignSvcs::NativeAdsImpressionTrackerType>
      native_ads_impression_tracker_type;
    bool skip_ext_category;
    ERIDReturnType erid_return_type;
  };

  typedef std::map<std::string, SourceTraits>
    SourceMap;

  typedef std::map<unsigned long, std::string> DebugAdSlotSizeMap;

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

  typedef std::vector<GoogleAdSlotContext> GoogleAdSlotContextArray;

  struct RequestInfo
  {
    typedef std::vector<unsigned long> AccountIdArray;

    RequestInfo()
      : debug_ccg(0),
        publisher_site_id(0),
        random(CampaignSvcs::RANDOM_PARAM_MAX),
        flag(0),
        filter_request(false),
        skip_ccg_keywords(false),
        truncate_domain(false),
        ipw_extension(false),
        user_create_time(Generics::Time::ZERO),
        is_app(false),
        native_ads_instantiate_type(SourceTraits::NAIT_NONE),
        native_ads_impression_tracker_type(AdServer::CampaignSvcs::NAITT_IMP),
        erid_return_type(SourceTraits::ERIDRT_EXT_BUZSAPE), // by default fill buz sape nroa
        skip_ext_category(false)
    {}

    Generics::Time current_time;
    std::string source_id;
    unsigned long debug_ccg;
    AccountIdArray publisher_account_ids;
    unsigned long publisher_site_id;
    unsigned long random;
    unsigned long flag;
    bool filter_request;
    bool skip_ccg_keywords;
    std::string search_words;
    std::string seat;
    std::optional<unsigned long> appnexus_member_id;
    bool truncate_domain;
    bool ipw_extension;
    std::string format;
    std::string default_debug_size;
    DebugAdSlotSizeMap debug_sizes;
    Generics::Time user_create_time;
    FrontendCommons::Location_var location;

    //std::string device_id;
    bool is_app;
    std::string application_id;
    std::string advertising_id; // ADVERTISING_ID
    std::string idfa;
    std::string ssp_devicetype_str;
    std::string ssp_video_placementtype_str;

    SourceTraits::NoticeInstantiateType notice_instantiate_type;
    SourceTraits::NoticeInstantiateType vast_notice_instantiate_type;
    SourceTraits::NoticeInstantiateType native_notice_instantiate_type;

    FrontendCommons::PlatformMatcher::PlatformNameSet platform_names;
    SourceTraits::NativeAdsInstantiateType native_ads_instantiate_type;
    AdServer::CampaignSvcs::NativeAdsImpressionTrackerType native_ads_impression_tracker_type;
    SourceTraits::ERIDReturnType erid_return_type;

    bool skip_ext_category;
    std::string notice_url;

    std::string bid_request_id;
    std::string bid_site_id;
    std::string bid_publisher_id;
    std::vector<std::string> ext_user_ids;
  };

  class RequestInfoFiller: public FrontendCommons::HTTPExceptions
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    typedef std::set<std::string> ExternalUserIdSet;

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
    typedef std::unordered_map<unsigned long, AccountTraits_var> AccountTraitsById;

  public:
    RequestInfoFiller(
      Logging::Logger* logger,
      unsigned long colo_id,
      CommonModule* common_module,
      const char* geo_ip_path,
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
      const FrontendCommons::HttpRequest& request,
      const Generics::Time& now) const
      noexcept;

    void
    fill_by_google_request(
      FrontendCommons::GrpcCampaignManagerPool::RequestParams& request_params,
      RequestInfo& request_info,
      std::string& keywords,
      GoogleAdSlotContextArray& as_slots_context,
      const Google::BidRequest& bid_request) const
      /*throw(InvalidParamException, Exception)*/;

    // OpenRTB, Yandex
    void
    fill_by_openrtb_request(
      FrontendCommons::GrpcCampaignManagerPool::RequestParams& request_params,
      RequestInfo& request_info,
      std::string& keywords,
      JsonProcessingContext& context,
      const char* bid_request) const
      /*throw(InvalidParamException, Exception)*/;

    void
    fill_by_appnexus_request(
      FrontendCommons::GrpcCampaignManagerPool::RequestParams& request_params,
      RequestInfo& request_info,
      std::string& keywords,
      JsonProcessingContext& context,
      const char* bid_request) const
      /*throw(InvalidParamException, Exception)*/;

    bool
    fill_adid(const RequestInfo& request_info) const noexcept;

    void
    init_request_param(
      FrontendCommons::GrpcCampaignManagerPool::RequestParams& request_params,
      RequestInfo& request_info) const
      noexcept;

    static void
    init_adslot(
      FrontendCommons::GrpcCampaignManagerPool::AdSlotInfo& adslot_info)
      noexcept;

    void
    fill_by_referer(
      FrontendCommons::GrpcCampaignManagerPool::RequestParams& request_params,
      std::string& search_words,
      const HTTP::HTTPAddress& referer,
      bool fill_search_words = true,
      bool fill_instantiate_type = true)
      const
      noexcept;

    void
    fill_by_user_agent(
      FrontendCommons::GrpcCampaignManagerPool::RequestParams& request_params,
      RequestInfo& request_info,
      String::SubString user_agent,
      bool filter_request,
      bool application = false)
      const
      noexcept;

    void
    fill_by_ip(
      RequestInfo& request_info,
      FrontendCommons::GrpcCampaignManagerPool::RequestParams& request_params,
      String::SubString ip)
      const
      noexcept;

  protected:
    typedef Sync::Policy::PosixThread SyncPolicy;

    typedef std::unique_ptr<GeoIPMapping::IPMapCity2> IPMapPtr;

    typedef Generics::GnuHashTable<Generics::SubStringHashAdapter, std::string>
      SourceNameMap;
    
    typedef JsonParamProcessor<JsonProcessingContext>
      JsonRequestParamProcessor;
    typedef ReferenceCounting::SmartPtr<JsonRequestParamProcessor>
      JsonRequestParamProcessor_var;
    typedef JsonCompositeParamProcessor<JsonProcessingContext>
      JsonCompositeRequestParamProcessor;
    typedef ReferenceCounting::SmartPtr<JsonCompositeRequestParamProcessor>
      JsonCompositeRequestParamProcessor_var;

    typedef FrontendCommons::RequestParamProcessor<RequestInfo>
      RequestInfoParamProcessor;
    typedef ReferenceCounting::SmartPtr<RequestInfoParamProcessor>
      RequestInfoParamProcessor_var;
    typedef Generics::GnuHashTable<
      Generics::SubStringHashAdapter, RequestInfoParamProcessor_var>
      ParamProcessorMap;

  protected:
    bool
    parse_debug_size_param_(
      DebugAdSlotSizeMap& debug_sizes,
      const String::SubString& name,
      const std::string& value) const
      noexcept;

    void
    fill_additional_url_(
      FrontendCommons::GrpcCampaignManagerPool::RequestParams& request_params,
      std::string& search_words,
      const HTTP::HTTPAddress& add_url)
      const
      noexcept;

    void
    fill_search_words_(
      FrontendCommons::GrpcCampaignManagerPool::RequestParams& request_params,
      std::string& search_words,
      const HTTP::HTTPAddress& url)
      const
      noexcept;

    void
    fill_request_type_(
      RequestInfo& request_info,
      FrontendCommons::GrpcCampaignManagerPool::RequestParams& request_params,
      const std::string& source_id)
      const
      noexcept;

    void
    fill_vast_instantiate_type_(
      FrontendCommons::GrpcCampaignManagerPool::RequestParams& request_params,
      const std::string& source_id) const
      noexcept;

    void
    fill_native_instantiate_type_(
      FrontendCommons::GrpcCampaignManagerPool::RequestParams& request_params,
      const std::string& source_id) const
      noexcept;

    void
    add_special_keywords_(
      std::string& keywords,
      const RequestInfo& request_info,
      const JsonProcessingContext* context = 0,
      const String::SubString& alt_app_id = String::SubString())
      const
      noexcept;

    void
    verify_user_id_(
      const std::string& signed_user_id,
      const std::string& source_id,
      FrontendCommons::GrpcCampaignManagerPool::RequestParams& request_params)
      const
      noexcept;

    bool
    use_external_user_id_(String::SubString external_user_id)
      const noexcept;

    void
    select_referer_(
      FrontendCommons::GrpcCampaignManagerPool::RequestParams& request_params,
      const JsonProcessingContext& context,
      HTTP::HTTPAddress& referer) const
      /*throw(eh::Exception)*/;

    std::string
    first_significant_domain_part_(const String::SubString& host) const
      /*throw(eh::Exception)*/;

    static
    std::string
    openrtb_ext_tag_id(
      const std::string& publisher_id,
      const std::string& id,
      const std::string& publisher_name,
      const std::string& name);

    static
    std::string
    normalize_ext_tag_id_(const String::SubString& src)
      noexcept;

    void
    add_param_processor_(
      const String::SubString& name,
      RequestInfoParamProcessor* processor)
      noexcept;

    void
    init_param_processors_() noexcept;

    void
    init_appnexus_processors_() noexcept;

    static std::string
    make_ssp_uid_by_device_(const JsonProcessingContext& ctx)
      /*throw(eh::Exception)*/;

    static std::string
    adapt_app_store_url_(const String::SubString& store_url)
      /*throw(eh::Exception)*/;

    static std::string
    norm_keyword_(const String::SubString& kw) noexcept;

    std::string openrtb_devicetype_to_string_(unsigned int devicetype) const;

    std::string openrtb_video_placement_to_string_(
      unsigned int video_placement_type) const;

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
    JsonRequestParamProcessor_var json_root_processor_;
    JsonRequestParamProcessor_var appnexus_root_processor_;

    const SourceMap sources_;
    const bool enable_profile_referer_;
    const AccountTraitsById account_traits_;

    std::unique_ptr<AdXmlRequestInfoFiller> adxml_request_info_filler_;

    std::unordered_map<unsigned int, std::string> openrtb_devicetype_mapping_;
    std::unordered_map<unsigned int, std::string> openrtb_video_placement_mapping_;
  };
} // namespace AdServer::Bidding::Grpc

namespace Request::Context::Grpc
{
  extern const std::string SOURCE_ID;
} // namespace Request::Context

#endif /*BIDDINGFRONTENDGRPC_REQUESTINFOFILLER_HPP*/
