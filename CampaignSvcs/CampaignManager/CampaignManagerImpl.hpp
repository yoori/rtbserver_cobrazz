#pragma once

#include <vector>

#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <CORBACommons/ServantImpl.hpp>

#include "CampaignManagerCore.hpp"

namespace AdServer
{
  namespace CampaignSvcs
  {
    class CampaignManagerImpl :
      public virtual CORBACommons::ReferenceCounting::ServantImpl<
        POA_AdServer::CampaignSvcs::CampaignManager>
    {
    public:
      explicit CampaignManagerImpl(CampaignManagerCore* core);

      bool ready() /*throw(eh::Exception)*/;

      void progress_comment(std::string& res) /*throw(eh::Exception)*/;

      void get_campaign_creative(
        const AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
        CORBA::String_out hostname,
        AdServer::CampaignSvcs::CampaignManager::RequestCreativeResult_out request_result)
        /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
          AdServer::CampaignSvcs::CampaignManager::NotReady)*/ override;

      void match_geo_channels(
        const AdServer::CampaignSvcs::CampaignManager::GeoInfoSeq& location,
        const AdServer::CampaignSvcs::CampaignManager::GeoCoordInfoSeq& coord_location,
        AdServer::CampaignSvcs::ChannelIdSeq_out geo_channels_result,
        AdServer::CampaignSvcs::ChannelIdSeq_out coord_channels_result)
        /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
          AdServer::CampaignSvcs::CampaignManager::NotReady)*/ override;

      void process_match_request(
        const AdServer::CampaignSvcs::CampaignManager::MatchRequestInfo&
          match_request_info)
        /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
          AdServer::CampaignSvcs::CampaignManager::NotReady)*/ override;

      void process_anonymous_request(
        const AdServer::CampaignSvcs::CampaignManager::AnonymousRequestInfo&
          anon_request_info)
        /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
          AdServer::CampaignSvcs::CampaignManager::NotReady)*/ override;

      void get_file(
        const char* file_name,
        CORBACommons::OctSeq_out file)
        /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException)*/ override;

      void instantiate_ad(
        const AdServer::CampaignSvcs::CampaignManager::
          InstantiateAdInfo& instantiate_ad_info,
        AdServer::CampaignSvcs::CampaignManager::
          InstantiateAdResult_out instantiate_ad_result)
        /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
          AdServer::CampaignSvcs::CampaignManager::NotReady)*/ override;

      void trace_campaign_selection(
        CORBA::ULong campaign_id,
        const AdServer::CampaignSvcs::CampaignManager::RequestParams&
          request_params,
        const AdServer::CampaignSvcs::CampaignManager::AdSlotInfo& ad_slot,
        CORBA::ULong auction_type,
        CORBA::Boolean test_request,
        CORBA::String_out trace_xml)
        /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException)*/ override;

      void trace_campaign_selection_index(CORBA::String_out trace_xml)
        /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException)*/ override;

      CORBA::Boolean get_campaign_creative_by_ccid(
        const ::AdServer::CampaignSvcs::CampaignManager::CreativeParams& params,
        CORBA::String_out creative_body)
        /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException)*/ override;

      void consider_passback(
        const AdServer::CampaignSvcs::CampaignManager::PassbackInfo& in)
        /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException)*/ override;

      void consider_passback_track(
        const AdServer::CampaignSvcs::CampaignManager::PassbackTrackInfo& in)
        /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
          AdServer::CampaignSvcs::CampaignManager::NotReady)*/ override;

      CORBA::Boolean get_click_url(
        const ::AdServer::CampaignSvcs::CampaignManager::ClickInfo& click_info,
        ::AdServer::CampaignSvcs::CampaignManager::ClickResultInfo_out click_result_info)
        /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
          AdServer::CampaignSvcs::CampaignManager::NotReady)*/ override;

      void verify_impression(
        const AdServer::CampaignSvcs::CampaignManager::ImpressionInfo& impression_info,
        ::AdServer::CampaignSvcs::CampaignManager::ImpressionResultInfo_out impression_result_info)
        /*throw(
          AdServer::CampaignSvcs::CampaignManager::ImplementationException,
          AdServer::CampaignSvcs::CampaignManager::NotReady)*/ override;

      void action_taken(
        const AdServer::CampaignSvcs::CampaignManager::ActionInfo& action_info)
        /*throw(
          AdServer::CampaignSvcs::CampaignManager::ImplementationException,
          AdServer::CampaignSvcs::CampaignManager::NotReady)*/ override;

      AdServer::CampaignSvcs::CampaignManager::ChannelSearchResultSeq*
      get_channel_links(
        const AdServer::CampaignSvcs::ChannelIdSeq& channels,
        bool match)
        /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException)*/ override;

      AdServer::CampaignSvcs::CampaignManager::DiscoverChannelResultSeq*
      get_discover_channels(
        const AdServer::CampaignSvcs::ChannelWeightSeq& channels,
        const char* country,
        const char* language,
        bool all)
        /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
          AdServer::CampaignSvcs::CampaignManager::NotReady)*/ override;

      AdServer::CampaignSvcs::CampaignManager::CategoryChannelNodeSeq*
      get_category_channels(const char* language)
        /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
          AdServer::CampaignSvcs::CampaignManager::NotReady)*/ override;

      ::AdServer::CampaignSvcs::CampaignManager::CampaignConfig*
      get_config(const AdServer::CampaignSvcs::
        CampaignManager::GetConfigInfo& get_config_props)
        /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException)*/ override;

      void verify_opt_operation(
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
        /*throw(AdServer::CampaignSvcs::CampaignManager::NotReady)*/ override;

      void consider_web_operation(
        const AdServer::CampaignSvcs::CampaignManager::WebOperationInfo& web_op_info)
        /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
          AdServer::CampaignSvcs::CampaignManager::IncorrectArgument,
          AdServer::CampaignSvcs::CampaignManager::NotReady)*/ override;

      ColocationFlagsSeq* get_colocation_flags()
        /*throw(AdServer::CampaignSvcs::CampaignManager::ImplementationException,
          AdServer::CampaignSvcs::CampaignManager::NotReady)*/ override;

      AdServer::CampaignSvcs::StringSeq* get_pub_pixels(
        const char* country,
        CORBA::ULong user_status,
        const AdServer::CampaignSvcs::PublisherAccountIdSeq& publisher_account_ids)
        /*throw(AdServer::CampaignSvcs::CampaignManager::NotReady,
          AdServer::CampaignSvcs::CampaignManager::ImplementationException)*/ override;

    protected:
      virtual ~CampaignManagerImpl() noexcept;

    private:
      template<typename Seq>
      static void
      pack_id_seq(
        const CampaignManagerCore::IdVector& source,
        Seq& target);

      static void
      pack_category_channel_nodes(
        const std::vector<CampaignManagerCore::CategoryChannelNodeInfo>& source,
        AdServer::CampaignSvcs::CampaignManager::CategoryChannelNodeSeq& target);

      CampaignManagerCore_var core_;
    };

    typedef ReferenceCounting::SmartPtr<CampaignManagerImpl>
      CampaignManagerImpl_var;
  }
}
