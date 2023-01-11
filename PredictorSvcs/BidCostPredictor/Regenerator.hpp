#ifndef BIDCOSTPREDICTOR_REGENERATOR_HPP
#define BIDCOSTPREDICTOR_REGENERATOR_HPP

// STD
#include <fstream>
#include <list>
#include <utility>
#include <string>
#include <string_view>

// THIS
#include <LogCommons/LogCommons.hpp>
#include <LogCommons/LogCommons.ipp>
#include "LogHelper.hpp"
#include "Processor.hpp"
#include "Utils.hpp"

namespace PredictorSvcs
{
namespace BidCostPredictor
{

namespace LogProcessing = AdServer::LogProcessing;

class Regenerator final :
        public Processor,
        public virtual ReferenceCounting::AtomicImpl
{
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

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  Regenerator(
          const std::string& input_dir,
          const std::string& output_dir,
          Logging::Logger* logger);

  ~Regenerator() override = default;

  void start() override;

  void stop() noexcept override;

  void wait() noexcept override;

  const char* name() noexcept override;

private:
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

private:
  const std::string input_dir_;

  const std::string output_dir_;

  const std::string prefix_;

  Logging::Logger_var logger_;
};

} // namespace BidCostPredictor
} // namespace PredictorSvcs

#endif //BIDCOSTPREDICTOR_REGENERATOR_HPP
