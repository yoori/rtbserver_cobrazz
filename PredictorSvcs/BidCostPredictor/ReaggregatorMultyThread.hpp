#ifndef BIDCOSTPREDICTOR_REAGGREGATORMULTYTHREAD_HPP
#define BIDCOSTPREDICTOR_REAGGREGATORMULTYTHREAD_HPP

// STD
#include <list>
#include <map>
#include <memory>
#include <functional>
#include <thread>

// UNIXCOMMONS
#include <Generics/TaskRunner.hpp>

// THIS
#include <Commons/DelegateTaskGoal.hpp>

#include <LogCommons/BidCostStat.hpp>
#include <PredictorSvcs/BidCostPredictor/ActiveObjectObserver.hpp>
#include <PredictorSvcs/BidCostPredictor/LogHelper.hpp>
#include <PredictorSvcs/BidCostPredictor/Persantage.hpp>
#include <PredictorSvcs/BidCostPredictor/PoolCollector.hpp>
#include <PredictorSvcs/BidCostPredictor/Processor.hpp>
#include <PredictorSvcs/BidCostPredictor/ShutdownManager.hpp>
#include <PredictorSvcs/BidCostPredictor/Utils.hpp>

namespace PredictorSvcs::BidCostPredictor
{

namespace LogProcessing = AdServer::LogProcessing;

class ReaggregatorMultyThread final : public Processor
{
private:
  using ThreadPtr = std::unique_ptr<std::jthread>;
  using DayTimestamp = LogProcessing::DayTimestamp;
  using Path = Utils::Path;
  using AggregatedFiles = std::multimap<DayTimestamp, Path>;
  using ProcessedFiles = std::list<Path>;
  using ProcessedFiles_var = std::shared_ptr<ProcessedFiles>;
  using ResultFile = Utils::GeneratedPath;
  using Collector = LogProcessing::BidCostStatInnerCollector;
  using LogTraits = LogProcessing::BidCostStatInnerTraits;

  enum class Addressee
  {
    Calculator = 1,
    Writer
  };

  enum class PoolType
  {
    Temp,
    Merge
  };

  enum class ThreadID
  {
    Read = 0,
    Calculate,
    Write,
    Clean,
    MAX_NUMBER
  };

public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  explicit ReaggregatorMultyThread(
    const std::string& input_dir,
    const std::string& output_dir,
    Logger* logger);

  std::string name() noexcept override;

protected:
  ~ReaggregatorMultyThread() override;

private:
  void activate_object_() override;

  void deactivate_object_() override;

  void wait_object_() override;

  void reaggregate();

  void remove_unique(AggregatedFiles& files);

  void do_read() noexcept;

  void do_flush(
    const LogProcessing::DayTimestamp& date) noexcept;

  void do_merge(
    Collector& temp_collector,
    const std::string& file_path,
    const bool need_task_read) noexcept;

  void do_write(
    const ProcessedFiles_var& processed_files,
    Collector& collector,
    const LogProcessing::DayTimestamp& date) noexcept;

  void do_stop(
    const Addressee addresee) noexcept;

  void do_clean(
    Collector& collector,
    const PoolType poolType) noexcept;

  void dump_file(
    const Path& output_dir,
    const std::string& prefix,
    const DayTimestamp& date,
    Collector& collector,
    ResultFile& result_file);

  template<ConceptMemberPtr MemPtr, class ...Args>
  bool post_task(
    const ThreadID id,
    MemPtr mem_ptr,
    Args&& ...args) noexcept;

private:
  const std::string input_dir_;

  const std::string output_dir_;

  const std::string prefix_;

  const Logger_var logger_;

  PoolCollector<Collector, 1000000> pool_collector_;

  PoolCollector<Collector, 100000> pool_temp_collector_;

  std::atomic<bool> is_success_completed_ = false;

  ActiveObjectObserver_var observer_;
  // Calculate thread
  AggregatedFiles aggregated_files_;
  // Calculate thread
  Collector collector_;
  // Calculate thread
  ProcessedFiles_var processed_files_;
  // Read thread
  DayTimestamp current_date_;
  // Read thread
  Persantage persantage_;
  // Read thread
  bool is_read_stoped_ = false;

  std::vector<Generics::TaskRunner_var> task_runners_;

  ThreadPtr helper_thread_;
};

} // namespace PredictorSvcs::BidCostPredictor

#endif //BIDCOSTPREDICTOR_REAGGREGATORMULTYTHREAD_HPP