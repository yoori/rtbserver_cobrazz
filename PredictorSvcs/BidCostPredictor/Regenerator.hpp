#ifndef BIDCOSTPREDICTOR_REGENERATOR_HPP
#define BIDCOSTPREDICTOR_REGENERATOR_HPP

// STD
#include <fstream>
#include <list>
#include <utility>
#include <string>
#include <string_view>
#include <thread>

// THIS
#include <LogCommons/LogCommons.hpp>
#include "LogHelper.hpp"
#include "Processor.hpp"
#include "Utils.hpp"

namespace PredictorSvcs::BidCostPredictor
{

namespace LogProcessing = AdServer::LogProcessing;

class Regenerator final : public Processor
{
private:
  using ThreadPtr = std::unique_ptr<std::jthread>;
  using DayTimestamp = LogProcessing::DayTimestamp;
  using Path = std::string;
  using FileInfo = std::pair<DayTimestamp, Path>;
  using ProcessFiles = std::list<FileInfo>;
  using GeneratedPath = Utils::GeneratedPath;
  using ProcessedFiles = std::list<GeneratedPath>;
  using Collector = LogProcessing::BidCostStatInnerCollector;
  using LogTraits = LogProcessing::BidCostStatInnerTraits;
  using Key = typename Collector::KeyT;
  using Data = typename Collector::DataT;
  using ValueT = typename Collector::ValueT;

public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  explicit Regenerator(
    const std::string& input_dir,
    const std::string& output_dir,
    Logger* logger);

  std::string name() noexcept override;

protected:
  ~Regenerator() override = default;

private:
  void activate_object_() override;

  void wait_object_() override;

  void run() noexcept;

  void regenerate(
    const ProcessFiles& process_files,
    const std::string& output_dir,
    const std::string& prefix);

  void save_file(
    const std::string& output_dir,
    const std::string& prefix,
    const LogProcessing::DayTimestamp& date,
    Collector& collector,
    ProcessedFiles& processed_files);

  void remove_processed_files(
    const ProcessedFiles& processed_files) noexcept;

  void clear_directory(const std::string& directory) noexcept;

private:
  const std::string input_dir_;

  const std::string output_dir_;

  const std::string prefix_;

  const Logger_var logger_;

  ThreadPtr thread_;
};

} // namespace PredictorSvcs::BidCostPredictor

#endif //BIDCOSTPREDICTOR_REGENERATOR_HPP
