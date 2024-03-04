
#ifndef _AD_SERVER_CAMPAIGN_SVCS_CAMPAIGN_MANAGER_CAMPAIGN_MANAGER_IMPL_HPP_
#define _AD_SERVER_CAMPAIGN_SVCS_CAMPAIGN_MANAGER_CAMPAIGN_MANAGER_IMPL_HPP_

#include <string>
#include <list>
#include <map>

#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/PtrHolder.hpp>
#include <Logger/Logger.hpp>
#include <Generics/ActiveObject.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Sync/SyncPolicy.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <String/StringManip.hpp>
#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/ServantImpl.hpp>

#include <Commons/CorbaAlgs.hpp>
#include <Commons/IPCrypter.hpp>
#include <Commons/SecToken.hpp>
#include <Commons/TextTemplateCache.hpp>
#include <LogCommons/AdRequestLogger.hpp>
#include <Commons/Kafka/KafkaProducer.hpp>

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
    
    class CampaignManagerImpl :
      public virtual Generics::CompositeActiveObject,
      public virtual CORBACommons::ReferenceCounting::ServantImpl<
        POA_AdServer::CampaignSvcs::CampaignManager>
    {
    public:
      static const unsigned long PARALLEL_TASKS_COUNT = 5;

      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
      DECLARE_EXCEPTION(InvalidArgument, Exception);
      DECLARE_EXCEPTION(CreativeInstantiateProblem, Exception);
      DECLARE_EXCEPTION(CreativeTemplateProblem, CreativeInstantiateProblem);
      DECLARE_EXCEPTION(CreativeOptionsProblem, CreativeInstantiateProblem);

    public:
      struct CreativeInstantiate;
      struct CreativeParams;

    public:
      typedef xsd::AdServer::Configuration::CampaignManagerType
        CampaignManagerConfig;

      /** Parametric constructor.
       *
       * @param callback Instance of a callback class.
       * @param logger Instance of a logger class.
       *
       */
      CampaignManagerImpl(
        const CampaignManagerConfig& configuration,
        const DomainParser::DomainConfig& domain_config,
        Generics::ActiveObjectCallback* callback,
        Logging::Logger* logger,
        CampaignManagerLogger* campaign_manager_logger,
        const CreativeInstantiate& creative_instantiate,
        const char* campaigns_types)
        /*throw(InvalidArgument, Exception, eh::Exception)*/;

      virtual bool ready() /*throw(eh::Exception)*/;

      //
      // IDL:AdServer/CampaignSvcs/CampaignManager/get_campaign_creative:1.0
      //
      virtual void get_campaign_creative(
        const AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
        CORBA::String_out hostname,
        AdServer::CampaignSvcs::CampaignManager::RequestCreativeResult_out request_result)
        /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
          AdServer::CampaignSvcs::CampaignManager::NotReady)*/;

      //
      // IDL:AdServer/CampaignSvcs/CampaignManager/match_geo_channels:1.0
      //
      virtual
      void
      match_geo_channels(
        const AdServer::CampaignSvcs::CampaignManager::GeoInfoSeq& location,
        const AdServer::CampaignSvcs::CampaignManager::GeoCoordInfoSeq& coord_location,
        AdServer::CampaignSvcs::ChannelIdSeq_out geo_channels_result,
        AdServer::CampaignSvcs::ChannelIdSeq_out coord_channels_result)
        /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
          AdServer::CampaignSvcs::CampaignManager::NotReady)*/;

      //
      // IDL:AdServer/CampaignSvcs/CampaignManager/process_match_request:1.0
      //
      virtual
      void
      process_match_request(
        const AdServer::CampaignSvcs::CampaignManager::MatchRequestInfo&
          match_request_info)
        /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
          AdServer::CampaignSvcs::CampaignManager::NotReady)*/;

      virtual
      void
      process_anonymous_request(
        const AdServer::CampaignSvcs::CampaignManager::AnonymousRequestInfo&
          anon_request_info)
        /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
          AdServer::CampaignSvcs::CampaignManager::NotReady)*/;

      virtual
      void
      get_file(
        const char* file_name,
        CORBACommons::OctSeq_out file)
        /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException)*/;

      virtual void
      instantiate_ad(
        const AdServer::CampaignSvcs::CampaignManager::
          InstantiateAdInfo& instantiate_ad_info,
        AdServer::CampaignSvcs::CampaignManager::
          InstantiateAdResult_out instantiate_ad_result)
        /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
          AdServer::CampaignSvcs::CampaignManager::NotReady)*/;

      virtual void
      trace_campaign_selection(
        CORBA::ULong campaign_id,
        const AdServer::CampaignSvcs::CampaignManager::RequestParams&
          request_params,
        const AdServer::CampaignSvcs::CampaignManager::AdSlotInfo& ad_slot,
        CORBA::ULong auction_type,
        CORBA::Boolean test_request,
        CORBA::String_out trace_xml)
        /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException)*/;

      virtual void trace_campaign_selection_index(
        CORBA::String_out trace_xml)
        /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException)*/;

      //
      // IDL:AdServer/CampaignSvcs/CampaignManager/get_campaign_creative_by_ccid:1.0
      //
      virtual CORBA::Boolean get_campaign_creative_by_ccid(
        const ::AdServer::CampaignSvcs::CampaignManager::CreativeParams& params,
        CORBA::String_out creative_body)
        /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException)*/;

      virtual void consider_passback(
        const AdServer::CampaignSvcs::CampaignManager::PassbackInfo& in)
        /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException)*/;

      virtual void consider_passback_track(
        const AdServer::CampaignSvcs::CampaignManager::PassbackTrackInfo& in)
        /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
          AdServer::CampaignSvcs::CampaignManager::NotReady)*/;

      //
      // IDL:AdServer/CampaignSvcs/CampaignManager/get_click_url:1.0
      //
      virtual CORBA::Boolean
      get_click_url(
        const ::AdServer::CampaignSvcs::CampaignManager::ClickInfo& click_info,
        ::AdServer::CampaignSvcs::CampaignManager::ClickResultInfo_out click_result_info)
        /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
          AdServer::CampaignSvcs::CampaignManager::NotReady)*/;

      //
      // IDL:AdServer/CampaignSvcs/CampaignManager/verify_impression:1.0
      //
      virtual void verify_impression(
        const AdServer::CampaignSvcs::CampaignManager::ImpressionInfo& impression_info,
        ::AdServer::CampaignSvcs::CampaignManager::ImpressionResultInfo_out impression_result_info)
        /*throw(
          AdServer::CampaignSvcs::CampaignManager::ImplementationException,
          AdServer::CampaignSvcs::CampaignManager::NotReady)*/;

      //
      // IDL:AdServer/CampaignSvcs/CampaignManager/action_taken:1.0
      //
      virtual void action_taken(
        const AdServer::CampaignSvcs::CampaignManager::ActionInfo& action_info)
        /*throw(
          AdServer::CampaignSvcs::CampaignManager::ImplementationException,
          AdServer::CampaignSvcs::CampaignManager::NotReady)*/;

      //
      // IDL:AdServer/CampaignSvcs/CampaignManager/get_channel_links:1.0
      //
      virtual AdServer::CampaignSvcs::CampaignManager::ChannelSearchResultSeq*
      get_channel_links(
        const AdServer::CampaignSvcs::ChannelIdSeq& channels,
        bool match)
        /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException)*/;

      //
      // IDL:AdServer/CampaignSvcs/CampaignManager/get_discover_channels:1.0
      //
      virtual AdServer::CampaignSvcs::CampaignManager::DiscoverChannelResultSeq*
      get_discover_channels(
        const AdServer::CampaignSvcs::ChannelWeightSeq& channels,
        const char* country,
        const char* language,
        bool all)
        /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
          AdServer::CampaignSvcs::CampaignManager::NotReady)*/;

      virtual AdServer::CampaignSvcs::CampaignManager::CategoryChannelNodeSeq*
      get_category_channels(const char* language)
        /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
          AdServer::CampaignSvcs::CampaignManager::NotReady)*/;

      //
      // IDL:AdServer/CampaignSvcs/CampaignManager/get_config:1.0
      //
      virtual ::AdServer::CampaignSvcs::CampaignManager::CampaignConfig*
      get_config(const AdServer::CampaignSvcs::
        CampaignManager::GetConfigInfo& get_config_props)
        /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException)*/;

      //
      // IDL:AdServer/CampaignSvcs/CampaignManager/verify_opt_operation:1.0
      //
      virtual void
      verify_opt_operation(
        ::CORBA::ULong time,
        ::CORBA::Long colo_id,
        const char* referer,
        AdServer::CampaignSvcs::CampaignManager::OptOperation operation,
        ::CORBA::ULong status,
        ::CORBA::ULong user_status,
        bool log_as_test,
        const char* browser,
        const char* os,
        const char* ct,
        const char* curct,
        const CORBACommons::UserIdInfo& user_id)
        /*throw(AdServer::CampaignSvcs::CampaignManager::NotReady)*/;

      
      virtual void
      consider_web_operation(
        const AdServer::CampaignSvcs::CampaignManager::WebOperationInfo& web_op_info)
        /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
          AdServer::CampaignSvcs::CampaignManager::IncorrectArgument,
          AdServer::CampaignSvcs::CampaignManager::NotReady)*/;

      virtual ColocationFlagsSeq*
      get_colocation_flags()
        /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
          AdServer::CampaignSvcs::CampaignManager::NotReady)*/;

      virtual AdServer::CampaignSvcs::StringSeq*
      get_pub_pixels(
        const char* country,
        CORBA::ULong user_status,
        const AdServer::CampaignSvcs::PublisherAccountIdSeq& publisher_account_ids)
        /*throw(AdServer::CampaignSvcs::CampaignManager::NotReady,
          AdServer::CampaignSvcs::CampaignManager::ImplementationException)*/;

      void
      progress_comment(std::string& res) /*throw(eh::Exception)*/;

    private:
      virtual
      ~CampaignManagerImpl() noexcept {}

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
          CampaignManagerImpl* manager,
          Generics::TaskRunner* task_runner)
          /*throw(eh::Exception)*/;

      protected:
        virtual
        ~CampaignManagerTaskMessage() noexcept = default;

        CampaignManagerImpl* manager_;
      };

      typedef ReferenceCounting::SmartPtr<CampaignManagerTaskMessage>
        CampaignManagerTaskMessage_var;

      /* Task that periodically flushes the request logger */
      class FlushLogsTaskMessage: public CampaignManagerTaskMessage
      {
      public:
        FlushLogsTaskMessage(CampaignManagerImpl* manager,
          Generics::TaskRunner* task_runner)
          /*throw(eh::Exception)*/;

        virtual void execute() noexcept;
      };

      class CheckConfigTaskMessage : public CampaignManagerTaskMessage
      {
      public:
        CheckConfigTaskMessage(CampaignManagerImpl* manager,
          Generics::TaskRunner* task_runner)
          /*throw(eh::Exception)*/;

        virtual void execute() noexcept;
      };

      class UpdateCTRProviderTask: public CampaignManagerTaskMessage
      {
      public:
        UpdateCTRProviderTask(CampaignManagerImpl* manager,
          Generics::TaskRunner* task_runner)
          /*throw(eh::Exception)*/;

        virtual void execute() noexcept;
      };

      class UpdateConvRateProviderTask: public CampaignManagerTaskMessage
      {
      public:
        UpdateConvRateProviderTask(CampaignManagerImpl* manager,
          Generics::TaskRunner* task_runner)
          /*throw(eh::Exception)*/;

        virtual void execute() noexcept;
      };

      class UpdateBidCostProviderTask: public CampaignManagerTaskMessage
      {
      public:
        UpdateBidCostProviderTask(CampaignManagerImpl* manager,
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
          const CORBACommons::UserIdInfo& user_id_val,
          bool enabled_notice_val)
          : generate_pubpixel_accounts(false),
            enabled_notice(enabled_notice_val),
            video_width(0),
            video_height(0),
            publisher_site_id(0),
            publisher_account_id(0),
            init_source_macroses(true)
        {
          AdServer::Commons::UserId user_id =
            CorbaAlgs::unpack_user_id(user_id_val);
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
      get_campaign_creative_(
        const AdServer::CampaignSvcs::CampaignManager::RequestParams&
          request_params,
        const AdServer::CampaignSvcs::CampaignManager::AdSlotInfo& ad_slot,
        AdSlotContext& ad_slot_context,
        const AdSlotMinCpm& ad_slot_min_cpm,
        const Generics::Time& session_start,
        AdServer::CampaignSvcs::CampaignManager::AdSlotResult& ad_slot_result,
        const Tag* tag,
        const Colocation* colocation,
        bool passback,
        AdServer::CampaignSvcs::CampaignManager::AdRequestDebugInfo* ad_request_debug_info,
        AdServer::CampaignSvcs::CampaignManager::AdSlotDebugInfo* ad_slot_debug_info,
        const ChannelIdHashSet& matched_channels)
        /*throw(eh::Exception)*/;

      const Tag*
      resolve_tag(
        std::string* tag_size,
        unsigned long* selected_publisher_account_id,
        const CampaignSvcs::CampaignManager::RequestParams& request_params,
        const CampaignConfig& campaign_config,
        const CampaignSvcs::CampaignManager::AdSlotInfo& ad_slot)
        const noexcept;

      void
      verify_vast_operation_(
        const AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
        const AdServer::CampaignSvcs::CampaignManager::AdSlotInfo& ad_slot,
        const Tag* tag,
        const Colocation* colocation)
        noexcept;

      void
      get_adslot_campaign_creative_(
        const CampaignConfig* campaign_config,
        AdServer::CampaignSvcs::CampaignManager::RequestCreativeResult& ad_request_result,
        AdServer::CampaignSvcs::CampaignManager::AdSlotResult& ad_slot_result,
        AdServer::CampaignSvcs::RevenueDecimal& adsspace_system_cpm,
        const AdServer::CampaignSvcs::CampaignManager::RequestParams&
          request_params,
        const Generics::Time& session_start,
        const Colocation* colocation,
        const AdServer::CampaignSvcs::CampaignManager::AdSlotInfo& ad_slot,
        const CreativeInstantiateRule& creative_instantiate_rule,
        AdServer::CampaignSvcs::CampaignManager::AdRequestDebugInfo* debug_info,
        AdServer::CampaignSvcs::CampaignManager::AdSlotDebugInfo* adslot_debug_info,
        AdSlotContext& ad_slot_context,
        const ChannelIdHashSet& matched_channels,
        unsigned long& request_tag_id)
        noexcept;

      CORBA::Boolean
      get_campaign_creative_by_ccid_impl(
        const ::AdServer::CampaignSvcs::CampaignManager::CreativeParams&
          params,
        CORBA::String_out creative_body)
        /*throw(eh::Exception)*/;

      void
      get_site_creative_(
        CampaignIndex* config_index,
        const Colocation* colocation,
        const Tag* requested_tag,
        const Tag::SizeMap& tag_sizes,
        const AdServer::CampaignSvcs::CampaignManager::RequestParams&
          request_params,
        const AdServer::CampaignSvcs::CampaignManager::AdSlotInfo& ad_slot,
        AdSlotContext& ad_slot_context,
        const AdSlotMinCpm& ad_slot_min_cpm,
        const FreqCapIdSet& full_freq_caps,
        const SeqOrderMap& seq_orders,
        RequestResultParams& request_result_params,
        AdSelectionResult& select_result,
        CORBA::String_out creative_body,
        CORBA::String_out creative_url,
        AdServer::CampaignSvcs::CampaignManager::AdRequestDebugInfo*
          ad_request_debug_info,
        AdServer::CampaignSvcs::CampaignManager::AdSlotDebugInfo*
          ad_slot_debug_info)
        /*throw(eh::Exception)*/;

      void
      get_bid_costs_(
        RevenueDecimal& low_predicted_pub_ecpm_system,
        RevenueDecimal& top_predicted_pub_ecpm_system,
        const AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
        const Tag* tag,
        const RevenueDecimal& min_pub_ecpm_system,
        const CampaignSelectionDataList& selected_campaigns);

      bool
      instantiate_text_creatives(
        const CampaignConfig* config,
        const Colocation* const colocation,
        const AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
        const AdServer::CampaignSvcs::CampaignManager::AdSlotInfo& ad_slot,
        const CampaignSelector::WeightedCampaignKeywordList& campaign_keywords,
        AdSelectionResult& ad_selection_result,
        RequestResultParams& request_result_params,
        CreativeParamsList& creative_params_list,
        AdServer::CampaignSvcs::CampaignManager::AdSlotDebugInfo* ad_slot_debug_info,
        CORBA::String_out creative_body,
        CORBA::String_out creative_url,
        AdSlotContext& ad_slot_context)
        /*throw(eh::Exception)*/;

      bool
      instantiate_display_creative(
        const CampaignConfig* config,
        const Colocation* colocation,
        const AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
        const AdServer::CampaignSvcs::CampaignManager::AdSlotInfo& ad_slot,
        const CampaignSelector::WeightedCampaign& weighted_campaign,
        AdSelectionResult& ad_selection_result,
        RequestResultParams& request_result_params,
        CreativeParams& creative_params,
        AdServer::CampaignSvcs::CampaignManager::AdSlotDebugInfo* debug_info,
        CORBA::String_out creative_body,
        CORBA::String_out creative_url,
        AdSlotContext& ad_slot_context)
        /*throw(eh::Exception)*/;

      bool
      instantiate_passback(
        CORBA::String_out mime_format,
        CORBA::String_out passback_body,
        const CampaignConfig* const campaign_config,
        const Colocation* colocation,
        const Tag* tag,
        const char* app_format,
        const AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
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
        const AdServer::CampaignSvcs::CampaignManager::
          CommonAdRequestInfo& request_params,
        const AccountIdList* pubpixel_accounts, // use predefined pub pixel accounts
        const AdServer::CampaignSvcs::
          PublisherAccountIdSeq* exclude_pubpixel_accounts,
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
        const AdServer::CampaignSvcs::CampaignManager::CommonAdRequestInfo& request_params,
        const CreativeInstantiateRule& instantiate_info,
        const AdSlotContext& ad_slot_context,
        const AccountIdList* consider_pub_pixel_accounts)
        /*throw(eh::Exception)*/;

      void
      fill_instantiate_params_(
        const AdServer::CampaignSvcs::CampaignManager::CommonAdRequestInfo& request_params,
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
        const AdServer::CampaignSvcs::
          PublisherAccountIdSeq* exclude_pubpixel_accounts)
        /*throw(eh::Exception)*/;

      static void
      fill_iurl_(
        std::string& iurl,
        const CampaignConfig* const campaign_config,
        const CreativeInstantiateRule& instantiate_info,
        const AdServer::CampaignSvcs::CampaignManager::CommonAdRequestInfo& request_params,
        const Creative* creative,
        const Size* size)
        noexcept;

      void
      fill_track_urls_(
        const AdSelectionResult& ad_selection_result,
        RequestResultParams& request_result_params,
        const AdServer::CampaignSvcs::CampaignManager::CommonAdRequestInfo& request_params,
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
        const AdServer::CampaignSvcs::CampaignManager::CommonAdRequestInfo& request_params,
        const CampaignConfig* const campaign_config,
        const Colocation* const colocation,
        const InstantiateParams& inst_params,
        const char* format,
        AdSelectionResult& ad_selection_result,
        RequestResultParams& request_result_params,
        CreativeParamsList& creative_instantiate_info,
        CORBA::String_out creative_body,
        const AdSlotContext& ad_slot_context,
        const AdServer::CampaignSvcs::
          PublisherAccountIdSeq* exclude_pubpixel_accounts)
        /*throw(CreativeTemplateProblem, CreativeOptionsProblem, eh::Exception)*/;

      void
      instantiate_creative_body_(
        const AdInstantiateType ad_instantiate_type,
        const AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
        const CampaignConfig* config,
        const Colocation* const colocation,
        const char* cr_size,
        const AdServer::CampaignSvcs::CampaignManager::AdSlotInfo& ad_slot,
        AdSelectionResult& ad_selection_result,
        RequestResultParams& request_result_params,
        CreativeParamsList& creative_params_list,
        CORBA::String_out& creative_body,
        CORBA::String_out& creative_url,
        const AdSlotContext& ad_slot_context,
        const String::SubString& ext_tag_id)
        /*throw(CreativeTemplateProblem, CreativeOptionsProblem, eh::Exception)*/;

      void
      instantiate_url_creative_(
        CORBA::String_out creative_body,
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
        const AdServer::CampaignSvcs::CampaignManager::CommonAdRequestInfo& request_params,
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
        const AdServer::CampaignSvcs::CampaignManager::CommonAdRequestInfo& request_params,
        const AdSelectionResult& ad_selection_result,
        const AdSlotContext& ad_slot_context,
        const char* app_format,
        const AdServer::CampaignSvcs::
          PublisherAccountIdSeq& exclude_pubpixel_accounts,
        bool fill_auction_price = true)
        /*throw(CreativeTemplateProblem, CreativeOptionsProblem, eh::Exception)*/;

      void
      init_yandex_tokens_(
        const CampaignConfig* campaign_config,
        const CreativeInstantiateRule& instantiate_info,
        RequestResultParams& request_result_params,
        const AdServer::CampaignSvcs::CampaignManager::
          CommonAdRequestInfo& request_params,
        const AdSlotContext& ad_slot_context,
        const Creative* creative)
        /*throw(CreativeInstantiateProblem)*/;

      void
      init_track_pixels_(
        const CampaignConfig* campaign_config,
        RequestResultParams& request_result_params,
        const AdServer::CampaignSvcs::CampaignManager::
          CommonAdRequestInfo& request_params,
        const AdSlotContext& ad_slot_context,
        const Creative* creative,
        const CreativeInstantiateRule& instantiate_info,
        bool need_absolute_urls);

      void
      init_native_tokens_(
        const CampaignConfig* campaign_config,
        const CreativeInstantiateRule& instantiate_info,
        RequestResultParams& request_result_params,
        const AdServer::CampaignSvcs::CampaignManager::
          CommonAdRequestInfo& request_params,
        const AdServer::CampaignSvcs::CampaignManager::
          AdSlotInfo& ad_slot,
        const AdSlotContext& ad_slot_context,
        const Creative* creative)
        /*throw(CreativeInstantiateProblem)*/;

      void
      fill_yandex_track_params_(
        std::string& yandex_track_params,
        const AdServer::CampaignSvcs::CampaignManager::
          CommonAdRequestInfo& request_params,
        const AdSlotContext& ad_slot_context)
        noexcept;

      void
      init_vast_tokens_(
        RequestResultParams& request_result_params,
        const Creative* creative)
        /*throw(CreativeInstantiateProblem)*/;

      void
      get_token_value_(
        std::string& token_value,
        const Colocation* colocation,
        const Tag* tag,
        const CreativeTemplate& template_descr,
        const std::string& token_name)
        noexcept;

      bool instantiate_creative_preview(
        const AdServer::CampaignSvcs::CampaignManager::CreativeParams& params,
        const CampaignConfig* const campaign_config,
        const Campaign* campaign,
        const Creative* creative,
        const Tag* tag,
        const Tag::Size& tag_size,
        CORBA::String_out creative_body)
        /*throw(CORBA::SystemException, eh::Exception)*/;

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
        const AdServer::CampaignSvcs::CampaignManager::CommonAdRequestInfo& request_params,
        const AdSlotContext& ad_slot_context)
        noexcept;

      void
      init_click_url_(
        ClickParams& click_params,
        const Colocation* colocation,
        const Tag* tag,
        const Tag::Size* tag_size,
        const InstantiateParams& inst_params,
        const AdServer::CampaignSvcs::CampaignManager::CommonAdRequestInfo& request_params,
        const AdSlotContext& ad_slot_context,
        const CampaignSelectionData& select_params,
        const std::string& base_click_url);

      bool check_request_constraints(
        const AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
        std::string& referer_str,
        std::string& original_url_str)
        /*throw(eh::Exception)*/;

      void log_incoming_request(
        const AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
        const AdServer::CampaignSvcs::CampaignManager::AdSlotInfo& ad_slot)
        /*throw(eh::Exception)*/;

      CampaignConfig_var
      configuration(bool required = false) const
        /*throw(AdServer::CampaignSvcs::CampaignManager::NotReady,
          eh::Exception)*/;

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
      reduce_request_type_(CORBA::ULong request_type)
        noexcept;

      void
      convert_ccg_keywords_(
        const CampaignConfig* campaign_config,
        const Tag* tag,
        CampaignKeywordMap& result_keywords,
        const AdServer::CampaignSvcs::CampaignManager::CCGKeywordSeq& keywords,
        bool profiling_available,
        const FreqCapIdSet& full_freq_caps)
        noexcept;

      void
      convert_external_categories_(
        CreativeCategoryIdSet& exclude_categories,
        const CampaignConfig& config,
        const AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
        const AdServer::CampaignSvcs::CampaignManager::ExternalCreativeCategoryIdSeq& category_seq)
        noexcept;

      static void
      fill_triggered_channels_(
        ChannelWeightMap& res, const ChannelWeightSeq& triggered_channels)
        noexcept;

      void
      get_channel_targeting_info_(
        CampaignSelectionData& select_params,
        const AdServer::CampaignSvcs::CampaignManager::RequestParams&
          request_params,
        const Campaign* campaign_candidate,
        const CampaignKeyword* campaign_keyword,
        AdServer::CampaignSvcs::CampaignManager::CreativeSelectDebugInfo*
          debug_info)
        /*throw(eh::Exception)*/;

      bool
      fill_category_channel_node_(
        AdServer::CampaignSvcs::CampaignManager::CategoryChannelNodeInfo& res,
        const CategoryChannelNode* node,
        const char* language)
        noexcept;

      bool
      fill_ad_slot_min_cpm_(
        AdSlotMinCpm& ad_slot_min_cpm,
        const CampaignConfig* campaign_config,
        const Tag* tag,
        const String::SubString& min_ecpm_currency_code,
        const CORBACommons::DecimalInfo& min_ecpm)
        noexcept;

      bool
      preview_ccid_(
        const CampaignConfig* config,
        const Colocation* colocation,
        const Tag* tag,
        const AdServer::CampaignSvcs::CampaignManager::RequestParams&
          request_params,
        const AdServer::CampaignSvcs::CampaignManager::AdSlotInfo& ad_slot,
        AdSlotContext& ad_slot_context,
        RequestResultParams& request_result_params,
        AdSelectionResult& select_result,
        CORBA::String_out creative_body)
        /*throw(eh::Exception)*/;

      bool
      match_geo_channels_(
        const AdServer::CampaignSvcs::CampaignManager::GeoInfoSeq& location,
        const AdServer::CampaignSvcs::CampaignManager::GeoCoordInfoSeq& coord_location,
        ChannelIdList& geo_channels,
        ChannelIdSet& coord_channels)
        /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
          AdServer::CampaignSvcs::CampaignManager::NotReady)*/;

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
        const AdServer::CampaignSvcs::CampaignManager::
          CommonAdRequestInfo& request_params,
        const AdServer::CampaignSvcs::
          PublisherAccountIdSeq& exclude_pubpixel_accounts)
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

      static void
      fill_tns_counter_device_type_(
        std::string& tns_counter_device_type,
        const StringSet& norm_platform_names)
        noexcept;

      void
      produce_ads_space_(
        const AdServer::CampaignSvcs::CampaignManager::
          RequestParams& request_params,
        unsigned long request_tag_id,
        const RevenueDecimal& adsspace_system_cpm);

      void
      produce_match_(
        const AdServer::CampaignSvcs::CampaignManager::MatchRequestInfo&
          request_params);

      void
      produce_action_message_(
        const AdServer::CampaignSvcs::CampaignManager::ActionInfo& action_info);

      void
      produce_ads_space_message_impl_(
        const Generics::Time& request_time,
        const AdServer::Commons::UserId& user_id,
        unsigned long request_tag_id,
        const String::SubString& referer,
        const AdServer::CampaignSvcs::CampaignManager::AdSlotSeq* ad_slots,
        const AdServer::CampaignSvcs::ULongSeq* publisher_account_ids,
        const String::SubString& peer_ip,
        const AdServer::CampaignSvcs::CampaignManager::GeoInfoSeq& location,
        const String::SubString& ssp_location,
        const RevenueDecimal& adsspace_system_cpm,
        const String::SubString& external_user_id);

      // config manips
      static void
      fill_campaign_contracts_(
        AdServer::CampaignSvcs::CampaignManager::ExtContractInfoSeq& contract_seq,
        const Contract* contract)
        noexcept;

      static void
      fill_contract_(
        ContractInfo& contract_info,
        const Contract& contract)
        noexcept;

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
      ReferenceCounting::PtrHolder<ConstCTRProvider_var> ctr_provider_;
      ReferenceCounting::PtrHolder<ConstCTRProvider_var> conv_rate_provider_;
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
      Commons::Kafka::Producer_var kafka_producer_;
      Commons::Kafka::Producer_var kafka_match_producer_;
    };

    typedef ReferenceCounting::SmartPtr<CampaignManagerImpl>
      CampaignManagerImpl_var;
  }
}

