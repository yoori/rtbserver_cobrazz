#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <CampaignSvcs/CampaignServer/CampaignServerApi.hpp>
#include <CampaignSvcs/CampaignServer/CampaignServer.grpc.pb.h>

namespace AdServer::CampaignSvcs
{
  class CampaignServerClient final : public CampaignServerApi
  {
  public:
    explicit CampaignServerClient(
      const std::string& endpoint,
      std::chrono::milliseconds timeout = std::chrono::seconds(30));

    CampaignConfigUpdateInfo* get_config(const CampaignGetConfigSettings& settings) override;
    bool need_config(const TimestampInfo& req_timestamp) override;
    EcpmSeq* get_ecpms(const TimestampInfo& request_timestamp) override;
    ExpressionChannelsInfo* get_expression_channels(const GetExpressionChannelsInfo& info) override;
    PassbackInfo* get_tag_passback(std::uint32_t tag_id, const char* app_format) override;
    BriefSimpleChannelAnswer* brief_simple_channels(const GetSimpleChannelsInfo& params) override;
    SimpleChannelAnswer* simple_channels(const GetSimpleChannelsInfo& params) override;
    ChannelServerChannelAnswer* chsv_simple_channels(const GetSimpleChannelsInfo& params) override;
    FraudConditionConfig* fraud_conditions() override;
    FreqCapConfigInfo* freq_caps() override;
    ColocationFlagsSeq* get_colocation_flags() override;
    DetectorsConfig* detectors(const TimestampInfo& request_timestamp) override;
    void update_stat() override;
    StatInfo* get_stat() override;
    BillStatInfo* get_bill_stat() override;
    ColocationPropInfo* get_colocation_prop(std::uint32_t colo_id) override;
    DeliveryLimitConfigInfo* get_delivery_limit_config() override;

  private:
    std::unique_ptr<adserver::campaign::server::CampaignServer::Stub> stub_;
    std::chrono::milliseconds timeout_;
  };
}
