// STD
#include <thread>

// THIS
#include "ModelEvaluatorBidCost.hpp"

namespace Aspect
{

inline constexpr char MODEL_EVALUATOR_BID_COST[] = "MODEL_EVALUATOR_BID_COST";

} // namespace Aspect

namespace PredictorSvcs::BidCostPredictor
{

ModelEvaluatorBidCostImpl::ModelEvaluatorBidCostImpl(
  const Points& points,
  DataModelProvider* data_provider,
  ModelBidCostFactory* model_factory,
  Logging::Logger* logger)
  : points_(sort_points(points)),
    data_provider_(ReferenceCounting::add_ref(data_provider)),
    model_factory_(ReferenceCounting::add_ref(model_factory)),
    logger_(ReferenceCounting::add_ref(logger)),
    persantage_(logger_, Aspect::MODEL_EVALUATOR_BID_COST, 5),
    collector_(1, 1, 1)
{
  using Severity = Generics::ActiveObjectCallback::Severity;

  observer_ = new ActiveObjectObserver(
    [this] (
      const Severity severity,
      const String::SubString& description,
      const char* error_code) {
      if (severity == Severity::CRITICAL_ERROR || severity == Severity::ERROR)
      {
        shutdown_manager_.stop();

        Stream::Error stream;
        stream << FNS
               << "ModelEvaluatorBidCost stopped due to incorrect operation of queues."
               << " Reason: "
               << description;
        logger_->critical(
          stream.str(),
          Aspect::MODEL_EVALUATOR_BID_COST,
          error_code);
      }
    });

  task_runner_ = new Generics::TaskRunner(observer_.in(), 1);
  task_runner_->activate_object();

  threads_number_ = std::max(8u, std::thread::hardware_concurrency());
  threads_number_ = std::min(36u, threads_number_);
  task_runner_pool_ = new Generics::TaskRunner(
    observer_,
    threads_number_);
  task_runner_pool_->activate_object();

  if (!data_provider_)
  {
    Stream::Error stream;
    stream << FNS
           << "data_provider is null";
    throw Exception(stream);
  }

  if (!model_factory_)
  {
    Stream::Error stream;
    stream << FNS
           << "model_factory is null";
    throw Exception(stream);
  }
}

ModelEvaluatorBidCostImpl::~ModelEvaluatorBidCostImpl()
{
  shutdown_manager_.stop();
  clear();
}

ModelBidCost_var ModelEvaluatorBidCostImpl::evaluate() noexcept
{
  if (shutdown_manager_.is_stoped())
  {
    std::ostringstream stream;
    stream << FNS
           << "ModelEvaluatorBidCost already is stopped";
    logger_->info(stream.str(), Aspect::MODEL_EVALUATOR_BID_COST);
    return {};
  }

  if (!is_idle_.load())
  {
    Stream::Error stream;
    stream << FNS
           << "ModelEvaluatorBidCost already is evaluated";
    logger_->critical(stream.str(), Aspect::MODEL_EVALUATOR_BID_COST);
    return {};
  }
  is_idle_.store(false);

  model_ = model_factory_->create();
  if (!model_)
  {
    Stream::Error stream;
    stream << FNS
           << "model is null";
    logger_->critical(stream.str(), Aspect::MODEL_EVALUATOR_BID_COST);
    return {};
  }

  if (!data_provider_->load(collector_))
  {
    Stream::Error stream;
    stream << FNS
           << "data_provider load of collector is failed";
    logger_->critical(stream.str(), Aspect::MODEL_EVALUATOR_BID_COST);
    return {};
  }

  if (collector_.empty())
  {
    std::ostringstream stream;
    stream << FNS
           << "Collector is empty";
    logger_->info(stream.str(), Aspect::MODEL_EVALUATOR_BID_COST);
    return std::move(model_);
  }

  try
  {
    run();
  }
  catch (const eh::Exception& exc)
  {
    shutdown_manager_.stop();

    Stream::Error stream;
    stream << FNS
           << "ModelEvaluatorBidCost is failed : "
           << "Reason: "
           << exc.what();
    logger_->critical(stream.str(), Aspect::MODEL_EVALUATOR_BID_COST);
    return {};
  }
  catch (...)
  {
    shutdown_manager_.stop();

    Stream::Error stream;
    stream << FNS
           << "ModelEvaluatorBidCost is failed : "
           << "Reason: Unknown error";
    logger_->critical(stream.str(), Aspect::MODEL_EVALUATOR_BID_COST);
    return {};
  }

  const bool success = wait();
  if (success)
  {
    return std::move(model_);
  }
  else
  {
    model_.reset();
    return {};
  }
}

bool ModelEvaluatorBidCostImpl::wait() noexcept
{
  shutdown_manager_.wait();
  return is_evaluation_success_.load();
}

void ModelEvaluatorBidCostImpl::clear() noexcept
{
  try
  {
    task_runner_pool_->wait_for_queue_exhausting();
  }
  catch (...)
  {
  }

  try
  {
    task_runner_pool_->deactivate_object();
    task_runner_pool_->wait_object();
  }
  catch (...)
  {
  }

  try
  {
    task_runner_->wait_for_queue_exhausting();
  }
  catch (...)
  {
  }

  try
  {
    task_runner_->deactivate_object();
    task_runner_->wait_object();
  }
  catch (...)
  {
  }

  task_runner_pool_.reset();
  task_runner_.reset();
}

template<ConceptMemberPtr MemPtr, class ...Args>
bool ModelEvaluatorBidCostImpl::post_task(
  const TaskRunnerID id,
  MemPtr mem_ptr,
  Args&& ...args) noexcept
{
  try
  {
    TaskRunner_var task_runner;
    switch (id)
    {
      case TaskRunnerID::Single:
      {
        task_runner = task_runner_;
        break;
      }
      case TaskRunnerID::Pool:
      {
        task_runner = task_runner_pool_;
        break;
      }
    }

    task_runner->enqueue_task(
      AdServer::Commons::make_delegate_task(
        std::bind(
          mem_ptr,
          this,
          std::forward<Args>(args)...)));

    return true;
  }
  catch (const eh::Exception& exc)
  {
    shutdown_manager_.stop();

    Stream::Error stream;
    stream << FNS
           << "Can't enqueue_task : "
           << "Reason: "
           << exc.what();
    logger_->critical(stream.str(), Aspect::MODEL_EVALUATOR_BID_COST);
  }
  catch (...)
  {
    shutdown_manager_.stop();

    Stream::Error stream;
    stream << FNS
           << "Can't enqueue_task:"
           << "Reason: Unknown error";
    logger_->critical(stream.str(), Aspect::MODEL_EVALUATOR_BID_COST);
  }

  return false;
}

ModelEvaluatorBidCostImpl::Points
ModelEvaluatorBidCostImpl::sort_points(const Points& points)
{
  Points result(points);
  std::sort(
    std::begin(result),
    std::end(result),
    [] (const auto& d1, const auto& d2) {
      return d1 > d2;
    });

  return result;
}

void ModelEvaluatorBidCostImpl::run()
{
  {
    std::ostringstream stream;
    stream << FNS
           << "ModelEvaluatorBidCost started";
    logger_->info(stream.str(), Aspect::MODEL_EVALUATOR_BID_COST);
  }

  if (!post_task(TaskRunnerID::Single, &ModelEvaluatorBidCostImpl::do_init))
  {
    Stream::Error stream;
    stream << FNS
           << "Initial post_task is failed";
    throw Exception(stream);
  }
}

void ModelEvaluatorBidCostImpl::stop() noexcept
{
  shutdown_manager_.stop();

  std::ostringstream stream;
  stream << FNS
         << "ModelEvaluatorBidCost was interrupted";
  logger_->info(stream.str(), Aspect::MODEL_EVALUATOR_BID_COST);
}

void ModelEvaluatorBidCostImpl::do_init() noexcept
{
  if (shutdown_manager_.is_stoped())
  {
    return;
  }

  remaining_iterations_ = collector_.size();
  persantage_.set_total_number(remaining_iterations_);
  collector_iterator_ = std::begin(collector_);
  const std::size_t count = std::min(
    static_cast<std::size_t>(threads_number_ * 3),
    remaining_iterations_);
  for (std::size_t i = 1; i <= count; ++i)
  {
    post_task(
      TaskRunnerID::Pool,
      &ModelEvaluatorBidCostImpl::do_calculate,
      collector_iterator_);
    ++collector_iterator_;
  }
}

void ModelEvaluatorBidCostImpl::do_calculate(const Iterator collector_it) noexcept
{
  if (shutdown_manager_.is_stoped())
  {
    return;
  }

  try
  {
    do_calculate_helper(collector_it);
  }
  catch (const eh::Exception& exc)
  {
    shutdown_manager_.stop();

    Stream::Error stream;
    stream << FNS
           << "Reason: "
           << exc.what();
    logger_->critical(stream.str(), Aspect::MODEL_EVALUATOR_BID_COST);
    return;
  }
  catch (...)
  {
    shutdown_manager_.stop();

    Stream::Error stream;
    stream << FNS
           << "Reason: Unknown error";
    logger_->critical(stream.str(), Aspect::MODEL_EVALUATOR_BID_COST);
    return;
  }

  post_task(
    TaskRunnerID::Single,
    &ModelEvaluatorBidCostImpl::do_next_task);

  post_task(
    TaskRunnerID::Single,
    &ModelEvaluatorBidCostImpl::do_decrease);
}

void ModelEvaluatorBidCostImpl::do_calculate_helper(const Iterator collector_it)
{
  if (shutdown_manager_.is_stoped())
  {
    return;
  }

  const auto& top_key = collector_it->first;
  const auto& cost_dict = *collector_it->second;
  const std::size_t size_cost_dict = cost_dict.size();

  Types::Imps unverified_imps = 0;
  Types::Imps imps = 0;
  auto it = cost_dict.rbegin();
  const auto it_end = cost_dict.rend();
  for (; it != it_end; ++it)
  {
    const auto& cost_data = it->second;
    unverified_imps += cost_data.unverified_imps();
    imps += cost_data.imps();

    if (imps >= TOP_LEVEL_WIN_RATE_MIN_IMPS)
    {
      break;
    }
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
      const auto check_win_rate = FixedNumber::mul(
        *top_level_win_rate,
        point,
        Generics::DMR_FLOOR);
      const auto& max_cost = cost_dict.rbegin()->first;

      std::optional<FixedNumber> target_cost;
      for (std::size_t base_cost_i = 0; base_cost_i < size_cost_dict; ++base_cost_i)
      {
        Types::Imps unverified_imps = 0;
        Types::Imps imps = 0;
        std::size_t cost_i = base_cost_i;
        for (; cost_i < size_cost_dict; ++cost_i)
        {
          const auto& cost_data = cost_dict.nth(cost_i)->second;
          unverified_imps += cost_data.unverified_imps();
          imps += cost_data.imps();

          if (imps >= LEVEL_WIN_RATE_MIN_IMPS)
          {
            break;
          }
        }

        if (imps >= LEVEL_WIN_RATE_MIN_IMPS && unverified_imps > 0)
        {
          const FixedNumber dividend(false, imps, 0);
          const FixedNumber divider(false, unverified_imps, 0);
          const auto local_win_rate = FixedNumber::div(dividend, divider);
          if (local_win_rate >= check_win_rate)
          {
            const auto index = std::min(cost_i, size_cost_dict - 1);
            target_cost = cost_dict.nth(index)->first;
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

void ModelEvaluatorBidCostImpl::do_save(
  const TagId& tag_id,
  const UrlPtr& url,
  const FixedNumber& point,
  const FixedNumber& target_cost,
  const FixedNumber& max_cost) noexcept
{
  if (shutdown_manager_.is_stoped())
  {
    return;
  }

  try
  {
    model_->set_cost(
      tag_id,
      *url,
      point,
      target_cost,
      max_cost);
  }
  catch (const eh::Exception& exc)
  {
    shutdown_manager_.stop();

    Stream::Error stream;
    stream << FNS
           << "Reason: "
           << exc.what();
    logger_->critical(stream.str(), Aspect::MODEL_EVALUATOR_BID_COST);
  }
  catch (...)
  {
    shutdown_manager_.stop();

    Stream::Error stream;
    stream << FNS
           << "Reason: Unknown error";
    logger_->critical(stream.str(), Aspect::MODEL_EVALUATOR_BID_COST);
  }
}

void ModelEvaluatorBidCostImpl::do_decrease() noexcept
{
  if (shutdown_manager_.is_stoped())
  {
    return;
  }

  remaining_iterations_ -= 1;
  if (remaining_iterations_ == 0)
  {
    is_evaluation_success_.store(true);
    shutdown_manager_.stop();
  }
  persantage_.increase();
}

void ModelEvaluatorBidCostImpl::do_next_task() noexcept
{
  if (shutdown_manager_.is_stoped())
  {
    return;
  }

  if (collector_iterator_ != std::end(collector_))
  {
    post_task(
      TaskRunnerID::Pool,
      &ModelEvaluatorBidCostImpl::do_calculate,
      collector_iterator_);
    ++collector_iterator_;
  }
}

} // namespace PredictorSvcs::BidCostPredictor