namespace AdServer
{
namespace CampaignSvcs
{
  inline
  CampaignConfig_var
  CampaignManagerImpl::configuration(bool required) const
    /*throw(AdServer::CampaignSvcs::CampaignManager::NotReady,
      eh::Exception)*/
  {
    CampaignConfig_var res;

    {
      SyncPolicy::ReadGuard guard(lock_);
      res = configuration_;
    }

    if(required && !res)
    {
      throw AdServer::CampaignSvcs::CampaignManager::NotReady(
        "Campaign configuration isn't loaded");
    }

    return res;
  }

  inline
  CampaignIndex_var
  CampaignManagerImpl::configuration_index() const /*throw(eh::Exception)*/
  {
    SyncPolicy::ReadGuard guard(lock_);
    return configuration_index_;
  }

  inline
  void
  CampaignManagerImpl::progress_comment(std::string& res)
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
  CampaignManagerImpl::fill_triggered_channels_(
    ChannelWeightMap& res, const ChannelWeightSeq& triggered_channels)
    noexcept
  {
    for(CORBA::ULong i = 0; i < triggered_channels.length(); ++i)
    {
      res.insert(std::make_pair(
        triggered_channels[i].channel_id, triggered_channels[i].weight));
    }
  }

  /** CampaignManagerImpl::CampaignManagerTaskMessage class */
  inline
  CampaignManagerImpl::
  CampaignManagerTaskMessage::CampaignManagerTaskMessage(
    CampaignManagerImpl* manager, Generics::TaskRunner* task_runner)
    /*throw(eh::Exception)*/
    : TaskBase(task_runner),
      manager_(manager)
  {}

