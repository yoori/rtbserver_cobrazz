#pragma once

#include <map>

#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>

#include "CTRFeatureCalculators.hpp"
#include "Model.hpp"

namespace AdServer::CampaignSvcs::CTR
{
  enum class PredictionAggregation
  {
    Sum,
    LogitSum
  };

  // Algorithm
  struct Algorithm: public ReferenceCounting::AtomicImpl
  {
    Algorithm()
      : threshold(RevenueDecimal::ZERO)
    {}

    std::string id;
    unsigned long weight;
    ModelList models;
    std::map<unsigned long, ModelList> campaign_models;
    RevenueDecimal threshold;
    PredictionAggregation aggregation = PredictionAggregation::Sum;

  protected:
    virtual ~Algorithm() noexcept
    {}
  };

  typedef ReferenceCounting::SmartPtr<const Algorithm>
    ConstAlgorithm_var;
  typedef ReferenceCounting::SmartPtr<Algorithm>
    Algorithm_var;
}
