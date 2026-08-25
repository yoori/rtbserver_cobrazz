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
    ModelArray models;
    std::map<unsigned long, ModelArray> campaign_models;
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
