#ifndef BIDCOSTPREDICTOR_MODELCTR_HPP
#define BIDCOSTPREDICTOR_MODELCTR_HPP

// STD
#include <string>

// THIS
#include <ReferenceCounting/Interface.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include "HelpCollector.hpp"
#include "ModelManager.hpp"

namespace PredictorSvcs
{
namespace BidCostPredictor
{

namespace Predictor = PredictorSvcs::BidCostPredictor;

class ModelCtr :
  public virtual ModelManager,
  public virtual ReferenceCounting::Interface
{
public:
  using TagId = typename Predictor::HelpKey::TagId;
  using Url = typename Predictor::HelpKey::Url;
  using Url_var = typename Predictor::HelpKey::Url_var;
  using Clicks = long;
  using Imps = long;
  using Data = std::pair<Clicks, Imps>;

public:
  ModelCtr() = default;

  virtual Data get_ctr(
    const TagId& tag_id,
    const Url& url) const = 0;

  virtual void set_ctr(
    const TagId& tag_id,
    const Url_var& url,
    const Clicks& clicks,
    const Imps& imps) = 0;

  virtual void clear() noexcept = 0;

protected:
  ~ModelCtr() override = default;
};

using ModelCtr_var = ReferenceCounting::SmartPtr<ModelCtr>;

} // namespace BidCostPredictor
} // namespace PredictorSvcs

#endif //BIDCOSTPREDICTOR_MODELCTR_HPP
