#ifndef BIDCOSTPREDICTOR_MODELBIDCOSTFACTORYIMPL_HPP
#define BIDCOSTPREDICTOR_MODELBIDCOSTFACTORYIMPL_HPP

// UNIXCOMMONS
#include "Logger/Logger.hpp"

// THIS
#include <PredictorSvcs/BidCostPredictor/ModelFactory.hpp>
#include <PredictorSvcs/BidCostPredictor/ModelBidCostImpl.hpp>

namespace PredictorSvcs::BidCostPredictor
{

class ModelBidCostFactoryImpl final:
  public ModelBidCostFactory,
  public virtual ReferenceCounting::AtomicImpl
{
public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;

public:
  ModelBidCostFactoryImpl(Logger* logger)
    : logger_(ReferenceCounting::add_ref(logger))
  {
  }

  ModelBidCost_var create() override
  {
    return ModelBidCost_var(new ModelBidCostImpl(logger_.in()));
  }

protected:
  ~ModelBidCostFactoryImpl() override = default;

private:
  const Logger_var logger_;
};

} // namespace PredictorSvcs::BidCostPredictor

#endif // BIDCOSTPREDICTOR_MODELBIDCOSTFACTORYIMPL_HPP
