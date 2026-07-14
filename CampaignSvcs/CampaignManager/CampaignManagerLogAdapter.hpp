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
      const CampaignManagerCore::CommonAdRequest& common_info,
      const CampaignManagerCore::ContextAdRequest& context_info,
      const CampaignManagerCore::LogAdRequest* log_request,
      const ChannelIdHashSet* channels,
      bool is_ad_request,
      bool track_passback,
      const CampaignManagerCore::AdSlotContext& ad_slot_context)
      /*throw(Exception)*/;

    static void
    fill_ad_request_selection_info(
      CampaignManagerLogger::AdRequestSelectionInfo& ad_request_selection_info,
      const CampaignConfig* campaign_config,
      const Colocation* colocation,
      const CampaignManagerCore::CommonAdRequest& common_info,
      const CampaignManagerCore::ContextAdRequest& context_info,
      const ChannelIdHashSet* channels,
      const CampaignManagerCore::AdSlotRequest& ad_slot,
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
      const ChannelIdArray& geo_channels)
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
      const CampaignManagerCore::CommonAdRequest& common_info,
      const CampaignManagerCore::ContextAdRequest& context_info,
      const ChannelIdHashSet* channels,
      const Tag* tag,
      const Tag::TagPricing* tag_pricing,
      const AdSelectionResult& ad_selection_result,
      unsigned long num_shown,
      unsigned long position,
      const CampaignManagerCore::AdSlotContext& ad_slot_context)
      /*throw(Exception)*/;

    static void
    fill_responded_channel_info_(
      CampaignManagerLogger::AdSelectionInfo& ad_info,
      const CampaignSelectionData& cs_data,
      const ChannelIdHashSet* channels)
      noexcept;

    static void
    convert_channel_ids_(
      ChannelIdHashSet& all_channels,
      ChannelIdHashSet& channels,
      CampaignManagerLogger::TriggerChannelMap& triggers,
      const CampaignManagerCore::ChannelTriggerMatchArray& behav_params)
      noexcept;

    static void
    fill_request_info_by_profiling_(
      CampaignManagerLogger::RequestInfo& request_info,
      const CampaignManagerCore::LogAdRequest& log_request,
      const ChannelIdHashSet& channels,
      const CampaignManagerCore::CommonAdRequest& common_info)
      /*throw(Exception)*/;

    static void
    fill_request_info_by_common_info_(
      CampaignManagerLogger::RequestInfo& request_info,
      const CampaignManagerCore::CommonAdRequest& common_info)
      /*throw(Exception)*/;
  };
}