  /** CampaignManagerImpl::FlushLogsTaskMessage */
  inline
  CampaignManagerImpl::FlushLogsTaskMessage::FlushLogsTaskMessage(
    CampaignManagerImpl* manager, Generics::TaskRunner* task_runner)
    /*throw(eh::Exception)*/
    : CampaignManagerTaskMessage(manager, task_runner)
  {}

  inline
  void CampaignManagerImpl::FlushLogsTaskMessage::execute() noexcept
  {
    manager_->flush_logs();
  }

  /** CampaignManagerImpl::CheckConfigTaskMessage class */
  inline
  CampaignManagerImpl::CheckConfigTaskMessage::CheckConfigTaskMessage(
    CampaignManagerImpl* manager, Generics::TaskRunner* task_runner)
    /*throw(eh::Exception)*/
    : CampaignManagerTaskMessage(manager, task_runner)
  {}

  inline
  void
  CampaignManagerImpl::CheckConfigTaskMessage::execute() noexcept
  {
    manager_->check_config();
  }

  // CampaignManagerImpl::UpdateCTRProviderTask
  inline
  CampaignManagerImpl::UpdateCTRProviderTask::UpdateCTRProviderTask(
    CampaignManagerImpl* manager,
    Generics::TaskRunner* task_runner)
    /*throw(eh::Exception)*/
    : CampaignManagerTaskMessage(manager, task_runner)
  {}

