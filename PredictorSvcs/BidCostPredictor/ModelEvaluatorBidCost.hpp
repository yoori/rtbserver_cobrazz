#ifndef BIDCOSTPREDICTOR_MODELEVALUATORWIN_HPP
#define BIDCOSTPREDICTOR_MODELEVALUATORWIN_HPP

// STD
#include <vector>

// THIS
#include <Commons/DelegateTaskGoal.hpp>
#include <Generics/ActiveObject.hpp>
#include <Generics/TaskRunner.hpp>
#include <LogCommons/LogCommons.hpp>
#include <Logger/Logger.hpp>
#include "ActiveObjectObserver.hpp"
#include "DataModelProvider.hpp"
#include "ModelEvaluator.hpp"
#include "ModelFactory.hpp"
#include "Persantage.hpp"
#include "ShutdownManager.hpp"
#include "Types.hpp"

namespace Aspect
{
extern const char* MODEL_EVALUATOR_BID_COST;
} // namespace Aspect

namespace PredictorSvcs
{
namespace BidCostPredictor
{

namespace LogProcessing = AdServer::LogProcessing;

class ModelEvaluatorBidCostImpl final:
  public ModelEvaluatorBidCost,
  public virtual ReferenceCounting::AtomicImpl,
  private ActiveObjectDelegate
{
  using Point = Types::FixedNumber;
  using Points = std::vector<Point>;
  using TaskRunner_var = Generics::TaskRunner_var;
  using Iterator = typename HelpCollector::const_iterator;
  using Url_var = typename HelpCollector::Key::Url_var;
  using TagId = typename HelpCollector::Key::TagId;
  using Cost = Types::Cost;
  using FixedNumber = Types::FixedNumber;
  using WinRate = Types::WinRate;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  enum class TaskRunnerID
  {
    Single,
    Pool
  };

public:
  explicit ModelEvaluatorBidCostImpl(
    const Points& points,
    DataModelProvider* data_provider,
    ModelBidCostFactory* model_factory,
    Logging::Logger* logger);

  ~ModelEvaluatorBidCostImpl() override;

  ModelBidCost_var evaluate() noexcept override;

  void stop() noexcept override;

private:
  void start();

  void wait() noexcept;

  void do_init() noexcept;

  void do_calculate(const Iterator it) noexcept;

  void do_calculate_helper(const Iterator it);

  void do_save(
    const TagId& tag_id,
    const Url_var& url,
    const Point& point,
    const Cost& target_cost,
    const Cost& max_cost) noexcept;

  void do_next_task() noexcept;

  void do_decrease() noexcept;

  void report_error(
    Severity severity,
    const String::SubString& description,
    const char* error_code = 0) noexcept override;

  template<class MemPtr,
          class ...Args>
  std::enable_if_t<
    std::is_member_function_pointer_v<MemPtr>,
    bool>
  post_task(
    const TaskRunnerID id,
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
        Aspect::MODEL_EVALUATOR_BID_COST);
      return false;
    }
  }

public:
  static constexpr long TOP_LEVEL_WIN_RATE_MIN_IMPS = 50;

  static constexpr long LEVEL_WIN_RATE_MIN_IMPS = 50;

private:
  Points points_;

  DataModelProvider_var data_provider_;

  ModelBidCostFactory_var model_factory_;

  Logging::Logger_var logger_;

  ActiveObjectObserver_var observer_;

  Persantage persantage_;

  HelpCollector collector_;

  ModelBidCost_var model_;

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

#endif //BIDCOSTPREDICTOR_MODELEVALUATORWIN_HPP
