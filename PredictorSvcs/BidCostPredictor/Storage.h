#ifndef BIDCOSTPREDICTOR_STORAGE_H
#define BIDCOSTPREDICTOR_STORAGE_H

// STD
#include <shared_mutex>

// THIS
#include <Commons/DelegateTaskGoal.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <ReferenceCounting/Interface.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include "ActiveObjectObserver.hpp"
#include "Container.h"
#include "Processor.hpp"
#include "ShutdownManager.hpp"

namespace Aspect
{
extern const char* STORAGE;
}

namespace PredictorSvcs
{
namespace BidCostPredictor
{

class Storage :
  public Container,
  public Processor,
  private ActiveObjectDelegate,
  public virtual ReferenceCounting::AtomicImpl
{
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
public:
  Storage(
    Logging::Logger* logger,
    const std::string& model_dir,
    const std::string& ctr_model_dir,
    const std::size_t update_period);

  ~Storage() override;

  Cost get_cost(
    const TagId& tag_id,
    const Url& url,
    const WinRate& win_rate,
    const Cost& current_cost) const override;

  Data get_ctr(
    const TagId& tag_id,
    const Url& url) const override;

  void start() override;

  void stop() noexcept override;

  void wait() noexcept override;

  const char* name() noexcept override;

private:
  void report_error(
    Severity severity,
    const String::SubString& description,
    const char* error_code) noexcept;

  void do_update() noexcept;

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
    if (!planner_ || !task_runner_)
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
      std::stringstream stream;
      stream << __PRETTY_FUNCTION__
             << ": Can't schedule task"
             << " Reason: "
             << exc.what();
      logger_->critical(
              stream.str(),
              Aspect::STORAGE);
      stop();
      return false;
    }
  }

private:
  Logging::Logger_var logger_;

  const std::string model_dir_;

  const std::string ctr_model_dir_;

  const std::size_t update_period_;

  Container_var container_;

  mutable std::shared_mutex shared_mutex_;

  ActiveObjectObserver_var observer_;

  Generics::TaskRunner_var task_runner_;

  Generics::Planner_var planner_;

  ShutdownManager shutdown_manager_;

  bool is_running_ = false;
};

using Storage_var = ReferenceCounting::SmartPtr<Storage>;

} // namespace BidCostPredictor
} // namespace PredictorSvcs

#endif //BIDCOSTPREDICTOR_STORAGE_H
