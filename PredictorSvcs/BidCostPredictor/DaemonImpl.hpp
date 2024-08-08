#ifndef BIDCOSTPREDICTOR_DAEMONIMPL_HPP
#define BIDCOSTPREDICTOR_DAEMONIMPL_HPP

// STD
#include <functional>
#include <sstream>

// UNIXCOMMONS
#include <Generics/ActiveObject.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>

// THIS
#include <Commons/DelegateTaskGoal.hpp>
#include <PredictorSvcs/BidCostPredictor/ActiveObjectObserver.hpp>
#include <PredictorSvcs/BidCostPredictor/Daemon.hpp>
#include <PredictorSvcs/BidCostPredictor/Process.hpp>
#include <PredictorSvcs/BidCostPredictor/ShutdownManager.hpp>
#include <PredictorSvcs/BidCostPredictor/Utils.hpp>

namespace PredictorSvcs::BidCostPredictor
{

class DaemonImpl final :
  public Daemon,
  public ReferenceCounting::AtomicImpl
{
private:
  using TaskRunner_var = Generics::TaskRunner_var;
  using Planner_var = Generics::Planner_var;

public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  DaemonImpl(
    const std::string& path_config,
    const std::string& pid_path,
    const std::size_t model_period,
    const std::size_t agg_period,
    const std::size_t reagg_period,
    Logger* logger);

protected:
  ~DaemonImpl() override = default;

private:
  void start_logic() override;

  void stop_logic() noexcept override;

  void wait_logic() noexcept override;

  void do_task_model() noexcept;

  void do_task_agg() noexcept;

  void do_task_reagg() noexcept;

  std::string get_path_exe();

  template<ConceptMemberPtr MemPtr, class ...Args>
  bool post_task(
    MemPtr mem_ptr,
    const std::size_t count_sec,
    Args&& ...args) noexcept;

private:
  const std::string path_config_;

  const std::size_t model_period_;

  const std::size_t agg_period_;

  const std::size_t reagg_period_;

  const std::string path_exe_;

  const Logger_var logger_;

  ActiveObjectObserver_var observer_;

  std::mutex mutex_process_;

  Process_var process_;

  ShutdownManager shutdown_manager_;

  TaskRunner_var task_runner_;

  Planner_var planner_;
};

} // namespace PredictorSvcs::BidCostPredictor

#endif //BIDCOSTPREDICTOR_DAEMONIMPL_HPP
