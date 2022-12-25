#ifndef BIDCOSTPREDICTOR_MODELPROCESSOR_HPP
#define BIDCOSTPREDICTOR_MODELPROCESSOR_HPP

// STD
#include <atomic>
#include <string>

// THIS
#include <LogCommons/LogCommons.hpp>
#include "DataModelProvider.hpp"
#include "ModelEvaluator.hpp"
#include "ModelManager.hpp"
#include "Processor.hpp"
#include "ShutdownManager.hpp"

namespace PredictorSvcs
{
namespace BidCostPredictor
{

namespace LogProcessing = AdServer::LogProcessing;

class ModelProcessor :
  public Processor,
  public virtual ReferenceCounting::AtomicImpl
{
  using FixedNumber = LogProcessing::FixedNumber;
  using Points = std::vector<FixedNumber>;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  ModelProcessor(
    const std::string& model_dir,
    const std::string& model_file_name,
    const std::string& temp_dir,
    const std::string& ctr_model_dir,
    const std::string& ctr_model_file_name,
    const std::string& ctr_temp_dir,
    const std::string& agg_dir,
    Logging::Logger* logger);

  ~ModelProcessor() override;

  void start() override;

  void wait() noexcept override;

  void stop() noexcept override;

  const char* name() noexcept override;

private:
  bool save_model(
    const ModelManager& model,
    const std::string& model_dir,
    const std::string& temp_model_dir,
    const std::string& file_name) noexcept;

  std::string serialize_time_point(
    const std::chrono::system_clock::time_point& time,
    const std::string& format);

private:
  const std::string model_dir_;

  const std::string model_file_name_;

  const std::string temp_model_dir_;

  const std::string ctr_model_dir_;

  const std::string ctr_model_file_name_;

  const std::string ctr_temp_model_dir_;

  const std::string agg_dir_;

  Logging::Logger_var logger_;

  DataModelProvider_var data_provider_;

  ModelEvaluatorBidCost_var model_evaluator_bid_cost_;

  ModelEvaluatorCtr_var model_evaluator_ctr_;

  std::atomic<bool> is_stopped_{false};

  ShutdownManager shutdown_manager_;
};

} // namespace BidCostPredictor
} // namespace PredictorSvcs

#endif //BIDCOSTPREDICTOR_MODELPROCESSOR_HPP
