#ifndef BIDCOSTPREDICTOR_MODEL_BIDCOST_HPP
#define BIDCOSTPREDICTOR_MODEL_BIDCOST_HPP

// STD
#include <memory>
#include <string>

// THIS
#include <LogCommons/LogCommons.hpp>
#include <ReferenceCounting/Interface.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include "HelpCollector.hpp"
#include "ModelManager.hpp"

namespace PredictorSvcs
{
namespace BidCostPredictor
{

namespace LogProcessing = AdServer::LogProcessing;
namespace Predictor = PredictorSvcs::BidCostPredictor;

class ModelBidCost :
  public virtual ModelManager,
  public virtual ReferenceCounting::Interface
{
public:
  using Key = typename Predictor::HelpCollector::Key;
  using TagId = typename Key::TagId;
  using Url = typename Key::Url;
  using Url_var = typename Key::Url_var;
  using FixedNumber = LogProcessing::FixedNumber;
  using WinRate = FixedNumber;
  using Cost = FixedNumber;

public:
  ModelBidCost() = default;

  virtual Cost get_cost(
    const TagId& tag_id,
    const Url& url,
    const WinRate& win_rate,
    const Cost& current_cost) const = 0;

  virtual void set_cost(
    const TagId& tag_id,
    const Url_var& url,
    const WinRate& win_rate,
    const Cost& cost,
    const Cost& max_cost) = 0;

  virtual void clear() noexcept = 0;

protected:
  ~ModelBidCost() override = default;
};

using ModelBidCost_var = ReferenceCounting::SmartPtr<ModelBidCost>;

} // namespace BidCostPredictor
} // namespace PredictorSvcs

#endif // BIDCOSTPREDICTOR_MODEL_BIDCOST_HPP
