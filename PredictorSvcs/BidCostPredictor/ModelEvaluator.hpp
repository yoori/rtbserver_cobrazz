#ifndef BIDCOSTPREDICTOR_MODELEVALUATOR_HPP
#define BIDCOSTPREDICTOR_MODELEVALUATOR_HPP

// UNIXCOMMONS
#include <ReferenceCounting/Interface.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

// THIS
#include <PredictorSvcs/BidCostPredictor/ModelBidCost.hpp>
#include <PredictorSvcs/BidCostPredictor/ModelCtr.hpp>

namespace PredictorSvcs::BidCostPredictor
{

template<class Model>
class ModelEvaluator : public virtual ReferenceCounting::Interface
{
public:
  using Model_var = ReferenceCounting::SmartPtr<Model>;

public:
  ModelEvaluator() = default;

  virtual Model_var evaluate() noexcept = 0;

  virtual void stop() noexcept = 0;

protected:
  virtual ~ModelEvaluator() = default;
};

using ModelEvaluatorBidCost = ModelEvaluator<ModelBidCost>;
using ModelEvaluatorBidCost_var = ReferenceCounting::SmartPtr<ModelEvaluatorBidCost>;

using ModelEvaluatorCtr = ModelEvaluator<ModelCtr>;
using ModelEvaluatorCtr_var = ReferenceCounting::SmartPtr<ModelEvaluatorCtr>;

} // namespace PredictorSvcs::BidCostPredictor

#endif //BIDCOSTPREDICTOR_MODELEVALUATOR_HPP
