#ifndef BIDCOSTPREDICTOR_AGGREGATOR_HPP
#define BIDCOSTPREDICTOR_AGGREGATOR_HPP

// STD
#include <list>
#include <map>
#include <queue>
#include <vector>
#include <thread>

// UNIXCOMMONS
#include <eh/Exception.hpp>
#include <Generics/Time.hpp>

// THIS
#include <LogCommons/BidCostStat.hpp>
#include <LogCommons/LogCommons.hpp>
#include <LogCommons/LogCommons.ipp>
#include "LogHelper.hpp"
#include "Persantage.hpp"
#include "Processor.hpp"
#include "Utils.hpp"

namespace PredictorSvcs::BidCostPredictor
{

namespace LogProcessing = AdServer::LogProcessing;

class Aggregator final : public Processor
{
private:
  using ThreadPtr = std::unique_ptr<std::jthread>;
  using Path = Utils::Path;
  using ProcessFiles = std::list<Path>;
  using ProcessedFiles = std::list<Path>;
  using GeneratedPath = Utils::GeneratedPath;
  using ResultFiles = std::list<GeneratedPath>;
  using Collector = LogProcessing::BidCostStatCollector;
  using LogTraits = LogProcessing::BidCostStatTraits;
  using KeyCollector = LogProcessing::BidCostStatKey;
  using CollectorInner = LogProcessing::BidCostStatInnerCollector;
  using LogInnerTraits = LogProcessing::BidCostStatInnerTraits;
  using KeyCollectorInner = LogProcessing::BidCostStatInnerKey;
  using DayTimestamp = LogProcessing::DayTimestamp;
  using PriorityQueue = std::priority_queue<
    DayTimestamp,
    std::vector<DayTimestamp>,
    std::greater<DayTimestamp>>;

public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  explicit Aggregator(
    const std::size_t max_process_files,
    const std::size_t dump_max_size,
    const std::string& input_dir,
    const std::string& output_dir,
    Logger* logger);

  std::string name() noexcept override;

protected:
  ~Aggregator() override;

private:
  void activate_object_() override;

  void wait_object_() override;

  void run() noexcept;

  void aggregate();

  void process_files(const ProcessFiles& files);

  void dump_file(
    const Path& output_dir,
    const std::string& prefix,
    const DayTimestamp& date,
    const CollectorInner& collector,
    ResultFiles& resultFiles);

  std::size_t merge(
    Collector& collector,
    Collector& temp_collector,
    PriorityQueue& priority_queue);

private:
  const std::size_t max_process_files_;

  const std::size_t dump_max_size_;

  const std::string input_dir_;

  const std::string output_dir_;

  const std::string prefix_stat_;

  const std::string prefix_agg_;

  const Logger_var logger_;

  Persantage persantage_;

  ThreadPtr thread_;
};

} // namespace PredictorSvcs::BidCostPredictor

#endif //BIDCOSTPREDICTOR_AGGREGATOR_HPP
