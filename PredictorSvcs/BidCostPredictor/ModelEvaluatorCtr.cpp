// STD
#include <thread>

// THIS
#include "ModelEvaluatorCtr.hpp"

namespace Aspect
{
const char* MODEL_EVALUATOR_CTR = "MODEL_EVALUATOR_CTR";
} // namespace Aspect

namespace PredictorSvcs
{
namespace BidCostPredictor
{

ModelEvaluatorCtrImpl::ModelEvaluatorCtrImpl(
  DataModelProvider* data_provider,
  ModelCtrFactory* model_factory,
  Logging::Logger* logger)
  : data_provider_(ReferenceCounting::add_ref(data_provider)),
    model_factory_(ReferenceCounting::add_ref(model_factory)),
    logger_(ReferenceCounting::add_ref(logger)),
    observer_(new ActiveObjectObserver(this)),
    persantage_(logger_, Aspect::MODEL_EVALUATOR_CTR, 5),
    collector_(10000000, 1),
    task_runner_(
      new Generics::TaskRunner(
        observer_,
        1))
{
  threads_number_ = std::max(8u, std::thread::hardware_concurrency());
  threads_number_ = std::min(36u, threads_number_);
  task_runner_pool_ = TaskRunner_var(
    new Generics::TaskRunner(
      observer_,
      threads_number_));
}

ModelCtr_var ModelEvaluatorCtrImpl::evaluate() noexcept
{
  if (shutdown_manager_.is_stoped())
  {
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << " : shutdown_manager already is stopped";
    logger_->critical(stream.str(), Aspect::MODEL_EVALUATOR_CTR);
    shutdown_manager_.stop();
    return {};
  }

  if (!data_provider_)
  {
    shutdown_manager_.stop();
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << " : data_provider is null";
    logger_->critical(stream.str(), Aspect::MODEL_EVALUATOR_CTR);
    return {};
  }

  if (!model_factory_)
  {
    shutdown_manager_.stop();
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << " : model_factory is null";
    logger_->critical(stream.str(), Aspect::MODEL_EVALUATOR_CTR);
    return {};
  }

  model_ = model_factory_->create();
  if (!model_)
  {
    shutdown_manager_.stop();
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << " : model is null";
    logger_->critical(stream.str(), Aspect::MODEL_EVALUATOR_CTR);
    return {};
  }

  if (!data_provider_->load(collector_))
  {
    shutdown_manager_.stop();
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << " : data_provider load collector is failed";
    logger_->critical(stream.str(), Aspect::MODEL_EVALUATOR_CTR);
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
           << " : ModelEvaluatorCtr is failed"
           << " : Reason: "
           << exc.what();
    logger_->critical(stream.str(), Aspect::MODEL_EVALUATOR_CTR);
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

void ModelEvaluatorCtrImpl::wait() noexcept
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

ModelEvaluatorCtrImpl::~ModelEvaluatorCtrImpl()
{
  shutdown_manager_.stop();
  observer_->clear_delegate();
  wait();
}

void ModelEvaluatorCtrImpl::start()
{
  logger_->info(
    std::string("ModelEvaluatorCtr started"),
    Aspect::MODEL_EVALUATOR_CTR);

  is_running_ = true;

  task_runner_pool_->activate_object();
  task_runner_->activate_object();

  if (!post_task(
    TaskRunnerID::Single,
    &ModelEvaluatorCtrImpl::do_init))
  {
    throw Exception("Initial post_task is failed");
  }
}

void ModelEvaluatorCtrImpl::stop() noexcept
{
  logger_->info(
    std::string("ModelEvaluatorCtr was interrupted"),
    Aspect::MODEL_EVALUATOR_CTR);
  shutdown_manager_.stop();
}

void ModelEvaluatorCtrImpl::do_init() noexcept
{
  if (shutdown_manager_.is_stoped())
    return;

  persantage_.set_total_number(collector_.size());
  remaining_iterations_ = collector_.size();
  iterator_ = std::begin(collector_);
  const std::size_t count =
    std::min(static_cast<std::size_t>(threads_number_ * 3), remaining_iterations_);
  for (std::size_t i = 1; i <= count; ++i)
  {
    post_task(
      TaskRunnerID::Pool,
      &ModelEvaluatorCtrImpl::do_calculate,
      iterator_);
    ++iterator_;
  }
}

void ModelEvaluatorCtrImpl::do_calculate(const Iterator it) noexcept
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
    logger_->critical(stream.str(), Aspect::MODEL_EVALUATOR_CTR);
  }
}

void ModelEvaluatorCtrImpl::do_calculate_helper(const Iterator it)
{
  if (shutdown_manager_.is_stoped())
    return;

  const auto& top_key = it->first;
  const auto& cost_dict = it->second;

  Imps all_imps = 0;
  Clicks all_clicks = 0;

  for (const auto& [cost, data] : *cost_dict)
  {
    all_imps += data.imps();
    all_clicks += data.clicks();
  }

  const auto& tag_id = top_key.tag_id();
  const auto& url = top_key.url_var();

  post_task(
    TaskRunnerID::Single,
    &ModelEvaluatorCtrImpl::do_next_task);

  post_task(
    TaskRunnerID::Single,
    &ModelEvaluatorCtrImpl::do_save,
    tag_id,
    url,
    all_clicks,
    all_imps);
}

void ModelEvaluatorCtrImpl::do_save(
  const TagId& tag_id,
  const Url_var& url,
  const Clicks& all_clicks,
  const Imps& all_imps) noexcept
{
  if (shutdown_manager_.is_stoped())
    return;

  try
  {
    if (all_clicks !=0 || all_imps != 0)
    {
      model_->set_ctr(tag_id, url, all_clicks, all_imps);
    }
  }
  catch (const eh::Exception& exc)
  {
    Stream::Error ostr;
    ostr << __PRETTY_FUNCTION__
         << " : Reason: "
         << exc.what();
    logger_->critical(
      ostr.str(),
      Aspect::MODEL_EVALUATOR_CTR);
    shutdown_manager_.stop();
    return;
  }

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

void ModelEvaluatorCtrImpl::do_next_task() noexcept
{
  if (shutdown_manager_.is_stoped())
    return;

  if (iterator_ != std::end(collector_))
  {
    post_task(
      TaskRunnerID::Pool,
      &ModelEvaluatorCtrImpl::do_calculate,
      iterator_);
    ++iterator_;
  }
}

void ModelEvaluatorCtrImpl::report_error(
  Severity severity,
  const String::SubString& description,
  const char* error_code) noexcept
{
  if (severity == Severity::CRITICAL_ERROR || severity == Severity::ERROR)
  {
    shutdown_manager_.stop();
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << " : ModelEvaluatorCtr stopped due to incorrect operation of queues."
           << " Reason: "
           << description;
    logger_->critical(
      stream.str(),
      Aspect::MODEL_EVALUATOR_CTR,
      error_code);
  }
}

} // namespace BidCostPredictor
} // namespace PredictorSvcs
