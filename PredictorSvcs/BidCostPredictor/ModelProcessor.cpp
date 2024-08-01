// STD
#include <chrono>
#include <filesystem>
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

inline constexpr char MODEL_PROCESSOR[] = "MODEL_PROCESSOR";

} // namespace Aspect

namespace PredictorSvcs::BidCostPredictor
{

ModelProcessor::ModelProcessor(
  const std::string& model_dir,
  const std::string& model_file_name,
  const std::string& temp_model_dir,
  const std::string& ctr_model_dir,
  const std::string& ctr_model_file_name,
  const std::string& ctr_temp_model_dir,
  const Imps& ctr_model_max_imps,
  const Imps& ctr_model_trust_imps,
  const Imps& ctr_model_tag_imps,
  const std::string& agg_dir,
  Logger* logger,
  CreativeProvider* creative_provider)
  : model_dir_(model_dir),
    model_file_name_(model_file_name),
    temp_model_dir_(temp_model_dir),
    ctr_model_dir_(ctr_model_dir),
    ctr_model_file_name_(ctr_model_file_name),
    ctr_temp_model_dir_(ctr_temp_model_dir),
    agg_dir_(agg_dir),
    logger_(ReferenceCounting::add_ref(logger)),
    data_provider_(new DataModelProviderImpl(
      ctr_model_max_imps,
      agg_dir_,
      logger_,
      creative_provider))
{
  const Points points {
    Point("0.95"),
    Point("0.75"),
    Point("0.5"),
    Point("0.25")
  };

  ModelBidCostFactory_var bid_cost_model_factory =
    new ModelBidCostFactoryImpl(logger_);
  model_evaluator_bid_cost_ = new ModelEvaluatorBidCostImpl(
    points,
    data_provider_,
    bid_cost_model_factory,
    logger_);

  ModelCtrFactory_var ctr_model_factory = new ModelCtrFactoryImpl(logger_);
  model_evaluator_ctr_ = new ModelEvaluatorCtrImpl(
    ctr_model_trust_imps,
    ctr_model_tag_imps,
    data_provider_,
    ctr_model_factory,
    logger_);
}

void ModelProcessor::activate_object_()
{
  std::ostringstream stream;
  stream << FNS
         << "Start model processor";
  logger_->info(stream.str(), Aspect::MODEL_PROCESSOR);

  thread_ = std::make_unique<std::jthread>([this] () {
    run();
  });
}

void ModelProcessor::deactivate_object_()
{
  if (is_calculation_completed.load())
  {
    return;
  }

  data_provider_->stop();
  model_evaluator_bid_cost_->stop();
  model_evaluator_ctr_->stop();
}

void ModelProcessor::wait_object_()
{
  thread_.reset();

  std::ostringstream stream;
  stream << FNS
         << "Model processor is stopped";
  logger_->info(stream.str(), Aspect::MODEL_PROCESSOR);
}

void ModelProcessor::run() noexcept
{
  try
  {
    {
      std::ostringstream stream;
      stream << FNS
             << "Start process bid cost model";
      logger_->info(stream.str(), Aspect::MODEL_PROCESSOR);
    }

    auto bid_cost_model = model_evaluator_bid_cost_->evaluate();
    if (bid_cost_model)
    {
      save_model(
        *bid_cost_model,
        model_dir_,
        temp_model_dir_,
        model_file_name_);

      std::ostringstream stream;
      stream << FNS
             << "Process bid cost model is success";
      logger_->info(stream.str(), Aspect::MODEL_PROCESSOR);
    }
    else
    {
      if (!active())
      {
        std::ostringstream stream;
        stream << FNS
               << "Process bid cost model is failed";
        logger_->critical(stream.str(), Aspect::MODEL_PROCESSOR);
      }
    }
  }
  catch (const eh::Exception& exc)
  {
    Stream::Error stream;
    stream << FNS
           << "Process bid cost model is failed. Reason: "
           << exc.what();
    logger_->critical(stream.str(), Aspect::MODEL_PROCESSOR);
  }
  catch (...)
  {
    Stream::Error stream;
    stream << FNS
           << "Process bid cost model is failed. Reason: Unknown error";
    logger_->critical(stream.str(), Aspect::MODEL_PROCESSOR);
  }

  if (!active())
  {
    return;
  }

  try
  {
    {
      std::ostringstream stream;
      stream << FNS
             << "Start process ctr model";
      logger_->info(stream.str(), Aspect::MODEL_PROCESSOR);
    }

    auto ctr_model = model_evaluator_ctr_->evaluate();
    if (ctr_model)
    {
      save_model(
        *ctr_model,
        ctr_model_dir_,
        ctr_temp_model_dir_,
        ctr_model_file_name_);

      std::ostringstream stream;
      stream << FNS
             << "Process ctr model is success";
      logger_->info(stream.str(), Aspect::MODEL_PROCESSOR);
    }
    else
    {
      if (!active())
      {
        std::ostringstream stream;
        stream << FNS
               << "Process ctr model is failed";
        logger_->critical(stream.str(), Aspect::MODEL_PROCESSOR);
      }
    }
  }
  catch (const eh::Exception& exc)
  {
    std::ostringstream stream;
    stream << FNS
           << "Process ctr model is failed. Reason: "
           << exc.what();
    logger_->critical(stream.str(), Aspect::MODEL_PROCESSOR);
  }
  catch (...)
  {
    std::ostringstream stream;
    stream << FNS
           << "Process ctr model is failed. Reason: Unknown error";
    logger_->critical(stream.str(), Aspect::MODEL_PROCESSOR);
  }

  is_calculation_completed.store(true);
  try
  {
    deactivate_object();
  }
  catch (...)
  {
  }
}

std::string ModelProcessor::name() noexcept
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
    const std::string name_dir = serialize_time_point(
      current_time,
      "%Y-%m-%d %H:%M:%S");
    const std::string temp_model_dir = temp_dir + "/" + name_dir;
    if (::mkdir(temp_model_dir.c_str(), 0755))
    {
      Stream::Error stream;
      stream << FNS
             << "Can't create directory="
             << temp_model_dir;
      throw Exception(stream);
    }
    Utils::CallOnDestroy call_on_destroy([temp_model_dir] () {
      std::filesystem::remove_all(temp_model_dir);
    });

