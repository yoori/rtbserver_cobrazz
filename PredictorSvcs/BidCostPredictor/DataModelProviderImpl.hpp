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
#include <PredictorSvcs/BidCostPredictor/ActiveObjectObserver.hpp>
#include <PredictorSvcs/BidCostPredictor/CreativeProvider.hpp>
#include <PredictorSvcs/BidCostPredictor/DataModelProvider.hpp>
#include <PredictorSvcs/BidCostPredictor/LogHelper.hpp>
#include <PredictorSvcs/BidCostPredictor/Persantage.hpp>
#include <PredictorSvcs/BidCostPredictor/PoolCollector.hpp>
#include <PredictorSvcs/BidCostPredictor/ShutdownManager.hpp>

namespace PredictorSvcs::BidCostPredictor
{

namespace LogProcessing = AdServer::LogProcessing;

class DataModelProviderImpl final :
  public DataModelProvider,
  public ReferenceCounting::AtomicImpl
{
private:
  using DayTimestamp = LogProcessing::DayTimestamp;
  using Path = std::string;
  using AggregatedFiles = std::list<Path>;
  using UrlPtr = Types::UrlPtr;
  using Url = std::string_view;
  using UrlHash = std::unordered_map<Url, UrlPtr>;
  using Imps = BidCostHelpCollector::Imps;
  using CcIdToCategories = CreativeProvider::CcIdToCategories;

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
    CreativeProvider* creative_provider,
    Logger* logger);

  bool load(BidCostHelpCollector& help_collector) noexcept override;

  bool load(CtrHelpCollector& collector) noexcept override;

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

  bool load(
    BidCostHelpCollector& bid_cost_collector,
    CtrHelpCollector& ctr_collector) noexcept;

private:
  const std::string input_dir_;

  const Logger_var logger_;

  const std::string prefix_;

  CcIdToCategories cc_id_to_categories_;

  ShutdownManager shutdown_manager_;

  ActiveObjectObserver_var observer_;

  std::atomic<bool> is_idle_ = true;

  std::atomic<bool> is_load_success_ = false;

  std::atomic<bool> is_interrupted_ = false;
  // Read thread
  AggregatedFiles aggregated_files_;
  // Read thread
  Persantage persantage_;
  // Read thread
  bool is_read_stoped_ = false;
  // Calculate thread
  BidCostHelpCollector bid_cost_collector_;
  // Calculate thread
  CtrHelpCollector ctr_collector_;
  // Calculate thread
  UrlHash url_hash_;

  std::vector<Generics::TaskRunner_var> task_runners_;
};

} // namespace PredictorSvcs::BidCostPredictor

#endif //BIDCOSTPREDICTOR_DATAPROVIDERWINIMPL_HPP
