#define BOOST_TEST_MODULE "c++ test for bid cost predictor"
#include <boost/test/included/unit_test.hpp>

// STD
#include <chrono>
#include <filesystem>
#include <regex>
#include <string>

// POSIX
#include <sys/stat.h>

// UNIXCOMMONS
#include <Generics/Uncopyable.hpp>

// THIS
#include <LogCommons/BidCostStat.hpp>
#include <LogCommons/LogCommons.hpp>
#include <Logger/StreamLogger.hpp>
#include "Aggregator.hpp"
#include "AggregatorMultyThread.hpp"
#include "BidCostCollector.hpp"
#include "CtrCollector.hpp"
#include "HelpCollector.hpp"
#include "LogHelper.hpp"
#include "ModelBidCostImpl.hpp"
#include "ModelCtrImpl.hpp"
#include "ModelEvaluatorBidCost.hpp"
#include "ModelEvaluatorCtr.hpp"
#include "ModelBidCostFactory.hpp"
#include "DataModelProviderImpl.hpp"
#include "Reaggregator.hpp"
#include "ReaggregatorMultyThread.hpp"
#include "ReferenceCounting/Interface.hpp"
#include "Utils.hpp"

namespace Aspect
{

inline constexpr char TEST_BID_PREDICTOR[] = "Test";

} // namespace Aspect

using namespace PredictorSvcs::BidCostPredictor;
namespace LogProcessing = AdServer::LogProcessing;

DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

using FixedNumber = LogProcessing::FixedNumber;

void copy_file(
  const std::string& path_in,
  const std::string& path_out)
{
  std::ifstream istream(path_in);
  if (!istream.is_open())
  {
    Stream::Error stream;
    stream << "Can't open file"
           << path_in;
    throw Exception(stream);
  }

  std::ofstream ostream(path_out);
  if (!ostream.is_open())
  {
    Stream::Error stream;
    stream << "Can't open file"
           << path_out;
    throw Exception(stream);
  }

  std::istreambuf_iterator<char> it_begin(istream);
  std::istreambuf_iterator<char> it_end;
  std::ostream_iterator<char> it_out(ostream);
  std::copy(it_begin, it_end, it_out);

  if (!ostream.good())
  {
    Stream::Error stream;
    stream << "Error copy file="
         << path_in
         << " to"
         << path_out;
    throw Exception(stream);
  }
}

class TestAgg final : private Generics::Uncopyable
{
private:
  using Collector = LogProcessing::BidCostStatCollector;
  using Key = typename Collector::KeyT;
  using Data = typename Collector::DataT;
  using LogTraits = LogProcessing::BidCostStatTraits;
  using CollectorInner = LogProcessing::BidCostStatInnerCollector;
  using KeyInner = typename CollectorInner::KeyT;
  using DataInner = typename CollectorInner::DataT;
  using LogInnerTraits = LogProcessing::BidCostStatInnerTraits;
  using DayTimestamp = LogProcessing::DayTimestamp;
  using Processors = std::list<Processor_var>;

public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;

public:
  explicit TestAgg(
    Logger* logger,
    const std::string& directory,
    const std::size_t number_dates_per_file = 5,
    const std::size_t number_record_per_date = 1000,
    const std::size_t number_files = 150);

  ~TestAgg() = default;

  bool run() noexcept;

  void add_processor(Processor_var&& processor);

private:
  bool run_test();

private:
  const Logger_var logger_;

  const std::string directory_;

  const std::size_t number_dates_per_file_;

  const std::size_t number_record_per_date_;

  const std::size_t number_files_;

  Collector collector_result_original_;

  Processors processors_;
};

TestAgg::TestAgg(
  Logger* logger,
  const std::string& directory,
  const std::size_t number_dates_per_file,
  const std::size_t number_record_per_date,
  const std::size_t number_files)
  : logger_(ReferenceCounting::add_ref(logger)),
    directory_(directory),
    number_dates_per_file_(number_dates_per_file),
    number_record_per_date_(number_record_per_date),
    number_files_(number_files)
{
}

void TestAgg::add_processor(Processor_var&& processor)
{
  if (!processor)
  {
    throw Exception("processor is null");
  }

  processors_.emplace_back(std::move(processor));
}

bool TestAgg::run() noexcept
{
  try
  {
    const bool status = run_test();
    if (status)
    {
      logger_->info(
        std::string("TEST OK"),
        Aspect::TEST_BID_PREDICTOR);
      return true;
    }
    else
    {
      logger_->info(
        std::string("TEST FAILED"),
        Aspect::TEST_BID_PREDICTOR);
      return false;
    }
  }
  catch (const eh::Exception& exc)
  {
    Stream::Error stream;
    stream << "TEST FAILED.\n Reason: "
           << exc.what();
    logger_->error(stream.str(), Aspect::TEST_BID_PREDICTOR);
  }
  catch (...)
  {
    Stream::Error stream;
    stream << "TEST FAILED.\n Reason: Unknown error";
    logger_->error(stream.str(), Aspect::TEST_BID_PREDICTOR);
  }

  return false;
}

