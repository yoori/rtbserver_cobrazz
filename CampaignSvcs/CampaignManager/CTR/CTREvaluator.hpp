#pragma once

#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include "CTRFeatureCalculators.hpp"
#include "ModelTraits.hpp"

namespace AdServer::CampaignSvcs::CTR
{
  using namespace AdInstances;

  struct CTREvaluator
  {
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    virtual RevenueDecimal
    get_ctr(
      const ModelTraits& model,
      const CampaignSelectParams* request_params,
      const Creative* creative,
      const HashArray* request_hashes,
      const HashArray* auction_hashes,
      const HashArray* candidate_hashes,
      const HashArray* opt_hashes) const = 0;
  };
}
