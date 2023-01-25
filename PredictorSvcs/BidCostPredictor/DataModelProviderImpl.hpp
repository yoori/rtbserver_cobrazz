#ifndef BIDCOSTPREDICTOR_DATAPROVIDERWINIMPL_HPP
#define BIDCOSTPREDICTOR_DATAPROVIDERWINIMPL_HPP

// STD
#include <string>
#include <vector>

// THIS
#include <Commons/DelegateTaskGoal.hpp>
#include <Generics/TaskRunner.hpp>
#include <LogCommons/BidCostStat.hpp>
#include <Logger/Logger.hpp>
#include "ActiveObjectObserver.hpp"
#include "DataModelProvider.hpp"
#include "LogHelper.hpp"
#include "Persantage.hpp"
#include "ShutdownManager.hpp"
#include "PoolCollector.hpp"

namespace Aspect
{
extern const char* DATA_PROVIDER;
}

namespace PredictorSvcs
{
namespace BidCostPredictor
{

namespace LogProcessing = AdServer::LogProcessing;

class DataModelProviderImpl final :
  public DataModelProvider,
  public virtual ReferenceCounting::AtomicImpl,
  private ActiveObjectDelegate
{
  using DayTimestamp = LogProcessing::DayTimestamp;

  using Collector = LogProcessing::BidCostStatInnerCollector;
  using LogTraits = LogProcessing::BidCostStatInnerTraits;

  using Path = std::string;
  using AggregatedFiles = std::list<Path>;

  using Key = typename HelpCollector::Key;
  using Url_var = typename Key::Url_var;
  using Url = std::string_view;
  using UrlHash = std::unordered_map<Url, Url_var>;

  using Imps = typename HelpCollector::Imps;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  static constexpr std::uint8_t COUNT_THREADS = 3;

  enum class ThreadID
  {
    Read = 0,
    Calculate,
    Clean
  };

public:
  DataModelProviderImpl(
    const Imps max_imps,
    const std::string& input_dir,
    Logging::Logger* logger);

  ~DataModelProviderImpl() override;

  bool load(HelpCollector& help_collector) noexcept override;

  void stop() noexcept override;

private:
  void start();

  void wait() noexcept;

  void do_read() noexcept;

  void do_calculate(Collector& temp_collector) noexcept;

  void do_stop() noexcept;

  void do_clean(Collector& collector) noexcept;

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
    {
      shutdown_manager_.stop();
      return false;
    }

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
              Aspect::DATA_PROVIDER);
      return false;
    }
  }

private:
  const std::string input_dir_;

  Logging::Logger_var logger_;

  ActiveObjectObserver_var observer_;

  const std::string prefix_;

  std::vector<Generics::TaskRunner_var> task_runners_;

  //PoolCollector<Collector, 1000000> pool_collector_;

  ShutdownManager shutdown_manager_;
  // Read thread
  AggregatedFiles aggregated_files_;
  // Read thread
  Persantage persantage_;
  // Calculate thread
  HelpCollector help_collector_;
  // Calculate thread
  UrlHash url_hash_;

  bool is_read_stoped_ = false;

  bool is_running_ = false;

  bool is_success_ = false;

  std::atomic<bool> is_interrupted_{false};
};

} // namespace BidCostPredictor
} // namespace PredictorSvcs

#endif //BIDCOSTPREDICTOR_DATAPROVIDERWINIMPL_HPP
