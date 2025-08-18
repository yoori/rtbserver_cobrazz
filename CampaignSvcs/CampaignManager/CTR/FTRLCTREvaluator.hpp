#ifndef CAMPAIGNMANAGER_CTR_FTRLCTREVALUATOR_HPP_
#define CAMPAIGNMANAGER_CTR_FTRLCTREVALUATOR_HPP_

#include "CTREvaluator.hpp"

namespace AdServer::CampaignSvcs::CTR
{
  struct FTRLCTREvaluator: public CTREvaluator
  {
    FTRLCTREvaluator(const String::SubString& model_file);

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
    float
    eval_weight_(const HashArray& auction_hashes) const;

  private:
    std::vector<float> feature_weights_;
  };
}

#endif /*CAMPAIGNMANAGER_CTR_FTRLCTREVALUATOR_HPP_*/
