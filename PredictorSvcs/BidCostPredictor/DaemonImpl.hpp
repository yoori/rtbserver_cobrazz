#ifndef BIDCOSTPREDICTOR_DAEMONIMPL_HPP
#define BIDCOSTPREDICTOR_DAEMONIMPL_HPP

// STD
#include <functional>

// THIS
#include <Commons/DelegateTaskGoal.hpp>
#include <Generics/ActiveObject.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include "ActiveObjectObserver.hpp"
#include "Daemon.hpp"
#include "Processor.hpp"
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
public:
  DaemonImpl(
    const std::string& pid_path,
    const std::string& model_dir,
    const std::string& model_file_name,
    const std::string& model_temp_dir,
    const std::string& ctr_model_dir,
    const std::string& ctr_model_file_name,
    const std::string& ctr_model_temp_dir,
    const std::string& model_agg_dir,
    const std::size_t model_period,
    const std::size_t agg_max_process_files,
    const std::size_t agg_dump_max_size,
    const std::string& agg_input_dir,
    const std::string& agg_output_dir,
    const std::size_t agg_period,
    const std::string& reagg_input_dir,
    const std::string& reagg_output_dir,
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
  const std::string model_dir_;

  const std::string model_file_name_;

  const std::string model_temp_dir_;

  const std::string ctr_model_dir_;

  const std::string ctr_model_file_name_;

  const std::string ctr_model_temp_dir_;

  const std::string model_agg_dir_;

  const std::size_t model_period_;

  const std::size_t agg_max_process_files_;

  const std::size_t agg_dump_max_size_;

  const std::string agg_input_dir_;

  const std::string agg_output_dir_;

  const std::size_t agg_period_;

  const std::string reagg_input_dir_;

  const std::string reagg_output_dir_;

  const std::size_t reagg_period_;

  Logging::Logger_var logger_;

  ActiveObjectObserver_var observer_;

  Generics::TaskRunner_var task_runner_;

  Generics::Planner_var planner_;

  Processor_var processor_;

  bool is_processor_stoped_ = false;

  std::mutex mutex_processor_;

  ShutdownManager shutdown_manager_;

  bool is_running_ = false;
};

} // namespace BidCostPredictor
} // namespace PredictorSvcs

#endif //BIDCOSTPREDICTOR_DAEMONIMPL_HPP
