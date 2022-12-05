#ifndef CAMPAIGNSVCS_CAMPAIGNSERVER_CAMPAIGNSERVERIMPL_V25_HPP
#define CAMPAIGNSVCS_CAMPAIGNSERVER_CAMPAIGNSERVERIMPL_V25_HPP

#include <ReferenceCounting/ReferenceCounting.hpp>

#include <eh/Exception.hpp>

#include <Generics/ActiveObject.hpp>
#include <Logger/Logger.hpp>

#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/ServantImpl.hpp>

/* AdServer::CampaignSvcs_v25 */
#include <CampaignSvcs/CampaignServer/Compatibility/CampaignCommons_v25.hpp>
#include <CampaignSvcs/CampaignServer/Compatibility/CampaignServer_v25.hpp>

#include <CampaignSvcs/CampaignServer/Compatibility/CampaignCommons_v25_s.hpp>
#include <CampaignSvcs/CampaignServer/Compatibility/CampaignServer_v25_s.hpp>

/* AdServer::CampaignSvcs_v26 */
#include <CampaignSvcs/CampaignServer/Compatibility/CampaignCommons_v26.hpp>
#include <CampaignSvcs/CampaignServer/Compatibility/CampaignServer_v26.hpp>

#include <CampaignSvcs/CampaignServer/Compatibility/CampaignCommons_v26_s.hpp>
#include <CampaignSvcs/CampaignServer/Compatibility/CampaignServer_v26_s.hpp>

