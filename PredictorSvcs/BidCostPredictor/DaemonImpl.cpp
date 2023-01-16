// THIS
#include "AggregatorMultyThread.hpp"
#include "DaemonImpl.hpp"
#include "ModelProcessor.hpp"
#include "ReaggregatorMultyThread.hpp"

namespace Aspect
{
const char* DAEMON_IMPL = "DAEMON_IMPL";
}

namespace PredictorSvcs
{
namespace BidCostPredictor
{

DaemonImpl::DaemonImpl(
  const std::string& pid_path,
  const std::string& bid_cost_model_dir,
  const std::string& bid_cost_model_file_name,
  const std::string& bid_cost_model_temp_dir,
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
  Logging::Logger* logger)
  : Daemon(pid_path, logger),
    bid_cost_model_dir_(bid_cost_model_dir),
    bid_cost_model_file_name_(bid_cost_model_file_name),
    bid_cost_model_temp_dir_(bid_cost_model_temp_dir),
    ctr_model_dir_(ctr_model_dir),
    ctr_model_file_name_(ctr_model_file_name),
    ctr_model_temp_dir_(ctr_model_temp_dir),
    model_agg_dir_(model_agg_dir),
    model_period_(model_period),
    agg_max_process_files_(agg_max_process_files),
    agg_dump_max_size_(agg_dump_max_size),
    agg_input_dir_(agg_input_dir),
    agg_output_dir_(agg_output_dir),
    agg_period_(agg_period),
    reagg_input_dir_(reagg_input_dir),
    reagg_output_dir_(reagg_output_dir),
    reagg_period_(reagg_period),
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
  post_task(&DaemonImpl::do_task_reagg, 0);
  post_task(&DaemonImpl::do_task_model, 0);
}

void DaemonImpl::stop_logic() noexcept
{
  logger_->info(std::string("Stopping DaemonImpl..."), Aspect::DAEMON_IMPL);

  shutdown_manager_.stop();
  {
    std::unique_lock lock(mutex_processor_);
    auto processor = processor_;
    is_processor_stoped_ = true;
    lock.unlock();

    if (processor)
    {
      processor->stop();
      processor->wait();
    }
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
    Processor_var processor(
      new ModelProcessor(
        bid_cost_model_dir_,
        bid_cost_model_file_name_,
        bid_cost_model_temp_dir_,
        ctr_model_dir_,
        ctr_model_file_name_,
        ctr_model_temp_dir_,
        model_agg_dir_,
        logger_));
    {
      std::lock_guard lock(mutex_processor_);
      if (is_processor_stoped_)
        return;
      processor_ = processor;
    }

    processor->start();
    processor->wait();

    {
      std::lock_guard lock(mutex_processor_);
      processor_.reset();
      if (is_processor_stoped_)
        return;
    }
  }
  catch (const eh::Exception& exc)
  {
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << " : Reason : "
           << exc.what();
    logger_->error(stream.str(), Aspect::DAEMON_IMPL);
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
    Processor_var processor(
      new AggregatorMultyThread(
        agg_max_process_files_,
        agg_dump_max_size_,
        agg_input_dir_,
        agg_output_dir_,
        logger_));
    {
      std::lock_guard lock(mutex_processor_);
      if (is_processor_stoped_)
        return;
      processor_ = processor;
    }

    processor->start();
    processor->wait();

    {
      std::lock_guard lock(mutex_processor_);
      processor_.reset();
      if (is_processor_stoped_)
        return;
    }
  }
  catch (const eh::Exception& exc)
  {
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << " : Reason : "
           << exc.what();
    logger_->error(stream.str(), Aspect::DAEMON_IMPL);
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
    Processor_var processor(
      new ReaggregatorMultyThread(
        reagg_input_dir_,
        reagg_output_dir_,
        logger_));
    {
      std::lock_guard lock(mutex_processor_);
      if (is_processor_stoped_)
        return;
      processor_ = processor;
    }

    processor->start();
    processor->wait();

    {
      std::lock_guard lock(mutex_processor_);
      processor_.reset();
      if (is_processor_stoped_)
        return;
    }
  }
  catch (const eh::Exception& exc)
  {
    Stream::Error ostr;
    ostr << __PRETTY_FUNCTION__
         << " : Reason : "
         << exc.what();
    logger_->error(ostr.str(), Aspect::DAEMON_IMPL);
  }

  post_task(
    &DaemonImpl::do_task_reagg,
    reagg_period_);
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
    logger_->critical(stream.str(), Aspect::AGGREGATOR, error_code);
  }
}

} // namespace BidCostPredictor
} // namespace PredictorSvcs