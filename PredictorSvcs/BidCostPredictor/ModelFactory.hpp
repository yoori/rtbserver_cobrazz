#ifndef BIDCOSTPREDICTOR_MODELFACTORY_HPP
#define BIDCOSTPREDICTOR_MODELFACTORY_HPP

// THIS
#include <ReferenceCounting/Interface.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include "ModelCtr.hpp"
#include "ModelBidCost.hpp"

namespace PredictorSvcs
{
namespace BidCostPredictor
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

} // namespace BidCostPredictor
} // namespace PredictorSvcs

#endif //BIDCOSTPREDICTOR_MODELFACTORY_HPP