#ifndef RTBSERVER_COBRAZZ_BIDCOSTPREDICTOR_PROCESSOR_HPP
#define RTBSERVER_COBRAZZ_BIDCOSTPREDICTOR_PROCESSOR_HPP

// THIS
#include <ReferenceCounting/Interface.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

namespace PredictorSvcs
{
namespace BidCostPredictor
{

class Processor : public virtual ReferenceCounting::Interface
{
public:
  Processor() = default;

  virtual void start() = 0;

  virtual void stop() noexcept = 0;

  virtual void wait() noexcept = 0;

  virtual const char* name() noexcept = 0;

protected:
  virtual ~Processor() = default;
};

using ProcessorPtr = ReferenceCounting::SmartPtr<Processor>;

} // namespace BidCostPredictor
} // namespace PredictorSvcs

#endif //RTBSERVER_COBRAZZ_BIDCOSTPREDICTOR_PROCESSOR_HPP
