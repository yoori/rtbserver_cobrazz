#ifndef CAMPAIGNMANAGER_CTR_TRIVIALCTREVALUATOR_HPP_
#define CAMPAIGNMANAGER_CTR_TRIVIALCTREVALUATOR_HPP_

#include <PredictorSvcs/BidCostPredictor/CtrPredictor.hpp>
#include "CTREvaluator.hpp"

namespace AdServer::CampaignSvcs::CTR
{
  struct TrivialCTREvaluator: public CTREvaluator
  {
    TrivialCTREvaluator(
      Logging::Logger* logger,
      const String::SubString& model_file);

    RevenueDecimal
    get_ctr(
      const ModelTraits& model,
      const CampaignSelectParams* request_params,
      const Creative* creative,
      const HashArray* request_hashes,
      const HashArray* auction_hashes,
      const HashArray* candidate_hashes,
      const HashArray* opt_hashes) const override;

  private:
    Logging::Logger_var logger_;
    PredictorSvcs::BidCostPredictor::CtrPredictor_var trivial_predictor_;
  };
}

#endif /*CAMPAIGNMANAGER_CTR_TRIVIALCTREVALUATOR_HPP_*/
