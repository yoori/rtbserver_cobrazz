#ifndef BIDCOSTPREDICTOR_PROCESSOR_HPP
#define BIDCOSTPREDICTOR_PROCESSOR_HPP

// UNIXCOMMONS
#include <Generics/CompositeActiveObject.hpp>
#include <ReferenceCounting/Interface.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

namespace PredictorSvcs::BidCostPredictor
{

class Processor : public Generics::CompositeSetActiveObject
{
public:
  Processor() = default;

  virtual std::string name() noexcept = 0;

protected:
  virtual ~Processor() = default;
};

using Processor_var = ReferenceCounting::SmartPtr<Processor>;

} // namespace PredictorSvcs::BidCostPredictor

#endif //BIDCOSTPREDICTOR_PROCESSOR_HPP
