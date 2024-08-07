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
  using TagId = Types::TagId;
  using Url = Types::Url;
  using WinRate = Types::WinRate;
  using Cost = Types::Cost;
  using CreativeCategoryId = Types::CreativeCategoryId;
  using Ctr = ModelCtr::Ctr;

public:
  Container() = default;

  virtual Cost get_cost(
    const TagId& tag_id,
    const Url& url,
    const WinRate& win_rate,
    const Cost& current_cost) const = 0;

  virtual Ctr get_ctr(
    const TagId& tag_id,
    const Url& url,
    const CreativeCategoryId& creative_category_id) const = 0;

protected:
    virtual ~Container() = default;
};

using Container_var = ReferenceCounting::SmartPtr<Container>;

} // namespace PredictorSvcs::BidCostPredictor

#endif //BIDCOSTPREDICTOR_CONTAINER_H