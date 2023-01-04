#ifndef BIDCOSTPREDICTOR_REAGGREGATOR_HPP
#define BIDCOSTPREDICTOR_REAGGREGATOR_HPP

// STD
#include <list>
#include <map>

// THIS
#include <LogCommons/BidCostStat.hpp>
#include <LogCommons/LogCommons.hpp>
#include <LogCommons/LogCommons.ipp>
#include "LogHelper.hpp"
#include "Persantage.hpp"
#include "Processor.hpp"
#include "Utils.hpp"

namespace PredictorSvcs
{
namespace BidCostPredictor
{

namespace LogProcessing = AdServer::LogProcessing;

class Reaggregator final :
        public Processor,
        public virtual ReferenceCounting::AtomicImpl
{
  using DayTimestamp = LogProcessing::DayTimestamp;

  using Path = Utils::Path;
  using AggregatedFiles = std::multimap<DayTimestamp, Path>;
  using ProcessedFiles = std::list<Path>;
  using ResultFile = Utils::GeneratedPath;

  using Collector = AdServer::LogProcessing::BidCostStatInnerCollector;
  using LogTraits = AdServer::LogProcessing::BidCostStatInnerTraits;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  explicit Reaggregator(
          const std::string& input_dir,
          const std::string& output_dir,
          const Logging::Logger_var& logger);

  ~Reaggregator() = default;

  void start();

  void stop() noexcept;

  void wait() noexcept;

  const char* name() noexcept override;

private:
  void reaggregate(
          const std::string& input_dir,
          const std::string& output_dir,
          const std::string& prefix);

  void removeUnique(AggregatedFiles& files);

  void processDate(
          const DayTimestamp& date,
          AggregatedFiles& aggregated_files,
          Collector& collector,
          ProcessedFiles& processed_files) noexcept;

  void dumpFile(
          const Path& output_dir,
          const std::string& prefix,
          const DayTimestamp& date,
          Collector& collector,
          ResultFile& result_file);

private:
  const std::string input_dir_;

  const std::string output_dir_;

  const std::string prefix_;

  Logging::Logger_var logger_;

  Persantage persantage_;
};

} // namespace BidCostPredictor
} // namespace PredictorSvcs

#endif //BIDCOSTPREDICTOR_REAGGREGATOR_HPP
