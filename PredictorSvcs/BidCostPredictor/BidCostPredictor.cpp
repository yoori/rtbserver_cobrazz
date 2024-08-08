// STD
#include <filesystem>
#include <regex>

// THIS
#include <Commons/DelegateTaskGoal.hpp>
#include "BidCostPredictor.hpp"
#include "ModelBidCostImpl.hpp"
#include "Utils.hpp"

namespace Aspect
{

inline constexpr char BID_COST_PREDICTOR[] = "BID_COST_PREDICTOR";

} // namespace Aspect

namespace PredictorSvcs::BidCostPredictor
{

BidCostPredictor::BidCostPredictor(
  Logger* logger,
  ActiveObjectCallback* callback,
  const std::string& bid_cost_model_dir,
  const std::size_t update_period_sec)
  : logger_(ReferenceCounting::add_ref(logger)),
    bid_cost_model_dir_(bid_cost_model_dir),
    update_period_sec_(update_period_sec),
    scheduler_(new Generics::Planner(callback)),
    task_runner_(new Generics::TaskRunner(callback, 1))
{
  add_child_object(scheduler_);
  add_child_object(task_runner_);

  model_bid_cost_ = new ModelBidCostImpl(logger_);
  const auto file_path = get_last_file(bid_cost_model_dir);
  model_bid_cost_->load(file_path);

  create_scheduler_task(update_period_sec_);
}

BidCostPredictor::Cost BidCostPredictor::predict(
  const TagId& tag_id,
  const Url& url,
  const WinRate& win_rate,
  const Cost& current_cost)
{
  ModelBidCost_var model_bid_cost;
  {
    std::shared_lock lock(shared_mutex_);
    model_bid_cost = model_bid_cost_;
  }

  return model_bid_cost->get_cost(tag_id, url, win_rate, current_cost);
}

void BidCostPredictor::create_scheduler_task(const std::size_t period) noexcept
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
    logger_->alert(stream.str(), Aspect::BID_COST_PREDICTOR);
  }
  catch (...)
  {
    Stream::Error stream;
    stream << FNS
           << "Unknown error";
    logger_->alert(stream.str(), Aspect::BID_COST_PREDICTOR);
  }
}

void BidCostPredictor::update() noexcept
{
  try
  {
    {
      std::ostringstream stream;
      stream << FNS
             << "Start updating bid cost model";
      logger_->info(stream.str(), Aspect::BID_COST_PREDICTOR);
    }

    ModelBidCost_var temp_model_bid_cost = new ModelBidCostImpl(logger_);
    const auto file_path = get_last_file(bid_cost_model_dir_);
    temp_model_bid_cost->load(file_path);

    std::unique_lock lock(shared_mutex_);
    model_bid_cost_.swap(temp_model_bid_cost);
    lock.unlock();

    {
      std::ostringstream stream;
      stream << FNS
             << "Bid cost model is success updated";
      logger_->info(stream.str(), Aspect::BID_COST_PREDICTOR);
    }
  }
  catch (const Exception& exc)
  {
    Stream::Error stream;
    stream << FNS
           << "Can't update ctr model reason : "
           << exc.what();
    logger_->error(stream.str(), Aspect::BID_COST_PREDICTOR);
  }
  catch (...)
  {
    Stream::Error stream;
    stream << FNS
           << "Can't update ctr model reason : "
           << "Unknown error";
    logger_->error(stream.str(), Aspect::BID_COST_PREDICTOR);
  }

  create_scheduler_task(update_period_sec_);
}

std::string BidCostPredictor::get_last_file(
  const std::string& path_dir)
{
  if (!std::filesystem::is_directory(path_dir))
  {
    Stream::Error stream;
    stream << FNS
           << "Not existing directory="
           << path_dir;
    throw Exception(stream);
  }

  auto directories = Utils::get_directory_files(
    path_dir,
    "",
    Utils::DirInfo::Directory);
  if (directories.empty())
  {
    Stream::Error stream;
    stream << FNS
           << "Directory not contain model directory";
    throw Exception(stream);
  }

  const std::regex time_regex("\\d{4}-\\d{2}-\\d{2} \\d{2}:\\d{2}:\\d{2}");

  auto result_it = std::end(directories);
  auto it = std::begin(directories);
  auto it_end = std::end(directories);
  for (; it != it_end; ++it)
  {
    if (std::regex_search(*it, time_regex))
    {
      result_it = it;
    }
  }

  if (result_it == std::end(directories))
  {
    Stream::Error stream;
    stream << FNS
           << "Directory not contain model directory "
           << "with format yyyy:mm:dd hh:mm:ss";
    throw Exception(stream);
  }

  const auto& result_path_directory = *result_it;
  const auto files = Utils::get_directory_files(
    result_path_directory,
    "",
    Utils::DirInfo::RegularFile);
  if (files.empty())
  {
    Stream::Error stream;
    stream << FNS
           << "Not exist file in directory="
           << result_path_directory;
    throw Exception(stream);
  }

  if (files.size() > 1)
  {
    Stream::Error stream;
    stream << FNS
           <<  "Logic error. Number files in the "
           <<  "directory must be equal one";
    throw Exception(stream);
  }

  return files.front();
}

} // namespace PredictorSvcs::BidCostPredictor