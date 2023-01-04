#ifndef BIDCOSTPREDICTOR_MODELCTRFACTORY_HPP
#define BIDCOSTPREDICTOR_MODELCTRFACTORY_HPP

// THIS
#include "ModelCtrImpl.hpp"
#include "ModelFactory.hpp"

namespace PredictorSvcs
{
namespace BidCostPredictor
{

class ModelCtrFactoryImpl :
        public ModelCtrFactory,
        public virtual ReferenceCounting::AtomicImpl
{
public:
  ModelCtrFactoryImpl(Logging::Logger_var& logger)
                     : logger_(logger)
  {
  }

  ModelCtr_var create() override
  {
    return ModelCtr_var(new ModelCtrImpl(logger_));
  }

  ~ModelCtrFactoryImpl() override = default;

private:
  Logging::Logger_var& logger_;
};

} // namespace BidCostPredictor
} // namespace PredictorSvcs

#endif //BIDCOSTPREDICTOR_MODELCTRFACTORY_HPP
