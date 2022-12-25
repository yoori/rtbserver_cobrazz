#ifndef RTBSERVER_COBRAZZ_REAGGREGATORMULTYTHREAD_HPP
#define RTBSERVER_COBRAZZ_REAGGREGATORMULTYTHREAD_HPP

// STD
#include <list>
#include <map>
#include <memory>
#include <functional>

// THIS
#include <Commons/DelegateTaskGoal.hpp>
#include <Generics/TaskRunner.hpp>
#include <LogCommons/BidCostStat.hpp>
#include "LogHelper.hpp"
#include "PoolCollector.hpp"
#include "Processor.hpp"
#include "ShutdownManager.hpp"
#include "Utils.hpp"

namespace Aspect
{
extern const char* REAGGREGATOR;
}

namespace PredictorSvcs
{
namespace BidCostPredictor
{

namespace LogProcessing = AdServer::LogProcessing;

class ReaggregatorMultyThread final :
        public Processor,
        public virtual ReferenceCounting::AtomicImpl,
        public Generics::ActiveObjectCallback
{
  using DayTimestamp = LogProcessing::DayTimestamp;

  using Path = Utils::Path;
  using AggregatedFiles = std::multimap<DayTimestamp, Path>;
  using ProcessedFiles = std::list<Path>;
  using ResultFile = Utils::GeneratedPath;

  using Collector = AdServer::LogProcessing::BidCostStatInnerCollector;
  using LogTraits = AdServer::LogProcessing::BidCostStatInnerTraits;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  static constexpr std::uint8_t COUNT_THREADS = 4;

  enum class Addressee
  {
    Calculator,
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
    Clean
  };

  struct PersantageInfo
  {
    PersantageInfo() = default;
    ~PersantageInfo() = default;

    std::size_t total_file_count = 0;
    std::size_t current_file_number = 0;
    std::size_t counter_percentage = 0;
  };

public:
  explicit ReaggregatorMultyThread(
          const std::string& input_dir,
          const std::string& output_dir,
          const Logging::Logger_var& logger);

  ~ReaggregatorMultyThread() override;

  void start() override;

  void stop() noexcept override;

  void wait() noexcept override;

  const char* name() noexcept override;

private:
  void reaggregate();

  void removeUnique(AggregatedFiles& files);

  void doRead() noexcept;

  void doFlush(
          const LogProcessing::DayTimestamp& date) noexcept;

  void doMerge(
          Collector& temp_collector,
          const std::string& file_path) noexcept;

  void doWrite(
          const ProcessedFiles& processed_files,
          Collector& collector,
          const LogProcessing::DayTimestamp& date) noexcept;

  void doStop(
          const Addressee addresee) noexcept;

  void dumpFile(
          const Path& output_dir,
          const std::string& prefix,
          const DayTimestamp& date,
          Collector& collector,
          ResultFile& result_file);

  void doClean(
          Collector& collector,
          const PoolType poolType);

  void report_error(
          Severity severity,
          const String::SubString& description,
          const char* error_code = 0) noexcept override;

  template<class MemPtr,
           class ...Args>
  std::enable_if_t<
      std::is_member_function_pointer_v<MemPtr>,
      bool>
  postTask(const ThreadID id,
          MemPtr mem_ptr,
          Args&& ...args) noexcept
  {
    if (task_runners_.size() <= static_cast<std::size_t>(id))
      return false;

    try
    {
      task_runners_[static_cast<std::size_t>(id)]->enqueue_task(
              AdServer::Commons::make_delegate_task(
                      std::bind(mem_ptr,
                                this,
                                std::forward<Args>(args)...)));
      return true;
    }
    catch (const eh::Exception& exc)
    {
      Stream::Error ostr;
      ostr << __PRETTY_FUNCTION__
           << ": Can't enqueue_task"
           << " Reason: "
           << exc.what();
      logger_->critical(
              ostr.str(),
              Aspect::REAGGREGATOR);
      shutdown_manager_.stop();
      return false;
    }
  }

private:
  const std::string input_dir_;

  const std::string output_dir_;

  const std::string prefix_;

  Logging::Logger_var logger_;

  std::vector<Generics::TaskRunner_var> task_runners_;

  PoolCollector<Collector, 1000000> pool_collector_;

  PoolCollector<Collector, 100000> pool_temp_collector_;
  // Calculate thread
  AggregatedFiles aggregated_files_;
  // Calculate thread
  Collector collector_;
  // Calculate thread
  ProcessedFiles processed_files_;
  // Read thread
  DayTimestamp current_date_;
  // Read thread
  PersantageInfo persantage_info_;
  // Read thread
  bool is_read_stoped_ = false;

  bool is_running_ = false;

  ShutdownManager shutdown_manager_;
};

} // namespace BidCostPredictor
} // namespace PredictorSvcs

#endif //RTBSERVER_COBRAZZ_REAGGREGATORMULTYTHREAD_HPP