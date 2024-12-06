#ifndef FRONTENDCOMMONS_GRPCCAMPAIGNMANAGERPOOL_HPP
#define FRONTENDCOMMONS_GRPCCAMPAIGNMANAGERPOOL_HPP

// STD
#include <chrono>
#include <optional>
#include <shared_mutex>
#include <unordered_map>

// BOOST
#include <boost/functional/hash.hpp>

// PROTO
#include "CampaignSvcs/CampaignManager/proto/CampaignManager_client.cobrazz.pb.hpp"

// UNIXCOMMONS
#include <UServerUtils/Grpc/Client/ConfigPoolCoro.hpp>
#include <UServerUtils/Grpc/Client/PoolClientFactory.hpp>
#include <UServerUtils/Grpc/Common/Scheduler.hpp>

// USERVER
#include <userver/engine/task/task_processor.hpp>

// THIS
#include <Commons/GrpcAlgs.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>

namespace FrontendCommons
{

inline constexpr char ASPECT_GRPC_CAMPAIGN_MANAGERS_POOL[] = "GRPC_CAMPAIGN_MANAGERS_POOL";

class GrpcCampaignManagerPool final : private Generics::Uncopyable
{
public:
  using CTRDecimal = AdServer::CampaignSvcs::CTRDecimal;
  using RevenueDecimal = AdServer::CampaignSvcs::RevenueDecimal;

  struct Endpoint final
  {
    using Host = std::string;
    using Port = std::size_t;
    using ServiceId = std::string;

    Endpoint(
      const Host& host,
      const Port port,
      const ServiceId& service_id)
      : host(host),
        port(port),
        service_id(service_id)
    {
    }

    Host host;
    Port port = 0;
    ServiceId service_id;
  };
  using Endpoints = std::vector<Endpoint>;
  using ServiceId = Endpoint::ServiceId;

  struct UserIdHashModInfo final
  {
    UserIdHashModInfo(const std::optional<std::uint32_t> value = {})
      : value(value)
    {
    }

    std::optional<std::uint32_t> value;
  };

  struct GeoInfo final
  {
    GeoInfo(
      const String::SubString& country,
      const String::SubString& region,
      const String::SubString& city)
      : country(country.data(), country.size()),
        region(region.data(), region.size()),
        city(city.data(), city.size())
    {
    }

    std::string country;
    std::string region;
    std::string city;
  };

  struct ChannelTriggerMatchInfo final
  {
    ChannelTriggerMatchInfo(
      const std::uint32_t channel_trigger_id,
      const std::uint32_t channel_id)
      : channel_trigger_id(channel_trigger_id),
        channel_id(channel_id)
    {
    }

    std::uint32_t channel_trigger_id = 0;
    std::uint32_t channel_id = 0;
  };

  struct GeoCoordInfo final
  {
    using CoordDecimal = AdServer::CampaignSvcs::CoordDecimal;
    GeoCoordInfo(
      const CoordDecimal& longitude,
      const CoordDecimal& latitude,
      const CoordDecimal& accuracy)
      : longitude(longitude),
        latitude(latitude),
        accuracy(accuracy)
    {
    }

    CoordDecimal longitude;
    CoordDecimal latitude;
    CoordDecimal accuracy;
  };

  struct TokenInfo final
  {
    TokenInfo(
      const std::string& name,
      const std::string& value)
      : name(name),
        value(value)
    {
    }

    std::string name;
    std::string value;
  };

  struct SeqOrderInfo final
  {
    SeqOrderInfo(
      const std::uint32_t ccg_id,
      const std::uint32_t set_id,
      const std::uint32_t imps)
      : ccg_id(ccg_id),
        set_id(set_id),
        imps(imps)
    {
    }

    std::uint32_t ccg_id = 0;
    std::uint32_t set_id = 0;
    std::uint32_t imps = 0;
  };

  struct CampaignFreq final
  {
    CampaignFreq(
      const std::uint32_t campaign_id,
      const std::uint32_t imps)
      : campaign_id(campaign_id),
        imps(imps)
    {
    }

    std::uint32_t campaign_id = 0;
    std::uint32_t imps = 0;
  };

  struct NativeDataToken final
  {
    NativeDataToken(
      const std::string& name,
      const bool required)
      : name(name),
        required(required)
    {
    }

    std::string name;
    bool required = false;
  };

  struct NativeImageToken final
  {
    NativeImageToken(
      const std::string& name,
      const bool required,
      const std::uint32_t width = 0,
      const std::uint32_t height = 0)
      : name(name),
        required(required),
        width(width),
        height(height)
    {
    }

    std::string name;
    bool required = false;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
  };

  struct CCGKeyword final
  {
    CCGKeyword(
      const std::uint32_t ccg_keyword_id,
      const std::uint32_t ccg_id,
      const std::uint32_t channel_id,
      const RevenueDecimal& max_cpc,
      const CTRDecimal& ctr,
      const std::string& click_url,
      const std::string& original_keyword)
      : ccg_keyword_id(ccg_keyword_id),
        ccg_id(ccg_id),
        channel_id(channel_id),
        max_cpc(max_cpc),
        ctr(ctr),
        click_url(click_url),
        original_keyword(original_keyword)
    {
    }

    std::uint32_t ccg_keyword_id = 0;
    std::uint32_t ccg_id = 0;
    std::uint32_t channel_id = 0;
    RevenueDecimal max_cpc;
    CTRDecimal ctr;
    std::string click_url;
    std::string original_keyword;
  };

  struct TrackCreativeInfo final
  {
    TrackCreativeInfo(
      const std::uint32_t ccid,
      const std::uint32_t ccg_keyword_id,
      const AdServer::Commons::RequestId& request_id,
      const CTRDecimal& ctr)
      : ccid(ccid),
        ccg_keyword_id(ccg_keyword_id),
        request_id(request_id),
        ctr(ctr)
    {
    }

    TrackCreativeInfo() = default;

    std::uint32_t ccid = 0;
    std::uint32_t ccg_keyword_id = 0;
    AdServer::Commons::RequestId request_id;
    CTRDecimal ctr;
  };

  struct TriggerMatchResult final
  {
    TriggerMatchResult() = default;
    ~TriggerMatchResult() = default;

    std::vector<ChannelTriggerMatchInfo> url_channels;
    std::vector<ChannelTriggerMatchInfo> pkw_channels;
    std::vector<ChannelTriggerMatchInfo> skw_channels;
    std::vector<ChannelTriggerMatchInfo> ukw_channels;
    std::vector<std::uint32_t> uid_channels;
  };

