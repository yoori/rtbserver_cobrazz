// STD
#include <thread>

// THIS
#include "ModelEvaluatorBidCost.hpp"

namespace Aspect
{
const char* MODEL_EVALUATOR_BID_COST = "MODEL_EVALUATOR_BID_COST";
}

namespace PredictorSvcs
{
namespace BidCostPredictor
{

ModelEvaluatorBidCostImpl::ModelEvaluatorBidCostImpl(
  const Points& points,
  DataModelProvider* data_provider,
  ModelBidCostFactory* model_factory,
  Logging::Logger* logger)
  : points_(points),
    data_provider_(ReferenceCounting::add_ref(data_provider)),
    model_factory_(ReferenceCounting::add_ref(model_factory)),
    logger_(ReferenceCounting::add_ref(logger)),
    observer_(new ActiveObjectObserver(this)),
    persantage_(logger_, Aspect::MODEL_EVALUATOR_BID_COST, 5),
    task_runner_(new Generics::TaskRunner(observer_, 1))
{
  threads_number_ = std::max(8u, std::thread::hardware_concurrency());
  threads_number_ = std::min(36u, threads_number_);
  task_runner_pool_ = TaskRunner_var(
    new Generics::TaskRunner(
      observer_,
      threads_number_));

  std::sort(
    std::begin(points_),
    std::end(points_),
    [] (const auto& d1, const auto& d2) {
      return d1 > d2;
    });

  collector_.prepare_adding(50000000);
}

ModelBidCost_var ModelEvaluatorBidCostImpl::evaluate() noexcept
{
  if (shutdown_manager_.is_stoped())
  {
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << " : shutdown_manager already is stopped";
    logger_->critical(stream.str(), Aspect::MODEL_EVALUATOR_BID_COST);
    return {};
  }

  if (!data_provider_)
  {
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << " : data_provider is null";
    logger_->critical(stream.str(), Aspect::MODEL_EVALUATOR_BID_COST);
    return {};
  }

  if (!model_factory_)
  {
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << " : model_factory is null";
    logger_->critical(stream.str(), Aspect::MODEL_EVALUATOR_BID_COST);
    return {};
  }

  model_ = model_factory_->create();
  if (!model_)
  {
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << " : model is null";
    logger_->critical(stream.str(), Aspect::MODEL_EVALUATOR_BID_COST);
    return {};
  }

  if (!data_provider_->load(collector_))
  {
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << " : data_provider load collector is failed";
    logger_->critical(stream.str(), Aspect::MODEL_EVALUATOR_BID_COST);
    return {};
  }

  try
  {
    start();
  }
  catch (const eh::Exception& exc)
  {
    shutdown_manager_.stop();
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << " : ModelEvaluatorBidCost is failed"
           << " : Reason: "
           << exc.what();
    logger_->critical(stream.str(), Aspect::MODEL_EVALUATOR_BID_COST);
    return {};
  }

  wait();

  if (is_success_)
  {
    return std::move(model_);
  }
  else
  {
    model_.reset();
    return {};
  }
}

void ModelEvaluatorBidCostImpl::wait() noexcept
{
  if (!is_running_)
    return;

  is_running_ = false;

  shutdown_manager_.wait();

  try
  {
    task_runner_pool_->wait_for_queue_exhausting();
  }
  catch (...)
  {}

  try
  {
    task_runner_pool_->deactivate_object();
    task_runner_pool_->wait_object();
    task_runner_pool_->clear();
  }
  catch (...)
  {}

  try
  {
    task_runner_->wait_for_queue_exhausting();
  }
  catch (...)
  {}

  try
  {
    task_runner_->deactivate_object();
    task_runner_->wait_object();
    task_runner_->clear();
  }
  catch (...)
  {}

  task_runner_pool_.reset();
  task_runner_.reset();
}

ModelEvaluatorBidCostImpl::~ModelEvaluatorBidCostImpl()
{
  shutdown_manager_.stop();
  observer_->clear_delegate();
  wait();
}

void ModelEvaluatorBidCostImpl::start()
{
  logger_->info(
          std::string("ModelEvaluatorBidCost started"),
          Aspect::MODEL_EVALUATOR_BID_COST);

  is_running_ = true;

  task_runner_pool_->activate_object();
  task_runner_->activate_object();

  if (!post_task(
    TaskRunnerID::Single,
    &ModelEvaluatorBidCostImpl::do_init))
  {
    throw Exception("Initial post_task is failed");
  }
}

void ModelEvaluatorBidCostImpl::stop() noexcept
{
  shutdown_manager_.stop();
  logger_->info(
    std::string("ModelEvaluatorWBidCost was interrupted"),
    Aspect::MODEL_EVALUATOR_BID_COST);
}

void ModelEvaluatorBidCostImpl::do_init() noexcept
{
  if (shutdown_manager_.is_stoped())
    return;

  remaining_iterations_ = collector_.size();
  persantage_.set_total_number(remaining_iterations_);
  iterator_ = std::begin(collector_);
  const std::size_t count =
    std::min(static_cast<std::size_t>(threads_number_ * 3), remaining_iterations_);
  for (std::size_t i = 1; i <= count; ++i)
  {
    post_task(
      TaskRunnerID::Pool,
      &ModelEvaluatorBidCostImpl::do_calculate,
      iterator_);
    ++iterator_;
  }
}

void ModelEvaluatorBidCostImpl::do_calculate(const Iterator it) noexcept
{
  if (shutdown_manager_.is_stoped())
    return;

  try
  {
    do_calculate_helper(it);
  }
  catch (const eh::Exception& exc)
  {
    shutdown_manager_.stop();
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << " : Reason: "
           << exc.what();
    logger_->critical(stream.str(), Aspect::MODEL_EVALUATOR_BID_COST);
  }

  post_task(
    TaskRunnerID::Single,
    &ModelEvaluatorBidCostImpl::do_next_task);

  post_task(
    TaskRunnerID::Single,
    &ModelEvaluatorBidCostImpl::do_decrease);
}

void ModelEvaluatorBidCostImpl::do_calculate_helper(const Iterator it)
{
  if (shutdown_manager_.is_stoped())
    return;

  const auto& top_key = it->first;
  const auto& cost_dict = it->second;

  std::vector<FixedNumber> all_costs;
  all_costs.reserve(cost_dict.size());
  for (const auto& [k, d] : cost_dict)
  {
    all_costs.emplace_back(k.cost());
  }
  const auto size_all_cost = cost_dict.size();

  if constexpr(!is_help_collector_map)
  {
    std::sort(
      std::begin(all_costs),
      std::end(all_costs));
  }

  long unverified_imps = 0;
  long imps = 0;
  auto it_begin = all_costs.rbegin();
  auto it_end = all_costs.rend();
  for (auto it = it_begin; it != it_end; ++it)
  {
    const auto it_cost = cost_dict.find(HelpInnerKey(*it));
    const auto& cost_data = it_cost->second;
    unverified_imps += cost_data.unverified_imps();
    imps += cost_data.imps();

    if (imps >= TOP_LEVEL_WIN_RATE_MIN_IMPS)
      break;
  }

  std::optional<FixedNumber> top_level_win_rate;
  if (unverified_imps > 0)
  {
    if (imps == 0)
    {
      top_level_win_rate = FixedNumber::ZERO;
    }
    else
    {
      const FixedNumber dividend(false, imps, 0);
      const FixedNumber divider(false, unverified_imps, 0);
      top_level_win_rate = FixedNumber::div(dividend, divider);
    }
  }

  if (top_level_win_rate
   && top_level_win_rate->is_nonnegative()
   && !top_level_win_rate->is_zero())
  {
    for (const auto& point : points_)
    {
      const auto check_win_rate =
        FixedNumber::mul(
          *top_level_win_rate,
          point,
          Generics::DMR_FLOOR);
      const auto& max_cost = all_costs.back();

      std::optional<FixedNumber> target_cost;
      for (std::size_t base_cost_i = 0; base_cost_i < size_all_cost; ++base_cost_i)
      {
        long int unverified_imps = 0;
        long int imps = 0;
        std::size_t cost_i = base_cost_i;
        for (; cost_i < size_all_cost; ++cost_i)
        {
          const HelpInnerKey key_inner(all_costs[cost_i]);
          if (auto it = cost_dict.find(key_inner);
            it != cost_dict.end())
          {
            const auto& cost_date = it->second;
            unverified_imps += cost_date.unverified_imps();
            imps += cost_date.imps();

            if (imps >= LEVEL_WIN_RATE_MIN_IMPS)
            {
              break;
            }
          }
          else
          {
            throw Exception("Logic error. Not found cost");
          }
        }

        if (imps >= LEVEL_WIN_RATE_MIN_IMPS && unverified_imps > 0)
        {
          const FixedNumber dividend(false, imps, 0);
          const FixedNumber divider(false, unverified_imps, 0);
          const auto local_win_rate = FixedNumber::div(dividend, divider);
          if (local_win_rate >= check_win_rate)
          {
            target_cost = all_costs[std::min(cost_i, size_all_cost - 1)];
            break;
          }
        }
      }

      if (target_cost)
      {
        const auto& tag_id = top_key.tag_id();
        const auto& url = top_key.url_var();

        post_task(
          TaskRunnerID::Single,
          &ModelEvaluatorBidCostImpl::do_save,
          tag_id,
          url,
          point,
          *target_cost,
          max_cost);
      }
    }
  }
}

void ModelEvaluatorBidCostImpl::do_decrease() noexcept
{
  if (shutdown_manager_.is_stoped())
    return;

  try
  {
    remaining_iterations_ -= 1;
    if (remaining_iterations_ == 0)
    {
      is_success_ = true;
      shutdown_manager_.stop();
    }

    persantage_.increase();
  }
  catch (...)
  {
  }
}

void ModelEvaluatorBidCostImpl::do_save(
  const TagId& tag_id,
  const Url_var& url,
  const FixedNumber& point,
  const FixedNumber& target_cost,
  const FixedNumber& max_cost) noexcept
{
  if (shutdown_manager_.is_stoped())
    return;

  try
  {
    model_->set_cost(tag_id, url, point, target_cost, max_cost);
  }
  catch (const eh::Exception& exc)
  {
    shutdown_manager_.stop();
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << " : Reason: "
           << exc.what();
    logger_->critical(
      stream.str(),
      Aspect::MODEL_EVALUATOR_BID_COST);
    return;
  }
}

void ModelEvaluatorBidCostImpl::do_next_task() noexcept
{
  if (shutdown_manager_.is_stoped())
    return;

  if (iterator_ != std::end(collector_))
  {
    post_task(
      TaskRunnerID::Pool,
      &ModelEvaluatorBidCostImpl::do_calculate,
      iterator_);
    ++iterator_;
  }
}

void ModelEvaluatorBidCostImpl::report_error(
  Severity severity,
  const String::SubString& description,
  const char* error_code) noexcept
{
  if (severity == Severity::CRITICAL_ERROR || severity == Severity::ERROR)
  {
    shutdown_manager_.stop();
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << " : ModelEvaluatorBidCost stopped due to incorrect operation of queues."
           << " Reason: "
           << description;
    logger_->critical(
      stream.str(),
      Aspect::MODEL_EVALUATOR_BID_COST,
      error_code);
  }
}

} // namespace BidCostPredictor
} // namespace PredictorSvcs