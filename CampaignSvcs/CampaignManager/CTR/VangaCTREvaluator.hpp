#ifndef CAMPAIGNMANAGER_CTR_VANGACTREVALUATOR_HPP_
#define CAMPAIGNMANAGER_CTR_VANGACTREVALUATOR_HPP_

#include <DTree/DTree.hpp>
#include <DTree/LogRegPredictor.hpp>

#include "CTREvaluator.hpp"

namespace AdServer::CampaignSvcs::CTR
{
  struct VangaCTREvaluator: public CTREvaluator
  {
    VangaCTREvaluator(const String::SubString& model_file);

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
    Gears::IntrusivePtr<Vanga::LogRegPredictor<Vanga::DTree> > vanga_predictor_;
  };
}

#endif /*CAMPAIGNMANAGER_CTR_VANGACTREVALUATOR_HPP_*/
