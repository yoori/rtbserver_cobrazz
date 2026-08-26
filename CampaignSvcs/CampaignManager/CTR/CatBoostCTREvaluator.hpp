#pragma once

#include <catboost/model_interface/wrapped_calcer.h>

#include "CTREvaluator.hpp"

namespace AdServer::CampaignSvcs::CTR
{
  using namespace AdInstances;

  class CatBoostCTREvaluator: public CTREvaluator
  {
  public:
    CatBoostCTREvaluator(
      const String::SubString& model_file,
      unsigned int features_size);

    virtual ~CatBoostCTREvaluator();

    std::unique_ptr<PredictionContext>
    create_prediction_context(
      const HashArray* request_hashes,
      const HashArray* auction_hashes) const override;

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
    class FeatureBufProvider;
    class PredictionContextImpl;

  private:
    std::unique_ptr<ModelCalcerWrapper> catboost_model_;
    std::unique_ptr<FeatureBufProvider> feature_buf_provider_;
  };
}
