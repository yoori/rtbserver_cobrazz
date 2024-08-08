#ifndef BIDCOSTPREDICTOR_APPLICATION_HPP
#define BIDCOSTPREDICTOR_APPLICATION_HPP

// UNIXCOMMSON
#include <eh/Exception.hpp>
#include <Generics/Uncopyable.hpp>

// THIS
#include "Types.hpp"

namespace PredictorSvcs::BidCostPredictor
{

class Application final : private Generics::Uncopyable
{
private:
  using Imps = Types::Imps;

public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  Application() noexcept = default;

  ~Application() = default;

  int run(int argc, char **argv);
};

} // namespace PredictorSvcs::BidCostPredictor

#endif //BIDCOSTPREDICTOR_APPLICATION_HPP
