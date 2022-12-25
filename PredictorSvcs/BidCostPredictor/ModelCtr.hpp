#ifndef BIDCOSTPREDICTOR_MODELCTR_HPP
#define BIDCOSTPREDICTOR_MODELCTR_HPP

// STD
#include <string>

// THIS
#include <ReferenceCounting/Interface.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include "CtrCollector.hpp"
#include "ModelManager.hpp"
#include "Types.hpp"

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
  using TagId = Types::TagId;
  using Url = Types::Url;
  using Url_var = Types::Url_var;
  using Ctr = typename LogProcessing::CtrData::Ctr;

public:
  ModelCtr() = default;

  virtual Ctr get_ctr(
    const TagId& tag_id,
    const Url& url) const = 0;

  virtual void set_ctr(
    const TagId& tag_id,
    const Url_var& url,
    const Ctr& ctr) = 0;

  virtual void clear() noexcept = 0;

protected:
  ~ModelCtr() override = default;
};

using ModelCtr_var = ReferenceCounting::SmartPtr<ModelCtr>;

} // namespace BidCostPredictor
} // namespace PredictorSvcs

#endif //BIDCOSTPREDICTOR_MODELCTR_HPP
