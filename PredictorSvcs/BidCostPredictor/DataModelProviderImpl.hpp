#ifndef BIDCOSTPREDICTOR_DATAPROVIDERWINIMPL_HPP
#define BIDCOSTPREDICTOR_DATAPROVIDERWINIMPL_HPP

// STD
#include <string>
#include <vector>

// UNIXCOMMONS
#include <Generics/TaskRunner.hpp>
#include <Logger/Logger.hpp>

// THIS
#include <Commons/DelegateTaskGoal.hpp>
#include <LogCommons/BidCostStat.hpp>
#include "ActiveObjectObserver.hpp"
#include "DataModelProvider.hpp"
#include "LogHelper.hpp"
#include "Persantage.hpp"
#include "PoolCollector.hpp"
#include "ShutdownManager.hpp"

namespace PredictorSvcs::BidCostPredictor
{

namespace LogProcessing = AdServer::LogProcessing;

class DataModelProviderImpl final :
  public DataModelProvider,
  public ReferenceCounting::AtomicImpl
{
private:
  using DayTimestamp = LogProcessing::DayTimestamp;
  using Collector = LogProcessing::BidCostStatInnerCollector;
  using LogTraits = LogProcessing::BidCostStatInnerTraits;
  using Path = std::string;
  using AggregatedFiles = std::list<Path>;
  using Key = typename HelpCollector::Key;
  using UrlPtr = typename Key::UrlPtr;
  using Url = std::string_view;
  using UrlHash = std::unordered_map<Url, UrlPtr>;
  using Imps = typename HelpCollector::Imps;

  enum class ThreadID
  {
    Read = 0,
    Calculate,
    Clean,
    MAX_NUMBER
  };

public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  DataModelProviderImpl(
    const Imps max_imps,
    const std::string& input_dir,
    Logger* logger);

  bool load(HelpCollector& help_collector) noexcept override;

  void stop() noexcept override;

protected:
  ~DataModelProviderImpl() override;

private:
  void run();

  void wait() noexcept;

  void clear() noexcept;

  void do_read() noexcept;

  void do_calculate(Collector& temp_collector) noexcept;

  void do_stop() noexcept;

  void do_clean(Collector& collector) noexcept;

  template<ConceptMemberPtr MemPtr, class ...Args>
  bool post_task(
    const ThreadID id,
    MemPtr mem_ptr,
    Args&& ...args) noexcept;

private:
  const std::string input_dir_;

  const Logger_var logger_;

  const std::string prefix_;

  ShutdownManager shutdown_manager_;

  ActiveObjectObserver_var observer_;

  std::atomic<bool> is_idle_ = true;

  std::atomic<bool> is_load_success_ = false;

  std::atomic<bool> is_interrupted_ = false;
  // Read thread
  AggregatedFiles aggregated_files_;
  // Read thread
  Persantage persantage_;
  // Calculate thread
  HelpCollector help_collector_;
  // Calculate thread
  UrlHash url_hash_;

  std::vector<Generics::TaskRunner_var> task_runners_;

  //PoolCollector<Collector, 1000000> pool_collector_;
};

} // namespace PredictorSvcs::BidCostPredictor

#endif //BIDCOSTPREDICTOR_DATAPROVIDERWINIMPL_HPP
