#ifndef BIDCOSTPREDICTOR_MODELBIDCOSTFACTORYIMPL_HPP
#define BIDCOSTPREDICTOR_MODELBIDCOSTFACTORYIMPL_HPP

// THIS
#include "Logger/Logger.hpp"
#include "ModelFactory.hpp"
#include "ModelBidCostImpl.hpp"

namespace PredictorSvcs
{
namespace BidCostPredictor
{

class ModelBidCostFactoryImpl :
        public ModelBidCostFactory,
        public virtual ReferenceCounting::AtomicImpl
{
public:
  ModelBidCostFactoryImpl(Logging::Logger_var& logger)
                         : logger_(logger)
  {
  }

  ModelBidCost_var create() override
  {
    return ModelBidCost_var(new ModelBidCostImpl(logger_));
  }

  ~ModelBidCostFactoryImpl() override = default;

private:
  Logging::Logger_var& logger_;
};

} // namespace BidCostPredictor
} // namespace PredictorSvcs

#endif // BIDCOSTPREDICTOR_MODELBIDCOSTFACTORYIMPL_HPP
