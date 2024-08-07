// THIS
#include "ContainerImpl.h"
#include "Storage.h"

namespace Aspect
{

inline constexpr char STORAGE[] = "STORAGE";

} // namespace Aspect

namespace PredictorSvcs::BidCostPredictor
{

Storage::Storage(
  Logging::Logger* logger,
  Generics::ActiveObjectCallback* callback,
  const std::string& bid_cost_model_dir,
  const std::string& ctr_model_dir,
  const std::size_t update_period)
  : logger_(ReferenceCounting::add_ref(logger)),
    bid_cost_model_dir_(bid_cost_model_dir),
    ctr_model_dir_(ctr_model_dir),
    update_period_(update_period),
    scheduler_(new Generics::Planner(callback)),
    task_runner_(new Generics::TaskRunner(callback, 1)),
    container_(new ContainerImpl(
      logger_,
      bid_cost_model_dir,
      ctr_model_dir))
{
  add_child_object(scheduler_);
  add_child_object(task_runner_);

  create_scheduler_task(update_period_);
}

Storage::Cost Storage::get_cost(
  const TagId& tag_id,
  const Url& url,
  const WinRate& win_rate,
  const Cost& current_cost) const
{
  std::shared_lock lock(shared_mutex_);
  Container_var container = container_;
  lock.unlock();

  if (!container)
  {
    Stream::Error stream;
    stream << FNS
           << "Container is null";
    throw Exception(stream);
  }

  return container->get_cost(
    tag_id,
    url,
    win_rate,
    current_cost);
}

Storage::Ctr Storage::get_ctr(
  const TagId& tag_id,
  const Url& url,
  const CreativeCategoryId& creative_category_id) const
{
  std::shared_lock lock(shared_mutex_);
  Container_var container = container_;
  lock.unlock();

  if (!container)
  {
    Stream::Error stream;
    stream << FNS
           << "Container is null";
    throw Exception(stream);
  }

  return container->get_ctr(tag_id, url, creative_category_id);
}

void Storage::create_scheduler_task(const std::size_t period) noexcept
{
  try
  {
    const auto time = Generics::Time::get_time_of_day()
      + Generics::Time::ONE_SECOND * period;
    scheduler_->schedule(
      AdServer::Commons::make_delegate_goal_task(
        [this] () {
          update();
        },
        task_runner_),
      time);
  }
  catch (const eh::Exception& exc)
  {
    Stream::Error stream;
    stream << FNS
           << exc.what();
    logger_->alert(stream.str(), Aspect::STORAGE);
  }
  catch (...)
  {
    Stream::Error stream;
    stream << FNS
           << "Unknown error";
    logger_->alert(stream.str(), Aspect::STORAGE);
  }
}

void Storage::update() noexcept
{
  try
  {
    {
      std::ostringstream stream;
      stream << FNS
             << "Start updating container";
      logger_->info(stream.str(), Aspect::STORAGE);
    }

    Container_var temp_container = new ContainerImpl(
      logger_,
      bid_cost_model_dir_,
      ctr_model_dir_);

    std::unique_lock lock(shared_mutex_);
    container_.swap(temp_container);
    lock.unlock();

    {
      std::ostringstream stream;
      stream << FNS
             << "Container is updated";
      logger_->info(stream.str(), Aspect::STORAGE);
    }
  }
  catch (const Exception& exc)
  {
    Stream::Error stream;
    stream << FNS
           << "Can't update container reason : "
           << exc.what();
    logger_->error(stream.str(), Aspect::STORAGE);
  }
  catch (...)
  {
    Stream::Error stream;
    stream << FNS
           << "Can't update container reason : "
           << "Unknown error";
    logger_->error(stream.str(), Aspect::STORAGE);
  }

  create_scheduler_task(update_period_);
}

} // namespace PredictorSvcs::BidCostPredictor