#pragma once

#include <string>
#include <list>
#include <map>
#include <optional>
#include <vector>

#include <eh/Exception.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/PtrHolder.hpp>
#include <Logger/Logger.hpp>
#include <Generics/ActiveObject.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Sync/SyncPolicy.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <String/StringManip.hpp>

#include <Commons/IPCrypter.hpp>
#include <Commons/Coro.hpp>
#include <Commons/SecToken.hpp>
#include <Commons/TextTemplateCache.hpp>
#include <LogCommons/AdRequestLogger.hpp>

#include <xsd/CampaignSvcs/CampaignManagerConfig.hpp>

#include "DomainParser.hpp"
#include "CampaignManagerDeclarations.hpp"
#include "CampaignConfig.hpp"
#include "CampaignIndex.hpp"
#include "CreativeTemplate.hpp"
#include "CampaignSelector.hpp"
#include "CampaignManagerLogger.hpp"
#include "CampaignConfigSource.hpp"
#include "PassbackTemplate.hpp"
#include "BillingStateContainer.hpp"

#include "CTRProvider.hpp"
#include "BidCostProvider.hpp"

namespace AdServer
{
  namespace CampaignSvcs
  {
    using namespace AdServer::CampaignSvcs::AdInstances;

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
      struct CreativeInstantiate;
      struct CreativeParams;

      typedef std::vector<unsigned char> ByteVector;
      typedef std::vector<unsigned long> IdVector;
      typedef std::vector<std::string> StringVector;
      typedef std::vector<AdServer::Commons::RequestId> RequestIdVector;

      struct TokenInfo
      {
        std::string name;
        std::string value;
      };

      typedef std::vector<TokenInfo> TokenVector;

      struct TokenImageInfo
      {
        std::string name;
        std::string value;
        unsigned long width = 0;
        unsigned long height = 0;
      };

      typedef std::vector<TokenImageInfo> TokenImageVector;

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

      struct ChannelWeight
      {
        unsigned long channel_id = 0;
        unsigned long weight = 0;
      };

      struct ChannelSearchResult
      {
        unsigned long channel_id = 0;
        unsigned long use_count = 0;
        IdVector matched_simple_channels;
        IdVector ccg_ids;
        std::string discover_query;
        std::string language;
      };

      struct DiscoverChannelResult
      {
        unsigned long channel_id = 0;
        std::string name;
        std::string query;
        std::string annotation;
        unsigned long weight = 0;
        IdVector categories;
        std::string country_code;
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

      struct PassbackInfo
      {
        AdServer::Commons::RequestId request_id;
        Generics::Time time;
        AdServer::Commons::Optional<unsigned long> user_id_hash_mod;
      };

      struct PassbackTrackInfo
      {
        Generics::Time time;
        std::string country;
        unsigned long colo_id = 0;
        unsigned long tag_id = 0;
        unsigned long user_status = 0;
      };

      struct AnonymousRequestInfo
      {
        Generics::Time time;
        unsigned long colo_id = 0;
        unsigned long user_status = 0;
        bool test_request = false;
        unsigned long search_engine_id = 0;
        std::string search_words;
        std::string client;
        std::string client_version;
        IdVector platform_ids;
        std::string full_platform;
        std::string web_browser;
        std::string user_agent;
        std::string search_engine_host;
        std::string country_code;
        bool page_keywords_present = false;
      };

      struct WebOperationInfo
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
        RequestIdVector request_ids;
        AdServer::Commons::RequestId global_request_id;
        std::string referer;
        std::string ip_address;
        std::string external_user_id;
        std::string user_agent;
      };

      struct ActionInfo
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
        IdVector platform_ids;
        std::string peer_ip;
        std::vector<GeoInfo> location;
      };

      typedef std::map<std::string, std::string> TokenValueMap;

      struct ClickInfo
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

