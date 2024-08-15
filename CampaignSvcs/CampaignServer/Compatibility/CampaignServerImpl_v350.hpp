#ifndef CAMPAIGNSVCS_CAMPAIGNSERVER_CAMPAIGNSERVERIMPL_V350_HPP
#define CAMPAIGNSVCS_CAMPAIGNSERVER_CAMPAIGNSERVERIMPL_V350_HPP

#include <ReferenceCounting/ReferenceCounting.hpp>

#include <eh/Exception.hpp>

#include <Generics/ActiveObject.hpp>
#include <Logger/Logger.hpp>

#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/ServantImpl.hpp>

/* AdServer::CampaignSvcs_v350 */
#include <CampaignSvcs/CampaignServer/Compatibility/CampaignCommons_v350.hpp>
#include <CampaignSvcs/CampaignServer/Compatibility/CampaignServer_v350.hpp>

#include <CampaignSvcs/CampaignServer/Compatibility/CampaignCommons_v350_s.hpp>
#include <CampaignSvcs/CampaignServer/Compatibility/CampaignServer_v350_s.hpp>

/* AdServer::CampaignSvcs_v360 */
#include <CampaignSvcs/CampaignCommons/CampaignCommons.hpp>
#include <CampaignSvcs/CampaignServer/CampaignServer.hpp>

#include <CampaignSvcs/CampaignCommons/CampaignCommons_s.hpp>
#include <CampaignSvcs/CampaignServer/CampaignServer_s.hpp>

namespace AdServer
{
  namespace CampaignSvcs
  {
    class CampaignServerImpl_v350:
      public virtual CORBACommons::ReferenceCounting::
        ServantImpl<POA_AdServer::CampaignSvcs_v350::CampaignServer>
    {
    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
      DECLARE_EXCEPTION(InvalidArgument, Exception);

    public:
      /** */
      CampaignServerImpl_v350(
        Logging::Logger* logger,
        POA_AdServer::CampaignSvcs_v360::CampaignServer* campaign_server);

    public:
      typedef Generics::SimpleDecimal<uint64_t, 18, 8> RevenueDecimal_v300;

      virtual CORBA::Boolean
      need_config(const AdServer::CampaignSvcs_v350::TimestampInfo& master_stamp);

      virtual AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo*
      get_config(
        const AdServer::CampaignSvcs_v350::CampaignGetConfigSettings& settings);

      virtual AdServer::CampaignSvcs_v350::EcpmSeq*
      get_ecpms(
        const AdServer::CampaignSvcs_v350::TimestampInfo& request_timestamp);

      virtual AdServer::CampaignSvcs_v350::BriefSimpleChannelAnswer*
      brief_simple_channels(
        const AdServer::CampaignSvcs_v350::CampaignServer::
          GetSimpleChannelsInfo& settings);

      virtual AdServer::CampaignSvcs_v350::SimpleChannelAnswer*
      simple_channels(
        const AdServer::CampaignSvcs_v350::CampaignServer::GetSimpleChannelsInfo& settings);

      virtual AdServer::CampaignSvcs_v350::ChannelServerChannelAnswer*
      chsv_simple_channels(
        const AdServer::CampaignSvcs_v350::CampaignServer::
        GetSimpleChannelsInfo& settings);

      virtual
      AdServer::CampaignSvcs_v350::ExpressionChannelsInfo*
      get_expression_channels(
        const AdServer::CampaignSvcs_v350::CampaignServer::GetExpressionChannelsInfo&);

      virtual
      AdServer::CampaignSvcs_v350::DiscoverSourceInfo*
      get_discover_channels(CORBA::ULong, CORBA::ULong);

      virtual
      AdServer::CampaignSvcs_v350::CampaignServer::PassbackInfo*
      get_tag_passback(CORBA::ULong, const char*);

      virtual
      AdServer::CampaignSvcs_v350::FraudConditionConfig*
      fraud_conditions();

      virtual
      AdServer::CampaignSvcs_v350::DetectorsConfig*
      detectors(const AdServer::CampaignSvcs_v350::TimestampInfo& request_timestamp);

      virtual
      AdServer::CampaignSvcs_v350::FreqCapConfigInfo*
      freq_caps();

      virtual
      AdServer::CampaignSvcs_v350::StatInfo*
      get_stat();

      virtual
      AdServer::CampaignSvcs_v350::ColocationFlagsSeq*
      get_colocation_flags();

      virtual
      void update_stat();

      virtual AdServer::CampaignSvcs_v350::BillStatInfo*
      get_bill_stat();

    protected:
      virtual
      ~CampaignServerImpl_v350() noexcept;

      typedef PortableServer::Servant_var<
        POA_AdServer::CampaignSvcs_v360::CampaignServer>
        CampaignServer_var;

    protected:
      void convert_config_(
        const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
        AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
        const Generics::Time& request_timestamp)
        /*throw(Exception, eh::Exception)*/;

      void convert_get_config_settings_(
        const AdServer::CampaignSvcs_v350::CampaignGetConfigSettings& settings,
        AdServer::CampaignSvcs_v360::CampaignGetConfigSettings& result_settings)
        /*throw(Exception, eh::Exception)*/;

      void
      convert_sizes_(
        AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      void convert_accounts_(
        AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      void convert_campaigns_(
        AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        const
        noexcept;

      void convert_creative_options_(
        AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_expression_channels_(
        AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_ecpm_seq_(
        AdServer::CampaignSvcs_v350::EcpmSeq& result_ecpms,
        const AdServer::CampaignSvcs_v360::EcpmSeq& ecpms,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_ecpms_(
        AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_sites_(
        AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_tags_(
        AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_frequency_caps_(
        AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_simple_channels_(
        AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_geo_channels_(
        AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_geo_coord_channels_(
        AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_block_channels_(
        AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_behav_params_(
        AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_colocations_(
        AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_creative_templates_(
        AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_currencies_(
        AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_campaign_keywords_(
        AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_creative_categories_(
        AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_adv_actions_(
        AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_category_channels_(
        AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_margin_rules_(
        AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_fraud_conditions_(
        AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_search_engines_(
        AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_web_browsers_(
        AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_platforms_(
        AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_string_dictionaries_(
        AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void convert_app_formats_(
        AdServer::CampaignSvcs_v350::CampaignConfigUpdateInfo& result_update_info,
        const AdServer::CampaignSvcs_v360::CampaignConfigUpdateInfo& update_info,
        const Generics::Time& request_timestamp)
        noexcept;

      static void
      convert_amount_distribution_(
        AdServer::CampaignSvcs_v350::AmountDistributionInfo& result_amount_distribution,
        const AdServer::CampaignSvcs_v360::AmountDistributionInfo& amount_distribution)
        /*throw(Exception, eh::Exception)*/;

    private:
      Logging::Logger_var logger_;
      CampaignServer_var campaign_server_;
    };

    typedef ReferenceCounting::SmartPtr<CampaignServerImpl_v350>
      CampaignServerImpl_v350_var;
  }
}

#endif // CAMPAIGNSVCS_CAMPAIGNSERVER_CAMPAIGNSERVERIMPL_V350_HPP
