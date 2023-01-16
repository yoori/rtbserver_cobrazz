#ifndef BIDCOSTPREDICTOR_REMOVER_HPP
#define BIDCOSTPREDICTOR_REMOVER_HPP

namespace PredictorSvcs
{
namespace BidCostPredictor
{

template<class Container, class It>
struct Remover final
{
  Remover(Container& container, It it)
  : container(container),
    it(it)
  {
  }

  ~Remover()
  {
    if (!container.empty())
      container.erase(it);
  }

  Container& container;
  It it;
};

} // namespace PredictorSvcs
} // namespace BidCostPredictor

#endif //BIDCOSTPREDICTOR_REMOVER_HPP
