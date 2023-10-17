#ifndef BIDCOSTPREDICTOR_MODELMANAGER_HPP
#define BIDCOSTPREDICTOR_MODELMANAGER_HPP

// STD
#include <string>

namespace PredictorSvcs
{
namespace BidCostPredictor
{

class ModelManager
{
public:
  ModelManager() = default;

  virtual void load(const std::string& path) = 0;

  virtual void save(const std::string& path) const = 0;

protected:
  virtual ~ModelManager() = default;
};

} // namespace BidCostPredictor
} // namespace PredictorSvcs

#endif //BIDCOSTPREDICTOR_MODELMANAGER_HPP