bool TestAgg::run_test()
{
  logger_->info(
    std::string("Start test..."),
    Aspect::TEST_BID_PREDICTOR);

  if (!std::filesystem::is_directory(directory_))
  {
    throw Exception("Not existing directory=" + directory_);
  }

  auto files = Utils::get_directory_files(directory_);
  for (const auto& file : files)
  {
    std::remove(file.c_str());
  }

  CollectorInner collector_inner_file;
  CollectorInner collector_inner_result;

  logger_->info(
    std::string("Start generate test files..."),
    Aspect::TEST_BID_PREDICTOR);
  const FixedNumber cost("0.01");
  for (std::uint32_t i = 1; i <= number_record_per_date_; ++i)
  {
    const unsigned long tag_id = i;
    const std::string id_string = std::to_string(i);
    const std::string ext_tag_id = "ext_tag_id" + id_string;
    const std::string url = "url=" + id_string;

    KeyInner key(tag_id, ext_tag_id, url, cost, 0);
    KeyInner key_result(tag_id, std::string(), url, cost, 0);

    const long unverified_imps = 1;
    const long imps = 2;
    const long clicks = 3;
    DataInner data(unverified_imps, imps, clicks);
    DataInner data_result(
      unverified_imps * number_files_,
      imps * number_files_,
      clicks * number_files_);

    collector_inner_file.add(key, data);
    collector_inner_result.add(key_result, data_result);
  }

  Collector collecor_result;
  Collector collector_file;
  Generics::Time time = Generics::Time::get_time_of_day();
  for (std::size_t d = 1; d <= number_dates_per_file_; ++d)
  {
    DayTimestamp date(time);
    time += Generics::Time::ONE_DAY;

    collector_file.add(date, collector_inner_file);
    collecor_result.add(date, collector_inner_result);
  }

  LogProcessing::LogIoProxy<LogTraits>::save(collector_file, directory_);
  const std::string prefix(LogTraits::B::log_base_name());
  files = Utils::get_directory_files(directory_, prefix);
  if (files.empty())
  {
    throw Exception("File not created. Fatal error");
  }

  if (files.size() > 1)
  {
    throw Exception("Logic error. Number files in directory more then 1");
  }

  const auto& path = *files.begin();

  std::stringstream stream;
  stream << directory_
         << "/"
         << prefix
         << ".";

  for (std::size_t i = 1; i < number_files_; ++i)
  {
    copy_file(path, stream.str() + std::to_string(i));
  }
  logger_->info(
    std::string("Test files generation completed successfully"),
    Aspect::TEST_BID_PREDICTOR);

  logger_->info(
    std::string("Start processing files..."),
    Aspect::TEST_BID_PREDICTOR);

  for (auto& processor :  processors_)
  {
    const auto t_start = std::chrono::high_resolution_clock::now();

    processor->activate_object();
    processor->wait_object();

    const auto t_end = std::chrono::high_resolution_clock::now();
    const double elapsed_time_ms = std::chrono::duration<double, std::milli>(
      t_end - t_start).count();
    std::stringstream stream;
    stream << processor->name()
           << " elapsed time[ms] = "
           << elapsed_time_ms;
    logger_->info(
      stream.str(),
      Aspect::TEST_BID_PREDICTOR);
  }

  logger_->info(
    std::string("Files processed successfully"),
    Aspect::TEST_BID_PREDICTOR);

  Collector collector_process;
  const std::string prefix_inner(
    LogInnerTraits::B::log_base_name());
  files = Utils::get_directory_files(directory_, prefix_inner);
  const std::regex date_regex("\\d{4}-\\d{2}-\\d{2}");
  for (const auto& file_path : files)
  {
    CollectorInner collector_inner;
    LogHelper<LogInnerTraits>::load(file_path, collector_inner);

    std::smatch date_match;
    if(std::regex_search(file_path, date_match, date_regex)
       && !date_match.empty())
    {
      std::stringstream stream;
      stream << date_match[0];

      DayTimestamp  date;
      stream >> date;
      collector_process.add(date, collector_inner);
    }
  }

  if (collector_process == collecor_result)
  {
    return true;
  }
  else
  {
    return false;
  }
}

BOOST_AUTO_TEST_SUITE(aggregation_reaggregation)

BOOST_AUTO_TEST_CASE(single_thread)
{
  const std::string directory = "/tmp";

  Logging::Logger_var logger(
    new Logging::OStream::Logger(
      Logging::OStream::Config(std::cerr)));

  const std::size_t max_process_files = 51;
  const std::size_t dump_max_size = 1303;

  if (!std::filesystem::is_directory(directory))
  {
    Stream::Error ostr;
    ostr << "Not existing directory="
         << directory;
    throw  Exception(ostr);
  }

  const std::string result_directory =
  directory + "/" + "agg_reagg";
  if (!std::filesystem::is_directory(result_directory))
  {
    if (mkdir(result_directory.c_str(), 0777) != 0)
    {
      Stream::Error stream;
      stream << FNS
             << "Can't create directory="
             << result_directory;
      throw Exception(stream);
    }
  }

  Processor_var aggregator_processor(
    new Aggregator(
      max_process_files,
      dump_max_size,
      result_directory,
      result_directory,
      logger));

  Processor_var reaggregator_processor(
    new Reaggregator(
      result_directory,
      result_directory,
      logger));

  const std::size_t number_dates_per_file = 5;
  const std::size_t number_record_per_date = 10007;
  const std::size_t number_files = 301;

  TestAgg test(
    logger,
    result_directory,
    number_dates_per_file,
    number_record_per_date,
    number_files);

  test.add_processor(std::move(aggregator_processor));
  test.add_processor(std::move(reaggregator_processor));
  BOOST_CHECK_EQUAL(test.run(), true);
}

