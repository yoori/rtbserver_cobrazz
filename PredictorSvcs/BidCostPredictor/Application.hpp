#ifndef BIDCOSTPREDICTOR_APPLICATION_HPP
#define BIDCOSTPREDICTOR_APPLICATION_HPP

// THIS
#include <eh/Exception.hpp>
#include <Generics/Uncopyable.hpp>
#include "Types.hpp"

namespace PredictorSvcs
{
namespace BidCostPredictor
{

class Application final : private Generics::Uncopyable
{
  using Imps = Types::Imps;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
public:

  Application() noexcept = default;

  ~Application() = default;

  int run(int argc, char **argv);
};

} // namespace BidCostPredictor
} // namespace PredictorSvcs

#endif //BIDCOSTPREDICTOR_APPLICATION_HPP