      struct ClickResultInfo
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
        RevenueDecimal ctr = RevenueDecimal::ZERO;
      };

      typedef std::vector<TrackCreativeInfo> TrackCreativeVector;

      struct ImpressionInfo
      {
        Generics::Time time;
        Generics::Time bid_time;
        AdServer::Commons::Optional<unsigned long> user_id_hash_mod;
        TrackCreativeVector creatives;
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

      typedef std::vector<TrackCreativeResultInfo> ImpressionResultInfo;

      struct ChannelTriggerMatchInfo
      {
        unsigned long channel_trigger_id = 0;
        unsigned long channel_id = 0;
      };

      typedef std::vector<ChannelTriggerMatchInfo> ChannelTriggerMatchVector;

      struct TriggerMatchResultInfo
      {
        ChannelTriggerMatchVector url_channels;
        ChannelTriggerMatchVector pkw_channels;
        ChannelTriggerMatchVector skw_channels;
        ChannelTriggerMatchVector ukw_channels;
        IdVector uid_channels;
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

      typedef std::vector<CCGKeywordInfo> CCGKeywordVector;

      struct CommonMatchRequestInfo
      {
        IdVector channels;
        ChannelTriggerMatchVector pkw_channels;
        IdVector hid_channels;
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

      struct CommonAdRequestInfo
      {
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
        StringVector urls;
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
        TokenVector tokens;
        bool set_cookie = false;
        std::string passback_type;
        std::string passback_url;
      };

      struct ContextAdRequestInfo
      {
        bool enabled_notice = false;
        std::string client;
        std::string client_version;
        IdVector platform_ids;
        IdVector geo_channels;
        std::string platform;
        std::string full_platform;
        std::string web_browser;
        std::string ip_hash;
        bool profile_referer = false;
        unsigned long page_load_id = 0;
        unsigned long full_referer_hash = 0;
        unsigned long short_referer_hash = 0;
      };

      struct InstantiateAdInfo
      {
        CommonAdRequestInfo common_info;
        std::vector<ContextAdRequestInfo> context_info;
        std::string format;
        unsigned long publisher_site_id = 0;
        unsigned long publisher_account_id = 0;
        unsigned long tag_id = 0;
        unsigned long tag_size_id = 0;
        TrackCreativeVector creatives;
        unsigned long creative_id = 0;
        AdServer::Commons::Optional<unsigned long> user_id_hash_mod;
        AdServer::Commons::UserId merged_user_id;
        IdVector pubpixel_accounts;
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
        RequestIdVector request_ids;
      };

      struct TraceRequestInfo
      {
        CommonAdRequestInfo common_info;
        ContextAdRequestInfo context_info;
        unsigned long publisher_site_id = 0;
        IdVector publisher_account_ids;
        bool profiling_available = false;
        IdVector full_freq_caps;
        IdVector channels;
        IdVector hid_channels;
        Generics::Time client_create_time;
        unsigned long tag_delivery_factor = 0;
        unsigned long ccg_delivery_factor = 0;
      };

      struct TraceAdSlotInfo
      {
        unsigned long ad_slot_id = 0;
        std::string format;
        unsigned long tag_id = 0;
        StringVector sizes;
        std::string ext_tag_id;
        RevenueDecimal min_ecpm = RevenueDecimal::ZERO;
        std::string min_ecpm_currency_code;
        StringVector currency_codes;
        bool passback = false;
        StringVector exclude_categories;
        StringVector required_categories;
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
        IdVector allowed_durations;
        std::vector<NativeDataTokenInfo> native_data_tokens;
        std::vector<NativeImageTokenInfo> native_image_tokens;
        unsigned long native_ads_impression_tracker_type = 0;
        bool fill_track_html = false;
        TokenVector tokens;
      };

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

      struct CreativeRequestInfo
      {
        CommonAdRequestInfo common_info;
        ContextAdRequestInfo context_info;
        unsigned long publisher_site_id = 0;
        IdVector publisher_account_ids;
        bool fill_track_pixel = false;
        bool fill_iurl = false;
        unsigned long ad_instantiate_type = 0;
        bool only_display_ad = false;
        IdVector full_freq_caps;
        std::vector<SeqOrderInfo> seq_orders;
        std::vector<CampaignFreqInfo> campaign_freqs;
        AdServer::Commons::UserId household_id;
        AdServer::Commons::UserId merged_user_id;
        unsigned long search_engine_id = 0;
        std::string search_words;
        bool page_keywords_present = false;
        bool profiling_available = false;
        bool fraud = false;
        IdVector channels;
        IdVector hid_channels;
        CCGKeywordVector ccg_keywords;
        CCGKeywordVector hid_ccg_keywords;
        TriggerMatchResultInfo trigger_match_result;
        Generics::Time client_create_time;
        Generics::Time session_start;
        IdVector exclude_pubpixel_accounts;
        unsigned long tag_delivery_factor = 0;
        unsigned long ccg_delivery_factor = 0;
        unsigned long preview_ccid = 0;
        std::vector<TraceAdSlotInfo> ad_slots;
        bool required_passback = false;
        unsigned long profiling_type = 0;
        bool disable_fraud_detection = false;
        bool need_debug_info = false;
        std::string page_keywords;
        std::string url_keywords;
        std::string ssp_location;
        std::string additional_info;
      };

      struct CreativeSelectDebugInfo
      {
        RevenueDecimal imp_revenue = RevenueDecimal::ZERO;
        RevenueDecimal click_revenue = RevenueDecimal::ZERO;
        RevenueDecimal action_revenue = RevenueDecimal::ZERO;
        RevenueDecimal ecpm_bid = RevenueDecimal::ZERO;
        std::string action_adv_url;
        std::string html_url;
        std::string triggered_expression;
        std::string full_expression;
      };

      struct CreativeSelectResultInfo
      {
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

      struct AdSlotDebugInfo
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
        std::vector<CreativeSelectDebugInfo> selected_creatives;
        std::string trace_ccg;
      };

      struct ContractInfo
      {
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

      struct AdSlotResultInfo
      {
        unsigned long ad_slot_id = 0;
        AdServer::Commons::RequestId request_id;
        bool passback = false;
        std::string passback_url;
        std::string creative_body;
        std::string notice_url;
        StringVector track_pixel_urls;
        std::string yandex_track_params;
        std::string creative_url;
        std::string track_pixel_params;
        std::string click_params;
        std::string mime_format;
        std::string iurl;
        bool test_request = false;
        std::vector<CreativeSelectResultInfo> selected_creatives;
        StringVector external_visual_categories;
        StringVector external_content_categories;
        std::string pub_currency_code;
        unsigned long overlay_width = 0;
        unsigned long overlay_height = 0;
        TokenVector tokens;
        TokenVector ext_tokens;
        bool track_impr = false;
        std::string tag_size;
        IdVector freq_caps;
        IdVector uc_freq_caps;
        AdSlotDebugInfo debug_info;
        TokenVector native_data_tokens;
        TokenImageVector native_image_tokens;
        std::string track_html_body;
        std::string erid;
        std::vector<ExtContractInfo> contracts;
      };

      struct AdRequestDebugInfo
      {
        unsigned long colo_id = 0;
        IdVector geo_channels;
        IdVector platform_channels;
        unsigned long last_platform_channel_id = 0;
        unsigned long user_group_id = 0;
      };

      struct CreativeRequestResultInfo
      {
        std::string hostname;
        std::vector<AdSlotResultInfo> ad_slots;
        Generics::Time process_time;
        AdRequestDebugInfo debug_info;
      };

    public:
      typedef xsd::AdServer::Configuration::CampaignManagerType
        CampaignManagerConfig;

      /** Parametric constructor.
       *
       * @param callback Instance of a callback class.
       * @param logger Instance of a logger class.
       *
       */
      CampaignManagerCore(
        const CampaignManagerConfig& configuration,
        const DomainParser::DomainConfig& domain_config,
        Generics::ActiveObjectCallback* callback,
        Logging::Logger* logger,
        CampaignManagerLogger* campaign_manager_logger,
        const CreativeInstantiate& creative_instantiate,
        const char* campaigns_types)
        /*throw(InvalidArgument, Exception, eh::Exception)*/;

      virtual bool ready() /*throw(eh::Exception)*/;

      AdServer::Commons::Task<CreativeRequestResultInfo>
      co_get_campaign_creative(
        const CreativeRequestInfo& request_params);

      void
      match_geo_channels(
        const std::vector<GeoInfo>& location,
        const std::vector<GeoCoordInfo>& coord_location,
        IdVector& geo_channels_result,
        IdVector& coord_channels_result)
        /*throw(NotReady, Exception)*/;

      void process_match_request(const MatchRequestInfo& match_request_info)
        /*throw(Exception, NotReady)*/;

      virtual
      void
      process_anonymous_request(
        const AnonymousRequestInfo& anon_request_info)
        /*throw(Exception, NotReady)*/;

      ByteVector
      get_file(const std::string& file_name)
        /*throw(Exception)*/;

      InstantiateAdResult instantiate_ad(
        const InstantiateAdInfo& instantiate_ad_info)
        /*throw(Exception, NotReady)*/;

      std::string trace_campaign_selection(
        unsigned long campaign_id,
        const TraceRequestInfo& request_params,
        const TraceAdSlotInfo& ad_slot,
        unsigned long auction_type,
        bool test_request)
        /*throw(Exception)*/;

      std::string trace_campaign_selection_index()
        /*throw(Exception)*/;

      bool get_campaign_creative_by_ccid(
        const PreviewCreativeParams& params,
        std::string& creative_body)
        /*throw(Exception)*/;

      void consider_passback(
        const PassbackInfo& in)
        /*throw(Exception)*/;

      void consider_passback_track(
        const PassbackTrackInfo& in)
        /*throw(Exception, NotReady)*/;

      bool
      get_click_url(
        const ClickInfo& click_info,
        ClickResultInfo& click_result_info)
        /*throw(Exception, NotReady)*/;

      ImpressionResultInfo verify_impression(
        const ImpressionInfo& impression_info)
        /*throw(Exception, NotReady)*/;

      void action_taken(const ActionInfo& action_info)
        /*throw(Exception, NotReady)*/;

      std::vector<ChannelSearchResult>
      get_channel_links(
        const IdVector& channels,
        bool match)
        /*throw(Exception)*/;

      std::vector<DiscoverChannelResult>
      get_discover_channels(
        const std::vector<ChannelWeight>& channels,
        const std::string& country,
        const std::string& language,
        bool all)
        /*throw(NotReady, Exception)*/;

      std::vector<CategoryChannelNodeInfo>
      get_category_channels(const std::string& language)
        /*throw(NotReady, Exception)*/;

      //
      // IDL:AdServer/CampaignSvcs/CampaignManager/get_config:1.0
      //
      CampaignConfig_var
      get_config(const ConfigRequestInfo& get_config_props)
        /*throw(Exception)*/;

      //
      // IDL:AdServer/CampaignSvcs/CampaignManager/verify_opt_operation:1.0
      //
      virtual void
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


      virtual void
      consider_web_operation(
        const WebOperationInfo& web_op_info)
        /*throw(InvalidArgument, NotReady)*/;

      std::vector<ColocationFlagsInfo>
      get_colocation_flags()
        /*throw(NotReady, Exception)*/;

      StringVector
      get_pub_pixels(
        const std::string& country,
        unsigned long user_status,
        const IdVector& publisher_account_ids)
        /*throw(NotReady, Exception)*/;

      void
      progress_comment(std::string& res) /*throw(eh::Exception)*/;

    private:
      virtual
      ~CampaignManagerCore() noexcept {}

    public:
      struct CreativeInstantiate
      {
        struct SourceRule
        {
          Commons::Optional<std::string> click_prefix;
          std::string mime_encoded_click_prefix;

          std::string preclick;
          std::string mime_encoded_preclick;

          std::string vast_preclick;
          std::string mime_encoded_vast_preclick;
        };

        typedef std::map<std::string, SourceRule>
          SourceRuleMap;

        CreativeInstantiateRuleMap creative_rules;
        SourceRuleMap source_rules;
        std::string template_local_prefix;
        std::string creative_template_dir;
      };

      struct ImageToken
      {
        std::string value;
        unsigned long width;
        unsigned long height;
      };

      struct RequestResultParams
      {
        RequestResultParams()
          : overlay_width(0), overlay_height(0)
        {}

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
        std::map<std::string, std::string> tokens;
        std::map<std::string, std::string> ext_tokens;
        std::map<std::string, std::string> native_data_tokens;
        std::map<std::string, ImageToken> native_image_tokens;
      };

      struct CreativeParams
      {
        std::string click_url;
        std::string action_adv_url; // TODO: remove - required only in debug info
      };

      typedef std::list<CreativeParams> CreativeParamsList;

      struct AdSlotMinCpm
      {
        RevenueDecimal min_pub_ecpm;
        RevenueDecimal min_pub_ecpm_system;

        AdSlotMinCpm() noexcept;
      };


      struct AdSlotContext
      {
        bool test_request;
        std::string passback_url;
        FreqCapIdSet full_freq_caps;
        bool request_blacklisted;
        Commons::Optional<RevenueDecimal> pub_imp_revenue;
        std::string tag_size;
        std::string tns_counter_device_type;
        unsigned long publisher_account_id;
        TokenValueMap tokens;

        AdSlotContext() noexcept;
      };

    private:
      /* Task implementations */

      typedef Generics::TaskGoal TaskBase;

      class CampaignManagerTaskMessage: public TaskBase
      {
      public:
        CampaignManagerTaskMessage(
          CampaignManagerCore* manager,
          Generics::TaskRunner* task_runner)
          /*throw(eh::Exception)*/;

      protected:
        virtual
        ~CampaignManagerTaskMessage() noexcept = default;

        CampaignManagerCore* manager_;
      };

      typedef ReferenceCounting::SmartPtr<CampaignManagerTaskMessage>
        CampaignManagerTaskMessage_var;

      /* Task that periodically flushes the request logger */
      class FlushLogsTaskMessage: public CampaignManagerTaskMessage
      {
      public:
        FlushLogsTaskMessage(CampaignManagerCore* manager,
          Generics::TaskRunner* task_runner)
          /*throw(eh::Exception)*/;

        virtual void execute() noexcept;
      };

      class CheckConfigTaskMessage : public CampaignManagerTaskMessage
      {
      public:
        CheckConfigTaskMessage(CampaignManagerCore* manager,
          Generics::TaskRunner* task_runner)
          /*throw(eh::Exception)*/;

        virtual void execute() noexcept;
      };

      class UpdateCTRProviderTask: public CampaignManagerTaskMessage
      {
      public:
        UpdateCTRProviderTask(CampaignManagerCore* manager,
          Generics::TaskRunner* task_runner)
          /*throw(eh::Exception)*/;

        virtual void execute() noexcept;
      };

      class UpdateConvRateProviderTask: public CampaignManagerTaskMessage
      {
      public:
        UpdateConvRateProviderTask(CampaignManagerCore* manager,
          Generics::TaskRunner* task_runner)
          /*throw(eh::Exception)*/;

        virtual void execute() noexcept;
      };

      class UpdateBidCostProviderTask: public CampaignManagerTaskMessage
      {
      public:
        UpdateBidCostProviderTask(CampaignManagerCore* manager,
          Generics::TaskRunner* task_runner)
          /*throw(eh::Exception)*/;

        virtual void execute() noexcept;
      };

      struct TraceLevel
      {
        enum
        {
          LOW = Logging::Logger::TRACE,
          MIDDLE,
          HIGH
        };
      };

      typedef Sync::Policy::PosixThread SyncPolicy;

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
        std::string click_url_f;
        std::string preclick_url;
        std::string preclick_url_f;

        std::string click0_url; // click_url without pubpreclick
        std::string click0_url_f;
        std::string preclick0_url; // preclick_url without pubpreclick
        std::string preclick0_url_f;
      };

      typedef std::list<std::string> CountryList;

      struct ConfirmCreativeAmount
      {
        ConfirmCreativeAmount(
          unsigned long cc_id_val,
          const RevenueDecimal& ctr_val)
          noexcept;

        unsigned long cc_id;
        RevenueDecimal ctr;
      };

      typedef std::vector<ConfirmCreativeAmount> ConfirmCreativeAmountArray;

      typedef Generics::GnuHashTable<
        Generics::SubStringHashAdapter, std::string>
        TokenToParamMap;

    private:
      void
      select_adslot_campaign_creative_(
        const CreativeRequestInfo& request_params,
        const TraceAdSlotInfo& ad_slot,
        AdSlotContext& ad_slot_context,
        const AdSlotMinCpm& ad_slot_min_cpm,
        const Generics::Time& session_start,
        AdSlotResultInfo& ad_slot_result,
        const Tag* tag,
        const Colocation* colocation,
        bool passback,
        AdRequestDebugInfo* ad_request_debug_info,
        AdSlotDebugInfo* ad_slot_debug_info,
        const ChannelIdHashSet& matched_channels)
        /*throw(eh::Exception)*/;

      const Tag*
      resolve_tag(
        std::string* tag_size,
        unsigned long* selected_publisher_account_id,
        unsigned long request_type,
        unsigned long random,
        unsigned long publisher_site_id,
        const IdVector& publisher_account_ids,
        const CampaignConfig& campaign_config,
        unsigned long tag_id,
        const StringVector& sizes,
        const StringVector& currency_codes)
        const noexcept;

      AdServer::Commons::Task<bool>
      co_get_adslot_campaign_creative_(
        const CampaignConfig* campaign_config,
        AdSlotResultInfo& ad_slot_result,
        AdServer::CampaignSvcs::RevenueDecimal& adsspace_system_cpm,
        const CreativeRequestInfo& core_request_params,
        const Generics::Time& session_start,
        const Colocation* colocation,
        const TraceAdSlotInfo& core_ad_slot,
        const CreativeInstantiateRule& creative_instantiate_rule,
        AdRequestDebugInfo* debug_info,
        AdSlotContext& ad_slot_context,
        const ChannelIdHashSet& matched_channels,
        unsigned long& request_tag_id);

      bool
      get_campaign_creative_by_ccid_impl(
        const PreviewCreativeParams& params,
        std::string& creative_body)
        /*throw(eh::Exception)*/;

      void
      get_site_creative_(
        CampaignIndex* config_index,
        const Colocation* colocation,
        const Tag* tag,
        const Tag::SizeMap& tag_sizes,
        const CreativeRequestInfo& request_params,
        const TraceAdSlotInfo& ad_slot,
        AdSlotContext& ad_slot_context,
        const AdSlotMinCpm& ad_slot_min_cpm,
        const FreqCapIdSet& full_freq_caps,
        const SeqOrderMap& seq_orders,
        RequestResultParams& request_result_params,
        AdSelectionResult& select_result,
        std::string& creative_body,
        std::string& creative_url,
        AdRequestDebugInfo*
          ad_request_debug_info,
        AdSlotDebugInfo* ad_slot_debug_info)
        /*throw(eh::Exception)*/;


      AdServer::Commons::Task<bool>
      co_get_site_creative_(
        CampaignIndex* config_index,
        const Colocation* colocation,
        const Tag* requested_tag,
        const Tag::SizeMap& tag_sizes,
        const CreativeRequestInfo& request_params,
        const TraceAdSlotInfo& ad_slot,
        AdSlotContext& ad_slot_context,
        const AdSlotMinCpm& ad_slot_min_cpm,
        const FreqCapIdSet& full_freq_caps,
        const SeqOrderMap& seq_orders,
        RequestResultParams& request_result_params,
        AdSelectionResult& select_result,
        std::string& creative_body,
        std::string& creative_url,
        AdRequestDebugInfo* ad_request_debug_info,
        AdSlotDebugInfo* ad_slot_debug_info);

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
        const CreativeRequestInfo& request_params,
        const TraceAdSlotInfo& ad_slot,
        const CampaignSelector::WeightedCampaignKeywordList& campaign_keywords,
        AdSelectionResult& ad_selection_result,
        RequestResultParams& request_result_params,
        CreativeParamsList& creative_params_list,
        AdSlotDebugInfo* ad_slot_debug_info,
        std::string& creative_body,
        std::string& creative_url,
        AdSlotContext& ad_slot_context)
        /*throw(eh::Exception)*/;

      bool
      instantiate_display_creative(
        const CampaignConfig* config,
        const Colocation* colocation,
        const CreativeRequestInfo& request_params,
        const TraceAdSlotInfo& ad_slot,
        const CampaignSelector::WeightedCampaign& weighted_campaign,
        AdSelectionResult& ad_selection_result,
        RequestResultParams& request_result_params,
        CreativeParams& creative_params,
        AdSlotDebugInfo* debug_info,
        std::string& creative_body,
        std::string& creative_url,
        AdSlotContext& ad_slot_context)
        /*throw(eh::Exception)*/;

      bool
      instantiate_passback(
        std::string& mime_format,
        std::string& passback_body,
        const CampaignConfig* const campaign_config,
        const Colocation* colocation,
        const Tag* tag,
        const char* app_format,
        const CreativeRequestInfo& request_params,
        const AdSlotContext& ad_slot_context,
        const String::SubString& ext_tag_id)
        /*throw(eh::Exception)*/;

      void
      fill_instantiate_request_params_(
        TokenValueMap& request_args,
        AccountIdList* consider_pub_pixel_accounts, // result pub pixel accounts that instantiated
        const CampaignConfig* const campaign_config,
        const Colocation* colocation,
        const Tag* tag,
        const Tag::Size* tag_size,
        const char* app_format,
        const CommonAdRequestInfo& request_params,
        const AccountIdList* pubpixel_accounts, // use predefined pub pixel accounts
        const IdVector* exclude_pubpixel_accounts,
        const CreativeInstantiateRule& instantiate_info,
        const AdSlotContext& ad_slot_context,
        const String::SubString& ext_tag_id)
        /*throw(eh::Exception)*/;

      void
      fill_instantiate_passback_params_(
        TokenValueMap& request_args,
        const CampaignConfig* const campaign_config,
        const Tag* tag,
        const InstantiateParams& inst_params,
        const CommonAdRequestInfo& request_params,
        const CreativeInstantiateRule& instantiate_info,
        const AdSlotContext& ad_slot_context,
        const AccountIdList* consider_pub_pixel_accounts)
        /*throw(eh::Exception)*/;

      void
      fill_instantiate_params_(
        const CommonAdRequestInfo& request_params,
        const CampaignConfig* const campaign_config,
        const Colocation* const colocation,
        const CreativeTemplate& template_descr,
        const Template* creative_template,
        const InstantiateParams& inst_params,
        const CreativeInstantiateRule& instantiate_info,
        const char* app_format,
        AdSelectionResult& ad_selection_result,
        RequestResultParams& request_result_params,
        CreativeParamsList& creative_params_list,
        TemplateParams_var& request_template_params,
        TemplateParamsList& creative_template_params,
        const AdSlotContext& ad_slot_context,
        const IdVector* exclude_pubpixel_accounts)
        /*throw(eh::Exception)*/;

      static void
      fill_iurl_(
        std::string& iurl,
        const CampaignConfig* const campaign_config,
        const CreativeInstantiateRule& instantiate_info,
        const CommonAdRequestInfo& request_params,
        const Creative* creative,
        const Size* size)
        noexcept;

      void
      fill_track_urls_(
        const AdSelectionResult& ad_selection_result,
        RequestResultParams& request_result_params,
        const CommonAdRequestInfo& request_params,
        bool track_impressions,
        const InstantiateParams& inst_params,
        const CreativeInstantiateRule& instantiate_info,
        const AccountIdList* consider_pub_pixel_accounts)
        noexcept;

      void
      instantiate_click_url(
        const CampaignConfig* const campaign_config,
        const OptionValue& click_url,
        std::string& result_click_url,
        const unsigned long* colo_id,
        const Tag* tag,
        const Tag::Size* tag_size,
        const Creative* creative,
        const CampaignKeywordBase* ckw,
        const TokenValueMap& tokens)
        /*throw(CreativeOptionsProblem, eh::Exception)*/;

      void
      instantiate_creative_(
        const CommonAdRequestInfo& request_params,
        const CampaignConfig* const campaign_config,
        const Colocation* const colocation,
        const InstantiateParams& inst_params,
        const char* format,
        AdSelectionResult& ad_selection_result,
        RequestResultParams& request_result_params,
        CreativeParamsList& creative_instantiate_info,
        std::string& creative_body,
        const AdSlotContext& ad_slot_context,
        const IdVector* exclude_pubpixel_accounts)
        /*throw(CreativeTemplateProblem, CreativeOptionsProblem, eh::Exception)*/;

      void
      instantiate_creative_body_(
        const AdInstantiateType ad_instantiate_type,
        const CreativeRequestInfo& request_params,
        const CampaignConfig* config,
        const Colocation* const colocation,
        const char* cr_size,
        const TraceAdSlotInfo& ad_slot,
        AdSelectionResult& ad_selection_result,
        RequestResultParams& request_result_params,
        CreativeParamsList& creative_params_list,
        std::string& creative_body,
        std::string& creative_url,
        const AdSlotContext& ad_slot_context,
        const String::SubString& ext_tag_id)
        /*throw(CreativeTemplateProblem, CreativeOptionsProblem, eh::Exception)*/;

      void
      instantiate_url_creative_(
        std::string& creative_body,
        RequestResultParams& request_result_params,
        const AdSelectionResult& ad_selection_result,
        const String::SubString& instantiate_url,
        AdInstantiateType ad_instantiate_type)
        /*throw(CreativeTemplateProblem)*/;

      void
      fill_instantiate_url_(
        std::string& instantiate_url,
        AdInstantiateType ad_instantiate_type,
        CreativeParamsList& creative_params_list,
        const RequestResultParams& request_result_params,
        const InstantiateParams& inst_params,
        const CreativeInstantiateRule& instantiate_info,
        const CommonAdRequestInfo& request_params,
        const AdSelectionResult& ad_selection_result,
        const AdSlotContext& ad_slot_context,
        const char* app_format,
        const AccountIdList& pub_pixel_accounts,
        bool fill_auction_price,
        bool fill_creative_params)
        /*throw(CreativeTemplateProblem, CreativeOptionsProblem, eh::Exception)*/;

      void
      init_instantiate_url_(
        std::string& instantiate_url,
        AdInstantiateType ad_instantiate_type,
        CreativeParamsList& creative_params_list,
        RequestResultParams& request_result_params,
        const CampaignConfig* const campaign_config,
        const Tag* tag,
        const InstantiateParams& inst_params,
        const CreativeInstantiateRule& instantiate_info,
        const CommonAdRequestInfo& request_params,
        const AdSelectionResult& ad_selection_result,
        const AdSlotContext& ad_slot_context,
        const char* app_format,
        const IdVector& exclude_pubpixel_accounts,
        bool fill_auction_price = true)
        /*throw(CreativeTemplateProblem, CreativeOptionsProblem, eh::Exception)*/;

      void
      init_yandex_tokens_(
        const CampaignConfig* campaign_config,
        const CreativeInstantiateRule& instantiate_info,
        RequestResultParams& request_result_params,
        const CommonAdRequestInfo& request_params,
        const AdSlotContext& ad_slot_context,
        const Creative* creative)
        /*throw(CreativeInstantiateProblem)*/;

      void
      init_track_pixels_(
        const CampaignConfig* campaign_config,
        RequestResultParams& request_result_params,
        const CommonAdRequestInfo& request_params,
        const AdSlotContext& ad_slot_context,
        const Creative* creative,
        const CreativeInstantiateRule& instantiate_info,
        bool need_absolute_urls);

      void
      init_native_tokens_(
        const CampaignConfig* campaign_config,
        const CreativeInstantiateRule& instantiate_info,
        RequestResultParams& request_result_params,
        const CommonAdRequestInfo& request_params,
        const TraceAdSlotInfo& ad_slot,
        const AdSlotContext& ad_slot_context,
        const Creative* creative)
        /*throw(CreativeInstantiateProblem)*/;

      void
      fill_yandex_track_params_(
        std::string& yandex_track_params,
        const CommonAdRequestInfo& request_params,
        const AdSlotContext& ad_slot_context)
        noexcept;

      void
      init_vast_tokens_(
        RequestResultParams& request_result_params,
        const Creative* creative)
        /*throw(CreativeInstantiateProblem)*/;

      bool instantiate_creative_preview(
        const PreviewCreativeParams& params,
        const CampaignConfig* const campaign_config,
        const Campaign* campaign,
        const Creative* creative,
        const Tag* tag,
        const Tag::Size& tag_size,
        std::string& creative_body)
        /*throw(eh::Exception)*/;

      /*partly init click params*/
      std::string
      init_click_params0_(
        const AdServer::Commons::RequestId& request_id,
        const Colocation* colocation,
        const Creative* creative,
        const Tag* tag,
        const Tag::Size* tag_size,
        const CampaignKeyword* campaign_keyword,
        const RevenueDecimal& ctr,
        const InstantiateParams& inst_params,
        const CommonAdRequestInfo& request_params,
        const AdSlotContext& ad_slot_context)
        noexcept;

      void
      init_click_url_(
        ClickParams& click_params,
        const Colocation* colocation,
        const Tag* tag,
        const Tag::Size* tag_size,
        const InstantiateParams& inst_params,
        const CommonAdRequestInfo& request_params,
        const AdSlotContext& ad_slot_context,
        const CampaignSelectionData& select_params,
        const std::string& base_click_url);

      bool check_request_constraints(
        const String::SubString& referer,
        const String::SubString& original_url,
        std::string& referer_str,
        std::string& original_url_str)
        /*throw(eh::Exception)*/;

      void log_incoming_request(
        unsigned long tag_id,
        const String::SubString& referer,
        const IdVector& channels,
        const IdVector& full_freq_caps)
        /*throw(eh::Exception)*/;

      CampaignConfig_var
      configuration(bool required = false) const
        /*throw(NotReady, eh::Exception)*/;

      CampaignIndex_var configuration_index() const /*throw(eh::Exception)*/;

      void
      fill_creative_instantiate_args_(
        CreativeInstantiateArgs& creative_instantiate_args,
        const CreativeInstantiateRule& creative_instantiate_rule,
        const Creative* creative,
        const ClickParams& click_params,
        unsigned long random)
        noexcept;

      // task methods
      void check_config() noexcept;

      void update_ctr_provider() noexcept;

      void update_conv_rate_provider() noexcept;

      void update_bid_cost_provider() noexcept;

      void flush_logs() noexcept;

      static AdRequestType
      reduce_request_type_(unsigned long request_type)
        noexcept;

      void
      convert_ccg_keywords_(
        const CampaignConfig* campaign_config,
        const Tag* tag,
        CampaignKeywordMap& result_keywords,
        const CCGKeywordVector& keywords,
        bool profiling_available,
        const FreqCapIdSet& full_freq_caps)
        noexcept;

      void
      convert_external_categories_(
        CreativeCategoryIdSet& exclude_categories,
        const CampaignConfig& config,
        unsigned long request_type,
        const StringVector& categories)
        noexcept;

      static void
      fill_triggered_channels_(
        ChannelWeightMap& res, const std::vector<ChannelWeight>& triggered_channels)
        noexcept;

      void
      get_channel_targeting_info_(
        CampaignSelectionData& select_params,
        const ChannelIdHashSet& simple_channels,
        const Campaign* campaign_candidate,
        const CampaignKeyword* campaign_keyword,
        CreativeSelectDebugInfo* debug_info)
        /*throw(eh::Exception)*/;

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
        const CreativeRequestInfo& request_params,
        const TraceAdSlotInfo& ad_slot,
        AdSlotContext& ad_slot_context,
        RequestResultParams& request_result_params,
        AdSelectionResult& select_result,
        std::string& creative_body)
        /*throw(eh::Exception)*/;

      bool
      match_geo_channels_(
        const std::vector<GeoInfo>& location,
        const std::vector<GeoCoordInfo>& coord_location,
        ChannelIdList& geo_channels,
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
        AccountList& result_account_ids,
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
        const CommonAdRequestInfo& request_params,
        const IdVector& exclude_pubpixel_accounts)
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

      bool
      apply_check_available_bid_result_(
        const Campaign* campaign, // change mutable
        const BillingStateContainer::BidCheckResult& check_result,
        const RevenueDecimal& ctr)
        noexcept;

      void
      confirm_amounts_(
        const CampaignConfig* config,
        const Generics::Time& now,
        const ConfirmCreativeAmountArray& creatives,
        CCGRateType rate_type)
        noexcept;

      AdServer::Commons::Task<bool>
      co_confirm_amounts_(
        const CampaignConfig* config,
        const Generics::Time& now,
        const ConfirmCreativeAmountArray& creatives,
        CCGRateType rate_type);

      static void
      fill_tns_counter_device_type_(
        std::string& tns_counter_device_type,
        const StringSet& norm_platform_names)
        noexcept;

      // config manips
      static void
      fill_contract_(
        ::AdServer::CampaignSvcs::ContractInfo& contract_info,
        const Contract& contract)
        noexcept;

      std::string trace_campaign_selection_(
        unsigned long campaign_id,
        const TraceRequestInfo& request_params,
        const TraceAdSlotInfo& ad_slot,
        unsigned long auction_type,
        bool test_request)
        /*throw(Exception)*/;

    protected:
      CampaignManagerConfig campaign_manager_config_;
      Generics::ActiveObjectCallback_var callback_;
      Logging::Logger_var logger_;

      DomainParser_var domain_parser_;
      CampaignManagerLogger_var campaign_manager_logger_;

      CreativeInstantiate creative_instantiate_;
      TokenToParamMap token_to_parameters_;

      CampaignConfigSource_var campaign_config_source_;
      BillingStateContainer_var check_billing_state_container_;
      BillingStateContainer_var confirm_billing_state_container_;
      ReferenceCounting::PtrHolder<CTR::ConstCTRProvider_var> ctr_provider_;
      ReferenceCounting::PtrHolder<CTR::ConstCTRProvider_var> conv_rate_provider_;
      ReferenceCounting::PtrHolder<ConstBidCostProvider_var> bid_cost_provider_;

      mutable SyncPolicy::Mutex lock_;

      CampaignConfig_var configuration_;
      IndexingProgress indexing_progress_;
      CampaignIndex_var configuration_index_;
      PassbackTemplateMap passback_templates_;

      Generics::TaskRunner_var task_runner_;
      Generics::TaskRunner_var update_task_runner_;
      Generics::Planner_var scheduler_;
      Commons::SecTokenGenerator_var sec_token_generator_;
      Commons::IPCrypter_var ip_crypter_;
      Generics::SignedUuidGenerator rid_signer_;
      CountryList country_whitelist_;
      Commons::BoundedFileCache_var template_files_;
    };

    typedef ReferenceCounting::SmartPtr<CampaignManagerCore>
      CampaignManagerCore_var;
  inline
  CampaignConfig_var
  CampaignManagerCore::configuration(bool required) const
    /*throw(NotReady, eh::Exception)*/
  {
    CampaignConfig_var res;

    {
      SyncPolicy::ReadGuard guard(lock_);
      res = configuration_;
    }

    if(required && !res)
    {
      throw CampaignManagerCore::NotReady(
        "Campaign configuration isn't loaded");
    }

    return res;
  }

  inline
  CampaignIndex_var
  CampaignManagerCore::configuration_index() const /*throw(eh::Exception)*/
  {
    SyncPolicy::ReadGuard guard(lock_);
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

  /** CampaignManagerCore::CampaignManagerTaskMessage class */
  inline
  CampaignManagerCore::
  CampaignManagerTaskMessage::CampaignManagerTaskMessage(
    CampaignManagerCore* manager, Generics::TaskRunner* task_runner)
    /*throw(eh::Exception)*/
    : TaskBase(task_runner),
      manager_(manager)
  {}

  /** CampaignManagerCore::FlushLogsTaskMessage */
  inline
  CampaignManagerCore::FlushLogsTaskMessage::FlushLogsTaskMessage(
    CampaignManagerCore* manager, Generics::TaskRunner* task_runner)
    /*throw(eh::Exception)*/
    : CampaignManagerTaskMessage(manager, task_runner)
  {}

  inline
  void CampaignManagerCore::FlushLogsTaskMessage::execute() noexcept
  {
    manager_->flush_logs();
  }

  /** CampaignManagerCore::CheckConfigTaskMessage class */
  inline
  CampaignManagerCore::CheckConfigTaskMessage::CheckConfigTaskMessage(
    CampaignManagerCore* manager, Generics::TaskRunner* task_runner)
    /*throw(eh::Exception)*/
    : CampaignManagerTaskMessage(manager, task_runner)
  {}

  inline
  void
  CampaignManagerCore::CheckConfigTaskMessage::execute() noexcept
  {
    manager_->check_config();
  }

  // CampaignManagerCore::UpdateCTRProviderTask
  inline
  CampaignManagerCore::UpdateCTRProviderTask::UpdateCTRProviderTask(
    CampaignManagerCore* manager,
    Generics::TaskRunner* task_runner)
    /*throw(eh::Exception)*/
    : CampaignManagerTaskMessage(manager, task_runner)
  {}

  inline
  void
  CampaignManagerCore::UpdateCTRProviderTask::execute() noexcept
  {
    manager_->update_ctr_provider();
  }

  // CampaignManagerCore::UpdateConvRateProviderTask
  inline
  CampaignManagerCore::UpdateConvRateProviderTask::UpdateConvRateProviderTask(
    CampaignManagerCore* manager,
    Generics::TaskRunner* task_runner)
    /*throw(eh::Exception)*/
    : CampaignManagerTaskMessage(manager, task_runner)
  {}

  inline
  void
  CampaignManagerCore::UpdateConvRateProviderTask::execute() noexcept
  {
    manager_->update_conv_rate_provider();
  }

  // CampaignManagerCore::UpdateBidCostProviderTask
  inline
  CampaignManagerCore::UpdateBidCostProviderTask::UpdateBidCostProviderTask(
    CampaignManagerCore* manager,
    Generics::TaskRunner* task_runner)
    /*throw(eh::Exception)*/
    : CampaignManagerTaskMessage(manager, task_runner)
  {}

  inline
  void
  CampaignManagerCore::UpdateBidCostProviderTask::execute() noexcept
  {
    manager_->update_bid_cost_provider();
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
}
