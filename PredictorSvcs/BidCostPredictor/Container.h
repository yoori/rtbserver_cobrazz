#ifndef BIDCOSTPREDICTOR_CONTAINER_H
#define BIDCOSTPREDICTOR_CONTAINER_H

// UNIXCOMMONS
#include <ReferenceCounting/Interface.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

// THIS
#include "ModelBidCost.hpp"
#include "ModelCtr.hpp"

namespace PredictorSvcs::BidCostPredictor
{

class Container : public virtual ReferenceCounting::Interface
{
public:
  using TagId = typename ModelBidCost::TagId;
  using Url = typename ModelBidCost::Url;
  using WinRate = typename ModelBidCost::WinRate;
  using Cost = typename ModelBidCost::Cost;
  using Ctr = typename ModelCtr::Ctr;

public:
  Container() = default;

  virtual Cost get_cost(
    const TagId& tag_id,
    const Url& url,
    const WinRate& win_rate,
    const Cost& current_cost) const = 0;

  virtual Ctr get_ctr(
    const TagId& tag_id,
    const Url& url) const = 0;

protected:
    virtual ~Container() = default;
};

using Container_var = ReferenceCounting::SmartPtr<Container>;

} // namespace PredictorSvcs::BidCostPredictor

#endif //BIDCOSTPREDICTOR_CONTAINER_H