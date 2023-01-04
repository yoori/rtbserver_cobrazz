#ifndef BIDCOSTPREDICTOR_DATAMODELPROVIDER_HPP
#define BIDCOSTPREDICTOR_DATAMODELPROVIDER_HPP

// THIS
#include <ReferenceCounting/Interface.hpp>
#include "HelpCollector.hpp"

namespace PredictorSvcs
{
namespace BidCostPredictor
{

class DataModelProvider :
        public virtual ReferenceCounting::Interface
{
public:
  DataModelProvider() = default;

  virtual bool load(HelpCollector& collector) noexcept = 0;

  virtual void stop() noexcept = 0;

protected:
  virtual ~DataModelProvider() = default;
};

using DataModelProvider_var = ReferenceCounting::SmartPtr<DataModelProvider>;

} // namespace BidCostPredictor
} // namespace PredictorSvcs

#endif //BIDCOSTPREDICTOR_DATAMODELPROVIDER_HPP