    const std::string temp_file_path = temp_model_dir + "/" + file_name;
    model.save(temp_file_path);

    const std::string result_model_dir = model_dir + "/" + name_dir;
    if (std::rename(temp_model_dir.c_str(), result_model_dir.c_str()))
    {
      Stream::Error stream;
      stream << FNS
            << "Can't rename dir="
            << temp_model_dir
            << " to dir="
            << result_model_dir;
      throw Exception(stream);
    }

    {
      std::ostringstream stream;
      stream << FNS
             << "Resulting file moved to " + result_model_dir;
      logger_->info(stream.str(), Aspect::MODEL_PROCESSOR);
    }

    return true;
  }
  catch (const eh::Exception& exc)
  {
    Stream::Error stream;
    stream << FNS
         << "Can't save model. Reason: "
         << exc.what();
    logger_->critical(stream.str(), Aspect::MODEL_PROCESSOR);
  }
  catch (...)
  {
    Stream::Error stream;
    stream << FNS
           << "Can't save model. Reason: Unknown error";
    logger_->critical(stream.str(), Aspect::MODEL_PROCESSOR);
  }

  return false;
}

std::string ModelProcessor::serialize_time_point(
  const std::chrono::system_clock::time_point& time,
  const std::string& format)
{
  const std::time_t tt = std::chrono::system_clock::to_time_t(time);
  const std::tm tm = *std::gmtime(&tt);
  std::stringstream stream;
  stream << std::put_time(&tm, format.c_str());
  return stream.str();
}

} // namespace PredictorSvcs::BidCostPredictor