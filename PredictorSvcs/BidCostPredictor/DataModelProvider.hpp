#ifndef BIDCOSTPREDICTOR_DATAMODELPROVIDER_HPP
#define BIDCOSTPREDICTOR_DATAMODELPROVIDER_HPP

// UNIXCOMMONS
#include <ReferenceCounting/Interface.hpp>

// THIS
#include "BidCostHelpCollector.hpp"
#include "CtrHelpCollector.hpp"

namespace PredictorSvcs::BidCostPredictor
{

class DataModelProvider : public virtual ReferenceCounting::Interface
{
public:
  DataModelProvider() = default;

  virtual bool load(BidCostHelpCollector& collector) noexcept = 0;

  virtual bool load(CtrHelpCollector& collector) noexcept = 0;

  virtual void stop() noexcept = 0;

protected:
  virtual ~DataModelProvider() = default;
};

using DataModelProvider_var = ReferenceCounting::SmartPtr<DataModelProvider>;

} // namespace PredictorSvcs::BidCostPredictor

#endif //BIDCOSTPREDICTOR_DATAMODELPROVIDER_HPP
