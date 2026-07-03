#pragma once

#include <eh/Exception.hpp>
#include <HTTP/UrlAddress.hpp>

#include <Commons/StringHolder.hpp>
#include <Commons/CorbaAlgs.hpp>
#include <Commons/Constants.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignSvcsVersionAdapter.hpp>
#include "CampaignManagerCore.hpp"
#include "CampaignManagerDeclarations.hpp"
#include "CampaignManagerLogger.hpp"
#include "CampaignConfig.hpp"

namespace AdServer::CampaignSvcs
{
  class CampaignManagerLogAdapter
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    static void
    fill_request_info(
      CampaignManagerLogger::RequestInfo& request_info,
      const CampaignConfig* campaign_config,
      const Colocation* colocation,
      const CampaignManagerCore::CommonAdRequestInfo& common_info,
      const CampaignManagerCore::ContextAdRequestInfo& context_info,
      const CampaignManagerCore::CreativeRequestInfo* request_params,
      const CampaignManagerCore::AdSlotContext& ad_slot_context)
      /*throw(Exception)*/;

    static void
    fill_ad_request_selection_info(
      CampaignManagerLogger::AdRequestSelectionInfo& ad_request_selection_info,
      const CampaignConfig* campaign_config,
      const Colocation* colocation,
      const CampaignManagerCore::CommonAdRequestInfo& common_info,
      const CampaignManagerCore::ContextAdRequestInfo& context_info,
      const CampaignManagerCore::CreativeRequestInfo* request_params,
      const CampaignManagerCore::TraceAdSlotInfo& ad_slot,
      const Tag* tag,
      const AdSelectionResult& ad_selection_request,
      const CampaignManagerCore::AdSlotContext& ad_slot_context,
      const CampaignManagerCore::AdSlotMinCpm& ad_slot_min_ecpm,
      const Tag::SizeMap& tag_sizes,
      bool disable_impression_tracking)
      /*throw(Exception)*/;

    static void
    fill_match_request_info(
      CampaignManagerLogger::MatchRequestInfo& result_match_request,
      const CampaignConfig* campaign_config,
      const CampaignManagerCore::MatchRequestInfo& match_request_info,
      const ChannelIdList& geo_channels)
      /*throw(Exception)*/;

  private:
    typedef CampaignManagerLogger::AdSelectionInfo AdSelectionInfo;
    typedef CampaignManagerLogger::AdSelectionInfoList AdSelectionInfoList;
    typedef AdSelectionInfo::Revenue Revenue;

    struct PubRevenues
    {
      Revenue net;
      Revenue comm;
    };

    struct DataPricing
    {
      const CampaignSelectionData* cs_data;
      const Tag::TagPricing* tag_pricing;
      Revenue adv_revenue_sys;

      DataPricing()
        : cs_data(0),
          tag_pricing(0)
      {}

      DataPricing(const CampaignSelectionData* cs_data_val, const Tag::TagPricing* tag_pricing_val)
        : cs_data(cs_data_val),
          tag_pricing(tag_pricing_val)
      {}
    };

  private:
    static void
    fill_ad_selection_info_(
      CampaignManagerLogger::AdSelectionInfo& ad_info,
      DataPricing& data_pricing,
      const CampaignConfig* campaign_config,
      const Colocation* colocation,
      const CampaignManagerCore::CommonAdRequestInfo& common_info,
      const CampaignManagerCore::ContextAdRequestInfo& context_info,
      const CampaignManagerCore::CreativeRequestInfo* request_params,
      const Tag* tag,
      const Tag::TagPricing* tag_pricing,
      const AdSelectionResult& ad_selection_result,
      unsigned long num_shown,
      unsigned long position,
      const CampaignManagerCore::AdSlotContext& ad_slot_context)
      /*throw(Exception)*/;

    static void
    convert_channel_ids_(
      ChannelIdHashSet& all_channels,
      ChannelIdHashSet& channels,
      CampaignManagerLogger::TriggerChannelMap& triggers,
      const CampaignManagerCore::ChannelTriggerMatchVector& behav_params)
      noexcept;

    static void
    fill_request_info_by_profiling_(
      CampaignManagerLogger::RequestInfo& request_info,
      const CampaignManagerCore::CreativeRequestInfo& request_params,
      const CampaignManagerCore::CommonAdRequestInfo& common_info)
      /*throw(Exception)*/;

    static void
    fill_request_info_by_common_info_(
      CampaignManagerLogger::RequestInfo& request_info,
      const CampaignManagerCore::CommonAdRequestInfo& common_info)
      /*throw(Exception)*/;
  };
}
