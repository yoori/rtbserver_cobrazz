#ifndef BIDCOSTPREDICTOR_MODELEVALUATORWIN_HPP
#define BIDCOSTPREDICTOR_MODELEVALUATORWIN_HPP

// STD
#include <vector>

// UNIXCOMMONS
#include <Generics/ActiveObject.hpp>
#include <Generics/TaskRunner.hpp>
#include <Logger/Logger.hpp>

// THIS
#include <Commons/DelegateTaskGoal.hpp>
#include <LogCommons/LogCommons.hpp>
#include <PredictorSvcs/BidCostPredictor/ActiveObjectObserver.hpp>
#include <PredictorSvcs/BidCostPredictor/DataModelProvider.hpp>
#include <PredictorSvcs/BidCostPredictor/ModelEvaluator.hpp>
#include <PredictorSvcs/BidCostPredictor/ModelFactory.hpp>
#include <PredictorSvcs/BidCostPredictor/Persantage.hpp>
#include <PredictorSvcs/BidCostPredictor/ShutdownManager.hpp>
#include <PredictorSvcs/BidCostPredictor/Types.hpp>

namespace PredictorSvcs::BidCostPredictor
{

namespace LogProcessing = AdServer::LogProcessing;

class ModelEvaluatorBidCostImpl final:
  public ModelEvaluatorBidCost,
  public ReferenceCounting::AtomicImpl
{
private:
  using Point = Types::FixedNumber;
  using Points = std::vector<Point>;
  using TaskRunner_var = Generics::TaskRunner_var;
  using Iterator = BidCostHelpCollector::ConstIterator;
  using UrlPtr = BidCostHelpCollector::Key::UrlPtr;
  using TagId = BidCostHelpCollector::Key::TagId;
  using Cost = Types::Cost;
  using FixedNumber = Types::FixedNumber;
  using WinRate = Types::WinRate;

  enum class TaskRunnerID
  {
    Single,
    Pool
  };

public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  explicit ModelEvaluatorBidCostImpl(
    const Points& points,
    DataModelProvider* data_provider,
    ModelBidCostFactory* model_factory,
    Logger* logger);

  ModelBidCost_var evaluate() noexcept override;

  void stop() noexcept override;

protected:
  ~ModelEvaluatorBidCostImpl() override;

private:
  template<ConceptMemberPtr MemPtr, class ...Args>
  bool post_task(
    const TaskRunnerID id,
    MemPtr mem_ptr,
    Args&& ...args) noexcept;

  Points sort_points(const Points& points);

  void run();

  bool wait() noexcept;

  void clear() noexcept;

  void do_init() noexcept;

  void do_calculate(const Iterator collector_it) noexcept;

  void do_calculate_helper(const Iterator collector_it);

  void do_save(
    const TagId& tag_id,
    const UrlPtr& url,
    const Point& point,
    const Cost& target_cost,
    const Cost& max_cost) noexcept;

  void do_next_task() noexcept;

  void do_decrease() noexcept;

public:
  static constexpr Types::Imps TOP_LEVEL_WIN_RATE_MIN_IMPS = 50;

  static constexpr Types::Imps LEVEL_WIN_RATE_MIN_IMPS = 50;

private:
  const Points points_;

  const DataModelProvider_var data_provider_;

  const ModelBidCostFactory_var model_factory_;

  const Logger_var logger_;

  Persantage persantage_;

  BidCostHelpCollector collector_;

  std::atomic<bool> is_idle_ = true;

  std::atomic<bool> is_evaluation_success_ = false;

  ShutdownManager shutdown_manager_;

  ActiveObjectObserver_var observer_;

  ModelBidCost_var model_;
  // Call on 'Single' thread
  std::size_t remaining_iterations_ = 0;
  // Call on 'Single' thread
  Iterator collector_iterator_;

  TaskRunner_var task_runner_;

  unsigned int threads_number_ = 0;

  TaskRunner_var task_runner_pool_;
};

} // namespace BidCostPredictor::PredictorSvcs

#endif //BIDCOSTPREDICTOR_MODELEVALUATORWIN_HPP
