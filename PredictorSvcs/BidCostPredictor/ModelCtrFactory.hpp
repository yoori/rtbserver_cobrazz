#ifndef BIDCOSTPREDICTOR_MODELCTRFACTORY_HPP
#define BIDCOSTPREDICTOR_MODELCTRFACTORY_HPP

// THIS
#include <PredictorSvcs/BidCostPredictor/ModelCtrImpl.hpp>
#include <PredictorSvcs/BidCostPredictor/ModelFactory.hpp>

namespace PredictorSvcs::BidCostPredictor
{

class ModelCtrFactoryImpl :
  public ModelCtrFactory,
  public ReferenceCounting::AtomicImpl
{
public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;

public:
  explicit ModelCtrFactoryImpl(Logger* logger)
    : logger_(ReferenceCounting::add_ref(logger))
  {
  }

  ModelCtr_var create() override
  {
    return ModelCtr_var(new ModelCtrImpl(logger_));
  }

protected:
  ~ModelCtrFactoryImpl() override = default;

private:
  Logging::Logger_var logger_;
};

} // namespace PredictorSvcs::BidCostPredictor

#endif //BIDCOSTPREDICTOR_MODELCTRFACTORY_HPP
