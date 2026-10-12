#pragma once

#include "CampaignServerApi.hpp"
#include <CampaignSvcs/CampaignServer/CampaignServer.grpc.pb.h>

namespace AdServer::CampaignSvcs
{
  namespace cs_rpc = adserver::campaign::server;

  class CampaignServerGrpcService final : public cs_rpc::CampaignServer::Service
  {
  public:
    explicit CampaignServerGrpcService(CampaignServerApi& source);
    grpc::Status get_config(
      grpc::ServerContext*,
      const cs_rpc::get_configRequest*,
      cs_rpc::get_configResponse*) override;
    grpc::Status need_config(
      grpc::ServerContext*,
      const cs_rpc::need_configRequest*,
      cs_rpc::need_configResponse*) override;
    grpc::Status get_ecpms(
      grpc::ServerContext*,
      const cs_rpc::get_ecpmsRequest*,
      cs_rpc::get_ecpmsResponse*) override;
    grpc::Status get_expression_channels(
      grpc::ServerContext*,
      const cs_rpc::get_expression_channelsRequest*,
      cs_rpc::get_expression_channelsResponse*) override;
    grpc::Status get_tag_passback(
      grpc::ServerContext*,
      const cs_rpc::get_tag_passbackRequest*,
      cs_rpc::get_tag_passbackResponse*) override;
    grpc::Status brief_simple_channels(
      grpc::ServerContext*,
      const cs_rpc::brief_simple_channelsRequest*,
      cs_rpc::brief_simple_channelsResponse*) override;
    grpc::Status simple_channels(
      grpc::ServerContext*,
      const cs_rpc::simple_channelsRequest*,
      cs_rpc::simple_channelsResponse*) override;
    grpc::Status chsv_simple_channels(
      grpc::ServerContext*,
      const cs_rpc::chsv_simple_channelsRequest*,
      cs_rpc::chsv_simple_channelsResponse*) override;
    grpc::Status fraud_conditions(
      grpc::ServerContext*,
      const cs_rpc::fraud_conditionsRequest*,
      cs_rpc::fraud_conditionsResponse*) override;
    grpc::Status freq_caps(
      grpc::ServerContext*,
      const cs_rpc::freq_capsRequest*,
      cs_rpc::freq_capsResponse*) override;
    grpc::Status get_colocation_flags(
      grpc::ServerContext*,
      const cs_rpc::get_colocation_flagsRequest*,
      cs_rpc::get_colocation_flagsResponse*) override;
    grpc::Status detectors(
      grpc::ServerContext*,
      const cs_rpc::detectorsRequest*,
      cs_rpc::detectorsResponse*) override;
    grpc::Status update_stat(
      grpc::ServerContext*,
      const cs_rpc::update_statRequest*,
      cs_rpc::update_statResponse*) override;
    grpc::Status get_stat(
      grpc::ServerContext*,
      const cs_rpc::get_statRequest*,
      cs_rpc::get_statResponse*) override;
    grpc::Status get_bill_stat(
      grpc::ServerContext*,
      const cs_rpc::get_bill_statRequest*,
      cs_rpc::get_bill_statResponse*) override;
    grpc::Status get_colocation_prop(
      grpc::ServerContext*,
      const cs_rpc::get_colocation_propRequest*,
      cs_rpc::get_colocation_propResponse*) override;
    grpc::Status get_delivery_limit_config(
      grpc::ServerContext*,
      const cs_rpc::get_delivery_limit_configRequest*,
      cs_rpc::get_delivery_limit_configResponse*) override;

  private:
    CampaignServerApi& source_;
  };
}
