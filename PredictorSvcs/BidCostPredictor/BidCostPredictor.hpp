#ifndef RTBSERVER_BIDCOSTPREDICTOR_HPP
#define RTBSERVER_BIDCOSTPREDICTOR_HPP

// STD
#include <shared_mutex>

// UNIXCOMMONS
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <Logger/Logger.hpp>
#include <ReferenceCounting/Interface.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

// THIS
#include <PredictorSvcs/BidCostPredictor/ModelBidCost.hpp>
#include <PredictorSvcs/BidCostPredictor/Types.hpp>

namespace PredictorSvcs::BidCostPredictor
{

class BidCostPredictor final
  :  public Generics::CompositeActiveObject,
     public ReferenceCounting::AtomicImpl
{
private:
  using Planner = Generics::Planner;
  using Planner_var = Generics::Planner_var;
  using TaskRunner = Generics::TaskRunner;
  using TaskRunner_var = Generics::TaskRunner_var;
  using SharedMutex = std::shared_mutex;

public:
  using Logger_var = Logging::Logger_var;
  using Logger = Logging::Logger;
  using ActiveObjectCallback = Generics::ActiveObjectCallback;
  using Ctr = Types::Ctr;
  using TagId = Types::TagId;
  using Url = Types::Url;
  using WinRate = Types::WinRate;
  using Cost = Types::Cost;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  explicit BidCostPredictor(
    Logger* logger,
    ActiveObjectCallback* callback,
    const std::string& bid_cost_model_dir,
    const std::size_t update_period_sec);

  Cost predict(
    const TagId& tag_id,
    const Url& url,
    const WinRate& win_rate,
    const Cost& current_cost);

protected:
  ~BidCostPredictor() override = default;

private:
  void create_scheduler_task(const std::size_t period) noexcept;

  void update() noexcept;

  std::string get_last_file(const std::string& path_dir);

private:
  const Logger_var logger_;

  const std::string bid_cost_model_dir_;

  const std::size_t update_period_sec_;

  mutable SharedMutex shared_mutex_;

  ModelBidCost_var model_bid_cost_;

  Planner_var scheduler_;

  TaskRunner_var task_runner_;
};

} // namespace PredictorSvcs::BidCostPredictor

#endif //RTBSERVER_BIDCOSTPREDICTOR_HPP
