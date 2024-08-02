#ifndef BIDCOSTPREDICTOR_DATAPROVIDERWINIMPL_HPP
#define BIDCOSTPREDICTOR_DATAPROVIDERWINIMPL_HPP

// STD
#include <deque>
#include <string>
#include <vector>

// UNIXCOMMONS
#include <Generics/TaskRunner.hpp>
#include <Logger/Logger.hpp>

// THIS
#include <Commons/DelegateTaskGoal.hpp>
#include <LogCommons/BidCostStat.hpp>
#include "ActiveObjectObserver.hpp"
#include "CreativeProvider.hpp"
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
  using CcIdToCategories = CreativeProvider::CcIdToCategories;
  using DayTimestamp = LogProcessing::DayTimestamp;
  using Path = std::string;
  using AggregatedFiles = std::list<Path>;
  using Key = typename HelpCollector::Key;
  using UrlPtr = typename Key::UrlPtr;
  using Url = std::string_view;
  using UrlHash = std::unordered_map<Url, UrlPtr>;
  using Imps = typename HelpCollector::Imps;

  struct ReadData final
  {
    using Key = LogProcessing::BidCostStatInnerKey;
    using Data = LogProcessing::BidCostStatInnerData;

    ReadData(Key&& key, Data&& data)
      : key(std::move(key)),
        data(std::move(data))
    {
    }

    Key key;
    Data data;
  };
  using ReadCollector = std::deque<ReadData>;
  using ReadCollectorPtr = std::shared_ptr<ReadCollector>;

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
    Logger* logger,
    CreativeProvider* creative_provider);

  bool load(HelpCollector& help_collector) noexcept override;

  void stop() noexcept override;

protected:
  ~DataModelProviderImpl() override;

private:
  void run();

  void wait() noexcept;

  void clear() noexcept;

  void do_read() noexcept;

  void do_calculate(ReadCollectorPtr& temp_collector) noexcept;

  void do_stop() noexcept;

  void do_clean(ReadCollectorPtr& temp_collector) noexcept;

  template<ConceptMemberPtr MemPtr, class ...Args>
  bool post_task(
    const ThreadID id,
    MemPtr mem_ptr,
    Args&& ...args) noexcept;

  ReadCollectorPtr load(const Path& file_path);

private:
  const std::string input_dir_;

  const Logger_var logger_;

  const CreativeProvider_var creative_provider_;

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

  CcIdToCategories cc_id_to_categories_;
};

} // namespace PredictorSvcs::BidCostPredictor

#endif //BIDCOSTPREDICTOR_DATAPROVIDERWINIMPL_HPP
