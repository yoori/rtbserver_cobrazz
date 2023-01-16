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
  Logging::Logger* logger)
  : max_process_files_(max_process_files),
    dump_max_size_(dump_max_size),
    input_dir_(input_dir),
    output_dir_(output_dir),
    prefix_stat_(LogTraits::B::log_base_name()),
    prefix_agg_(LogInnerTraits::B::log_base_name()),
    logger_(ReferenceCounting::add_ref(logger)),
    observer_(new ActiveObjectObserver(this)),
    persantage_(logger_, Aspect::AGGREGATOR, 5),
    processed_files_(std::make_shared<ProcessedFiles>())
{
  for (std::uint8_t i = 1; i <= COUNT_THREADS; ++i)
  {
    task_runners_.emplace_back(new Generics::TaskRunner(observer_, 1));
  }
}

AggregatorMultyThread::~AggregatorMultyThread()
{
  shutdown_manager_.stop();
  observer_->clear_delegate();
  wait();
}

void AggregatorMultyThread::start()
{
  logger_->info(
    std::string("Aggregate: started"),
    Aspect::AGGREGATOR);

  if (!Utils::exist_directory(input_dir_))
  {
    Stream::Error ostr;
    ostr << __PRETTY_FUNCTION__
         << ": Not existing input directory="
         << input_dir_;
    throw Exception(ostr);
  }

  if (!Utils::exist_directory(output_dir_))
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
    shutdown_manager_.stop();
    Stream::Error ostr;
    ostr << __PRETTY_FUNCTION__
         << ": Can't init task runner : "
         << ex.what();
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

  if (!is_success_completed_)
  {
    try
    {
      auto need_remove_files =
        Utils::get_directory_files(
          output_dir_,
          "~");
      for (const auto& path : need_remove_files)
      {
        std::remove(path.c_str());
      }
    }
    catch (...)
    {}
  }
}

const char* AggregatorMultyThread::name() noexcept
{
  return "AggregatorMultyThread";
}

void AggregatorMultyThread::stop() noexcept
{
  logger_->critical(
    std::string("Aggregator was interrupted..."),
    Aspect::AGGREGATOR);
  shutdown_manager_.stop();
}

void AggregatorMultyThread::aggregate()
{
  input_files_ = Utils::get_directory_files(
    input_dir_,
    prefix_stat_);

  persantage_.set_total_number(input_files_.size());

  for (int i = 1; i <= 3; ++i)
  {
    if (!post_task(
      ThreadID::Read,
      &AggregatorMultyThread::do_read))
      throw Exception("Fatal error: inital read is failed");
  }
}

void AggregatorMultyThread::do_read() noexcept
{
  using FlushPointer = void(AggregatorMultyThread::*)();

  if (shutdown_manager_.is_stoped())
    return;

  if (input_files_.empty())
  {
    if (!is_read_stoped_)
    {
      post_task(
        ThreadID::Calculate,
        static_cast<FlushPointer>(&AggregatorMultyThread::do_flush));

      post_task(
        ThreadID::Calculate,
        &AggregatorMultyThread::do_stop,
        Addressee::Calculator);
    }

    is_read_stoped_ = true;
    return;
  }

  if (count_process_file_ == max_process_files_)
  {
    post_task(
      ThreadID::Calculate,
      static_cast<FlushPointer>(&AggregatorMultyThread::do_flush));
    count_process_file_ = 0;
  }

  persantage_.increase();

  const auto it = input_files_.begin();
  const auto file_path = *it;
  input_files_.erase(it);

  try
  {
    Collector temp_collector;
    LogHelper<LogTraits>::load(
      file_path,
      temp_collector);
    post_task(
      ThreadID::Calculate,
      &AggregatorMultyThread::do_merge,
      std::move(temp_collector),
      std::move(file_path));
    count_process_file_ += 1;
  }
  catch (const eh::Exception& exc)
  {
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << ": Can't process file="
           << file_path
           << " Reason: "
           << exc.what();
    logger_->error(stream.str(), Aspect::AGGREGATOR);
    post_task(
      ThreadID::Read,
      &AggregatorMultyThread::do_read);
    return;
  }
}

void AggregatorMultyThread::do_flush() noexcept
{
  using FlushPointer = void(AggregatorMultyThread::*)(const ProcessedFiles_var&);

  if (shutdown_manager_.is_stoped())
    return;

  for (auto& [key, inner_collector] : agg_collector_)
  {
    post_task(
      ThreadID::Write,
      &AggregatorMultyThread::do_write,
      std::move(inner_collector),
      key.adv_sdate(),
      false);
  }

  post_task(
    ThreadID::Write,
    static_cast<FlushPointer>(&AggregatorMultyThread::do_flush),
    std::move(processed_files_));

  processed_files_ = std::make_shared<ProcessedFiles>();
  agg_collector_.clear();
  while (!priority_queue_.empty())
  {
    priority_queue_.pop();
  }
}

void AggregatorMultyThread::do_flush(
  const ProcessedFiles_var& processed_files) noexcept
{
  if (shutdown_manager_.is_stoped())
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

    file_cleaner.clear_temp();
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

void AggregatorMultyThread::do_merge(
  const Collector& temp_collector,
  const Path& original_file) noexcept
{
  if (shutdown_manager_.is_stoped())
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
      post_task(
        ThreadID::Read,
        &AggregatorMultyThread::do_read);
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
      auto& inner_collector = it->second;

      record_count -= inner_collector.size();

      post_task(
        ThreadID::Write,
        &AggregatorMultyThread::do_write,
        std::move(inner_collector),
        date,
        need_add_read);
      need_add_read = false;
    }
  }
  catch (const eh::Exception& exc)
  {
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << ": Critical error: "
           << exc.what();
    logger_->critical(stream.str(), Aspect::AGGREGATOR);
    shutdown_manager_.stop();
  }
}

void AggregatorMultyThread::do_write(
  const CollectorInner& collector,
  const DayTimestamp& date,
  const bool need_add_read) noexcept
{
  if (shutdown_manager_.is_stoped())
    return;

  if (need_add_read)
  {
    post_task(
      ThreadID::Read,
      &AggregatorMultyThread::do_read);
  }

  if (collector.empty())
    return;

  try
  {
    const auto generated_path =
      Utils::generate_file_path(
        output_dir_,
        prefix_agg_,
        date);
    const auto& temp_path = generated_path.first;

    LogHelper<LogInnerTraits>::save(temp_path, collector);
    result_files_.emplace_back(generated_path);
  }
  catch (const eh::Exception& exc)
  {
    shutdown_manager_.stop();
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << ": Critical error: "
           << exc.what();
    logger_->critical(stream.str(), Aspect::AGGREGATOR);
  }
}

void AggregatorMultyThread::do_stop(
  const Addressee addresee) noexcept
{
  if (shutdown_manager_.is_stoped())
    return;

  if (addresee == Addressee::Calculator)
  {
    post_task(
      ThreadID::Write,
      &AggregatorMultyThread::do_stop,
      Addressee::Writer);
  }
  else if (addresee == Addressee::Writer)
  {
    is_success_completed_ = true;
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
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << " : Aggregator stopped due to incorrect operation of queues."
           << " Reason: "
           << description;
    logger_->critical(stream.str(), Aspect::AGGREGATOR, error_code);
    shutdown_manager_.stop();
  }
}

Generics::TaskRunner& AggregatorMultyThread::get_task_runner(
  const ThreadID id) noexcept
{
  return *task_runners_[static_cast<std::size_t>(id)];
}

} // namespace BidCostPredictor
} // namespace PredictorSvcs