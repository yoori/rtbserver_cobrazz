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
#include <PredictorSvcs/BidCostPredictor/ActiveObjectObserver.hpp>
#include <PredictorSvcs/BidCostPredictor/ModelEvaluator.hpp>
#include <PredictorSvcs/BidCostPredictor/ModelFactory.hpp>
#include <PredictorSvcs/BidCostPredictor/Persantage.hpp>
#include <PredictorSvcs/BidCostPredictor/DataModelProvider.hpp>
#include <PredictorSvcs/BidCostPredictor/ModelEvaluator.hpp>
#include <PredictorSvcs/BidCostPredictor/ShutdownManager.hpp>


namespace PredictorSvcs::BidCostPredictor
{

namespace LogProcessing = AdServer::LogProcessing;

class ModelEvaluatorCtrImpl final :
  public ModelEvaluatorCtr,
  public ReferenceCounting::AtomicImpl
{
private:
  using CreativeCategoryId = Types::CreativeCategoryId;
  using TagId = Types::TagId;
  using Url = Types::Url;
  using Imps = Types::Imps;
  using Clicks = Types::Clicks;
  using FixedNumber = Types::FixedNumber;

public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  explicit ModelEvaluatorCtrImpl(
    const Imps trust_imps,
    const Imps tag_imps,
    DataModelProvider* data_provider,
    ModelCtrFactory* model_factory,
    Logger* logger);

  ModelCtr_var evaluate() noexcept override;

  void stop() noexcept override;

protected:
  ~ModelEvaluatorCtrImpl() override = default;

private:
  void calculate(
    const CtrHelpCollector& collector,
    ModelCtr& model);

private:
  const Imps trust_imps_;

  const Imps tag_imps_;

  const DataModelProvider_var data_provider_;

  const ModelCtrFactory_var model_factory_;

  const Logger_var logger_;

  std::atomic<bool> is_stopped_{false};
};

} // namespace PredictorSvcs::BidCostPredictor

#endif //BIDCOSTPREDICTOR_MODELEVALUATORCTR_HPP