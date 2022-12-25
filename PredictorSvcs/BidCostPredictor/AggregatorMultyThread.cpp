// STD
#include <regex>

// THIS
#include <Commons/DelegateTaskGoal.hpp>
#include <Logger/ActiveObjectCallback.hpp>
#include <Logger/Logger.hpp>
#include <Logger/StreamLogger.hpp>
#include "AggregatorMultyThread.hpp"
#include "FileCleaner.hpp"
#include "LogHelper.hpp"
#include "Remover.hpp"
#include "Utils.hpp"

namespace Aspect
{
const char* AGGREGATOR = "Aggregator";
}

namespace PredictorSvcs
{
namespace BidCostPredictor
{

AggregatorMultyThread::AggregatorMultyThread(
        const std::size_t max_process_files,
        const std::size_t dump_max_size,
        const std::string& input_dir,
        const std::string& output_dir,
        const Logging::Logger_var& logger)
        : max_process_files_(max_process_files),
          dump_max_size_(dump_max_size),
          input_dir_(input_dir),
          output_dir_(output_dir),
          prefix_stat_(LogTraits::B::log_base_name()),
          prefix_agg_(LogInnerTraits::B::log_base_name()),
          logger_(logger),
          processed_files_(std::make_shared<ProcessedFiles>())
{
  for (std::uint8_t i = 1; i <= COUNT_THREADS; ++i)
  {
    task_runners_.emplace_back(new Generics::TaskRunner(this, 1));
  }
}

AggregatorMultyThread::~AggregatorMultyThread()
{
  wait();
}

void AggregatorMultyThread::start()
{
  logger_->info(
          std::string("Aggregate: started"),
          Aspect::AGGREGATOR);

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

  if (max_process_files_ == 0)
  {
    Stream::Error ostr;
    ostr << __PRETTY_FUNCTION__
         << ": max_process_files can't be 0";
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
  catch (const eh::Exception& ex)
  {
    Stream::Error ostr;
    ostr << __PRETTY_FUNCTION__
         << ": Can't init task runner : "
         << ex.what();
    shutdown_manager_.stop();
    throw Exception(ostr);
  }

  aggregate();
}

void AggregatorMultyThread::wait() noexcept
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

const char* AggregatorMultyThread::name() noexcept
{
  return "AggregatorMultyThread";
}

void AggregatorMultyThread::stop() noexcept
{
  logger_->critical(
          std::string("Aggregator was abborted..."),
          Aspect::AGGREGATOR);
  shutdown_manager_.stop();
}

void AggregatorMultyThread::aggregate()
{
  input_files_ = Utils::GetDirectoryFiles(
          input_dir_,
          prefix_stat_);

  persantage_info_.total_files_number = input_files_.size();

  for (int i = 1; i <= 3; ++i)
  {
    if (!postTask(
            ThreadID::Read,
            &AggregatorMultyThread::doRead))
      throw Exception("Fatal error: inital read is failed");
  }
}

void AggregatorMultyThread::doRead() noexcept
{
  using FlushPointer = void(AggregatorMultyThread::*)();

  if (shutdown_manager_.isStoped())
    return;

  if (input_files_.empty())
  {
    if (!is_read_stoped_)
    {
      postTask(
              ThreadID::Calculate,
              static_cast<FlushPointer>(&AggregatorMultyThread::doFlush));
      postTask(
              ThreadID::Calculate,
              &AggregatorMultyThread::doStop,
              Addressee::Calculator);
    }

    is_read_stoped_ = true;
    return;
  }

  if (count_process_file_ == max_process_files_)
  {
    postTask(
            ThreadID::Calculate,
            static_cast<FlushPointer>(&AggregatorMultyThread::doFlush));
    count_process_file_ = 0;
  }

  const std::size_t percentage
        = (persantage_info_.current_file_number * 100) / persantage_info_.total_files_number;
  if (percentage >= persantage_info_.counter_percentage * 5)
  {
    persantage_info_.counter_percentage += 1;
    logger_->info(
            "Percentage of processed files = "
            + std::to_string(percentage),
            Aspect::AGGREGATOR);
  }
  persantage_info_.current_file_number += 1;

  const auto it = input_files_.begin();
  Remover remover(input_files_, it);
  auto& file_path = *it;

  try
  {
    Collector temp_collector;
    LogHelper<LogTraits>::load(
            file_path,
            temp_collector);
    postTask(
            ThreadID::Calculate,
            &AggregatorMultyThread::doMerge,
            std::move(temp_collector),
            std::move(file_path));
    count_process_file_ += 1;
  }
  catch (const eh::Exception& exc)
  {
    Stream::Error ostr;
    ostr << __PRETTY_FUNCTION__
         << ": Can't process file="
         << file_path
         << " Reason: "
         << exc.what();
    logger_->error(ostr.str(), Aspect::AGGREGATOR);
    postTask(
            ThreadID::Read,
            &AggregatorMultyThread::doRead);
    return;
  }
}

void AggregatorMultyThread::doFlush() noexcept
{
  using FlushPointer = void(AggregatorMultyThread::*)(const ProcessedFiles_var&);

  if (shutdown_manager_.isStoped())
    return;

  for (auto& [key, inner_collector] : agg_collector_)
  {
    postTask(
            ThreadID::Write,
            &AggregatorMultyThread::doWrite,
            std::move(inner_collector),
            key.adv_sdate(),
            false);
  }

  postTask(ThreadID::Write,
           static_cast<FlushPointer>(&AggregatorMultyThread::doFlush),
           std::move(processed_files_));

  processed_files_ = std::make_shared<ProcessedFiles>();
  agg_collector_.clear();
  while (!priority_queue_.empty())
  {
    priority_queue_.pop();
  }
}

void AggregatorMultyThread::doFlush(
        const ProcessedFiles_var& processed_files) noexcept
{
  if (shutdown_manager_.isStoped())
    return;

  try
  {
    FileCleaner file_cleaner(result_files_);
    for (const auto& [temp_path, result_path] : result_files_)
    {
      if (std::rename(temp_path.c_str(), result_path.c_str()))
      {
        Stream::Error ostr;
        ostr << "Can't rename file="
             << temp_path
             << " to"
             << result_path;
        throw Exception(ostr);
      }
    }

    for (const auto& original_file : *processed_files)
    {
      std::remove(original_file.c_str());
    }

    file_cleaner.clearTemp();
  }
  catch (const eh::Exception& exc)
  {
    std::stringstream stream;
    stream << "Error process files=[\n";
    for (const auto& original_file : *processed_files)
    {
      stream << original_file
             << "\n";
    }
    stream << "]";
    logger_->error(stream.str(), Aspect::AGGREGATOR);
  }

  result_files_.clear();
}

void AggregatorMultyThread::doMerge(
        const Collector& temp_collector,
        const Path& original_file) noexcept
{
  if (shutdown_manager_.isStoped())
    return;

  try
  {
    const auto count_added = merge(
            temp_collector,
            agg_collector_,
            priority_queue_);
    record_count += count_added;
    processed_files_->emplace_back(original_file);

    if (record_count < dump_max_size_)
    {
      postTask(
              ThreadID::Read,
              &AggregatorMultyThread::doRead);
    }

    bool need_add_read = true;
    while(record_count >= dump_max_size_)
    {
      if (agg_collector_.empty())
        throw Exception("Logic error. agg_collector is empty");

      if (priority_queue_.empty())
        throw Exception("Logic error. priority_queue is empty");

      const auto date = priority_queue_.top();
      priority_queue_.pop();
      auto it = agg_collector_.find(KeyCollector(date));
      if (it == agg_collector_.end())
      {
        Stream::Error ostr;
        ostr << ": Logic error. Not existing date="
             << date
             << " in agg_collector";
        throw Exception(ostr);
      }
      Remover remover(agg_collector_, it);

      record_count -= it->second.size();

      postTask(
              ThreadID::Write,
              &AggregatorMultyThread::doWrite,
              std::move(it->second),
              date,
              need_add_read);
      need_add_read = false;
    }
  }
  catch (const eh::Exception& exc)
  {
    Stream::Error ostr;
    ostr << __PRETTY_FUNCTION__
         << ": Critical error: "
         << exc.what();
    logger_->critical(ostr.str(), Aspect::AGGREGATOR);
    shutdown_manager_.stop();
  }
}

void AggregatorMultyThread::doWrite(
        const CollectorInner& collector,
        const DayTimestamp& date,
        const bool need_add_read) noexcept
{
  if (shutdown_manager_.isStoped())
    return;

  if (need_add_read)
    postTask(
            ThreadID::Read,
            &AggregatorMultyThread::doRead);


  if (collector.empty())
    return;

  try
  {
    const auto generated_path =
            Utils::GenerateFilePath(
                    output_dir_,
                    prefix_agg_,
                    date);
    const auto& temp_path = generated_path.first;

    LogHelper<LogInnerTraits>::save(temp_path, collector);
    result_files_.emplace_back(generated_path);
  }
  catch (const eh::Exception& exc)
  {
    Stream::Error ostr;
    ostr << __PRETTY_FUNCTION__
         << ": Critical error: "
         << exc.what();
    logger_->critical(ostr.str(), Aspect::AGGREGATOR);
    shutdown_manager_.stop();
  }
}

void AggregatorMultyThread::doStop(
        const Addressee addresee) noexcept
{
  if (shutdown_manager_.isStoped())
    return;

  if (addresee == Addressee::Calculator)
  {
    postTask(
            ThreadID::Write,
            &AggregatorMultyThread::doStop,
            Addressee::Writer);
  }
  else if (addresee == Addressee::Writer)
  {
    shutdown_manager_.stop();
    logger_->info(
            std::string("Aggregator completed successfully"),
            Aspect::AGGREGATOR);
  }
}

std::size_t AggregatorMultyThread::merge(
        const Collector& temp_collector,
        Collector& collector,
        PriorityQueue& priority_queue)
{
  std::size_t count_added = 0;
  for (auto& [temp_key, temp_inner_collector] : temp_collector)
  {
    if (temp_inner_collector.empty())
      continue;

    if (collector.find(temp_key) == collector.end())
    {
      priority_queue.emplace(temp_key.adv_sdate());
    }

    auto& inner_collector = collector.find_or_insert(temp_key);
    for (const auto& [k, v]: temp_inner_collector) {
      KeyCollectorInner new_k(
              k.tag_id(),
              std::string(),
              k.url(),
              k.cost());

      if (auto it = inner_collector.find(new_k);
          it == inner_collector.end())
      {
        count_added += 1;
        inner_collector.add(new_k, v);
      }
      else
      {
        it->second += v;
      }
    }
  }

  return count_added;
}

void AggregatorMultyThread::report_error(
        Severity severity,
        const String::SubString& description,
        const char* error_code) noexcept
{
  if (severity == Severity::CRITICAL_ERROR || severity == Severity::ERROR)
  {
    Stream::Error ostr;
    ostr << __PRETTY_FUNCTION__
         << " : Aggregator stopped due to incorrect operation of queues."
         << " Reason: "
         << description;
    logger_->critical(ostr.str(), Aspect::AGGREGATOR, error_code);
    shutdown_manager_.stop();
  }
}

Generics::TaskRunner&
AggregatorMultyThread::getTaskRunner(const ThreadID id) noexcept
{
  return *task_runners_[static_cast<std::size_t>(id)];
}

} // namespace BidCostPredictor
} // namespace PredictorSvcs