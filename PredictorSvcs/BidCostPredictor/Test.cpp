// STD
#include <chrono>
#include <regex>

// THIS
#include <Logger/StreamLogger.hpp>
#include "Aggregator.hpp"
#include "AggregatorMultyThread.hpp"
#include "LogHelper.hpp"
#include "Reaggregator.hpp"
#include "ReaggregatorMultyThread.hpp"
#include "Utils.hpp"
#include "Test.hpp"

namespace Aspect
{
const char TEST_BID_PREDICTOR[] = "Test";
}

namespace PredictorSvcs
{
namespace BidCostPredictor
{
namespace Test
{

void test_suit(const std::string& directory)
{
  Logging::Logger_var logger(
    new Logging::OStream::Logger(
      Logging::OStream::Config(std::cerr)));

  const std::size_t max_process_files = 50;
  const std::size_t dump_max_size = 1303;

  if (!Utils::exist_directory(directory))
  {
    Stream::Error ostr;
    ostr << "Not existing directory="
         << directory;
    throw  Exception(ostr);
  }

  Processor_var aggregator_processor(
    new Aggregator(
      max_process_files,
      dump_max_size,
      directory,
      directory,
      logger));

  Processor_var aggregator_multy_processor(
    new AggregatorMultyThread(
      max_process_files,
      dump_max_size,
      directory,
      directory,
      logger));

  Processor_var reaggregator_processor(
    new Reaggregator(
      directory,
      directory,
      logger));

  Processor_var reaggregator_multy_processor(
    new Reaggregator(
      directory,
      directory,
      logger));

  const std::size_t number_dates_per_file = 5;
  const std::size_t number_record_per_date = 10000;
  const std::size_t number_files = 300;

  Test test1(
    directory,
    number_dates_per_file,
    number_record_per_date,
    number_files);
  test1.add_processor(std::move(aggregator_processor));
  test1.add_processor(std::move(reaggregator_processor));
  test1.run();

  Test test2(
    directory,
    number_dates_per_file,
    number_record_per_date,
    number_files);
  test2.add_processor(std::move(aggregator_multy_processor));
  test2.add_processor(std::move(reaggregator_multy_processor));
  test2.run();
}

Test::Test(
  const std::string& directory,
  const std::size_t number_dates_per_file,
  const std::size_t number_record_per_date,
  const std::size_t number_files)
  : directory_(directory),
    number_dates_per_file_(number_dates_per_file),
    number_record_per_date_(number_record_per_date),
    number_files_(number_files),
    logger_(
      new Logging::OStream::Logger(
        Logging::OStream::Config(std::cerr)))
{
}

void Test::add_processor(Processor_var&& processor)
{
  if (!processor)
    throw Exception("processor is null");

  processors_.emplace_back(std::move(processor));
}

bool Test::run() noexcept
{
  try
  {
    const bool status = run_helper();
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
    std::stringstream stream;
    stream << "TEST FAILED.\n Reason: "
           << exc.what();
    logger_->error(
            stream.str(),
            Aspect::TEST_BID_PREDICTOR);
    return false;
  }
}

bool Test::run_helper()
{
  logger_->info(
    std::string("Start test..."),
    Aspect::TEST_BID_PREDICTOR);

  if (!Utils::exist_directory(directory_))
    throw Exception("Not existing directory=" + directory_);

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

    KeyInner key(tag_id, ext_tag_id, url, cost);
    KeyInner key_result(tag_id, std::string(), url, cost);

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
  Generics::Time time =
    Generics::Time::get_time_of_day();
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
    throw Exception("File not created. Fatal error");

  if (files.size() > 1)
    throw Exception("Logic error. Number files in directory more then 1");

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
    auto t_start = std::chrono::high_resolution_clock::now();

    processor->start();
    processor->wait();

    auto t_end = std::chrono::high_resolution_clock::now();
    double elapsed_time_ms =
      std::chrono::duration<double, std::milli>(t_end - t_start).count();
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

void Test::copy_file(
  const std::string& path_in,
  const std::string& pah_out)
{
  std::ifstream istream(path_in);
  if (!istream.is_open())
  {
    Stream::Error ostr;
    ostr << "Can't open file" << path_in;
    throw Exception(ostr);
  }

  std::ofstream ostream(pah_out);
  if (!ostream.is_open())
  {
    Stream::Error ostr;
    ostr << "Can't open file" << pah_out;
    throw Exception(ostr);
  }

  std::istreambuf_iterator<char> it_begin(istream);
  std::istreambuf_iterator<char> it_end;
  std::ostream_iterator<char> it_out(ostream);
  std::copy(it_begin, it_end, it_out);

  if (!ostream.good())
  {
    Stream::Error ostr;
    ostr << "Error copy file=" << path_in
         << " to" << pah_out;
    throw Exception(ostr);
  }
}

} // namespace Test
} // namespace BidCostPredictor
} // namespace PredictorSvcs