  struct AdSlotInfo final
  {
    AdSlotInfo() = default;
    ~AdSlotInfo() = default;

    std::uint32_t ad_slot_id = 0;
    std::string format;
    std::uint32_t tag_id = 0;
    std::vector<std::string> sizes;
    std::string ext_tag_id;
    RevenueDecimal min_ecpm;
    std::string min_ecpm_currency_code;
    std::vector<std::string> currency_codes;
    bool passback = false;
    std::int32_t up_expand_space = 0;
    std::int32_t right_expand_space = 0;
    std::int32_t left_expand_space = 0;
    std::int32_t tag_visibility = 0;
    std::int32_t tag_predicted_viewability = 0;
    std::int32_t down_expand_space = 0;
    std::uint32_t video_min_duration = 0;
    std::int32_t video_max_duration = 0;
    std::int32_t video_skippable_max_duration = 0;
    std::int32_t video_allow_skippable = 0;
    std::int32_t video_allow_unskippable = 0;
    std::uint32_t video_width = 0;
    std::uint32_t video_height = 0;
    std::vector<std::string> exclude_categories;
    std::vector<std::string> required_categories;
    std::uint32_t debug_ccg = 0;
    std::vector<std::uint32_t> allowed_durations;
    std::vector<NativeDataToken> native_data_tokens;
    std::vector<NativeImageToken> native_image_tokens;
    std::uint32_t native_ads_impression_tracker_type = 0;
    bool fill_track_html = false;
    std::vector<TokenInfo> tokens;
  };

  struct CommonAdRequestInfo final
  {
    CommonAdRequestInfo() = default;
    ~CommonAdRequestInfo() = default;

    Generics::Time time;
    AdServer::Commons::RequestId request_id;
    std::string creative_instantiate_type;
    std::uint32_t request_type = 0;
    std::uint32_t random = 0;
    bool test_request = false;
    bool log_as_test = false;
    std::uint32_t colo_id = 0;
    std::string external_user_id;
    std::string source_id;
    std::vector<GeoInfo> location;
    std::vector<GeoCoordInfo> coord_location;
    std::string full_referer;
    std::string referer;
    std::vector<std::string> urls;
    std::string security_token;
    std::string pub_impr_track_url;
    std::string pub_param;
    std::string preclick_url;
    std::string click_prefix_url;
    std::string original_url;
    AdServer::Commons::UserId track_user_id;
    AdServer::Commons::UserId user_id;
    std::uint32_t user_status = 0;
    std::string signed_user_id;
    std::string peer_ip;
    std::string user_agent;
    std::string cohort;
    std::uint32_t hpos = 0;
    std::string ext_track_params;
    std::vector<TokenInfo> tokens;
    bool set_cookie = false;
    std::string passback_type;
    std::string passback_url;
  };

  struct ContextAdRequestInfo final
  {
    ContextAdRequestInfo() = default;
    ~ContextAdRequestInfo() = default;

    bool enabled_notice = false;
    std::string client;
    std::string client_version;
    std::vector<std::uint32_t> platform_ids;
    std::vector<std::uint32_t> geo_channels;
    std::string platform;
    std::string full_platform;
    std::string web_browser;
    std::string ip_hash;
    bool profile_referer = false;
    std::uint32_t page_load_id = 0;
    std::uint32_t full_referer_hash = 0;
    std::uint32_t short_referer_hash = 0;
  };

  struct RequestParams
  {
    RequestParams() = default;
    ~RequestParams() = default;

    CommonAdRequestInfo common_info;
    ContextAdRequestInfo context_info;
    std::uint32_t publisher_site_id = 0;
    std::vector<std::uint32_t> publisher_account_ids;
    bool fill_track_pixel = false;
    bool fill_iurl = false;
    std::uint32_t ad_instantiate_type = 0;
    bool only_display_ad = false;
    std::vector<std::uint32_t> full_freq_caps;
    std::vector<SeqOrderInfo> seq_orders;
    std::vector<CampaignFreq> campaign_freqs;
    AdServer::Commons::UserId household_id;
    AdServer::Commons::UserId merged_user_id;
    std::uint32_t search_engine_id = 0;
    std::string search_words;
    bool page_keywords_present = false;
    bool profiling_available = false;
    bool fraud = false;
    std::vector<std::uint32_t> channels;
    std::vector<std::uint32_t> hid_channels;
    std::vector<CCGKeyword> ccg_keywords;
    std::vector<CCGKeyword> hid_ccg_keywords;
    TriggerMatchResult trigger_match_result;
    Generics::Time client_create_time;
    Generics::Time session_start;
    std::vector<std::uint32_t> exclude_pubpixel_accounts;
    std::uint32_t tag_delivery_factor = 0;
    std::uint32_t ccg_delivery_factor = 0;
    std::uint32_t preview_ccid = 0;
    std::vector<AdSlotInfo> ad_slots;
    bool required_passback = false;
    std::uint32_t profiling_type = 0;
    bool disable_fraud_detection = false;
    bool need_debug_info = false;
    std::string page_keywords;
    std::string url_keywords;
    std::string ssp_location;
  };

  struct InstantiateAdInfo final
  {
    InstantiateAdInfo() = default;
    ~InstantiateAdInfo() = default;

    CommonAdRequestInfo common_info;
    std::vector<ContextAdRequestInfo> context_info;
    std::string format;
    std::uint32_t publisher_site_id = 0;
    std::uint32_t publisher_account_id = 0;
    std::uint32_t tag_id = 0;
    std::uint32_t tag_size_id = 0;
    std::vector<TrackCreativeInfo> creatives;
    std::uint32_t creative_id = 0;
    UserIdHashModInfo user_id_hash_mod;
    AdServer::Commons::UserId merged_user_id;
    std::vector<std::uint32_t> pubpixel_accounts;
    std::string open_price;
    std::string openx_price;
    std::string liverail_price;
    std::string google_price;
    std::string ext_tag_id;
    std::uint32_t video_width = 0;
    std::uint32_t video_height = 0;
    bool consider_request = false;
    bool enabled_notice = false;
    bool emulate_click = false;
    RevenueDecimal pub_imp_revenue;
    bool pub_imp_revenue_defined = false;
  };