  inline
  void
  CampaignManagerImpl::UpdateCTRProviderTask::execute() noexcept
  {
    manager_->update_ctr_provider();
  }

  // CampaignManagerImpl::UpdateConvRateProviderTask
  inline
  CampaignManagerImpl::UpdateConvRateProviderTask::UpdateConvRateProviderTask(
    CampaignManagerImpl* manager,
    Generics::TaskRunner* task_runner)
    /*throw(eh::Exception)*/
    : CampaignManagerTaskMessage(manager, task_runner)
  {}

  inline
  void
  CampaignManagerImpl::UpdateConvRateProviderTask::execute() noexcept
  {
    manager_->update_conv_rate_provider();
  }

  // CampaignManagerImpl::UpdateBidCostProviderTask
  inline
  CampaignManagerImpl::UpdateBidCostProviderTask::UpdateBidCostProviderTask(
    CampaignManagerImpl* manager,
    Generics::TaskRunner* task_runner)
    /*throw(eh::Exception)*/
    : CampaignManagerTaskMessage(manager, task_runner)
  {}

  inline
  void
  CampaignManagerImpl::UpdateBidCostProviderTask::execute() noexcept
  {
    manager_->update_bid_cost_provider();
  }

  // CampaignManagerImpl::ConfirmCreativeAmount
  inline
  CampaignManagerImpl::ConfirmCreativeAmount::ConfirmCreativeAmount(
    unsigned long cc_id_val,
    const RevenueDecimal& ctr_val)
    noexcept
    : cc_id(cc_id_val),
      ctr(ctr_val)
  {}
}
}

#endif // _AD_SERVER_CAMPAIGN_SVCS_CAMPAIGN_MANAGER_CAMPAIGN_MANAGER_IMPL_HPP_
