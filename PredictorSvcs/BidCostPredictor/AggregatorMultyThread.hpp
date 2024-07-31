#ifndef AGGREGATORMULTYTHREAD_HPP
#define AGGREGATORMULTYTHREAD_HPP

// STD
#include <deque>
#include <list>
#include <map>
#include <queue>
#include <vector>
#include <thread>

// UNIXCOMMONS
#include <eh/Exception.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/Time.hpp>

// THIS
#include <Commons/DelegateTaskGoal.hpp>
#include <LogCommons/BidCostStat.hpp>
#include <LogCommons/LogCommons.hpp>
#include "ActiveObjectObserver.hpp"
#include "LogHelper.hpp"
#include "Persantage.hpp"
#include "PoolCollector.hpp"
#include "Processor.hpp"
#include "ShutdownManager.hpp"
#include "Utils.hpp"

namespace PredictorSvcs::BidCostPredictor
{

namespace LogProcessing = AdServer::LogProcessing;

class AggregatorMultyThread final : public Processor
{
private:
  using ThreadPtr = std::unique_ptr<std::jthread>;
  using TaskRunner_var = Generics::TaskRunner_var;
  using Path = Utils::Path;
  using InputFiles = std::list<Path>;
  using ProcessedFiles = std::list<Path>;
  using ProcessedFiles_var = std::shared_ptr<ProcessedFiles>;
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
    std::deque<DayTimestamp>,
    std::greater<DayTimestamp>>;

  enum class Addressee
  {
    Calculator,
    Writer
  };

  enum class ThreadID
  {
    Read = 0,
    Calculate,
    Write,
    MAX_NUMBER
  };

public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  explicit AggregatorMultyThread(
    const std::size_t max_process_files,
    const std::size_t dump_max_size,
    const std::string& input_dir,
    const std::string& output_dir,
    Logger* logger);

  std::string name() noexcept override;

protected:
  ~AggregatorMultyThread() override;

private:
  void activate_object_() override;

  void deactivate_object_() override;

  void wait_object_() override;

  void aggregate();

  std::size_t merge(
    const Collector& temp_collector,
    Collector& collector,
    PriorityQueue& priority_queue);

  void do_read() noexcept;

  void do_write(
    const CollectorInner& collector,
    const DayTimestamp& date,
    const bool need_add_read) noexcept;
  // Calculate thread
  void do_flush() noexcept;
  // Write thread
  void do_flush(
    const ProcessedFiles_var& processed_files) noexcept;

  void do_merge(
    const Collector& collector,
    const Path& original_file) noexcept;

  void do_stop(const Addressee addresee) noexcept;

  template<ConceptMemberPtr MemPtr, class ...Args>
  bool post_task(
    const ThreadID id,
    MemPtr mem_ptr,
    Args&& ...args) noexcept;

private:
  const std::size_t max_process_files_;

  const std::size_t dump_max_size_;

  const std::string input_dir_;

  const std::string output_dir_;

  const std::string prefix_stat_;

  const std::string prefix_agg_;

  const Logger_var logger_;

  ActiveObjectObserver_var observer_;

  // Read thread
  Persantage persantage_;
  // Read thread
  InputFiles input_files_;
  // Calculate thread
  ProcessedFiles_var processed_files_;
  // Calculate thread
  Collector agg_collector_;
  // Calculate thread
  PriorityQueue priority_queue_;
  // Write thread
  ResultFiles result_files_;

  std::atomic<bool> is_success_completed_ = false;

  std::vector<TaskRunner_var> task_runners_;

  ThreadPtr helper_thread_;
};

} // namespace PredictorSvcs::BidCostPredictor

#endif //AGGREGATORMULTYTHREAD_HPP