  using ConfigPoolClient = UServerUtils::Grpc::Client::ConfigPoolCoro;
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using Time = Generics::Time;
  using TaskProcessor = userver::engine::TaskProcessor;
  using SchedulerPtr = UServerUtils::Grpc::Common::SchedulerPtr;
  using OptOperation = AdServer::CampaignSvcs::Proto::OptOperation;

  using GetPubPixelsRequest = AdServer::CampaignSvcs::Proto::GetPubPixelsRequest;
  using GetPubPixelsRequestPtr = std::unique_ptr<GetPubPixelsRequest>;
  using GetPubPixelsResponse = AdServer::CampaignSvcs::Proto::GetPubPixelsResponse;
  using GetPubPixelsResponsePtr = std::unique_ptr<GetPubPixelsResponse>;
  using ConsiderWebOperationRequest = AdServer::CampaignSvcs::Proto::ConsiderWebOperationRequest;
  using ConsiderWebOperationRequestPtr = std::unique_ptr<ConsiderWebOperationRequest>;
  using ConsiderWebOperationResponse = AdServer::CampaignSvcs::Proto::ConsiderWebOperationResponse;
  using ConsiderWebOperationResponsePtr = std::unique_ptr<ConsiderWebOperationResponse>;
  using ConsiderPassbackTrackRequest = AdServer::CampaignSvcs::Proto::ConsiderPassbackTrackRequest;
  using ConsiderPassbackTrackRequestPtr = std::unique_ptr<ConsiderPassbackTrackRequest>;
  using ConsiderPassbackTrackResponse = AdServer::CampaignSvcs::Proto::ConsiderPassbackTrackResponse;
  using ConsiderPassbackTrackResponsePtr = std::unique_ptr<ConsiderPassbackTrackResponse>;
  using ConsiderPassbackRequest = AdServer::CampaignSvcs::Proto::ConsiderPassbackRequest;
  using ConsiderPassbackRequestPtr = std::unique_ptr<ConsiderPassbackRequest>;
  using ConsiderPassbackResponse = AdServer::CampaignSvcs::Proto::ConsiderPassbackResponse;
  using ConsiderPassbackResponsePtr = std::unique_ptr<ConsiderPassbackResponse>;
  using ActionTakenRequest = AdServer::CampaignSvcs::Proto::ActionTakenRequest;
  using ActionTakenRequestPtr = std::unique_ptr<ActionTakenRequest>;
  using ActionTakenResponse = AdServer::CampaignSvcs::Proto::ActionTakenResponse;
  using ActionTakenResponsePtr = std::unique_ptr<ActionTakenResponse>;
  using ProcessMatchRequestRequest = AdServer::CampaignSvcs::Proto::ProcessMatchRequestRequest;
  using ProcessMatchRequestRequestPtr = std::unique_ptr<ProcessMatchRequestRequest>;
  using ProcessMatchRequestResponse = AdServer::CampaignSvcs::Proto::ProcessMatchRequestResponse;
  using ProcessMatchRequestResponsePtr = std::unique_ptr<ProcessMatchRequestResponse>;
  using VerifyOptOperationRequest = AdServer::CampaignSvcs::Proto::VerifyOptOperationRequest;
  using VerifyOptOperationRequestPtr = std::unique_ptr<VerifyOptOperationRequest>;
  using VerifyOptOperationResponse = AdServer::CampaignSvcs::Proto::VerifyOptOperationResponse;
  using VerifyOptOperationResponsePtr = std::unique_ptr<VerifyOptOperationResponse>;
  using GetCampaignCreativeRequest = AdServer::CampaignSvcs::Proto::GetCampaignCreativeRequest;
  using GetCampaignCreativeRequestPtr = std::unique_ptr<GetCampaignCreativeRequest>;
  using GetCampaignCreativeResponse = AdServer::CampaignSvcs::Proto::GetCampaignCreativeResponse;
  using GetCampaignCreativeResponsePtr = std::unique_ptr<GetCampaignCreativeResponse>;
  using GetColocationFlagsRequest = AdServer::CampaignSvcs::Proto::GetColocationFlagsRequest;
  using GetColocationFlagsRequestPtr = std::unique_ptr<GetColocationFlagsRequest>;
  using GetColocationFlagsResponse = AdServer::CampaignSvcs::Proto::GetColocationFlagsResponse;
  using GetColocationFlagsResponsePtr = std::unique_ptr<GetColocationFlagsResponse>;
  using InstantiateAdRequest = AdServer::CampaignSvcs::Proto::InstantiateAdRequest;
  using InstantiateAdRequestPtr = std::unique_ptr<InstantiateAdRequest>;
  using InstantiateAdResponse = AdServer::CampaignSvcs::Proto::InstantiateAdResponse;
  using InstantiateAdResponsePtr = std::unique_ptr<InstantiateAdResponse>;
  using GetClickUrlRequest = AdServer::CampaignSvcs::Proto::GetClickUrlRequest;
  using GetClickUrlRequestPtr = std::unique_ptr<GetClickUrlRequest>;
  using GetClickUrlResponse = AdServer::CampaignSvcs::Proto::GetClickUrlResponse;
  using GetClickUrlResponsePtr = std::unique_ptr<GetClickUrlResponse>;
  using GetFileRequest = AdServer::CampaignSvcs::Proto::GetFileRequest;
  using GetFileRequestPtr = std::unique_ptr<GetFileRequest>;
  using GetFileResponse = AdServer::CampaignSvcs::Proto::GetFileResponse;
  using GetFileResponsePtr = std::unique_ptr<GetFileResponse>;
  using VerifyImpressionRequest = AdServer::CampaignSvcs::Proto::VerifyImpressionRequest;
  using VerifyImpressionRequestPtr = std::unique_ptr<VerifyImpressionRequest>;
  using VerifyImpressionResponse = AdServer::CampaignSvcs::Proto::VerifyImpressionResponse;
  using VerifyImpressionResponsePtr = std::unique_ptr<VerifyImpressionResponse>;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

private:
  class ClientHolder;
  class FactoryClientHolder;

  using ClientHolderPtr = std::shared_ptr<ClientHolder>;
  using ClientHolders = std::vector<ClientHolderPtr>;
  using FactoryClientHolderPtr = std::unique_ptr<FactoryClientHolder>;
  using ServiceIds = std::list<ServiceId>;
  using ServiceIdToClientHolder = std::unordered_map<std::string_view, ClientHolderPtr>;

public:
  explicit GrpcCampaignManagerPool(
    Logger* logger,
    TaskProcessor& task_processor,
    const SchedulerPtr& scheduler,
    const Endpoints& endpoints,
    const ConfigPoolClient& config_pool_client,
    const std::size_t grpc_client_timeout_ms = 1000,
    const std::size_t time_duration_client_bad_sec = 30);

