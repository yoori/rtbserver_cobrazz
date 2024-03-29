#ifndef AGGREGATORMULTYTHREAD_HPP
#define AGGREGATORMULTYTHREAD_HPP

// STD
#include <deque>
#include <list>
#include <map>
#include <queue>
#include <vector>

// THIS
#include <eh/Exception.hpp>
#include <Commons/DelegateTaskGoal.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/Time.hpp>
#include <LogCommons/BidCostStat.hpp>
#include <LogCommons/LogCommons.hpp>
#include "ActiveObjectObserver.hpp"
#include "LogHelper.hpp"
#include "Persantage.hpp"
#include "PoolCollector.hpp"
#include "Processor.hpp"
#include "ShutdownManager.hpp"
#include "Utils.hpp"

namespace Aspect
{
extern const char* AGGREGATOR;
}

namespace PredictorSvcs
{
namespace BidCostPredictor
{

namespace LogProcessing = AdServer::LogProcessing;

class AggregatorMultyThread final :
  public Processor,
  public virtual ReferenceCounting::AtomicImpl,
  private ActiveObjectDelegate
{
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
  using PriorityQueue =
    std::priority_queue<
      DayTimestamp,
      std::deque<DayTimestamp>,
      std::greater<DayTimestamp>
    >;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  static constexpr std::uint8_t COUNT_THREADS = 3;

  enum class Addressee
  {
    Calculator,
    Writer
  };

  enum class ThreadID
  {
    Read = 0,
    Calculate,
    Write
  };

public:
  explicit AggregatorMultyThread(
    const std::size_t max_process_files,
    const std::size_t dump_max_size,
    const std::string& input_dir,
    const std::string& output_dir,
    Logging::Logger* logger);

  ~AggregatorMultyThread() override;

  void start() override;

  void wait() noexcept override;

  void stop() noexcept override;

  const char* name() noexcept override;

private:
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

  void do_stop(
    const Addressee addresee) noexcept;

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
        Aspect::AGGREGATOR);
      return false;
    }
  }

  Generics::TaskRunner& get_task_runner(const ThreadID id) noexcept;

private:
  const std::size_t max_process_files_;

  const std::size_t dump_max_size_;

  const std::string input_dir_;

  const std::string output_dir_;

  const std::string prefix_stat_;

  const std::string prefix_agg_;

  Logging::Logger_var logger_;

  ActiveObjectObserver_var observer_;

  std::vector<Generics::TaskRunner_var> task_runners_;
  // Read thread
  Persantage persantage_;
  // Read thread
  std::size_t count_process_file_ = 0;
  // Read thread
  bool is_read_stoped_ = false;
  // Read thread
  InputFiles input_files_;
  // Calculate thread
  ProcessedFiles_var processed_files_;
  // Calculate thread
  Collector agg_collector_;
  // Calculate thread
  PriorityQueue priority_queue_;
  // Calculate thread
  std::size_t record_count = 0;
  // Write thread
  ResultFiles result_files_;

  bool is_running_ = false;

  ShutdownManager shutdown_manager_;

  bool is_success_completed_ = false;
};

} // namespace BidCostPredictor
} // namespace PredictorSvcs

#endif //AGGREGATORMULTYTHREAD_HPP