#ifndef BIDCOSTPREDICTOR_MODELEVALUATORCTR_HPP
#define BIDCOSTPREDICTOR_MODELEVALUATORCTR_HPP

// STD
#include <atomic>

// THIS
#include <Commons/DelegateTaskGoal.hpp>
#include <Generics/ActiveObject.hpp>
#include <Generics/TaskRunner.hpp>
#include <LogCommons/LogCommons.hpp>
#include <Logger/Logger.hpp>
#include "ActiveObjectObserver.hpp"
#include "ModelEvaluator.hpp"
#include "ModelFactory.hpp"
#include "Persantage.hpp"
#include "DataModelProvider.hpp"
#include "ModelEvaluator.hpp"
#include "ShutdownManager.hpp"

namespace Aspect
{
extern const char* MODEL_EVALUATOR_CTR;
} // namespace Aspect

namespace PredictorSvcs
{
namespace BidCostPredictor
{

namespace LogProcessing = AdServer::LogProcessing;

class ModelEvaluatorCtrImpl final:
  public ModelEvaluatorCtr,
  public virtual ReferenceCounting::AtomicImpl
{
  using Iterator = typename HelpCollector::const_iterator;
  using TagId = typename ModelCtr::TagId;
  using Url = typename ModelCtr::Url;
  using Url_var = typename ModelCtr::Url_var;
  using Imps = Types::Imps;
  using Clicks = Types::Clicks;
  using FixedNumber = Types::FixedNumber;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  explicit ModelEvaluatorCtrImpl(
    const Imps trust_imps,
    const Imps tag_imps,
    DataModelProvider* data_provider,
    ModelCtrFactory* model_factory,
    Logging::Logger* logger);

  ~ModelEvaluatorCtrImpl() override;

  ModelCtr_var evaluate() noexcept override;

  void stop() noexcept override;

private:
  void calculate();

private:
  const Imps trust_imps_;

  const Imps tag_imps_;

  DataModelProvider_var data_provider_;

  ModelCtrFactory_var model_factory_;

  Logging::Logger_var logger_;

  HelpCollector collector_;

  ModelCtr_var model_;

  bool is_success_ = false;

  std::atomic<bool> is_stopped_{false};
};

} // namespace BidCostPredictor
} // namespace PredictorSvcs

#endif //BIDCOSTPREDICTOR_MODELEVALUATORCTR_HPP