  ~GrpcCampaignManagerPool() = default;

  GetPubPixelsResponsePtr get_pub_pixels(
    const std::string& country,
    const std::uint32_t user_status,
    const std::vector<std::uint32_t>& publisher_account_ids) noexcept;

  ConsiderWebOperationResponsePtr consider_web_operation(
    const Generics::Time& time,
    const std::uint32_t colo_id,
    const std::uint32_t tag_id,
    const std::uint32_t cc_id,
    const std::string& ct,
    const std::string& curct,
    const std::string& browser,
    const std::string& os,
    const std::string& app,
    const std::string& source,
    const std::string& operation,
    const std::string& user_bind_src,
    const std::uint32_t result,
    const std::uint32_t user_status,
    const bool test_request,
    const std::vector<std::string>& request_ids,
    const std::string& global_request_id,
    const std::string& referer,
    const std::string& ip_address,
    const std::string& external_user_id,
    const std::string& user_agent) noexcept;

  ConsiderPassbackTrackResponsePtr consider_passback_track(
    const Generics::Time& time,
    const std::string& country,
    const std::uint32_t colo_id,
    const std::uint32_t tag_id,
    const std::uint32_t user_status) noexcept;

  ConsiderPassbackResponsePtr consider_passback(
    const Generics::Uuid& request_id,
    const UserIdHashModInfo& user_id_hash_mod,
    const std::string& passback,
    const Generics::Time& time) noexcept;

  ActionTakenResponsePtr action_taken(
    const Generics::Time& time,
    const bool test_request,
    const bool log_as_test,
    const bool campaign_id_defined,
    const std::uint32_t campaign_id,
    const bool action_id_defined,
    const std::uint32_t action_id,
    const std::string& order_id,
    const bool action_value_defined,
    const std::string& action_value,
    const std::string& referer,
    const std::uint32_t user_status,
    const Generics::Uuid& user_id,
    const std::string& ip_hash,
    const std::vector<std::uint32_t>& platform_ids,
    const std::string& peer_ip,
    const std::vector<GeoInfo>& location) noexcept;

  ProcessMatchRequestResponsePtr process_match_request(
    const Generics::Uuid& user_id,
    const std::string& household_id,
    const Generics::Time& request_time,
    const std::string& source,
    const std::vector<std::uint32_t>& channels,
    const std::vector<ChannelTriggerMatchInfo>& pkw_channels,
    const std::vector<std::uint32_t>& hid_channels,
    const std::uint32_t colo_id,
    const std::vector<GeoInfo>& location,
    const std::vector<GeoCoordInfo>& coord_location,
    const std::string& full_referer) noexcept;

  GetColocationFlagsResponsePtr get_colocation_flags() noexcept;

  VerifyOptOperationResponsePtr verify_opt_operation(
    const std::uint32_t time,
    const std::int32_t colo_id,
    const std::string& referer,
    const OptOperation operation,
    const std::uint32_t status,
    const std::uint32_t user_status,
    const bool log_as_test,
    const std::string& browser,
    const std::string& os,
    const std::string& ct,
    const std::string& curct,
    const AdServer::Commons::UserId& user_id) noexcept;

  GetCampaignCreativeResponsePtr get_campaign_creative(
    const RequestParams& request_params) noexcept;

  InstantiateAdResponsePtr instantiate_ad(
    const std::string_view service_id,
    const InstantiateAdInfo& instantiate_ad) noexcept;

  GetClickUrlResponsePtr get_click_url(
    const std::string_view service_id,
    const Generics::Time& time,
    const Generics::Time& bid_time,
    const std::uint32_t colo_id,
    const std::uint32_t tag_id,
    const std::uint32_t tag_size_id,
    const std::uint32_t ccid,
    const std::uint32_t ccg_keyword_id,
    const std::uint32_t creative_id,
    const AdServer::Commons::UserId& match_user_id,
    const AdServer::Commons::UserId& cookie_user_id,
    const AdServer::Commons::RequestId& request_id,
    const UserIdHashModInfo& user_id_hash_mod,
    const std::string& relocate,
    const std::string& referer,
    const bool log_click,
    const CTRDecimal& ctr,
    const std::vector<TokenInfo>& tokens) noexcept;

  GetFileResponsePtr get_file(
    const std::string_view service_id,
    const std::string& file_name) noexcept;

  VerifyImpressionResponsePtr verify_impression(
    const Generics::Time& time,
    const Generics::Time& bid_time,
    const UserIdHashModInfo& user_id_hash_mod,
    const std::vector<TrackCreativeInfo>& creatives,
    const std::uint32_t pub_imp_revenue_type,
    const CTRDecimal& pub_imp_revenue,
    const std::uint32_t request_type,
    const std::uint32_t verify_type,
    const AdServer::Commons::UserId& user_id,
    const std::string& referer,
    const std::int32_t viewability,
    const std::string& action_name) noexcept;

private:
  GetPubPixelsRequestPtr create_get_pub_pixels_request(
    const std::string& country,
    const std::uint32_t user_status,
    const std::vector<std::uint32_t>& publisher_account_ids);

  ConsiderWebOperationRequestPtr create_consider_web_operation_request(
    const Generics::Time& time,
    const std::uint32_t colo_id,
    const std::uint32_t tag_id,
    const std::uint32_t cc_id,
    const std::string& ct,
    const std::string& curct,
    const std::string& browser,
    const std::string& os,
    const std::string& app,
    const std::string& source,
    const std::string& operation,
    const std::string& user_bind_src,
    const std::uint32_t result,
    const std::uint32_t user_status,
    const bool test_request,
    const std::vector<std::string>& request_ids,
    const std::string& global_request_id,
    const std::string& referer,
    const std::string& ip_address,
    const std::string& external_user_id,
    const std::string& user_agent);

  ConsiderPassbackTrackRequestPtr create_consider_passback_track_request(
    const Generics::Time& time,
    const std::string& country,
    const std::uint32_t colo_id,
    const std::uint32_t tag_id,
    const std::uint32_t user_status);

