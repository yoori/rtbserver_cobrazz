#ifndef BIDCOSTPREDICTOR_REAGGREGATOR_HPP
#define BIDCOSTPREDICTOR_REAGGREGATOR_HPP

// STD
#include <list>
#include <map>
#include <thread>

// THIS
#include <LogCommons/BidCostStat.hpp>
#include <LogCommons/LogCommons.hpp>
#include <LogCommons/LogCommons.ipp>
#include <PredictorSvcs/BidCostPredictor/LogHelper.hpp>
#include <PredictorSvcs/BidCostPredictor/Persantage.hpp>
#include <PredictorSvcs/BidCostPredictor/Processor.hpp>
#include <PredictorSvcs/BidCostPredictor/Utils.hpp>

namespace PredictorSvcs::BidCostPredictor
{

namespace LogProcessing = AdServer::LogProcessing;

class Reaggregator final : public Processor
{
private:
  using ThreadPtr = std::unique_ptr<std::jthread>;
  using DayTimestamp = LogProcessing::DayTimestamp;
  using Path = Utils::Path;
  using AggregatedFiles = std::multimap<DayTimestamp, Path>;
  using ProcessedFiles = std::list<Path>;
  using ResultFile = Utils::GeneratedPath;
  using Collector = AdServer::LogProcessing::BidCostStatInnerCollector;
  using LogTraits = AdServer::LogProcessing::BidCostStatInnerTraits;

public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  explicit Reaggregator(
    const std::string& input_dir,
    const std::string& output_dir,
    Logger* logger);

  std::string name() noexcept override;

protected:
  ~Reaggregator() override;

private:
  void activate_object_() override;

  void wait_object_() override;

  void run() noexcept;

  void reaggregate(
    const std::string& input_dir,
    const std::string& output_dir,
    const std::string& prefix);

  void remove_unique(AggregatedFiles& files);

  void process_date(
    const DayTimestamp& date,
    AggregatedFiles& aggregated_files,
    Collector& collector,
    ProcessedFiles& processed_files) noexcept;

  void dump_file(
    const Path& output_dir,
    const std::string& prefix,
    const DayTimestamp& date,
    Collector& collector,
    ResultFile& result_file);

private:
  const std::string input_dir_;

  const std::string output_dir_;

  const std::string prefix_;

  const Logger_var logger_;

  Persantage persantage_;

  ThreadPtr thread_;
};

} // namespace PredictorSvcs::BidCostPredictor

#endif //BIDCOSTPREDICTOR_REAGGREGATOR_HPP
