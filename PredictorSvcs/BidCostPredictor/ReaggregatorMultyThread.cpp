// THIS
#include <Logger/StreamLogger.hpp>
#include "LogHelper.hpp"
#include "ReaggregatorMultyThread.hpp"
#include "Remover.hpp"
#include "Utils.hpp"

// STD
#include <regex>

namespace Aspect
{
const char* REAGGREGATOR = "REAGGREGATOR";
}

namespace PredictorSvcs
{
namespace BidCostPredictor
{

ReaggregatorMultyThread::ReaggregatorMultyThread(
        const std::string& input_dir,
        const std::string& output_dir,
        const Logging::Logger_var& logger)
        : input_dir_(input_dir),
          output_dir_(output_dir),
          prefix_(LogTraits::B::log_base_name()),
          logger_(logger),
          observer_(new ActiveObjectObserver(this)),
          persantage_(logger_, Aspect::REAGGREGATOR, 5)
{
  for (std::uint8_t i = 1; i <= COUNT_THREADS; ++i)
  {
    task_runners_.emplace_back(new Generics::TaskRunner(observer_, 1));
  }

  collector_ = pool_collector_.getCollector();
}

ReaggregatorMultyThread::~ReaggregatorMultyThread()
{
  shutdown_manager_.stop();
  observer_->clearDelegate();
  wait();
}

void ReaggregatorMultyThread::start()
{
  logger_->info(
          std::string("Reaggregator: started"),
          Aspect::REAGGREGATOR);

  if (!Utils::ExistDirectory(input_dir_))
  {
    Stream::Error ostr;
    ostr << __PRETTY_FUNCTION__
         << ": Not existing input directory="
         << input_dir_;
    throw Exception(ostr);
  }

  if (!Utils::ExistDirectory(output_dir_))
  {
    Stream::Error ostr;
    ostr << __PRETTY_FUNCTION__
         << ": Not existing output directory="
         << output_dir_;
    throw Exception(ostr);
  }

  if (task_runners_.empty())
  {
    throw Exception("task_runners is empty");
  }

  is_running_ = true;

  try
  {
    for (auto& task_runner : task_runners_)
    {
      task_runner->activate_object();
    }
  }
  catch(const eh::Exception& ex)
  {
    shutdown_manager_.stop();
    Stream::Error ostr;
    ostr << __PRETTY_FUNCTION__
         << ": Can't init task runner : "
         << ex.what();
    throw Exception(ostr);
  }

  reaggregate();
}

void ReaggregatorMultyThread::wait() noexcept
{
  if (!is_running_)
    return;

  is_running_ = false;

  shutdown_manager_.wait();

  for (auto& task_runner : task_runners_)
  {
    try
    {
      task_runner->wait_for_queue_exhausting();
    }
    catch (...)
    {}

    try
    {
      task_runner->deactivate_object();
      task_runner->wait_object();
    }
    catch (...)
    {}
  }

  for (auto& task_runner : task_runners_)
  {
    try
    {
      task_runner->clear();
    }
    catch (...)
    {}
  }

  task_runners_.clear();
}

void ReaggregatorMultyThread::stop() noexcept
{
  logger_->info(std::string("Reaggregator was abborted..."));
  shutdown_manager_.stop();
}

const char* ReaggregatorMultyThread::name() noexcept
{
  return "ReaggregatorMultyThread";
}

void ReaggregatorMultyThread::reaggregate()
{
  auto input_files =
          Utils::GetDirectoryFiles(input_dir_,
                                   prefix_);

  const std::regex date_regex("\\d{4}-\\d{2}-\\d{2}");
  for (const auto& file_path : input_files)
  {
    std::smatch date_match;
    if(std::regex_search(file_path, date_match, date_regex)
       && !date_match.empty())
    {
      std::stringstream stream;
      stream << date_match[0];

      DayTimestamp  date;
      stream >> date;

      aggregated_files_.emplace(date, file_path);
    }
  }

  removeUnique(aggregated_files_);

  if (aggregated_files_.empty())
  {
    shutdown_manager_.stop();
    logger_->info(
            std::string("Everything is already aggregated"),
            Aspect::REAGGREGATOR);
    return;
  }

  persantage_.setTotalNumber(aggregated_files_.size());

  logger_->info("Total file needed to process = "
                + std::to_string(aggregated_files_.size()),
                Aspect::REAGGREGATOR);

  current_date_ = aggregated_files_.begin()->first;

  for (int i = 1; i <= 2; ++i)
  {
    if (!postTask(
            ThreadID::Read,
            &ReaggregatorMultyThread::doRead))
    {
      Stream::Error ostr;
      ostr << __PRETTY_FUNCTION__
           << " Fatal error: initial read is failed";
      throw Exception(ostr);
    }
  }
}

void ReaggregatorMultyThread::removeUnique(AggregatedFiles& files)
{
  if (files.empty())
    return;

  std::list<DayTimestamp> need_remove;

  auto it = files.begin();
  auto it_next = it;
  auto it_end = files.end();
  std::size_t count = 1;
  while (++it_next != it_end)
  {
    if (it->first == it_next->first)
    {
      count += 1;
      continue;
    }
    else
    {
      if (count == 1)
        need_remove.emplace_back(it->first);
      it = it_next;
      count = 1;
    }
  }
  if (count == 1)
    need_remove.emplace_back(it->first);

  for (const auto& date : need_remove)
  {
    files.erase(date);
  }
}

void ReaggregatorMultyThread::doClean(
        Collector& collector,
        const PoolType poolType)
{
  if (shutdown_manager_.isStoped())
    return;

  if (poolType == PoolType::Merge)
  {
    pool_collector_.addCollector(std::move(collector));
  }
  else if (poolType == PoolType::Temp)
  {
    pool_temp_collector_.addCollector(std::move(collector));
  }
}

void ReaggregatorMultyThread::doWrite(
        const ProcessedFiles& processed_files,
        Collector& collector,
        const LogProcessing::DayTimestamp& date) noexcept
{
  if (shutdown_manager_.isStoped())
    return;

  postTask(
          ThreadID::Read,
          &ReaggregatorMultyThread::doRead);

  if (collector.empty())
    return;

  ResultFile result_file;

  try
  {
    dumpFile(
            output_dir_,
            prefix_,
            date,
            collector,
            result_file);

    postTask(
            ThreadID::Clean,
            &ReaggregatorMultyThread::doClean,
            std::move(collector),
            PoolType::Merge);

    const auto& [temp_path, result_path] = result_file;
    if (std::rename(
            temp_path.c_str(),
            result_path.c_str())) {
      Stream::Error ostr;
      ostr << __PRETTY_FUNCTION__
           << ": Can't rename from="
           << temp_path
           << ", to="
           << result_path;
      throw Exception(ostr.str());
    }

    std::for_each(std::begin(processed_files),
                  std::end(processed_files),
                  [](const auto &path)
                  {
                    std::remove(path.c_str());
                  });
  }
  catch (const eh::Exception& exc)
  {
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << ": Reason:"
           << "[date="
           << date
           << "] "
           << exc.what();
    logger_->error(stream.str(), Aspect::REAGGREGATOR);

    const auto& [temp_path, result_path] = result_file;
    std::remove(temp_path.c_str());
    std::remove(result_path.c_str());
  }
}

void ReaggregatorMultyThread::doRead() noexcept
{
  if (shutdown_manager_.isStoped())
    return;

  if (aggregated_files_.empty())
  {
    if (!is_read_stoped_)
    {
      postTask(
              ThreadID::Calculate,
              &ReaggregatorMultyThread::doFlush,
              current_date_);

      postTask(
              ThreadID::Calculate,
              &ReaggregatorMultyThread::doStop,
              Addressee::Calculator);
    }

    is_read_stoped_ = true;
    return;
  }

  persantage_.increase();

  const auto it = aggregated_files_.begin();
  const auto date = it->first;
  const auto file_path = it->second;
  aggregated_files_.erase(it);

  auto temp_collector = pool_temp_collector_.getCollector();
  try
  {
    LogHelper<LogTraits>::load(file_path, temp_collector);
  }
  catch (const eh::Exception& exc)
  {
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << ": Can't add file="
           << file_path
           << " to collector. Reason: "
           << exc.what();
    logger_->error(stream.str(), Aspect::REAGGREGATOR);

    postTask(
            ThreadID::Read,
            &ReaggregatorMultyThread::doRead);
    return;
  }

  const bool need_flush = (date != current_date_);
  if (need_flush)
  {
    if (!postTask(
            ThreadID::Calculate,
            &ReaggregatorMultyThread::doFlush,
            current_date_))
      return;

    current_date_ = date;
  }
  else
  {
    postTask(ThreadID::Read,
             &ReaggregatorMultyThread::doRead);
  }

  postTask(
          ThreadID::Calculate,
          &ReaggregatorMultyThread::doMerge,
          std::move(temp_collector),
          file_path);
}

void ReaggregatorMultyThread::doFlush(
        const LogProcessing::DayTimestamp& date) noexcept
{
  if (shutdown_manager_.isStoped())
    return;

  try
  {
    Collector save_collector(std::move(collector_));
    collector_ = pool_collector_.getCollector();

    postTask(
            ThreadID::Write,
            &ReaggregatorMultyThread::doWrite,
            std::move(processed_files_),
            std::move(save_collector),
            date);

    processed_files_ = ProcessedFiles();
  }
  catch (const eh::Exception& exc)
  {
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << ": Can't Flush="
           << " to collector. Reason: "
           << exc.what() << "\n";

    for (const auto& path_file : processed_files_)
    {
      stream << "File="
             << path_file
             << " not reaggregated";
    }
    logger_->error(stream.str(), Aspect::REAGGREGATOR);
  }
}

void ReaggregatorMultyThread::doMerge(
        Collector& temp_collector,
        const std::string& file_path) noexcept
{
  if (shutdown_manager_.isStoped())
    return;

  try
  {
    collector_ += temp_collector;
    processed_files_.emplace_back(file_path);

    postTask(
            ThreadID::Clean,
            &ReaggregatorMultyThread::doClean,
            std::move(temp_collector),
            PoolType::Temp);
  }
  catch (const eh::Exception& exc)
  {
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << ": Can't add file="
           << file_path
           << " to collector. Reason: "
           << exc.what() << "\n"
           << "File=" << file_path
           <<  " not aggreagted";
    logger_->error(stream.str(), Aspect::REAGGREGATOR);
  }
}

void ReaggregatorMultyThread::doStop(
        const Addressee addresee) noexcept
{
  if (shutdown_manager_.isStoped())
    return;

  if (addresee == Addressee::Calculator)
  {
    postTask(
            ThreadID::Write,
            &ReaggregatorMultyThread::doStop,
            Addressee::Writer);
  }
  else if (addresee == Addressee::Writer)
  {
    shutdown_manager_.stop();
    logger_->info(
            std::string("Reaggregator completed successfully"),
            Aspect::REAGGREGATOR);
  }
}

void ReaggregatorMultyThread::dumpFile(
        const Path& output_dir,
        const std::string& prefix,
        const DayTimestamp& date,
        Collector& collector,
        ResultFile& result_file)
{
  auto generated_path =
          Utils::GenerateFilePath(output_dir, prefix, date);
  const auto& temp_file_path =
          generated_path.first;

  LogHelper<LogTraits>::save(temp_file_path, collector);
  result_file = std::move(generated_path);
}

void ReaggregatorMultyThread::report_error(
        Severity severity,
        const String::SubString& description,
        const char* error_code) noexcept
{
  if (severity == Severity::CRITICAL_ERROR || severity == Severity::ERROR)
  {
    shutdown_manager_.stop();
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << " : Aggregator stopped due to incorrect operation of queues."
           << " Reason: "
           << description;
    logger_->critical(
            stream.str(),
            Aspect::REAGGREGATOR,
            error_code);
  }
}

} // namespace BidCostPredictor
} // namespace PredictorSvcs
