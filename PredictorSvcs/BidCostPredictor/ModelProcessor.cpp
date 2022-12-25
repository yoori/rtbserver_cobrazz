// STD
#include <chrono>
#include <iomanip>

// POSIX
#include <sys/stat.h>

// THIS
#include "DataModelProviderImpl.hpp"
#include "ModelBidCostFactory.hpp"
#include "ModelCtrFactory.hpp"
#include "ModelEvaluatorBidCost.hpp"
#include "ModelEvaluatorCtr.hpp"
#include "ModelProcessor.hpp"

namespace Aspect
{
const char* MODEL_PROCESSOR = "MODEL_PROCESSOR";
} // namespace Aspect

namespace PredictorSvcs
{
namespace BidCostPredictor
{

ModelProcessor::ModelProcessor(
  const std::string& model_dir,
  const std::string& model_file_name,
  const std::string& temp_model_dir,
  const std::string& ctr_model_dir,
  const std::string& ctr_model_file_name,
  const std::string& ctr_temp_model_dir,
  const std::string& agg_dir,
  Logging::Logger* logger)
  : model_dir_(model_dir),
    model_file_name_(model_file_name),
    temp_model_dir_(temp_model_dir),
    ctr_model_dir_(ctr_model_dir),
    ctr_model_file_name_(ctr_model_file_name),
    ctr_temp_model_dir_(ctr_temp_model_dir),
    agg_dir_(agg_dir),
    logger_(ReferenceCounting::add_ref(logger))
{
  data_provider_ =
    DataModelProvider_var(
      new DataModelProviderImpl(
        agg_dir_,
        logger_));

  Points points {
    FixedNumber("0.95"),
    FixedNumber("0.75"),
    FixedNumber("0.5"),
    FixedNumber("0.25")
  };

  ModelBidCostFactory_var bid_cost_model_factory(
    new ModelBidCostFactoryImpl(logger_));

  model_evaluator_bid_cost_ =
    ModelEvaluatorBidCost_var(
      new ModelEvaluatorBidCostImpl(
        points,
        data_provider_,
        bid_cost_model_factory,
        logger_));

  ModelCtrFactory_var ctr_model_factory(
    new ModelCtrFactoryImpl(logger_));

  model_evaluator_ctr_ =
    ModelEvaluatorCtr_var(
      new ModelEvaluatorCtrImpl(
        data_provider_,
        ctr_model_factory,
        logger_));
}

ModelProcessor::~ModelProcessor()
{
  wait();
}

void ModelProcessor::start()
{
  if (shutdown_manager_.is_stoped())
    return;

  try
  {
    logger_->info(
      std::string("Start process bid cost model"),
      Aspect::MODEL_PROCESSOR);
    auto bid_cost_model = model_evaluator_bid_cost_->evaluate();
    if (bid_cost_model)
    {
      save_model(
        *bid_cost_model,
        model_dir_,
        temp_model_dir_,
        model_file_name_);
      logger_->info(
        std::string("Process bid cost model is success"),
        Aspect::MODEL_PROCESSOR);
    }
    else
    {
      if (!is_stopped_.load())
      {
        logger_->critical(
          std::string("Process bid cost model is failed"),
          Aspect::MODEL_PROCESSOR);
      }
    }
  }
  catch (const eh::Exception& exc)
  {
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << " : Process bid cost model is failed. Reason: "
           << exc.what();
    logger_->critical(stream.str(), Aspect::MODEL_PROCESSOR);
  }

  if (shutdown_manager_.is_stoped())
    return;

  try
  {
    logger_->info(
      std::string("Start process ctr model"),
      Aspect::MODEL_PROCESSOR);

    auto ctr_model = model_evaluator_ctr_->evaluate();
    if (ctr_model)
    {
      save_model(
        *ctr_model,
        ctr_model_dir_,
        ctr_temp_model_dir_,
        ctr_model_file_name_);
      logger_->info(
        std::string("Process ctr model is success"),
        Aspect::MODEL_PROCESSOR);
    }
    else
    {
      if (!is_stopped_.load())
      {
        logger_->critical(
          std::string("Process ctr model is failed"),
          Aspect::MODEL_PROCESSOR);
      }
    }
  }
  catch (const eh::Exception& exc)
  {
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << " : Process ctr model is failed. Reason: "
           << exc.what();
    logger_->critical(stream.str(), Aspect::MODEL_PROCESSOR);
  }

  shutdown_manager_.stop();
}

void ModelProcessor::wait() noexcept
{
  shutdown_manager_.wait();
}

void ModelProcessor::stop() noexcept
{
  logger_->info(
    std::string("Start stoping model processor"),
    Aspect::MODEL_PROCESSOR);

  is_stopped_.store(true);

  shutdown_manager_.stop();

  if (data_provider_)
    data_provider_->stop();

  if (model_evaluator_bid_cost_)
    model_evaluator_bid_cost_->stop();

  if (model_evaluator_ctr_)
    model_evaluator_ctr_->stop();

  logger_->info(
    std::string("model processor is stopped"),
    Aspect::MODEL_PROCESSOR);
}

const char* ModelProcessor::name() noexcept
{
  return "ModelProcessor";
}

bool ModelProcessor::save_model(
  const ModelManager& model,
  const std::string& model_dir,
  const std::string& temp_dir,
  const std::string& file_name) noexcept
{
  try
  {
    const auto current_time = std::chrono::system_clock::now();
    const std::string name_dir =
      serialize_time_point(
        current_time,
        "%Y-%m-%d %H:%M:%S");
    const std::string temp_model_dir =
      temp_dir + "/" + name_dir;
    if (mkdir(temp_model_dir.c_str(), 0755))
    {
      Stream::Error ostr;
      ostr << __PRETTY_FUNCTION__
           << "Can't create directory="
           << temp_model_dir;
      throw Exception(ostr);
    }
    const std::string temp_file_path =
      temp_model_dir + "/" + file_name;
    model.save(temp_file_path);

    const std::string result_model_dir = model_dir + "/" + name_dir;
    if (std::rename(temp_model_dir.c_str(), result_model_dir.c_str()))
    {
      Stream::Error error;
      error << __PRETTY_FUNCTION__
            << " : Can't rename dir="
            << temp_model_dir
            << " to dir="
            << result_model_dir;
      throw Exception(error);
    }
    logger_->info(
      "Resulting file moved to " + result_model_dir,
      Aspect::MODEL_PROCESSOR);

    return true;
  }
  catch (const eh::Exception& exc)
  {
    Stream::Error ostr;
    ostr << __PRETTY_FUNCTION__
         << " Can't save model. Reason: "
         << exc.what();
    logger_->critical(ostr.str(), Aspect::MODEL_PROCESSOR);

    return false;
  }
}

std::string ModelProcessor::serialize_time_point(
  const std::chrono::system_clock::time_point& time,
  const std::string& format)
{
  const std::time_t tt = std::chrono::system_clock::to_time_t(time);
  std::tm tm = *std::gmtime(&tt);
  std::stringstream stream;
  stream << std::put_time(&tm, format.c_str());
  return stream.str();
}

} // namespace BidCostPredictor
} // namespace PredictorSvcs