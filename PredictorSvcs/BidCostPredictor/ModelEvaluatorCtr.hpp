#ifndef BIDCOSTPREDICTOR_MODELEVALUATORCTR_HPP
#define BIDCOSTPREDICTOR_MODELEVALUATORCTR_HPP

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
        public virtual ReferenceCounting::AtomicImpl,
        private ActiveObjectDelegate
{
  using TaskRunner_var = Generics::TaskRunner_var;
  using Iterator = typename HelpCollector::const_iterator;

  using FixedNumber = LogProcessing::FixedNumber;

  using TagId = typename ModelCtr::TagId;
  using Url = typename ModelCtr::Url;
  using Url_var = typename ModelCtr::Url_var;
  using Clicks = typename ModelCtr::Clicks;
  using Imps = typename ModelCtr::Imps;
  using Data = typename ModelCtr::Data;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  enum class TaskRunnerID
  {
    Single,
    Pool
  };

public:
  explicit ModelEvaluatorCtrImpl(
          const DataModelProvider_var& data_provider,
          const ModelCtrFactory_var& model_factory,
          const Logging::Logger_var& logger);

  ~ModelEvaluatorCtrImpl() override;

  ModelCtr_var evaluate() noexcept override;

  void stop() noexcept override;

private:
  void start();

  void wait() noexcept;

  void doInit() noexcept;

  void doCalculate(const Iterator it) noexcept;

  void doCalculateHelper(const Iterator it);

  void doSave(
          const TagId& tag_id,
          const Url_var& url,
          const Clicks& all_clicks,
          const Imps& all_imps) noexcept;

  void doNextTask() noexcept;

  void report_error(
          Severity severity,
          const String::SubString& description,
          const char* error_code = 0) noexcept override;

  template<class MemPtr,
          class ...Args>
  std::enable_if_t<
          std::is_member_function_pointer_v<MemPtr>,
          bool>
  postTask(const TaskRunnerID id,
           MemPtr mem_ptr,
           Args&& ...args) noexcept
  {
    try
    {
      TaskRunner_var task_runner;
      if (id == TaskRunnerID::Single)
      {
        task_runner = task_runner_;
      }
      else
      {
        task_runner = task_runner_pool_;
      }
      task_runner->enqueue_task(
              AdServer::Commons::make_delegate_task(
                      std::bind(mem_ptr,
                                this,
                                std::forward<Args>(args)...)));
      return true;
    }
    catch (const eh::Exception& exc)
    {
      shutdown_manager_.stop();
      std::stringstream stream;
      stream << __PRETTY_FUNCTION__
             << ": Can't enqueue_task"
             << " Reason: "
             << exc.what();
      logger_->critical(
              stream.str(),
              Aspect::MODEL_EVALUATOR_CTR);
      return false;
    }
  }

private:
  DataModelProvider_var data_provider_;

  ModelCtrFactory_var model_factory_;

  Logging::Logger_var logger_;

  ActiveObjectObserver_var observer_;

  Persantage persantage_;

  HelpCollector collector_;

  ModelCtr_var model_;

  bool is_running_ = false;

  TaskRunner_var task_runner_;

  unsigned int threads_number_ = 0;

  TaskRunner_var task_runner_pool_;
  // Single thread
  std::size_t remaining_iterations_ = 0;
  // Single thread
  Iterator iterator_;

  ShutdownManager shutdown_manager_;

  bool is_success_ = false;
};

} // namespace BidCostPredictor
} // namespace PredictorSvcs

#endif //BIDCOSTPREDICTOR_MODELEVALUATORCTR_HPP
