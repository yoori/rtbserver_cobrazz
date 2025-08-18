#include "TrivialCTREvaluator.hpp"

namespace AdServer::CampaignSvcs::CTR
{
  TrivialCTREvaluator::TrivialCTREvaluator(
    Logging::Logger* logger,
    const String::SubString& file)
  {
    trivial_predictor_ = new PredictorSvcs::BidCostPredictor::CtrPredictor(logger);
    trivial_predictor_->load(file.str());
  }

  RevenueDecimal
  TrivialCTREvaluator::get_ctr(
    const ModelTraits& model,
    const CampaignSelectParams* request_params,
    const Creative* creative,
    const HashArray* request_hashes,
    const HashArray* auction_hashes,
    const HashArray* candidate_hashes,
    const HashArray* opt_hashes) const
  {
    return trivial_predictor_->predict(                                                                  
      request_params->tag ? request_params->tag->tag_id : 0,                                                
      request_params->referer_hostname,                                                                      
      creative->content_categories);
  }
}
