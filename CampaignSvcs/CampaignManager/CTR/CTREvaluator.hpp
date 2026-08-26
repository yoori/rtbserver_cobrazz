#pragma once

#include <memory>

#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include "CTRFeatureCalculators.hpp"
#include "ModelTraits.hpp"

namespace AdServer::CampaignSvcs::CTR
{
  using namespace AdInstances;

  struct CTREvaluator
  {
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    class PredictionContext
    {
    public:
      virtual double
      predict(
        const HashArray* candidate_hashes,
        const HashArray* opt_hashes) = 0;

      virtual ~PredictionContext() noexcept = default;
    };

    virtual std::unique_ptr<PredictionContext>
    create_prediction_context(
      const HashArray*,
      const HashArray*,
      Generics::MonoAllocatorArena*) const
    {
      return nullptr;
    }

    virtual double
    predict(
      const ModelTraits& model,
      const CampaignSelectParams* request_params,
      const Creative* creative,
      const HashArray* request_hashes,
      const HashArray* auction_hashes,
      const HashArray* candidate_hashes,
      const HashArray* opt_hashes) const = 0;
  };
}
