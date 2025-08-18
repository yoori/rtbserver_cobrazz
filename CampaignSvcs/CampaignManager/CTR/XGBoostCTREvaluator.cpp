#include "XGBoostCTREvaluator.hpp"

namespace AdServer::CampaignSvcs::CTR
{
  XGBoostCTREvaluator::XGBoostCTREvaluator(
    const String::SubString& model_file)
    : xgboost_predictor_pool_(new XGBoostPredictorPool(model_file))
  {}

  RevenueDecimal
  XGBoostCTREvaluator::get_ctr(
    const ModelTraits& model,
    const CampaignSelectParams* /*request_params*/,
    const Creative* /*creative*/,
    const HashArray* request_hashes,
    const HashArray* auction_hashes,
    const HashArray* candidate_hashes,
    const HashArray* opt_hashes) const
  {
    XGBoostPredictorPool::Predictor_var xgboost_predictor =
      xgboost_predictor_pool_->get_predictor();

    float ctr;

    if (request_hashes)
    {
      ctr = xgboost_predictor->predict(
        *request_hashes,
        auction_hashes,
        candidate_hashes);

      if(ctr < DBL_MIN) // prevent sub normal states (FP_ZERO, FP_SUBNORMAL)
      {
        return RevenueDecimal::ZERO;
      }
    }
    else
    {
      ctr = 0.5;
    }

    return Generics::convert_float<RevenueDecimal>(ctr);
  }
}
