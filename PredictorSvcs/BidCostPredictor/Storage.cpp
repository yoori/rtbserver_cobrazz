// THIS
#include "ContainerImpl.h"
#include "Storage.h"

namespace Aspect
{
const char* STORAGE = "STORAGE";
}

namespace PredictorSvcs
{
namespace BidCostPredictor
{

Storage::Storage(
        const Logging::Logger_var& logger,
        const std::string& model_dir,
        const std::string& ctr_model_dir,
        const std::size_t update_period)
        : logger_(logger),
          model_dir_(model_dir),
          ctr_model_dir_(ctr_model_dir),
          update_period_(update_period),
          container_(new ContainerImpl(logger_, model_dir, ctr_model_dir)),
          observer_(new ActiveObjectObserver(this))
{
}

Storage::~Storage()
{
  observer_->clearDelegate();
  shutdown_manager_.stop();
  wait();
}

Storage::Cost Storage::getCost(
        const TagId& tag_id,
        const Url& url,
        const WinRate& win_rate,
        const Cost& current_cost) const
{
  if (!container_)
    throw Exception("Contianer is null");

  std::shared_lock lock(shared_mutex_);
  Container_var container = container_;
  lock.unlock();

  return container->getCost(
          tag_id,
          url,
          win_rate,
          current_cost);
}

Storage::Data Storage::getCtr(
        const TagId& tag_id,
        const Url& url) const
{
  if (!container_)
    throw Exception("Contianer is null");

  std::shared_lock lock(shared_mutex_);
  Container_var container = container_;
  lock.unlock();

  return container->getCtr(tag_id, url);
}

void Storage::start()
{
  is_running_ = true;

  task_runner_ = Generics::TaskRunner_var(
          new Generics::TaskRunner(
                  observer_,
                  1));
  planner_ = Generics::Planner_var(new Generics::Planner(observer_));

  task_runner_->activate_object();
  planner_->activate_object();

  postTask(&Storage::doUpdate, update_period_);

  logger_->info(std::string("Storage is started"), Aspect::STORAGE);
}

void Storage::stop() noexcept
{
  shutdown_manager_.stop();
  logger_->info(
          std::string("Storage was interrupted"),
          Aspect::STORAGE);
}

void Storage::wait() noexcept
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
          std::string("Storage is stoped"),
          Aspect::STORAGE);
}

const char* Storage::name() noexcept
{
  return "Storage";
}

void Storage::doUpdate() noexcept
{
  if (shutdown_manager_.isStoped())
    return;

  try
  {
    logger_->info(
            std::string("Start updating container"),
            Aspect::STORAGE);

    Container_var temp_container(
            new ContainerImpl(
                    logger_,
                    model_dir_,
                    ctr_model_dir_));

    std::unique_lock lock(shared_mutex_);
    container_.swap(temp_container);
    lock.unlock();

    logger_->info(
            std::string("Container is updated"),
            Aspect::STORAGE);
  }
  catch (const Exception& exc)
  {
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << " Can't update container reason : "
           << exc.what();
    logger_->error(stream.str(), Aspect::STORAGE);
  }

  postTask(&Storage::doUpdate, update_period_);
}

void Storage::report_error(
        Severity severity,
        const String::SubString& description,
        const char* error_code) noexcept
{
  if (severity == Severity::CRITICAL_ERROR || severity == Severity::ERROR)
  {
    shutdown_manager_.stop();
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << " : Storage stopped due to incorrect operation of queues."
           << " Reason: "
           << description;
    logger_->critical(stream.str(), Aspect::STORAGE, error_code);
  }
}

} // namespace BidCostPredictor
} // namespace PredictorSvcs