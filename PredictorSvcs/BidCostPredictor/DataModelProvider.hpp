#ifndef BIDCOSTPREDICTOR_DATAMODELPROVIDER_HPP
#define BIDCOSTPREDICTOR_DATAMODELPROVIDER_HPP

// UNIXCOMMONS
#include <ReferenceCounting/Interface.hpp>

// THIS
#include "HelpCollector.hpp"

namespace PredictorSvcs::BidCostPredictor
{

class DataModelProvider : public virtual ReferenceCounting::Interface
{
public:
  DataModelProvider() = default;

  virtual bool load(HelpCollector& collector) noexcept = 0;

  virtual void stop() noexcept = 0;

protected:
  virtual ~DataModelProvider() = default;
};

using DataModelProvider_var = ReferenceCounting::SmartPtr<DataModelProvider>;

} // namespace PredictorSvcs::BidCostPredictor

#endif //BIDCOSTPREDICTOR_DATAMODELPROVIDER_HPP
