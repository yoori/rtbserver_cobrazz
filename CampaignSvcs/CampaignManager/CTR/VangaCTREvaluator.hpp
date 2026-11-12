#pragma once

#include <DTree/DTree.hpp>

#include "CTREvaluator.hpp"

namespace AdServer::CampaignSvcs::CTR
{
  struct VangaCTREvaluator: public CTREvaluator
  {
    VangaCTREvaluator(const String::SubString& model_file);

    double
    predict(
      const ModelTraits& model,
      const CampaignSelectParams* request_params,
      const Creative* creative,
      const HashArray* request_hashes,
      const HashArray* auction_hashes,
      const HashArray* candidate_hashes,
      const HashArray* opt_hashes) const override;

  private:
    Vanga::DTree_var vanga_predictor_;
  };
}
