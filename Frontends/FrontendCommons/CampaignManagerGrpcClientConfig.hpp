#pragma once

#include <utility>

#include <CampaignSvcs/CampaignManagerClient/CampaignManagerDistributedGrpcClient.hpp>
#include <xsd/Frontends/FeConfig.hpp>

namespace FrontendCommons
{
  inline
  AdServer::CampaignSvcs::CampaignManagerDistributedGrpcClient::CampaignManagerRefs
  read_campaign_manager_grpc_refs(
    const xsd::AdServer::Configuration::CommonFeConfigurationType&
      common_config)
  {
    AdServer::CampaignSvcs::CampaignManagerDistributedGrpcClient::
      CampaignManagerRefs result;

    for(const auto& group : common_config.CampaignManagerGrpcGroup())
    {
      const auto& endpoints = group.Endpoint();
      result.reserve(result.size() + endpoints.size());
      for(const auto& endpoint : endpoints)
      {
        AdServer::CampaignSvcs::CampaignManagerDistributedGrpcClient::
          CampaignManagerRef converted_ref;
        converted_ref.object_ref = endpoint;
        if(endpoint.service_index().present())
        {
          converted_ref.service_index = *endpoint.service_index();
        }
        result.push_back(std::move(converted_ref));
      }
    }

    return result;
  }
}
