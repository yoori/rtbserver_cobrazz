#ifndef BIDCOSTPREDICTOR_AGGREGATOR_HPP
#define BIDCOSTPREDICTOR_AGGREGATOR_HPP

// STD
#include <list>
#include <map>
#include <queue>
#include <vector>

// THIS
#include <eh/Exception.hpp>
#include <Generics/Time.hpp>
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

class Aggregator final :
  public Processor,
  public virtual ReferenceCounting::AtomicImpl
{
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
  using PriorityQueue =
    std::priority_queue<
      DayTimestamp,
      std::vector<DayTimestamp>,
      std::greater<DayTimestamp>
    >;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  explicit Aggregator(
    const std::size_t max_process_files,
    const std::size_t dump_max_size,
    const std::string& input_dir,
    const std::string& output_dir,
    Logging::Logger* logger);

  ~Aggregator() override = default;

  void start() override;

  void stop() noexcept override;

  void wait() noexcept override;

  const char* name() noexcept override;

private:
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

  Logging::Logger_var logger_;

  Persantage persantage_;
};

} // namespace BidCostPredictor
} // namespace PredictorSvcs

#endif //BIDCOSTPREDICTOR_AGGREGATOR_HPP
