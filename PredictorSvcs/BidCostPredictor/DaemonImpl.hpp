#ifndef BIDCOSTPREDICTOR_DAEMONIMPL_HPP
#define BIDCOSTPREDICTOR_DAEMONIMPL_HPP

// STD
#include <functional>
#include <sstream>

// THIS
#include <Commons/DelegateTaskGoal.hpp>
#include <Generics/ActiveObject.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include "ActiveObjectObserver.hpp"
#include "Daemon.hpp"
#include "Process.hpp"
#include "ShutdownManager.hpp"

namespace Aspect
{
extern const char* DAEMON_IMPL;
}

namespace PredictorSvcs
{
namespace BidCostPredictor
{

class DaemonImpl final :
  public Daemon,
  public virtual ReferenceCounting::AtomicImpl,
  private ActiveObjectDelegate
{
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
public:
  DaemonImpl(
    const std::string& path_config,
    const std::string& pid_path,
    const std::size_t model_period,
    const std::size_t agg_period,
    const std::size_t reagg_period,
    Logging::Logger* logger);

  ~DaemonImpl() override;

private:
  void start_logic() override;

  void stop_logic() noexcept override;

  void wait_logic() noexcept override;

  void do_task_model() noexcept;

  void do_task_agg() noexcept;

  void do_task_reagg() noexcept;

  void report_error(
    Severity severity,
    const String::SubString& description,
    const char* error_code) noexcept;

  std::string get_path_exe();

  template<class MemPtr,
          class ...Args>
  std::enable_if_t<
          std::is_member_function_pointer_v<MemPtr>,
          bool>
  post_task(
    MemPtr mem_ptr,
    const std::size_t count_sec,
    Args&& ...args) noexcept
  {
    if (!planner_)
      return false;

    try
    {
      const auto time =
        Generics::Time::get_time_of_day() +
        Generics::Time::ONE_SECOND * count_sec;
      planner_->schedule(
        AdServer::Commons::make_delegate_goal_task(
          std::bind(
            mem_ptr,
            this,
            std::forward<Args>(args)...),
          task_runner_),
        time);
      return true;
    }
    catch (const eh::Exception& exc)
    {
      stop();
      std::stringstream stream;
      stream << __PRETTY_FUNCTION__
             << ": Can't schedule task"
             << " Reason: "
             << exc.what();
      logger_->critical(
        stream.str(),
        Aspect::DAEMON_IMPL);
      return false;
    }
  }

private:
  const std::string path_config_;

  const std::size_t model_period_;

  const std::size_t agg_period_;

  const std::size_t reagg_period_;

  const std::string path_exe_;

  Logging::Logger_var logger_;

  ActiveObjectObserver_var observer_;

  Generics::TaskRunner_var task_runner_;

  Generics::Planner_var planner_;

  Process_var process_;

  std::mutex mutex_process_;

  ShutdownManager shutdown_manager_;

  bool is_running_ = false;
};

} // namespace BidCostPredictor
} // namespace PredictorSvcs

#endif //BIDCOSTPREDICTOR_DAEMONIMPL_HPP