BOOST_AUTO_TEST_CASE(multiple_thread)
{
  const std::string directory = "/tmp";

  Logging::Logger_var logger(
    new Logging::OStream::Logger(
      Logging::OStream::Config(std::cerr)));

  const std::size_t max_process_files = 51;
  const std::size_t dump_max_size = 1303;

  if (!std::filesystem::is_directory(directory))
  {
    Stream::Error ostr;
    ostr << "Not existing directory="
         << directory;
    throw  Exception(ostr);
  }

  const std::string result_directory =
  directory + "/" + "agg_reagg";
  if (!std::filesystem::is_directory(result_directory))
  {
    if (mkdir(result_directory.c_str(), 0777) != 0)
    {
      Stream::Error stream;
      stream << FNS
             << "Can't create directory="
             << result_directory;
      throw Exception(stream);
    }
  }

  Processor_var aggregator_processor(
    new AggregatorMultyThread(
      max_process_files,
      dump_max_size,
      result_directory,
      result_directory,
      logger));

  Processor_var reaggregator_processor(
    new ReaggregatorMultyThread(
      result_directory,
      result_directory,
      logger));

  const std::size_t number_dates_per_file = 5;
  const std::size_t number_record_per_date = 10007;
  const std::size_t number_files = 301;

  TestAgg test(
    logger,
    result_directory,
    number_dates_per_file,
    number_record_per_date,
    number_files);

  test.add_processor(std::move(aggregator_processor));
  test.add_processor(std::move(reaggregator_processor));
  BOOST_CHECK_EQUAL(test.run(), true);
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(data_model_provider)

BOOST_AUTO_TEST_CASE(provider)
{
  using namespace LogProcessing;
  using Collector = BidCostStatInnerCollector;
  using Key = typename Collector::KeyT;
  using Data = typename Collector::DataT;
  using LogTraits = LogProcessing::BidCostStatInnerTraits;

  const std::string directory = "/tmp";

  Logging::Logger_var logger(
    new Logging::OStream::Logger(
      Logging::OStream::Config(std::cerr)));
  logger->info(
    std::string("Start data model provider test"),
    Aspect::TEST_BID_PREDICTOR);

  if (!std::filesystem::is_directory(directory))
  {
    Stream::Error stream;
    stream << "Not existing directory="
           << directory;
    throw  Exception(stream);
  }

  const std::string result_directory = directory + "/" + "provider";
  if (!std::filesystem::is_directory(result_directory))
  {
    if (mkdir(result_directory.c_str(), 0777) != 0)
    {
      Stream::Error stream;
      stream << __PRETTY_FUNCTION__
             << "Can't create directory="
             << result_directory;
      throw Exception(stream);
    }
  }

  const auto need_delete_files = Utils::get_directory_files(result_directory);
  for (const auto& path : need_delete_files)
  {
    std::remove(path.c_str());
  }

  const std::size_t number_file = 250;
  const std::string url_prefix = "url";
  const std::size_t number_url_per_file = 30;
  const std::size_t number_tags_per_url = 3;
  const std::size_t tag_initial = 1;
  const std::size_t number_cost = 100;
  const auto initial_cost = FixedNumber("0.01");
  const auto step_cost = FixedNumber("0.0012");
  const long unverified_imps = 1;
  const long imps = 2;
  const long clicks = 3;
  const std::string url = "url";

  Collector collector_file;
  for (std::size_t url_i = 1; url_i <= number_url_per_file; ++url_i)
  {
    const std::string url = url_prefix + std::to_string(url_i);
    for (std::size_t tag_id = tag_initial; tag_id <= tag_initial + number_tags_per_url; ++tag_id)
    {
      FixedNumber cost = initial_cost;
      for (std::size_t cost_i = 1; cost_i <= number_cost; ++cost_i)
      {
        Key key(tag_id, std::string(), url, cost, 0);
        cost += step_cost;
        Data data(unverified_imps, imps, clicks);
        collector_file.add(key, data);
      }
    }
  }

  HelpCollector help_hollector_check(100000);
  for (std::size_t url_i = 1; url_i <= number_url_per_file; ++url_i)
  {
    const std::string url = url_prefix + std::to_string(url_i);
    for (std::size_t tag_id = tag_initial; tag_id <= tag_initial + number_tags_per_url; ++tag_id)
    {
      HelpCollector::Key key(tag_id, std::make_shared<std::string>(url));
      HelpCollector::InnerCollector collector_inner(100000);
      FixedNumber cost = FixedNumber("0.01");
      for (std::size_t cost_i = 1; cost_i <= number_cost; ++cost_i)
      {
        HelpCollector::InnerKey key_inner(cost);
        HelpCollector::InnerData data_inner(
          unverified_imps * number_file,
          imps * number_file,
          clicks * number_file);
        collector_inner.add(key_inner, data_inner);
        cost += step_cost;
      }
      help_hollector_check.add(key, collector_inner);
    }
  }

  const std::string prefix(LogTraits::B::log_base_name());
  const std::string path_originale_file =
  result_directory + "/" + prefix + ".";
  LogHelper<LogTraits>::save(path_originale_file, collector_file);

  for (std::size_t i = 1; i < number_file; ++i)
  {
    copy_file(
      path_originale_file,
      path_originale_file + std::to_string(i));
  }

  DataModelProvider_var provider(
    new DataModelProviderImpl(
      100000,
      result_directory,
      logger));
  HelpCollector help_hollector_result(100000);
  provider->load(help_hollector_result);

  BOOST_CHECK_EQUAL(help_hollector_result == help_hollector_check, true);

  logger->info(
    std::string("Data model provider test is finished"),
    Aspect::TEST_BID_PREDICTOR);


}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(bid_cost_model)

class DataModelProviderEmpty :
  public DataModelProvider,
  public virtual ReferenceCounting::AtomicImpl
{
public:
  DataModelProviderEmpty() = default;

  ~DataModelProviderEmpty() override = default;

  bool load(HelpCollector& collector) noexcept override
  {
    const unsigned long tag_id = 1;
    const std::string url = "url";

    HelpCollector::Key key(tag_id, std::make_shared<std::string>(url));
    HelpCollector::InnerCollector inner_collector(100000);

    auto cost = FixedNumber("0.01");
    long unverified_imps = 100;
    long imps = 40;
    long clicks = 5;
    {
      HelpCollector::InnerKey key_inner(cost);
      HelpCollector::InnerData data_inner(
        unverified_imps,
        imps,
        clicks);
      inner_collector.add(key_inner, data_inner);
    }

    cost = FixedNumber("0.015");
    unverified_imps = 100;
    imps = 9;
    clicks = 5;
    {
      HelpCollector::InnerKey key_inner(cost);
      HelpCollector::InnerData data_inner(
        unverified_imps,
        imps,
        clicks);
      inner_collector.add(key_inner, data_inner);
    }

    collector.add(key, inner_collector);
    return true;
  }

  void stop() noexcept override
  {
  }
};

BOOST_AUTO_TEST_CASE(bid_cost_model_empty)
{
  const std::string path_file = "/tmp/model_data";
  std::remove(path_file.c_str());

  Logging::Logger_var logger(
    new Logging::OStream::Logger(
      Logging::OStream::Config(std::cerr)));

  DataModelProvider_var data_provider(new DataModelProviderEmpty);
  ModelBidCostFactory_var model_factory(new ModelBidCostFactoryImpl(logger));

  std::vector<FixedNumber> points{
    FixedNumber("0.95"),
    FixedNumber("0.75"),
    FixedNumber("0.5"),
    FixedNumber("0.25")
  };

  ModelEvaluatorBidCost_var model_evaluator(
    new ModelEvaluatorBidCostImpl(
      points,
      data_provider,
      model_factory,
      logger));

  auto model = model_evaluator->evaluate();
  model->save(path_file);

  LogProcessing::BidCostCollector bid_collector;
  LogHelper<LogProcessing::BidCostTraits>::load(path_file, bid_collector);
  BOOST_CHECK_EQUAL(bid_collector.empty(), true);
}

class DataModelProvider1 :
  public DataModelProvider,
  public virtual ReferenceCounting::AtomicImpl
{
public:
  DataModelProvider1() = default;

  ~DataModelProvider1() override = default;

  bool load(HelpCollector& collector) noexcept override
  {
    const unsigned long tag_id = 1;
    const std::string url = "url";

    HelpCollector::Key key(tag_id, std::make_shared<std::string>(url));
    HelpCollector::InnerCollector inner_collector(100000);

    auto cost = FixedNumber("0.02");
    long unverified_imps = 100;
    long imps = 40;
    long clicks = 5;
    {
      HelpCollector::InnerKey key_inner(cost);
      HelpCollector::InnerData data_inner(
        unverified_imps,
        imps,
        clicks);
      inner_collector.add(key_inner, data_inner);
    }

    cost = FixedNumber("0.01");
    unverified_imps = 100;
    imps = 10;
    clicks = 5;
    {
      HelpCollector::InnerKey key_inner(cost);
      HelpCollector::InnerData data_inner(
        unverified_imps,
        imps,
        clicks);
      inner_collector.add(key_inner, data_inner);
    }

    collector.add(key, inner_collector);
    return true;
  }

  void stop() noexcept override
  {
  }
};

BOOST_AUTO_TEST_CASE(bid_cost_model_1)
{
  const std::string path_file = "/tmp/model_data";
  std::remove(path_file.c_str());

  Logging::Logger_var logger(
    new Logging::OStream::Logger(
      Logging::OStream::Config(std::cerr)));

  DataModelProvider_var data_provider(new DataModelProvider1);
  ModelBidCostFactory_var model_factory(new ModelBidCostFactoryImpl(logger));

  std::vector<FixedNumber> points{
    FixedNumber("0.95"),
    FixedNumber("0.75"),
    FixedNumber("0.5"),
    FixedNumber("0.25")
  };

  ModelEvaluatorBidCost_var model_evaluator(
    new ModelEvaluatorBidCostImpl(
      points,
      data_provider,
      model_factory,
      logger));

    auto model = model_evaluator->evaluate();
    model->save(path_file);

    LogProcessing::BidCostCollector bid_collector;
    LogHelper<LogProcessing::BidCostTraits>::load(path_file, bid_collector);

    // top_level_win_rate = 0.25
    // check_win_rate = [0,2375; 0,1875; 0,125; 0,0625]
    // local_win_rate = 0.25
    // target_cost = 0.02, max_cost = 0.02

    BOOST_CHECK_EQUAL(bid_collector.size() == 4, true);
    LogProcessing::BidCostKey key1(1, "url", FixedNumber("0.95"));
    auto it1 = bid_collector.find(key1);
    BOOST_CHECK_EQUAL(it1 != bid_collector.end(), true);
    BOOST_CHECK_EQUAL(it1->second.cost(), FixedNumber("0.02"));
    BOOST_CHECK_EQUAL(it1->second.max_cost(), FixedNumber("0.02"));

    LogProcessing::BidCostKey key2(1, "url", FixedNumber("0.75"));
    auto it2 = bid_collector.find(key2);
    BOOST_CHECK_EQUAL(it2 != bid_collector.end(), true);
    BOOST_CHECK_EQUAL(it2->second.cost(), FixedNumber("0.02"));
    BOOST_CHECK_EQUAL(it2->second.max_cost(), FixedNumber("0.02"));

    LogProcessing::BidCostKey key3(1, "url", FixedNumber("0.5"));
    auto it3 = bid_collector.find(key3);
    BOOST_CHECK_EQUAL(it3 != bid_collector.end(), true);
    BOOST_CHECK_EQUAL(it3->second.cost(), FixedNumber("0.02"));
    BOOST_CHECK_EQUAL(it3->second.max_cost(), FixedNumber("0.02"));

   LogProcessing::BidCostKey key4(1, "url", FixedNumber("0.25"));
   auto it4 = bid_collector.find(key4);
   BOOST_CHECK_EQUAL(it4 != bid_collector.end(), true);
   BOOST_CHECK_EQUAL(it4->second.cost(), FixedNumber("0.02"));
   BOOST_CHECK_EQUAL(it4->second.max_cost(), FixedNumber("0.02"));
}

class DataModelProvider2 :
  public DataModelProvider,
  public virtual ReferenceCounting::AtomicImpl
{
public:
  DataModelProvider2() = default;

  ~DataModelProvider2() override = default;

  bool load(HelpCollector& collector) noexcept override
  {
    const unsigned long tag_id = 1;
    const std::string url = "url";

    HelpCollector::Key key(tag_id, std::make_shared<std::string>(url));
    HelpCollector::InnerCollector inner_collector(100000);

    auto cost = FixedNumber("0.03");
    long unverified_imps = 400;
    long imps = 100;
    long clicks = 5;
    {
      HelpCollector::InnerKey key_inner(cost);
      HelpCollector::InnerData data_inner(
        unverified_imps,
        imps,
        clicks);
      inner_collector.add(key_inner, data_inner);
    }

    cost = FixedNumber("0.02");
    unverified_imps = 100;
    imps = 40;
    clicks = 5;
    {
      HelpCollector::InnerKey key_inner(cost);
      HelpCollector::InnerData data_inner(
        unverified_imps,
        imps,
        clicks);
      inner_collector.add(key_inner, data_inner);
    }

    cost = FixedNumber("0.01");
    unverified_imps = 100;
    imps = 10;
    clicks = 5;
    {
      HelpCollector::InnerKey key_inner(cost);
      HelpCollector::InnerData data_inner(
        unverified_imps,
        imps,
        clicks);
      inner_collector.add(key_inner, data_inner);
    }

    collector.add(key, inner_collector);
    return true;
  }

  void stop() noexcept override
  {
  }
};

BOOST_AUTO_TEST_CASE(bid_cost_model_2)
{
  const std::string path_file = "/tmp/model_data";
  std::remove(path_file.c_str());

  Logging::Logger_var logger(
  new Logging::OStream::Logger(
  Logging::OStream::Config(std::cerr)));

  DataModelProvider_var data_provider(new DataModelProvider2);
  ModelBidCostFactory_var model_factory(new ModelBidCostFactoryImpl(logger));

  std::vector<FixedNumber> points{
    FixedNumber("0.95"),
    FixedNumber("0.75"),
    FixedNumber("0.5"),
    FixedNumber("0.25")
  };

  ModelEvaluatorBidCost_var model_evaluator(
    new ModelEvaluatorBidCostImpl(
      points,
      data_provider,
      model_factory,
      logger));

  auto model = model_evaluator->evaluate();
  model->save(path_file);

  LogProcessing::BidCostCollector bid_collector;
  LogHelper<LogProcessing::BidCostTraits>::load(path_file, bid_collector);

  // top_level_win_rate = 0.25
  // check_win_rate = [0,2375; 0,1875; 0,125; 0,0625]
  // local_win_rate = 0.25
  // target_cost = 0.02, max_cost = 0.03

  BOOST_CHECK_EQUAL(bid_collector.size() == 4, true);
  LogProcessing::BidCostKey key1(1, "url", FixedNumber("0.95"));
  auto it1 = bid_collector.find(key1);
  BOOST_CHECK_EQUAL(it1 != bid_collector.end(), true);
  BOOST_CHECK_EQUAL(it1->second.cost(), FixedNumber("0.02"));
  BOOST_CHECK_EQUAL(it1->second.max_cost(), FixedNumber("0.03"));

  LogProcessing::BidCostKey key2(1, "url", FixedNumber("0.75"));
  auto it2 = bid_collector.find(key2);
  BOOST_CHECK_EQUAL(it2 != bid_collector.end(), true);
  BOOST_CHECK_EQUAL(it2->second.cost(), FixedNumber("0.02"));
  BOOST_CHECK_EQUAL(it2->second.max_cost(), FixedNumber("0.03"));

  LogProcessing::BidCostKey key3(1, "url", FixedNumber("0.5"));
  auto it3 = bid_collector.find(key3);
  BOOST_CHECK_EQUAL(it3 != bid_collector.end(), true);
  BOOST_CHECK_EQUAL(it3->second.cost(), FixedNumber("0.02"));
  BOOST_CHECK_EQUAL(it3->second.max_cost(), FixedNumber("0.03"));

  LogProcessing::BidCostKey key4(1, "url", FixedNumber("0.25"));
  auto it4 = bid_collector.find(key4);
  BOOST_CHECK_EQUAL(it4 != bid_collector.end(), true);
  BOOST_CHECK_EQUAL(it4->second.cost(), FixedNumber("0.02"));
  BOOST_CHECK_EQUAL(it4->second.max_cost(), FixedNumber("0.03"));
}

class DataModelProvider3 :
  public DataModelProvider,
  public virtual ReferenceCounting::AtomicImpl
{
public:
  DataModelProvider3() = default;

  ~DataModelProvider3() override = default;

  bool load(HelpCollector& collector) noexcept override
  {
    const unsigned long tag_id = 1;
    const std::string url = "url";

    HelpCollector::Key key(tag_id, std::make_shared<std::string>(url));
    HelpCollector::InnerCollector inner_collector(100000);

    auto cost = FixedNumber("0.03");
    long unverified_imps = 400;
    long imps = 100;
    long clicks = 5;
    {
      HelpCollector::InnerKey key_inner(cost);
      HelpCollector::InnerData data_inner(
        unverified_imps,
        imps,
        clicks);
      inner_collector.add(key_inner, data_inner);
    }

    cost = FixedNumber("0.02");
    unverified_imps = 1000;
    imps = 40;
    clicks = 5;
    {
      HelpCollector::InnerKey key_inner(cost);
      HelpCollector::InnerData data_inner(
        unverified_imps,
        imps,
        clicks);
      inner_collector.add(key_inner, data_inner);
    }

    cost = FixedNumber("0.01");
    unverified_imps = 1000;
    imps = 10;
    clicks = 5;
    {
      HelpCollector::InnerKey key_inner(cost);
      HelpCollector::InnerData data_inner(
        unverified_imps,
        imps,
        clicks);
      inner_collector.add(key_inner, data_inner);
    }

    collector.add(key, inner_collector);
    return true;
  }

  void stop() noexcept override
  {
  }
};

BOOST_AUTO_TEST_CASE(bid_cost_model_3)
{
  const std::string path_file = "/tmp/model_data";
  std::remove(path_file.c_str());

  Logging::Logger_var logger(
    new Logging::OStream::Logger(
      Logging::OStream::Config(std::cerr)));

  DataModelProvider_var data_provider(new DataModelProvider3);
  ModelBidCostFactory_var model_factory(new ModelBidCostFactoryImpl(logger));

  std::vector<FixedNumber> points{
    FixedNumber("0.95"),
    FixedNumber("0.75"),
    FixedNumber("0.5"),
    FixedNumber("0.25")
  };

  ModelEvaluatorBidCost_var model_evaluator(
    new ModelEvaluatorBidCostImpl(
      points,
      data_provider,
      model_factory,
      logger));

  auto model = model_evaluator->evaluate();
  model->save(path_file);

  LogProcessing::BidCostCollector bid_collector;
  LogHelper<LogProcessing::BidCostTraits>::load(path_file, bid_collector);

  // top_level_win_rate = 0,025
  // check_win_rate = [0,02375; 0,01875; 0,0125; 0,00625]
  // local_win_rate = 0.025
  // target_cost = 0.03, max_cost = 0.03

  BOOST_CHECK_EQUAL(bid_collector.size() == 4, true);
  LogProcessing::BidCostKey key1(1, "url", FixedNumber("0.95"));
  auto it1 = bid_collector.find(key1);
  BOOST_CHECK_EQUAL(it1 != bid_collector.end(), true);
  BOOST_CHECK_EQUAL(it1->second.cost(), FixedNumber("0.03"));
  BOOST_CHECK_EQUAL(it1->second.max_cost(), FixedNumber("0.03"));

  LogProcessing::BidCostKey key2(1, "url", FixedNumber("0.75"));
  auto it2 = bid_collector.find(key2);
  BOOST_CHECK_EQUAL(it2 != bid_collector.end(), true);
  BOOST_CHECK_EQUAL(it2->second.cost(), FixedNumber("0.03"));
  BOOST_CHECK_EQUAL(it2->second.max_cost(), FixedNumber("0.03"));

  LogProcessing::BidCostKey key3(1, "url", FixedNumber("0.5"));
  auto it3 = bid_collector.find(key3);
  BOOST_CHECK_EQUAL(it3 != bid_collector.end(), true);
  BOOST_CHECK_EQUAL(it3->second.cost(), FixedNumber("0.03"));
  BOOST_CHECK_EQUAL(it3->second.max_cost(), FixedNumber("0.03"));

  LogProcessing::BidCostKey key4(1, "url", FixedNumber("0.25"));
  auto it4 = bid_collector.find(key4);
  BOOST_CHECK_EQUAL(it4 != bid_collector.end(), true);
  BOOST_CHECK_EQUAL(it4->second.cost(), FixedNumber("0.03"));
  BOOST_CHECK_EQUAL(it4->second.max_cost(), FixedNumber("0.03"));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(bid_cost_model_search)

BOOST_AUTO_TEST_CASE(test1)
{
  Logging::Logger_var logger(
    new Logging::OStream::Logger(
      Logging::OStream::Config(std::cerr)));

  ModelBidCost_var model(new ModelBidCostImpl(logger));

  model->set_cost(
    1,
    std::make_shared<std::string>("url"),
    LogProcessing::FixedNumber("0.1"),
    LogProcessing::FixedNumber("1.2"),
    LogProcessing::FixedNumber("3.0"));

  model->set_cost(
    1,
    std::make_shared<std::string>("url"),
    LogProcessing::FixedNumber("0.2"),
    LogProcessing::FixedNumber("1.5"),
    LogProcessing::FixedNumber("2.5"));

  model->set_cost(
    1,
    std::make_shared<std::string>("url"),
    LogProcessing::FixedNumber("0.3"),
    LogProcessing::FixedNumber("2.0"),
    LogProcessing::FixedNumber("2.2"));

  model->set_cost(
    2,
    std::make_shared<std::string>("url"),
    LogProcessing::FixedNumber("0.4"),
    LogProcessing::FixedNumber("0.1"),
    LogProcessing::FixedNumber("10.0"));

  auto result = model->get_cost(
    1,
    "url",
    LogProcessing::FixedNumber("0.1"),
    LogProcessing::FixedNumber("2.0"));
  BOOST_CHECK_EQUAL(result == LogProcessing::FixedNumber("1.2"), true);

  result = model->get_cost(
    1,
    "url",
    LogProcessing::FixedNumber("0.2"),
    LogProcessing::FixedNumber("2.0"));
  BOOST_CHECK_EQUAL(result == LogProcessing::FixedNumber("1.5"), true);

  result = model->get_cost(
    1,
    "url",
    LogProcessing::FixedNumber("0.1"),
    LogProcessing::FixedNumber("5.0"));
  BOOST_CHECK_EQUAL(result == LogProcessing::FixedNumber("5.0"), true);

  result = model->get_cost(
    1,
    "url",
    LogProcessing::FixedNumber("0.1"),
    LogProcessing::FixedNumber("0.5"));
  BOOST_CHECK_EQUAL(result == LogProcessing::FixedNumber("0.5"), true);

  result = model->get_cost(
    777,
    "url",
    LogProcessing::FixedNumber("0.1"),
    LogProcessing::FixedNumber("0.5"));
  BOOST_CHECK_EQUAL(result == LogProcessing::FixedNumber("0.5"), true);
}

BOOST_AUTO_TEST_CASE(test_load)
{
  LogProcessing::BidCostCollector collector;
  {
    LogProcessing::BidCostKey key(
      1,
      std::make_shared<std::string>("url"),
      LogProcessing::FixedNumber("0.1"));

    LogProcessing::BidCostData data(
      LogProcessing::FixedNumber("1.2"),
      LogProcessing::FixedNumber("3.0"));

    collector.add(key, data);
  }

  {
    LogProcessing::BidCostKey key(
     1,
     std::make_shared<std::string>("url"),
     LogProcessing::FixedNumber("0.2"));

    LogProcessing::BidCostData data(
      LogProcessing::FixedNumber("1.5"),
      LogProcessing::FixedNumber("2.5"));

    collector.add(key, data);
  }

  {
    LogProcessing::BidCostKey key(
      1,
      std::make_shared<std::string>("url"),
      LogProcessing::FixedNumber("0.3"));

    LogProcessing::BidCostData data(
      LogProcessing::FixedNumber("2.0"),
      LogProcessing::FixedNumber("2.2"));

    collector.add(key, data);
  }

  {
    LogProcessing::BidCostKey key(
      2,
      std::make_shared<std::string>("url"),
      LogProcessing::FixedNumber("0.4"));

    LogProcessing::BidCostData data(
      LogProcessing::FixedNumber("0.1"),
      LogProcessing::FixedNumber("10.0"));

     collector.add(key, data);
  }

  const std::string path_file = "/tmp/model_data";
  std::remove(path_file.c_str());
  LogHelper<LogProcessing::BidCostTraits>::save(path_file, collector);

  Logging::Logger_var logger(
    new Logging::OStream::Logger(
      Logging::OStream::Config(std::cerr)));

  ModelBidCost_var model(new ModelBidCostImpl(logger));
  model->load(path_file);

  auto result = model->get_cost(
    1,
    "url",
    LogProcessing::FixedNumber("0.1"),
    LogProcessing::FixedNumber("2.0"));
  BOOST_CHECK_EQUAL(result == LogProcessing::FixedNumber("1.2"), true);

  result = model->get_cost(
    1,
    "url",
    LogProcessing::FixedNumber("0.2"),
    LogProcessing::FixedNumber("2.0"));
  BOOST_CHECK_EQUAL(result == LogProcessing::FixedNumber("1.5"), true);

  result = model->get_cost(
    1,
    "url",
    LogProcessing::FixedNumber("0.1"),
    LogProcessing::FixedNumber("5.0"));
  BOOST_CHECK_EQUAL(result == LogProcessing::FixedNumber("5.0"), true);

  result = model->get_cost(
    1,
    "url",
    LogProcessing::FixedNumber("0.1"),
    LogProcessing::FixedNumber("0.5"));
  BOOST_CHECK_EQUAL(result == LogProcessing::FixedNumber("0.5"), true);

  result = model->get_cost(
    777,
    "url",
    LogProcessing::FixedNumber("0.1"),
    LogProcessing::FixedNumber("0.5"));
  BOOST_CHECK_EQUAL(result == LogProcessing::FixedNumber("0.5"), true);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(ctr_model_search)

BOOST_AUTO_TEST_CASE(test1)
{
  LogProcessing::CtrCollector collector;
  {
    LogProcessing::CtrKey key(
      1,
      std::make_shared<std::string>("url"));

    LogProcessing::CtrData data(
      Types::FixedNumber("0.1"));
    collector.add(key, data);
  }

  {
    LogProcessing::CtrKey key(
      2,
      std::make_shared<std::string>("url"));

    LogProcessing::CtrData data(
      Types::FixedNumber("0.2"));
    collector.add(key, data);
  }

  {
    LogProcessing::CtrKey key(
      3,
      std::make_shared<std::string>("url2"));

    LogProcessing::CtrData data(
      Types::FixedNumber("0.3"));
    collector.add(key, data);
  }

  {
    LogProcessing::CtrKey key(
      4,
      std::make_shared<std::string>("url3"));

    LogProcessing::CtrData data(
      Types::FixedNumber("0.4"));
    collector.add(key, data);
  }

  const std::string path_file = "/tmp/model_data";
  std::remove(path_file.c_str());
  LogHelper<LogProcessing::CtrTraits>::save(path_file, collector);

  Logging::Logger_var logger(
    new Logging::OStream::Logger(
      Logging::OStream::Config(std::cerr)));

  ModelCtr_var model(new ModelCtrImpl(logger));
  model->load(path_file);

  auto ctr = model->get_ctr(1, "url");
  BOOST_CHECK_EQUAL(ctr == Types::FixedNumber("0.1"), true);

  ctr = model->get_ctr(777, "url");
  BOOST_CHECK_EQUAL(ctr.is_zero(), true);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(help_collector)

BOOST_AUTO_TEST_CASE(test1)
{
  HelpCollector collector(100);

  HelpCollector::Key key(1, Types::UrlPtr(new std::string("url1")));
  auto& inner_collecrtor = collector.find_or_insert(key);
  BOOST_CHECK_EQUAL(inner_collecrtor.empty(), true);
  BOOST_CHECK_EQUAL(inner_collecrtor.total_imps() == 0, true);
  BOOST_CHECK_EQUAL(inner_collecrtor.total_clicks() == 0, true);

  HelpCollector::InnerKey key_inner1(Types::FixedNumber("0.1"));
  HelpCollector::InnerData data_inner1(1000000, 10, 1);
  inner_collecrtor.add(key_inner1, data_inner1);

  auto& inner_collecrtor2 = collector.find_or_insert(key);
  BOOST_CHECK_EQUAL(inner_collecrtor2.total_imps() == 10, true);
  BOOST_CHECK_EQUAL(inner_collecrtor2.total_clicks() == 1, true);

  HelpCollector::InnerKey key_inner2(Types::FixedNumber("0.1"));
  HelpCollector::InnerData data_inner2(1000000, 15, 2);
  inner_collecrtor.add(key_inner2, data_inner2);
  BOOST_CHECK_EQUAL(inner_collecrtor.total_imps() == 25, true);
  BOOST_CHECK_EQUAL(inner_collecrtor.total_clicks() == 3, true);

  HelpCollector::InnerKey key_inner3(Types::FixedNumber("0.1"));
  HelpCollector::InnerData data_inner3(1000000, 100, 5);
  inner_collecrtor.add(key_inner3, data_inner3);
  BOOST_CHECK_EQUAL(inner_collecrtor.total_imps() == 125, true);
  BOOST_CHECK_EQUAL(inner_collecrtor.total_clicks() == 8, true);

  HelpCollector::InnerKey key_inner4(Types::FixedNumber("0.1"));
  HelpCollector::InnerData data_inner4(1000000, 100, 5);
  inner_collecrtor.add(key_inner4, data_inner4);
  BOOST_CHECK_EQUAL(inner_collecrtor.total_imps() == 125, true);
  BOOST_CHECK_EQUAL(inner_collecrtor.total_clicks() == 8, true);
}

BOOST_AUTO_TEST_SUITE_END()