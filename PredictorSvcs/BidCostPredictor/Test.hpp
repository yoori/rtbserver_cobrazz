#ifndef BIDCOSTPREDICTOR_TEST_HPP
#define BIDCOSTPREDICTOR_TEST_HPP

// STD
#include <list>

// THIS
#include <Generics/Uncopyable.hpp>
#include <LogCommons/BidCostStat.hpp>
#include <LogCommons/LogCommons.hpp>
#include "Processor.hpp"
#include "LogHelper.hpp"

namespace PredictorSvcs
{
namespace BidCostPredictor
{
namespace Test
{

namespace LogProcessing = AdServer::LogProcessing;

DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

void testSuit(const std::string& directory);

class Test : private Generics::Uncopyable
{
  using Collector = LogProcessing::BidCostStatCollector;
  using Key = typename Collector::KeyT;
  using Data = typename Collector::DataT;
  using LogTraits = LogProcessing::BidCostStatTraits;

  using CollectorInner = LogProcessing::BidCostStatInnerCollector;
  using KeyInner = typename CollectorInner::KeyT;
  using DataInner = typename CollectorInner::DataT;
  using LogInnerTraits = LogProcessing::BidCostStatInnerTraits;

  using DayTimestamp = LogProcessing::DayTimestamp;
  using FixedNumber = LogProcessing::FixedNumber;

  using Processors = std::list<Processor_var>;
public:
  Test(const std::string& directory,
       const std::size_t number_dates_per_file = 5,
       const std::size_t number_record_per_date = 1000,
       const std::size_t number_files = 150);

  ~Test() = default;

  bool run() noexcept;

  void addProcessor(Processor_var&& processor);

private:
  void copyFile(
          const std::string& path_file,
          const std::string& pah_out_file);

  bool runHelper();

private:
  const std::string directory_;

  const std::size_t number_dates_per_file_;

  const std::size_t number_record_per_date_;

  const std::size_t number_files_;

  Logging::Logger_var logger_;

  Collector collector_result_original_;

  Processors processors_;
};

} // namespace Test
} // namespace BidCostPredictor
} // namespace PredictorSvcs

#endif //BIDCOSTPREDICTOR_TEST_HPP
