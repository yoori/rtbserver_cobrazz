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
        const DataModelProvider_var& data_provider,
        const ModelCtrFactory_var& model_factory,
        const Logging::Logger_var& logger)
        : data_provider_(data_provider),
          model_factory_(model_factory),
          logger_(logger),
          observer_(new ActiveObjectObserver(this)),
          persantage_(logger_, Aspect::MODEL_EVALUATOR_CTR, 5),
          task_runner_(new Generics::TaskRunner(
                  observer_,
                  1))
{
  threads_number_ = std::max(8u, std::thread::hardware_concurrency());
  threads_number_ = std::min(36u, threads_number_);
  task_runner_pool_ = TaskRunner_var(
          new Generics::TaskRunner(
                  observer_,
                  threads_number_));
  collector_.prepare_adding(50000000);
}

ModelCtr_var ModelEvaluatorCtrImpl::evaluate() noexcept
{
  if (shutdown_manager_.isStoped())
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
  observer_->clearDelegate();
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

  if (!postTask(TaskRunnerID::Single,
                &ModelEvaluatorCtrImpl::doInit))
  {
    throw Exception("Initial postTask is failed");
  }
}

void ModelEvaluatorCtrImpl::stop() noexcept
{
  logger_->info(
          std::string("ModelEvaluatorCtr was interrupted"),
          Aspect::MODEL_EVALUATOR_CTR);
  shutdown_manager_.stop();
}

void ModelEvaluatorCtrImpl::doInit() noexcept
{
  if (shutdown_manager_.isStoped())
    return;

  persantage_.setTotalNumber(collector_.size());
  remaining_iterations_ = collector_.size();
  iterator_ = std::begin(collector_);
  const std::size_t count =
          std::min(static_cast<std::size_t>(threads_number_ * 3), remaining_iterations_);
  for (std::size_t i = 1; i <= count; ++i)
  {
    postTask(
            TaskRunnerID::Pool,
            &ModelEvaluatorCtrImpl::doCalculate,
            iterator_);
    ++iterator_;
  }
}

void ModelEvaluatorCtrImpl::doCalculate(const Iterator it) noexcept
{
  if (shutdown_manager_.isStoped())
    return;

  try
  {
    doCalculateHelper(it);
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

void ModelEvaluatorCtrImpl::doCalculateHelper(const Iterator it)
{
  if (shutdown_manager_.isStoped())
    return;

  postTask(
          TaskRunnerID::Single,
          &ModelEvaluatorCtrImpl::doNextTask);

  const auto& top_key = it->first;
  const auto& cost_dict = it->second;

  Imps all_imps = 0;
  Clicks all_clicks = 0;

  for (const auto& [cost, data] : cost_dict)
  {
    all_imps += data.imps();
    all_clicks += data.clicks();
  }

  const auto& tag_id = top_key.tagId();
  const auto& url = top_key.urlVar();

  postTask(
          TaskRunnerID::Single,
          &ModelEvaluatorCtrImpl::doSave,
          tag_id,
          url,
          all_clicks,
          all_imps);
}

void ModelEvaluatorCtrImpl::doSave(
        const TagId& tag_id,
        const Url_var& url,
        const Clicks& all_clicks,
        const Imps& all_imps) noexcept
{
  if (shutdown_manager_.isStoped())
    return;

  try
  {
    model_->setCtr(tag_id, url, all_clicks, all_imps);
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
}

void ModelEvaluatorCtrImpl::doNextTask() noexcept
{
  if (shutdown_manager_.isStoped())
    return;

  if (iterator_ != std::end(collector_))
  {
    postTask(
            TaskRunnerID::Pool,
            &ModelEvaluatorCtrImpl::doCalculate,
            iterator_);
    ++iterator_;
  }

  remaining_iterations_ -= 1;
  if (remaining_iterations_ == 0)
  {
    is_success_ = true;
    shutdown_manager_.stop();
  }

  persantage_.increase();
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
