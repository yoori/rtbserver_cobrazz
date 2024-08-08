#ifndef BIDCOSTPREDICTOR_MODELFACTORY_HPP
#define BIDCOSTPREDICTOR_MODELFACTORY_HPP

// UNIXCOMMONS
#include <ReferenceCounting/Interface.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

// THIS
#include <PredictorSvcs/BidCostPredictor/ModelCtr.hpp>
#include <PredictorSvcs/BidCostPredictor/ModelBidCost.hpp>

namespace PredictorSvcs::BidCostPredictor
{

template<class Model>
class ModelFactory : public virtual ReferenceCounting::Interface
{
public:
  using Model_var = ReferenceCounting::SmartPtr<Model>;

public:
  ModelFactory() = default;

  virtual Model_var create() = 0;

protected:
  virtual ~ModelFactory() = default;
};

using ModelBidCostFactory = ModelFactory<ModelBidCost>;
using ModelBidCostFactory_var = ReferenceCounting::SmartPtr<ModelBidCostFactory>;

using ModelCtrFactory = ModelFactory<ModelCtr>;
using ModelCtrFactory_var = ReferenceCounting::SmartPtr<ModelCtrFactory>;

} // namespace PredictorSvcs::BidCostPredictor

#endif //BIDCOSTPREDICTOR_MODELFACTORY_HPP