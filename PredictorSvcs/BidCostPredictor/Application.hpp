#ifndef RTBSERVER_COBRAZZ_BIDCOSTPREDICTOR_APPLICATION_HPP
#define RTBSERVER_COBRAZZ_BIDCOSTPREDICTOR_APPLICATION_HPP

// THIS
#include <eh/Exception.hpp>
#include <Generics/Uncopyable.hpp>

namespace PredictorSvcs
{
namespace BidCostPredictor
{

class Application final : private Generics::Uncopyable
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  Application() noexcept = default;

  ~Application() = default;

  int run(int argc, char **argv);
};

} // namespace BidCostPredictor
} // namespace PredictorSvcs

#endif //RTBSERVER_COBRAZZ_BIDCOSTPREDICTOR_APPLICATION_HPP
