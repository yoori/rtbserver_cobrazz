#pragma once

#include <cstdint>
#include <CampaignSvcs/CampaignServer/CampaignServer.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignSvcsVersionAdapter.hpp>

namespace AdServer::CampaignSvcs
{
  // Domain interface; IDL value types remain until the separate native-types cleanup.
  class CampaignServerApi
  {
  public:
    using GetExpressionChannelsInfo = CampaignServer::GetExpressionChannelsInfo;
    using GetSimpleChannelsInfo = CampaignServer::GetSimpleChannelsInfo;
    using PassbackInfo = CampaignServer::PassbackInfo;
    using ColocationPropInfo = CampaignServer::ColocationPropInfo;
    using DeliveryLimitConfigInfo = CampaignServer::DeliveryLimitConfigInfo;

    virtual ~CampaignServerApi();

    virtual CampaignConfigUpdateInfo* get_config(const CampaignGetConfigSettings& settings) = 0;
    virtual bool need_config(const TimestampInfo& req_timestamp) = 0;
    virtual EcpmSeq* get_ecpms(const TimestampInfo& request_timestamp) = 0;
    virtual ExpressionChannelsInfo* get_expression_channels(const GetExpressionChannelsInfo& params)
      = 0;
    virtual PassbackInfo* get_tag_passback(std::uint32_t tag_id, const char* app_format) = 0;
    virtual BriefSimpleChannelAnswer* brief_simple_channels(const GetSimpleChannelsInfo& info) = 0;
    virtual SimpleChannelAnswer* simple_channels(const GetSimpleChannelsInfo& params) = 0;
    virtual ChannelServerChannelAnswer* chsv_simple_channels(const GetSimpleChannelsInfo& info) = 0;
    virtual FraudConditionConfig* fraud_conditions() = 0;
    virtual FreqCapConfigInfo* freq_caps() = 0;
    virtual ColocationFlagsSeq* get_colocation_flags() = 0;
    virtual DetectorsConfig* detectors(const TimestampInfo& request_timestamp) = 0;
    virtual void update_stat() = 0;
    virtual StatInfo* get_stat() = 0;
    virtual BillStatInfo* get_bill_stat() = 0;
    virtual ColocationPropInfo* get_colocation_prop(std::uint32_t colo_id) = 0;
    virtual DeliveryLimitConfigInfo* get_delivery_limit_config() = 0;
  };

  inline CampaignServerApi::~CampaignServerApi() = default;
}
