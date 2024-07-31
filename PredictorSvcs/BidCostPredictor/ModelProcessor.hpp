#ifndef BIDCOSTPREDICTOR_MODELPROCESSOR_HPP
#define BIDCOSTPREDICTOR_MODELPROCESSOR_HPP

// STD
#include <atomic>
#include <string>
#include <thread>

// THIS
#include <LogCommons/LogCommons.hpp>
#include "DataModelProvider.hpp"
#include "ModelCtr.hpp"
#include "ModelEvaluator.hpp"
#include "ModelManager.hpp"
#include "Processor.hpp"
#include "ShutdownManager.hpp"

namespace PredictorSvcs::BidCostPredictor
{

namespace LogProcessing = AdServer::LogProcessing;

class ModelProcessor final : public Processor
{
private:
  using ThreadPtr = std::unique_ptr<std::jthread>;
  using Point = Types::Point;
  using Points = Types::Points;
  using Imps = Types::Imps;

public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  ModelProcessor(
    const std::string& model_dir,
    const std::string& model_file_name,
    const std::string& temp_dir,
    const std::string& ctr_model_dir,
    const std::string& ctr_model_file_name,
    const std::string& ctr_temp_dir,
    const Imps& ctr_model_max_imps,
    const Imps& ctr_model_trust_imps,
    const Imps& ctr_model_tag_imps,
    const std::string& agg_dir,
    Logger* logger);

  std::string name() noexcept override;

protected:
  ~ModelProcessor() override = default;

private:
  void activate_object_() override;

  void deactivate_object_() override;

  void wait_object_() override;

  void run() noexcept;

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

  const Logger_var logger_;

  const DataModelProvider_var data_provider_;

  ModelEvaluatorBidCost_var model_evaluator_bid_cost_;

  ModelEvaluatorCtr_var model_evaluator_ctr_;

  std::atomic<bool> is_calculation_completed{false};

  ThreadPtr thread_;
};

} // namespace PredictorSvcs::BidCostPredictor

#endif //BIDCOSTPREDICTOR_MODELPROCESSOR_HPP
