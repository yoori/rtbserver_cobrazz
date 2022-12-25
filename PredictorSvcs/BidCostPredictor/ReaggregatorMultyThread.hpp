#ifndef BIDCOSTPREDICTOR_REAGGREGATORMULTYTHREAD_HPP
#define BIDCOSTPREDICTOR_REAGGREGATORMULTYTHREAD_HPP

// STD
#include <list>
#include <map>
#include <memory>
#include <functional>

// THIS
#include <Commons/DelegateTaskGoal.hpp>
#include <Generics/TaskRunner.hpp>
#include <LogCommons/BidCostStat.hpp>
#include "ActiveObjectObserver.hpp"
#include "LogHelper.hpp"
#include "Persantage.hpp"
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
  private ActiveObjectDelegate
{
  using DayTimestamp = LogProcessing::DayTimestamp;

  using Path = Utils::Path;
  using AggregatedFiles = std::multimap<DayTimestamp, Path>;
  using ProcessedFiles = std::list<Path>;
  using ProcessedFiles_var = std::shared_ptr<ProcessedFiles>;
  using ResultFile = Utils::GeneratedPath;

  using Collector = LogProcessing::BidCostStatInnerCollector;
  using LogTraits = LogProcessing::BidCostStatInnerTraits;

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

public:
  explicit ReaggregatorMultyThread(
    const std::string& input_dir,
    const std::string& output_dir,
    Logging::Logger* logger);

  ~ReaggregatorMultyThread() override;

  void start() override;

  void stop() noexcept override;

  void wait() noexcept override;

  const char* name() noexcept override;

private:
  void reaggregate();

  void remove_unique(AggregatedFiles& files);

  void do_read() noexcept;

  void do_flush(
    const LogProcessing::DayTimestamp& date) noexcept;

  void do_merge(
    Collector& temp_collector,
    const std::string& file_path) noexcept;

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

  void report_error(
    Severity severity,
    const String::SubString& description,
    const char* error_code = 0) noexcept override;

  template<class MemPtr,
           class ...Args>
  std::enable_if_t<
    std::is_member_function_pointer_v<MemPtr>,
    bool>
  post_task(
    const ThreadID id,
    MemPtr mem_ptr,
    Args&& ...args) noexcept
  {
    if (task_runners_.size() <= static_cast<std::size_t>(id))
      return false;

    try
    {
      task_runners_[static_cast<std::size_t>(id)]->enqueue_task(
        AdServer::Commons::make_delegate_task(
          std::bind(
            mem_ptr,
            this,
            std::forward<Args>(args)...)));
      return true;
    }
    catch (const eh::Exception& exc)
    {
      shutdown_manager_.stop();
      std::stringstream stream;
      stream << __PRETTY_FUNCTION__
             << ": Can't enqueue_task"
             << " Reason: "
             << exc.what();
      logger_->critical(
        stream.str(),
        Aspect::REAGGREGATOR);

      return false;
    }
  }

private:
  const std::string input_dir_;

  const std::string output_dir_;

  const std::string prefix_;

  Logging::Logger_var logger_;

  ActiveObjectObserver_var observer_;

  std::vector<Generics::TaskRunner_var> task_runners_;

  PoolCollector<Collector, 1000000> pool_collector_;

  PoolCollector<Collector, 100000> pool_temp_collector_;
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

  bool is_running_ = false;

  ShutdownManager shutdown_manager_;

  bool is_success_completed_ = false;
};

} // namespace BidCostPredictor
} // namespace PredictorSvcs

#endif //BIDCOSTPREDICTOR_REAGGREGATORMULTYTHREAD_HPP