  ConsiderPassbackRequestPtr create_consider_passback_request(
    const Generics::Uuid& request_id,
    const UserIdHashModInfo& user_id_hash_mod,
    const std::string& passback,
    const Generics::Time& time);

  ActionTakenRequestPtr create_action_taken_request(
    const Generics::Time& time,
    const bool test_request,
    const bool log_as_test,
    const bool campaign_id_defined,
    const std::uint32_t campaign_id,
    const bool action_id_defined,
    const std::uint32_t action_id,
    const std::string& order_id,
    const bool action_value_defined,
    const std::string& action_value,
    const std::string& referer,
    const std::uint32_t user_status,
    const Generics::Uuid& user_id,
    const std::string& ip_hash,
    const std::vector<std::uint32_t>& platform_ids,
    const std::string& peer_ip,
    const std::vector<GeoInfo>& location);

  ProcessMatchRequestRequestPtr create_process_match_request_request(
    const Generics::Uuid& user_id,
    const std::string& household_id,
    const Generics::Time& request_time,
    const std::string& source,
    const std::vector<std::uint32_t>& channels,
    const std::vector<ChannelTriggerMatchInfo>& pkw_channels,
    const std::vector<std::uint32_t>& hid_channels,
    const std::uint32_t colo_id,
    const std::vector<GeoInfo>& location,
    const std::vector<GeoCoordInfo>& coord_location,
    const std::string& full_referer);

  GetColocationFlagsRequestPtr create_get_colocation_flags_request();

  VerifyOptOperationRequestPtr create_verify_opt_operation_request(
    const std::uint32_t time,
    const std::int32_t colo_id,
    const std::string& referer,
    const OptOperation operation,
    const std::uint32_t status,
    const std::uint32_t user_status,
    const bool log_as_test,
    const std::string& browser,
    const std::string& os,
    const std::string& ct,
    const std::string& curct,
    const AdServer::Commons::UserId& user_id);

  void fill_common_ad_request_info(
    const CommonAdRequestInfo& common_info,
    AdServer::CampaignSvcs::Proto::CommonAdRequestInfo& common_info_proto);

  void fill_context_ad_request_info(
    const ContextAdRequestInfo& context_info,
    AdServer::CampaignSvcs::Proto::ContextAdRequestInfo& context_info_proto);

  GetCampaignCreativeRequestPtr create_get_campaign_creative_request(
    const RequestParams& request_params);

  InstantiateAdRequestPtr create_instantiate_ad_request(
    const InstantiateAdInfo& instantiate_ad);

  GetClickUrlRequestPtr create_get_click_url_request(
    const Generics::Time& time,
    const Generics::Time& bid_time,
    const std::uint32_t colo_id,
    const std::uint32_t tag_id,
    const std::uint32_t tag_size_id,
    const std::uint32_t ccid,
    const std::uint32_t ccg_keyword_id,
    const std::uint32_t creative_id,
    const AdServer::Commons::UserId& match_user_id,
    const AdServer::Commons::UserId& cookie_user_id,
    const AdServer::Commons::RequestId& request_id,
    const UserIdHashModInfo& user_id_hash_mod,
    const std::string& relocate,
    const std::string& referer,
    const bool log_click,
    const CTRDecimal& ctr,
    const std::vector<TokenInfo>& tokens);

  GetFileRequestPtr create_get_file_request(
    const std::string& file_name);

  VerifyImpressionRequestPtr create_verify_impression_request(
    const Generics::Time& time,
    const Generics::Time& bid_time,
    const UserIdHashModInfo& user_id_hash_mod,
    const std::vector<TrackCreativeInfo>& creatives,
    const std::uint32_t pub_imp_revenue_type,
    const CTRDecimal& pub_imp_revenue,
    const std::uint32_t request_type,
    const std::uint32_t verify_type,
    const AdServer::Commons::UserId& user_id,
    const std::string& referer,
    const std::int32_t viewability,
    const std::string& action_name);

  template<class Client, class Request, class Response, class ...Args>
  std::unique_ptr<Response> do_request_service(
    const ClientHolderPtr& client_holder,
    Args&& ...args) noexcept;

  template<class Client, class Request, class Response, class ...Args>
  std::unique_ptr<Response> do_request(
    const std::optional<std::string_view> service_id,
    Args&& ...args) noexcept;

private:
  const Logger_var logger_;

  TaskProcessor& task_processor_;

  const SchedulerPtr scheduler_;

  const FactoryClientHolderPtr factory_client_holder_;

  const std::size_t grpc_client_timeout_ms_;

  ClientHolders client_holders_;

  ServiceIds service_ids_;

  ServiceIdToClientHolder service_id_to_client_holder_;

