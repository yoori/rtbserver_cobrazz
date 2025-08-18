#ifndef CAMPAIGNMANAGER_CTR_ALGORITHM_HPP_
#define CAMPAIGNMANAGER_CTR_ALGORITHM_HPP_

#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>

#include "CTRFeatureCalculators.hpp"
#include "Model.hpp"

namespace AdServer::CampaignSvcs::CTR
{
  // Algorithm
  struct Algorithm: public ReferenceCounting::AtomicImpl
  {
    Algorithm()
      : threshold(RevenueDecimal::ZERO)
    {}

    std::string id;
    unsigned long weight;
    ModelList models;
    RevenueDecimal threshold;

  protected:
    virtual ~Algorithm() noexcept
    {}
  };

  typedef ReferenceCounting::SmartPtr<const Algorithm>
    ConstAlgorithm_var;
  typedef ReferenceCounting::SmartPtr<Algorithm>
    Algorithm_var;
}

#endif /*CAMPAIGNMANAGER_CTR_ALGORITHM_HPP_*/
