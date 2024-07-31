#ifndef BIDCOSTPREDICTOR_MODELMANAGER_HPP
#define BIDCOSTPREDICTOR_MODELMANAGER_HPP

// STD
#include <string>

namespace PredictorSvcs::BidCostPredictor
{

class ModelManager : public virtual ReferenceCounting::Interface
{
public:
  ModelManager() = default;

  virtual void load(const std::string& path) = 0;

  virtual void save(const std::string& path) const = 0;

protected:
  virtual ~ModelManager() = default;
};

using ModelManager_var = ReferenceCounting::SmartPtr<ModelManager>;

} // namespace PredictorSvcs::BidCostPredictor

#endif //BIDCOSTPREDICTOR_MODELMANAGER_HPP
