#pragma once

#include "CTREvaluator.hpp"
#include "XGBoostPredictor.hpp"

namespace AdServer::CampaignSvcs::CTR
{
  struct XGBoostCTREvaluator: public CTREvaluator
  {
    XGBoostCTREvaluator(const String::SubString& model_file);

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
    XGBoostPredictorPool_var xgboost_predictor_pool_;
  };
}
