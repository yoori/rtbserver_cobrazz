#ifndef BIDCOSTPREDICTOR_MODEL_BIDCOST_HPP
#define BIDCOSTPREDICTOR_MODEL_BIDCOST_HPP

// STD
#include <memory>
#include <string>

// UNIXCOMMONS
#include <ReferenceCounting/Interface.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

// THIS
#include <LogCommons/LogCommons.hpp>
#include "ModelManager.hpp"
#include "Types.hpp"

namespace PredictorSvcs::BidCostPredictor
{

class ModelBidCost : public ModelManager
{
public:
  using TagId = Types::TagId;
  using Url = Types::Url;
  using UrlPtr = Types::UrlPtr;
  using WinRate = Types::WinRate;
  using Cost = Types::Cost;

public:
  explicit ModelBidCost() = default;

  virtual Cost get_cost(
    const TagId& tag_id,
    const Url& url,
    const WinRate& win_rate,
    const Cost& current_cost) const = 0;

  virtual void set_cost(
    const TagId& tag_id,
    const UrlPtr& url,
    const WinRate& win_rate,
    const Cost& cost,
    const Cost& max_cost) = 0;

  virtual void clear() noexcept = 0;

protected:
  ~ModelBidCost() override = default;
};

using ModelBidCost_var = ReferenceCounting::SmartPtr<ModelBidCost>;

} // namespace PredictorSvcs::BidCostPredictor

#endif // BIDCOSTPREDICTOR_MODEL_BIDCOST_HPP