  std::atomic<std::size_t> counter_{0};
};

class GrpcCampaignManagerPool::ClientHolder final : private Generics::Uncopyable
{
public:
  using GetCampaignCreativeClient = AdServer::CampaignSvcs::Proto::CampaignManager_get_campaign_creative_ClientPool;
  using GetCampaignCreativeClientPtr = AdServer::CampaignSvcs::Proto::CampaignManager_get_campaign_creative_ClientPoolPtr;
  using ProcessMatchRequestClient = AdServer::CampaignSvcs::Proto::CampaignManager_process_match_request_ClientPool;
  using ProcessMatchRequestClientPtr = AdServer::CampaignSvcs::Proto::CampaignManager_process_match_request_ClientPoolPtr;
  using MatchGeoChannelsClient = AdServer::CampaignSvcs::Proto::CampaignManager_match_geo_channels_ClientPool;
  using MatchGeoChannelsClientPtr = AdServer::CampaignSvcs::Proto::CampaignManager_match_geo_channels_ClientPoolPtr;
  using InstantiateAdClient = AdServer::CampaignSvcs::Proto::CampaignManager_instantiate_ad_ClientPool;
  using InstantiateAdClientPtr = AdServer::CampaignSvcs::Proto::CampaignManager_instantiate_ad_ClientPoolPtr;
  using GetChannelLinksClient = AdServer::CampaignSvcs::Proto::CampaignManager_get_channel_links_ClientPool;
  using GetChannelLinksClientPtr = AdServer::CampaignSvcs::Proto::CampaignManager_get_channel_links_ClientPoolPtr;
  using GetDiscoverChannelsClient = AdServer::CampaignSvcs::Proto::CampaignManager_get_discover_channels_ClientPool;
  using GetDiscoverChannelsClientPtr = AdServer::CampaignSvcs::Proto::CampaignManager_get_discover_channels_ClientPoolPtr;
  using GetCategoryChannelsClient = AdServer::CampaignSvcs::Proto::CampaignManager_get_category_channels_ClientPool;
  using GetCategoryChannelsClientPtr = AdServer::CampaignSvcs::Proto::CampaignManager_get_category_channels_ClientPoolPtr;
  using ConsiderPassbackClient = AdServer::CampaignSvcs::Proto::CampaignManager_consider_passback_ClientPool;
  using ConsiderPassbackClientPtr = AdServer::CampaignSvcs::Proto::CampaignManager_consider_passback_ClientPoolPtr;
  using ConsiderPassbackTrackClient = AdServer::CampaignSvcs::Proto::CampaignManager_consider_passback_track_ClientPool;
  using ConsiderPassbackTrackClientPtr = AdServer::CampaignSvcs::Proto::CampaignManager_consider_passback_track_ClientPoolPtr;
  using GetClickUrlClient = AdServer::CampaignSvcs::Proto::CampaignManager_get_click_url_ClientPool;
  using GetClickUrlClientPtr = AdServer::CampaignSvcs::Proto::CampaignManager_get_click_url_ClientPoolPtr;
  using VerifyImpressionClient = AdServer::CampaignSvcs::Proto::CampaignManager_verify_impression_ClientPool;
  using VerifyImpressionClientPtr = AdServer::CampaignSvcs::Proto::CampaignManager_verify_impression_ClientPoolPtr;
  using ActionTakenClient = AdServer::CampaignSvcs::Proto::CampaignManager_action_taken_ClientPool;
  using ActionTakenClientPtr = AdServer::CampaignSvcs::Proto::CampaignManager_action_taken_ClientPoolPtr;
  using VerifyOptOperationClient = AdServer::CampaignSvcs::Proto::CampaignManager_verify_opt_operation_ClientPool;
  using VerifyOptOperationClientPtr = AdServer::CampaignSvcs::Proto::CampaignManager_verify_opt_operation_ClientPoolPtr;
  using ConsiderWebOperationClient = AdServer::CampaignSvcs::Proto::CampaignManager_consider_web_operation_ClientPool;
  using ConsiderWebOperationClientPtr = AdServer::CampaignSvcs::Proto::CampaignManager_consider_web_operation_ClientPoolPtr;
  using GetConfigClient = AdServer::CampaignSvcs::Proto::CampaignManager_get_config_ClientPool;
  using GetConfigClientPtr = AdServer::CampaignSvcs::Proto::CampaignManager_get_config_ClientPoolPtr;
  using TraceCampaignSelectionIndexClient = AdServer::CampaignSvcs::Proto::CampaignManager_trace_campaign_selection_index_ClientPool;
  using TraceCampaignSelectionIndexClientPtr = AdServer::CampaignSvcs::Proto::CampaignManager_trace_campaign_selection_index_ClientPoolPtr;
  using TraceCampaignSelectionClient = AdServer::CampaignSvcs::Proto::CampaignManager_trace_campaign_selection_ClientPool;
  using TraceCampaignSelectionClientPtr = AdServer::CampaignSvcs::Proto::CampaignManager_trace_campaign_selection_ClientPoolPtr;
  using GetCampaignCreativeByCcidClient = AdServer::CampaignSvcs::Proto::CampaignManager_get_campaign_creative_by_ccid_ClientPool;
  using GetCampaignCreativeByCcidClientPtr = AdServer::CampaignSvcs::Proto::CampaignManager_get_campaign_creative_by_ccid_ClientPoolPtr;
  using GetColocationFlagsClient = AdServer::CampaignSvcs::Proto::CampaignManager_get_colocation_flags_ClientPool;
  using GetColocationFlagsClientPtr = AdServer::CampaignSvcs::Proto::CampaignManager_get_colocation_flags_ClientPoolPtr;
  using GetPubPixelsClient = AdServer::CampaignSvcs::Proto::CampaignManager_get_pub_pixels_ClientPool;
  using GetPubPixelsClientPtr = AdServer::CampaignSvcs::Proto::CampaignManager_get_pub_pixels_ClientPoolPtr;
  using ProcessAnonymousRequestClient = AdServer::CampaignSvcs::Proto::CampaignManager_process_anonymous_request_ClientPool;
  using ProcessAnonymousRequestClientPtr = AdServer::CampaignSvcs::Proto::CampaignManager_process_anonymous_request_ClientPoolPtr;
  using GetFileClient = AdServer::CampaignSvcs::Proto::CampaignManager_get_file_ClientPool;
  using GetFileClientPtr = AdServer::CampaignSvcs::Proto::CampaignManager_get_file_ClientPoolPtr;

  struct Clients final
  {
    GetCampaignCreativeClientPtr get_campaign_creative_client;
    ProcessMatchRequestClientPtr process_match_request_client;
    MatchGeoChannelsClientPtr match_geo_channels_client;
    InstantiateAdClientPtr instantiate_ad_client;
    GetChannelLinksClientPtr get_channel_links_client;
    GetDiscoverChannelsClientPtr get_discover_channels_client;
    GetCategoryChannelsClientPtr get_category_channels_client;
    ConsiderPassbackClientPtr consider_passback_client;
    ConsiderPassbackTrackClientPtr consider_passback_track_client;
    GetClickUrlClientPtr get_click_url_client;
    VerifyImpressionClientPtr verify_impression_client;
    ActionTakenClientPtr action_taken_client;
    VerifyOptOperationClientPtr verify_opt_operation_client;
    ConsiderWebOperationClientPtr consider_web_operation_client;
    GetConfigClientPtr get_config_client;
    TraceCampaignSelectionIndexClientPtr trace_campaign_selection_index_client;
    TraceCampaignSelectionClientPtr trace_campaign_selection_client;
    GetCampaignCreativeByCcidClientPtr get_campaign_creative_by_ccid_client;
    GetColocationFlagsClientPtr get_colocation_flags_client;
    GetPubPixelsClientPtr get_pub_pixels_client;
    ProcessAnonymousRequestClientPtr process_anonymous_request_client;
    GetFileClientPtr get_file_client;
  };
  using ClientsPtr = std::shared_ptr<Clients>;

private:
  using Status = UServerUtils::Grpc::Client::Status;
  using Mutex = std::shared_mutex;

public:
  explicit ClientHolder(
    const ClientsPtr& clients,
    const Generics::Time& time_duration_client_bad)
    : clients_(clients),
      time_duration_client_bad_(time_duration_client_bad)
  {
  }

