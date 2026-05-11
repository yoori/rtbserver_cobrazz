#pragma once

#include <utility>

#include <CampaignSvcs/CampaignManagerClient/CampaignManagerCorbaClient.hpp>
#include <xsd/Frontends/FeConfig.hpp>

namespace FrontendCommons
{
  inline
  AdServer::CampaignSvcs::CampaignManagerCorbaClient::CampaignManagerRefs
  read_campaign_manager_refs(
    const xsd::AdServer::Configuration::CommonFeConfigurationType&
      common_config)
  {
    AdServer::CampaignSvcs::CampaignManagerCorbaClient::CampaignManagerRefs
      result;

    if(common_config.CampaignManagerRef().present())
    {
      const auto& refs = common_config.CampaignManagerRef()->Ref();
      result.reserve(refs.size());
      for(const auto& ref : refs)
      {
        AdServer::CampaignSvcs::CampaignManagerCorbaClient::CampaignManagerRef
          converted_ref;
        converted_ref.object_ref = ref.ref();
        if(ref.service_index().present())
        {
          converted_ref.service_index = *ref.service_index();
        }
        result.push_back(std::move(converted_ref));
      }
    }

    return result;
  }
}