namespace AdServer
{
  namespace CampaignSvcs
  {
    // 2.4 -> 2.3 adapter
    class CampaignServerImpl_v25:
      public virtual CORBACommons::ReferenceCounting::
        ServantImpl<POA_AdServer::CampaignSvcs_v25::CampaignServer>
    {
    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
      DECLARE_EXCEPTION(InvalidArgument, Exception);

    public:
      /** */
      CampaignServerImpl_v25(
        Logging::Logger* logger,
        POA_AdServer::CampaignSvcs_v26::CampaignServer* campaign_server)
        /*throw(InvalidArgument, Exception, eh::Exception)*/;

      virtual ~CampaignServerImpl_v25() noexcept;

    public:
      virtual CORBA::Boolean
      need_config(const AdServer::CampaignSvcs_v25::TimestampInfo& master_stamp)
        /*throw(AdServer::CampaignSvcs_v25::CampaignServer::ImplementationException)*/;

      virtual AdServer::CampaignSvcs_v25::CampaignConfigUpdateInfo*
      get_config(
        const AdServer::CampaignSvcs_v25::CampaignGetConfigSettings& settings)
        /*throw(AdServer::CampaignSvcs_v25::CampaignServer::ImplementationException,
              AdServer::CampaignSvcs_v25::CampaignServer::NotReady)*/;

      virtual AdServer::CampaignSvcs_v25::EcpmSeq*
      get_ecpms(
        const AdServer::CampaignSvcs_v25::TimestampInfo& request_timestamp)
        /*throw(AdServer::CampaignSvcs_v25::CampaignServer::ImplementationException)*/;

      virtual AdServer::CampaignSvcs_v25::SimpleChannelAnswer*
      simple_channels(
        const AdServer::CampaignSvcs_v25::CampaignServer::GetSimpleChannelsInfo& settings)
        /*throw(AdServer::CampaignSvcs_v25::CampaignServer::ImplementationException,
              AdServer::CampaignSvcs_v25::CampaignServer::NotReady)*/;

      virtual
      AdServer::CampaignSvcs_v25::ExpressionChannelsInfo*
      get_expression_channels(
        const AdServer::CampaignSvcs_v25::CampaignServer::GetExpressionChannelsInfo&)
        /*throw(AdServer::CampaignSvcs_v25::CampaignServer::ImplementationException,
          AdServer::CampaignSvcs_v25::CampaignServer::NotReady)*/;

      virtual
      AdServer::CampaignSvcs_v25::DiscoverSourceInfo*
      get_discover_channels(CORBA::ULong, CORBA::ULong)
        /*throw(AdServer::CampaignSvcs_v25::CampaignServer::ImplementationException,
              AdServer::CampaignSvcs_v25::CampaignServer::NotReady)*/;

      virtual
      char* get_tag_passback(CORBA::ULong)
        /*throw(AdServer::CampaignSvcs_v25::CampaignServer::ImplementationException,
              AdServer::CampaignSvcs_v25::CampaignServer::NotReady)*/;

      virtual
      AdServer::CampaignSvcs_v25::FraudConditionConfig*
      fraud_conditions()
        /*throw(AdServer::CampaignSvcs_v25::CampaignServer::ImplementationException,
          AdServer::CampaignSvcs_v25::CampaignServer::NotReady)*/;

      virtual
      AdServer::CampaignSvcs_v25::DetectorsConfig*
      detectors(const AdServer::CampaignSvcs_v25::TimestampInfo& request_timestamp)
        /*throw(AdServer::CampaignSvcs_v25::CampaignServer::ImplementationException,
          AdServer::CampaignSvcs_v25::CampaignServer::NotReady)*/;

      virtual
      AdServer::CampaignSvcs_v25::FreqCapConfigInfo*
      freq_caps()
        /*throw(AdServer::CampaignSvcs_v25::CampaignServer::ImplementationException,
          AdServer::CampaignSvcs_v25::CampaignServer::NotReady)*/;

      virtual
      AdServer::CampaignSvcs_v25::StatInfo*
      get_stat() /*throw(
        AdServer::CampaignSvcs_v25::CampaignServer::NotSupport,
        AdServer::CampaignSvcs_v25::CampaignServer::NotReady)*/;

      virtual
      void update_stat() /*throw(
        AdServer::CampaignSvcs_v25::CampaignServer::NotSupport,
        AdServer::CampaignSvcs_v25::CampaignServer::ImplementationException)*/;

    protected:
      typedef PortableServer::Servant_var<
        POA_AdServer::CampaignSvcs_v26::CampaignServer>
        CampaignServer_var;

    protected:
      void convert_config_(
        const AdServer::CampaignSvcs_v26::CampaignConfigUpdateInfo& update_info,
        AdServer::CampaignSvcs_v25::CampaignConfigUpdateInfo& result_update_info,
        const Generics::Time& request_timestamp)
        /*throw(Exception, eh::Exception)*/;

      void convert_get_config_settings_(
        const AdServer::CampaignSvcs_v25::CampaignGetConfigSettings& settings,
        AdServer::CampaignSvcs_v26::CampaignGetConfigSettings& result_settings)
        /*throw(Exception, eh::Exception)*/;

      void convert_accounts_(
        AdServer::CampaignSvcs_v25::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v26::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;
      
      void convert_campaigns_(
        AdServer::CampaignSvcs_v25::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v26::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        const
        noexcept;

      void convert_creative_options_(
        AdServer::CampaignSvcs_v25::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v26::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_expression_channels_(
        AdServer::CampaignSvcs_v25::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v26::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_ecpm_seq_(
        AdServer::CampaignSvcs_v25::EcpmSeq& result_ecpms,
        const AdServer::CampaignSvcs_v26::EcpmSeq& ecpms,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_ecpms_(
        AdServer::CampaignSvcs_v25::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v26::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_sites_(
        AdServer::CampaignSvcs_v25::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v26::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_tags_(
        AdServer::CampaignSvcs_v25::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v26::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_frequency_caps_(
        AdServer::CampaignSvcs_v25::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v26::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_simple_channels_(
        AdServer::CampaignSvcs_v25::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v26::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_geo_channels_(
        AdServer::CampaignSvcs_v25::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v26::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_behav_params_(
        AdServer::CampaignSvcs_v25::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v26::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_colocations_(
        AdServer::CampaignSvcs_v25::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v26::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_creative_templates_(
        AdServer::CampaignSvcs_v25::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v26::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_currencies_(
        AdServer::CampaignSvcs_v25::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v26::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_campaign_keywords_(
        AdServer::CampaignSvcs_v25::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v26::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_creative_categories_(
        AdServer::CampaignSvcs_v25::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v26::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_adv_actions_(
        AdServer::CampaignSvcs_v25::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v26::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_keyword_ecpms_(
        AdServer::CampaignSvcs_v25::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v26::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_category_channels_(
        AdServer::CampaignSvcs_v25::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v26::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_margin_rules_(
        AdServer::CampaignSvcs_v25::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v26::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_fraud_conditions_(
        AdServer::CampaignSvcs_v25::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v26::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_search_engines_(
        AdServer::CampaignSvcs_v25::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v26::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_web_browsers_(
        AdServer::CampaignSvcs_v25::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v26::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_platforms_(
        AdServer::CampaignSvcs_v25::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v26::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_string_dictionaries_(
        AdServer::CampaignSvcs_v25::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v26::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

    private:
      Logging::Logger_var logger_;
      CampaignServer_var campaign_server_;
    };

    typedef ReferenceCounting::SmartPtr<CampaignServerImpl_v25>
      CampaignServerImpl_v25_var;
  }
}

#endif // CAMPAIGNSVCS_CAMPAIGNSERVER_CAMPAIGNSERVERIMPL_V25_HPP