  ~ClientHolder() = default;

  bool is_bad() const noexcept
  {
    {
      std::shared_lock lock(mutex_);
      if (!marked_as_bad_)
        return false;
    }

    const Generics::Time now = Generics::Time::get_time_of_day();
    {
      std::shared_lock lock(mutex_);
      if (marked_as_bad_ && now < marked_as_bad_time_ + time_duration_client_bad_)
      {
        return true;
      }
    }

    std::unique_lock lock(mutex_);
    if (marked_as_bad_ && now >= marked_as_bad_time_ + time_duration_client_bad_)
    {
      marked_as_bad_ = false;
    }

    return marked_as_bad_;
  }

  template<class Client, class Request, class Response>
  std::unique_ptr<Response> do_request(
    std::unique_ptr<Request>&& request,
    const std::size_t timeout) noexcept
  {
    using ClientPtr = std::shared_ptr<Client>;

    ClientPtr client;
    if constexpr (std::is_same_v<Client, GetPubPixelsClient>)
    {
      client = clients_->get_pub_pixels_client;
    }
    else if constexpr (std::is_same_v<Client, GetCampaignCreativeClient>)
    {
      client = clients_->get_campaign_creative_client;
    }
    else if constexpr (std::is_same_v<Client, ProcessMatchRequestClient>)
    {
      client = clients_->process_match_request_client;
    }
    else if constexpr (std::is_same_v<Client, MatchGeoChannelsClient>)
    {
      client = clients_->match_geo_channels_client;
    }
    else if constexpr (std::is_same_v<Client, InstantiateAdClient>)
    {
      client = clients_->instantiate_ad_client;
    }
    else if constexpr (std::is_same_v<Client, GetChannelLinksClient>)
    {
      client = clients_->get_channel_links_client;
    }
    else if constexpr (std::is_same_v<Client, GetDiscoverChannelsClient>)
    {
      client = clients_->get_discover_channels_client;
    }
    else if constexpr (std::is_same_v<Client, GetCategoryChannelsClient>)
    {
      client = clients_->get_category_channels_client;
    }
    else if constexpr (std::is_same_v<Client, ConsiderPassbackClient>)
    {
      client = clients_->consider_passback_client;
    }
    else if constexpr (std::is_same_v<Client, ConsiderPassbackTrackClient>)
    {
      client = clients_->consider_passback_track_client;
    }
    else if constexpr (std::is_same_v<Client, GetClickUrlClient>)
    {
      client = clients_->get_click_url_client;
    }
    else if constexpr (std::is_same_v<Client, VerifyImpressionClient>)
    {
      client = clients_->verify_impression_client;
    }
    else if constexpr (std::is_same_v<Client, ActionTakenClient>)
    {
      client = clients_->action_taken_client;
    }
    else if constexpr (std::is_same_v<Client, VerifyOptOperationClient>)
    {
      client = clients_->verify_opt_operation_client;
    }
    else if constexpr (std::is_same_v<Client, ConsiderWebOperationClient>)
    {
      client = clients_->consider_web_operation_client;
    }
    else if constexpr (std::is_same_v<Client, GetConfigClient>)
    {
      client = clients_->get_config_client;
    }
    else if constexpr (std::is_same_v<Client, TraceCampaignSelectionIndexClient>)
    {
      client = clients_->trace_campaign_selection_index_client;
    }
    else if constexpr (std::is_same_v<Client, TraceCampaignSelectionClient>)
    {
      client = clients_->trace_campaign_selection_client;
    }
    else if constexpr (std::is_same_v<Client, GetCampaignCreativeByCcidClient>)
    {
      client = clients_->get_campaign_creative_by_ccid_client;
    }
    else if constexpr (std::is_same_v<Client, GetColocationFlagsClient>)
    {
      client = clients_->get_colocation_flags_client;
    }
    else if constexpr (std::is_same_v<Client, ProcessAnonymousRequestClient>)
    {
      client = clients_->process_anonymous_request_client;
    }
    else if constexpr (std::is_same_v<Client, GetFileClient>)
    {
      client = clients_->get_file_client;
    }
    else
    {
      static_assert(GrpcAlgs::AlwaysFalseV<Client>);
    }

    for (std::size_t i = 1; i <= 5; ++i)
    {
      auto result = client->write(std::move(request), timeout);
      if (result.status == Status::Ok)
      {
        return std::move(result.response);
      }
    }

    set_bad();
    return {};
  }

private:
  void set_bad() noexcept
  {
    try
    {
      const Generics::Time now = Generics::Time::get_time_of_day();

      std::unique_lock lock(mutex_);
      marked_as_bad_ = true;
      marked_as_bad_time_ = now;
    }
    catch (...)
    {
    }
  }

private:
  const ClientsPtr clients_;

  const Generics::Time time_duration_client_bad_;

  mutable Mutex mutex_;

  mutable bool marked_as_bad_ = false;

