// THIS
#include "DaemonImpl.hpp"

// POSIX
#include <unistd.h>

// STD
#include <stdio.h>

namespace Aspect
{
const char* DAEMON_IMPL = "DAEMON_IMPL";
}

namespace PredictorSvcs
{
namespace BidCostPredictor
{

DaemonImpl::DaemonImpl(
  const std::string& path_config,
  const std::string& pid_path,
  const std::size_t model_period,
  const std::size_t agg_period,
  const std::size_t reagg_period,
  Logging::Logger* logger)
  : Daemon(pid_path, logger),
    path_config_(path_config),
    model_period_(model_period),
    agg_period_(agg_period),
    reagg_period_(reagg_period),
    path_exe_(get_path_exe()),
    logger_(ReferenceCounting::add_ref(logger)),
    observer_(new ActiveObjectObserver(this))
{
}

DaemonImpl::~DaemonImpl()
{
  observer_->clear_delegate();
  shutdown_manager_.stop();
  wait_logic();
}

void DaemonImpl::start_logic()
{
  is_running_ = true;

  task_runner_ =
    Generics::TaskRunner_var(
      new Generics::TaskRunner(observer_, 1));
  planner_ =
    Generics::Planner_var(
      new Generics::Planner(observer_));

  task_runner_->activate_object();
  planner_->activate_object();

  logger_->info(std::string("DaemonImpl is started"), Aspect::DAEMON_IMPL);

  post_task(&DaemonImpl::do_task_agg, 0);
  post_task(&DaemonImpl::do_task_reagg, 1);
  post_task(&DaemonImpl::do_task_model, 2);
}

void DaemonImpl::stop_logic() noexcept
{
  logger_->info(std::string("Stopping DaemonImpl..."), Aspect::DAEMON_IMPL);

  shutdown_manager_.stop();

  std::lock_guard lock(mutex_process_);
  if (process_)
  {
    process_->stop();
  }
}

void DaemonImpl::wait_logic() noexcept
{
  if (!is_running_)
    return;

  is_running_ = false;

  shutdown_manager_.wait();

  try
  {
    if (planner_)
    {
      planner_->deactivate_object();
      planner_->wait_object();
    }
  }
  catch (...)
  {}

  try
  {
    if (task_runner_)
      task_runner_->wait_for_queue_exhausting();
  }
  catch (...)
  {}

  try
  {
    if (task_runner_)
    {
      task_runner_->deactivate_object();
      task_runner_->wait_object();
    }
  }
  catch (...)
  {}

  task_runner_.reset();
  planner_.reset();

  logger_->info(
    std::string("DaemonImpl is stoped"),
    Aspect::DAEMON_IMPL);
}

void DaemonImpl::do_task_model() noexcept
{
  if (shutdown_manager_.is_stoped())
    return;

  try
  {
    std::vector<std::string> options;
    options.emplace_back("model");
    options.emplace_back(path_config_);

    Process_var process;
    {
      std::lock_guard lock(mutex_process_);
      process = Process_var(new Process(path_exe_, options));
      process_ = process;
    }

    if (shutdown_manager_.is_stoped())
      return;

    process->launch();
    process->wait();

    std::lock_guard lock(mutex_process_);
    process_.reset();
  }
  catch (const eh::Exception& exc)
  {
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << " : Reason : "
           << exc.what();
    logger_->error(stream.str(), Aspect::DAEMON_IMPL);

    std::lock_guard lock(mutex_process_);
    process_.reset();
  }

  post_task(
    &DaemonImpl::do_task_model,
    model_period_);
}

void DaemonImpl::do_task_agg() noexcept
{
  if (shutdown_manager_.is_stoped())
    return;

  try
  {
    std::vector<std::string> options;
    options.emplace_back("aggregate");
    options.emplace_back(path_config_);

    Process_var process;
    {
      std::lock_guard lock(mutex_process_);
      process = Process_var(new Process(path_exe_, options));
      process_ = process;
    }

    if (shutdown_manager_.is_stoped())
      return;

    process->launch();
    process->wait();

    std::lock_guard lock(mutex_process_);
    process_.reset();
  }
  catch (const eh::Exception& exc)
  {
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << " : Reason : "
           << exc.what();
    logger_->error(stream.str(), Aspect::DAEMON_IMPL);

    std::lock_guard lock(mutex_process_);
    process_.reset();
  }

  post_task(
    &DaemonImpl::do_task_agg,
    agg_period_);
}

void DaemonImpl::do_task_reagg() noexcept
{
  if (shutdown_manager_.is_stoped())
    return;

  try
  {
    std::vector<std::string> options;
    options.emplace_back("reaggregate");
    options.emplace_back(path_config_);

    Process_var process;
    {
      std::lock_guard lock(mutex_process_);
      process = Process_var(new Process(path_exe_, options));
      process_ = process;
    }
    process->launch();
    process->wait();

    if (shutdown_manager_.is_stoped())
      return;

    std::lock_guard lock(mutex_process_);
    process_.reset();
  }
  catch (const eh::Exception& exc)
  {
    Stream::Error ostr;
    ostr << __PRETTY_FUNCTION__
         << " : Reason : "
         << exc.what();
    logger_->error(ostr.str(), Aspect::DAEMON_IMPL);

    std::lock_guard lock(mutex_process_);
    process_.reset();
  }

  post_task(
    &DaemonImpl::do_task_reagg,
    reagg_period_);
}

std::string DaemonImpl::get_path_exe()
{
  char arg[20] = {0};
  char exe_path[PATH_MAX + 1] = {0};
  if (sprintf(arg, "/proc/%d/exe", getpid()) < 0)
  {
    Stream::Error ostr;
    ostr << __PRETTY_FUNCTION__
         << " : Reason : "
         << "sprintf is failed";
    throw Exception(ostr);
  }

  if (readlink(arg, exe_path, PATH_MAX) == -1)
  {
    Stream::Error ostr;
    ostr << __PRETTY_FUNCTION__
         << " : Reason : "
         << "readlink is failed";
    throw Exception(ostr);
  }

  return std::string(exe_path);
}

void DaemonImpl::report_error(
  Severity severity,
  const String::SubString& description,
  const char* error_code) noexcept
{
  if (severity == Severity::CRITICAL_ERROR || severity == Severity::ERROR)
  {
    shutdown_manager_.stop();
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << " : DaemonImpl stopped due to incorrect operation of queues."
           << " Reason: "
           << description;
    logger_->critical(stream.str(), Aspect::DAEMON_IMPL, error_code);
  }
}

} // namespace BidCostPredictor
} // namespace PredictorSvcs