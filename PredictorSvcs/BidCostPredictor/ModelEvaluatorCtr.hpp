#ifndef BIDCOSTPREDICTOR_MODELEVALUATORCTR_HPP
#define BIDCOSTPREDICTOR_MODELEVALUATORCTR_HPP

// STD
#include <atomic>

// UNIXCOMMONS
#include <Generics/ActiveObject.hpp>
#include <Generics/TaskRunner.hpp>
#include <Logger/Logger.hpp>

// THIS
#include <Commons/DelegateTaskGoal.hpp>
#include <LogCommons/LogCommons.hpp>
#include "ActiveObjectObserver.hpp"
#include "ModelEvaluator.hpp"
#include "ModelFactory.hpp"
#include "Persantage.hpp"
#include "DataModelProvider.hpp"
#include "ModelEvaluator.hpp"
#include "ShutdownManager.hpp"


namespace PredictorSvcs::BidCostPredictor
{

namespace LogProcessing = AdServer::LogProcessing;

class ModelEvaluatorCtrImpl final :
  public ModelEvaluatorCtr,
  public ReferenceCounting::AtomicImpl
{
private:
  using TagId = typename ModelCtr::TagId;
  using Url = typename ModelCtr::Url;
  using UrlPtr = typename ModelCtr::UrlPtr;
  using Imps = Types::Imps;
  using Clicks = Types::Clicks;
  using FixedNumber = Types::FixedNumber;

public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  explicit ModelEvaluatorCtrImpl(
    const Imps trust_imps,
    const Imps tag_imps,
    DataModelProvider* data_provider,
    ModelCtrFactory* model_factory,
    Logging::Logger* logger);

  ModelCtr_var evaluate() noexcept override;

  void stop() noexcept override;

protected:
  ~ModelEvaluatorCtrImpl() override = default;

private:
  void calculate(
    const HelpCollector collector,
    ModelCtr& model);

private:
  const Imps trust_imps_;

  const Imps tag_imps_;

  const DataModelProvider_var data_provider_;

  const ModelCtrFactory_var model_factory_;

  const Logging::Logger_var logger_;

  std::atomic<bool> is_stopped_{false};
};

} // namespace PredictorSvcs::BidCostPredictor

#endif //BIDCOSTPREDICTOR_MODELEVALUATORCTR_HPP