  Generics::Time marked_as_bad_time_;
};

class GrpcCampaignManagerPool::FactoryClientHolder final : private Generics::Uncopyable
{
public:
  using GetCampaignCreativeClient = AdServer::CampaignSvcs::Proto::CampaignManager_get_campaign_creative_ClientPool;
  using ProcessMatchRequestClient = AdServer::CampaignSvcs::Proto::CampaignManager_process_match_request_ClientPool;
  using MatchGeoChannelsClient = AdServer::CampaignSvcs::Proto::CampaignManager_match_geo_channels_ClientPool;
  using InstantiateAdClient = AdServer::CampaignSvcs::Proto::CampaignManager_instantiate_ad_ClientPool;
  using GetChannelLinksClient = AdServer::CampaignSvcs::Proto::CampaignManager_get_channel_links_ClientPool;
  using GetDiscoverChannelsClient = AdServer::CampaignSvcs::Proto::CampaignManager_get_discover_channels_ClientPool;
  using GetCategoryChannelsClient = AdServer::CampaignSvcs::Proto::CampaignManager_get_category_channels_ClientPool;
  using ConsiderPassbackClient = AdServer::CampaignSvcs::Proto::CampaignManager_consider_passback_ClientPool;
  using ConsiderPassbackTrackClient = AdServer::CampaignSvcs::Proto::CampaignManager_consider_passback_track_ClientPool;
  using GetClickUrlClient = AdServer::CampaignSvcs::Proto::CampaignManager_get_click_url_ClientPool;
  using VerifyImpressionClient = AdServer::CampaignSvcs::Proto::CampaignManager_verify_impression_ClientPool;
  using ActionTakenClient = AdServer::CampaignSvcs::Proto::CampaignManager_action_taken_ClientPool;
  using VerifyOptOperationClient = AdServer::CampaignSvcs::Proto::CampaignManager_verify_opt_operation_ClientPool;
  using ConsiderWebOperationClient = AdServer::CampaignSvcs::Proto::CampaignManager_consider_web_operation_ClientPool;
  using GetConfigClient = AdServer::CampaignSvcs::Proto::CampaignManager_get_config_ClientPool;
  using TraceCampaignSelectionIndexClient = AdServer::CampaignSvcs::Proto::CampaignManager_trace_campaign_selection_index_ClientPool;
  using TraceCampaignSelectionClient = AdServer::CampaignSvcs::Proto::CampaignManager_trace_campaign_selection_ClientPool;
  using GetCampaignCreativeByCcidClient = AdServer::CampaignSvcs::Proto::CampaignManager_get_campaign_creative_by_ccid_ClientPool;
  using GetColocationFlagsClient = AdServer::CampaignSvcs::Proto::CampaignManager_get_colocation_flags_ClientPool;
  using GetPubPixelsClient = AdServer::CampaignSvcs::Proto::CampaignManager_get_pub_pixels_ClientPool;
  using ProcessAnonymousRequestClient = AdServer::CampaignSvcs::Proto::CampaignManager_process_anonymous_request_ClientPool;
  using GetFileClient = AdServer::CampaignSvcs::Proto::CampaignManager_get_file_ClientPool;
  using GrpcPoolClientFactory = UServerUtils::Grpc::Client::PoolClientFactory;

private:
  using Mutex = std::shared_mutex;
  using Host = typename Endpoint::Host;
  using Port = typename Endpoint::Port;
  using Key = std::pair<Host, Port>;
  using Value = ClientHolder::ClientsPtr;
  using Cache = std::unordered_map<Key, Value, boost::hash<Key>>;

public:
  FactoryClientHolder(
    Logger* logger,
    const SchedulerPtr& scheduler,
    const ConfigPoolClient& config,
    TaskProcessor& task_processor,
    const Generics::Time& time_duration_client_bad)
    : logger_(ReferenceCounting::add_ref(logger)),
      scheduler_(scheduler),
      config_(config),
      task_processor_(task_processor),
      time_duration_client_bad_(time_duration_client_bad)
  {
  }

  ClientHolderPtr create(const Host& host, const Port port)
  {
    Key key(host, port);
    {
      std::shared_lock lock(mutex_);
      auto it = cache_.find(key);
      if (it != std::end(cache_))
      {
        auto clients = it->second;
        lock.unlock();
        return std::make_shared<ClientHolder>(
          clients,
          time_duration_client_bad_);
      }
    }

    auto clients = std::make_shared<ClientHolder::Clients>();
    auto config = config_;

    std::unique_lock lock(mutex_);
    auto it = cache_.find(key);
    if (it == std::end(cache_))
    {
      std::stringstream endpoint_stream;
      endpoint_stream << host
                      << ":"
                      << port;
      std::string endpoint = endpoint_stream.str();
      config.endpoint = endpoint;

      GrpcPoolClientFactory factory(
        logger_.in(),
        scheduler_,
        config);

      clients->get_campaign_creative_client = factory.create<GetCampaignCreativeClient>(task_processor_);
      clients->process_match_request_client = factory.create<ProcessMatchRequestClient>(task_processor_);
      clients->match_geo_channels_client = factory.create<MatchGeoChannelsClient>(task_processor_);
      clients->instantiate_ad_client = factory.create<InstantiateAdClient>(task_processor_);
      clients->get_channel_links_client = factory.create<GetChannelLinksClient>(task_processor_);
      clients->get_discover_channels_client = factory.create<GetDiscoverChannelsClient>(task_processor_);
      clients->get_category_channels_client = factory.create<GetCategoryChannelsClient>(task_processor_);
      clients->consider_passback_client = factory.create<ConsiderPassbackClient>(task_processor_);
      clients->consider_passback_track_client = factory.create<ConsiderPassbackTrackClient>(task_processor_);
      clients->get_click_url_client = factory.create<GetClickUrlClient>(task_processor_);
      clients->verify_impression_client = factory.create<VerifyImpressionClient>(task_processor_);
      clients->action_taken_client = factory.create<ActionTakenClient>(task_processor_);
      clients->verify_opt_operation_client = factory.create<VerifyOptOperationClient>(task_processor_);
      clients->consider_web_operation_client = factory.create<ConsiderWebOperationClient>(task_processor_);
      clients->get_config_client = factory.create<GetConfigClient>(task_processor_);
      clients->trace_campaign_selection_index_client = factory.create<TraceCampaignSelectionIndexClient>(task_processor_);
      clients->trace_campaign_selection_client = factory.create<TraceCampaignSelectionClient>(task_processor_);
      clients->get_campaign_creative_by_ccid_client = factory.create<GetCampaignCreativeByCcidClient>(task_processor_);
      clients->get_colocation_flags_client = factory.create<GetColocationFlagsClient>(task_processor_);
      clients->get_pub_pixels_client = factory.create<GetPubPixelsClient>(task_processor_);
      clients->process_anonymous_request_client = factory.create<ProcessAnonymousRequestClient>(task_processor_);
      clients->get_file_client = factory.create<GetFileClient>(task_processor_);

      it = cache_.try_emplace(
        key,
        std::move(clients)).first;
    }

    clients = it->second;
    lock.unlock();

    return std::make_shared<ClientHolder>(
      clients,
      time_duration_client_bad_);
  }

private:
  const Logger_var logger_;

  const SchedulerPtr scheduler_;

  const ConfigPoolClient config_;

  TaskProcessor& task_processor_;

  const Generics::Time time_duration_client_bad_;

  Mutex mutex_;

  Cache cache_;
};

using GrpcCampaignManagerPoolPtr = std::shared_ptr<GrpcCampaignManagerPool>;

} // namespace FrontendCommons

#endif // FRONTENDCOMMONS_GRPCCAMPAIGNMANAGERPOOL_HPP