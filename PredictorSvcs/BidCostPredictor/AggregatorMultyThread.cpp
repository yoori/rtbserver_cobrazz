// STD
#include <filesystem>
#include <regex>

// THIS
#include <Commons/DelegateTaskGoal.hpp>
#include <Logger/ActiveObjectCallback.hpp>
#include <Logger/Logger.hpp>
#include <Logger/StreamLogger.hpp>
#include "AggregatorMultyThread.hpp"
#include "FileCleaner.hpp"
#include "LogHelper.hpp"
#include "Utils.hpp"

namespace Aspect
{

inline constexpr char AGGREGATOR[] = "Aggregator";

} // namespace Aspect

namespace PredictorSvcs::BidCostPredictor
{

AggregatorMultyThread::AggregatorMultyThread(
  const std::size_t max_process_files,
  const std::size_t dump_max_size,
  const std::string& input_dir,
  const std::string& output_dir,
  const std::optional<ArchiveParams>& archive_params,
  Logger* logger)
  : max_process_files_(max_process_files),
    dump_max_size_(dump_max_size),
    input_dir_(input_dir),
    output_dir_(output_dir),
    archive_params_(archive_params),
    prefix_stat_(LogTraits::B::log_base_name()),
    prefix_agg_(LogInnerTraits::B::log_base_name()),
    logger_(ReferenceCounting::add_ref(logger)),
    persantage_(logger_, Aspect::AGGREGATOR, 5),
    processed_files_(std::make_shared<ProcessedFiles>())
{
  using Severity = Generics::ActiveObjectCallback::Severity;

  if (!std::filesystem::is_directory(input_dir_))
  {
    Stream::Error stream;
    stream << FNS
           << "Not existing input directory="
           << input_dir_;
    throw Exception(stream);
  }

  if (!std::filesystem::is_directory(output_dir_))
  {
    Stream::Error stream;
    stream << FNS
           << "Not existing output directory="
           << output_dir_;
    throw Exception(stream);
  }

  if (max_process_files_ == 0)
  {
    Stream::Error stream;
    stream << FNS
           << "max_process_files can't be 0";
    throw Exception(stream);
  }

  observer_ = new ActiveObjectObserver(
    [this] (
      const Severity severity,
      const String::SubString& description,
      const char* error_code) {
      if (severity == Severity::CRITICAL_ERROR || severity == Severity::ERROR)
      {
        Stream::Error stream;
        stream << FNS
               << "AggregatorMultyThread stopped due to incorrect operation of queues."
               << " Reason: "
               << description;
        logger_->critical(stream.str(), Aspect::AGGREGATOR, error_code);

        deactivate_object();
      }
    });

  const auto size = static_cast<std::uint8_t>(ThreadID::MAX_NUMBER);
  for (std::uint8_t i = 0; i < size; ++i)
  {
    task_runners_.emplace_back(new Generics::TaskRunner(observer_, 1));
  }
}

AggregatorMultyThread::~AggregatorMultyThread()
{
  try
  {
    const auto need_remove_files = Utils::get_directory_files(
      output_dir_,
      "~");
    for (const auto& path : need_remove_files)
    {
      std::remove(path.c_str());
    }
  }
  catch (...)
  {
  }
}

template<ConceptMemberPtr MemPtr, class ...Args>
bool AggregatorMultyThread::post_task(
  const ThreadID id,
  MemPtr mem_ptr,
  Args&& ...args) noexcept
{
  try
  {
    if (task_runners_.size() <= static_cast<std::size_t>(id))
    {
      Stream::Error stream;
      stream << "Thread id="
             << static_cast<std::size_t>(id)
             << " greater than number of threads="
             << task_runners_.size();
      throw Exception(stream);
    }

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
    Stream::Error stream;
    stream << FNS
           << "Can't enqueue_task. Reason: "
           << exc.what();
    logger_->critical(stream.str(), Aspect::AGGREGATOR);

    deactivate_object();
  }
  catch (...)
  {
    Stream::Error stream;
    stream << FNS
           << "Can't enqueue_task. Reason: Unknown error";
    logger_->critical(stream.str(), Aspect::AGGREGATOR);

    deactivate_object();
  }

  return false;
}

void AggregatorMultyThread::activate_object_()
{
  {
    std::ostringstream stream;
    stream << FNS
           << "Aggregator: started";
    logger_->info(stream.str(), Aspect::AGGREGATOR);
  }

  try
  {
    for (auto& task_runner: task_runners_)
    {
      task_runner->activate_object();
    }
  }
  catch (const eh::Exception& exc)
  {
    Stream::Error stream;
    stream << FNS
           << "Can't init task runner : "
           << exc.what();
    throw Exception(stream);
  }
  catch (...)
  {
    Stream::Error stream;
    stream << FNS
           << "Can't init task runner : Unknown error";
    throw Exception(stream);
  }

  helper_thread_ = std::make_unique<std::jthread>([this] () {
    try
    {
      aggregate();
    }
    catch (const eh::Exception& exc)
    {
      try
      {
        deactivate_object();
      }
      catch (...)
      {
      }

      Stream::Error stream;
      stream << FNS
             << "Aggregator is failed. Reason: "
             << exc.what();
      logger_->critical(stream.str(), Aspect::AGGREGATOR);
    }
    catch (...)
    {
      try
      {
        deactivate_object();
      }
      catch (...)
      {
      }

      Stream::Error stream;
      stream << FNS
             << "Aggregator is failed. Reason: Unknown error";
      logger_->critical(stream.str(), Aspect::AGGREGATOR);
    }
  });
}

void AggregatorMultyThread::wait_object_()
{
  helper_thread_.reset();

  for (auto& task_runner : task_runners_)
  {
    try
    {
      task_runner->wait_for_queue_exhausting();
    }
    catch (...)
    {
    }

    try
    {
      task_runner->deactivate_object();
      task_runner->wait_object();
    }
    catch (...)
    {
    }
  }

  task_runners_.clear();

  {
    std::ostringstream stream;
    stream << FNS
           << "Reaggregator: is stopped";
    logger_->info(stream.str(), Aspect::AGGREGATOR);
  }
}

void AggregatorMultyThread::deactivate_object_()
{
  if (!is_success_completed_.load())
  {
    Stream::Error stream;
    stream << FNS
           << "Aggregator was aborted...";
    logger_->critical(stream.str(), Aspect::AGGREGATOR);
  }
}

std::string AggregatorMultyThread::name() noexcept
{
  return "Aggregator";
}

void AggregatorMultyThread::aggregate()
{
  input_files_ = Utils::get_directory_files(
    input_dir_,
    prefix_stat_);

  persantage_.set_total_number(input_files_.size());

  for (int i = 1; i <= 3; i += 1)
  {
    if (!post_task(ThreadID::Read, &AggregatorMultyThread::do_read))
    {
      Stream::Error stream;
      stream << FNS
             << "Initial read is failed";
      throw Exception(stream);
    }
  }
}

void AggregatorMultyThread::do_read() noexcept
{
  using FlushPointer = void(AggregatorMultyThread::*)();

  if (!active())
  {
    return;
  }

  if (input_files_.empty())
  {
    if (!is_read_stopped_)
    {
      post_task(
        ThreadID::Calculate,
        static_cast<FlushPointer>(&AggregatorMultyThread::do_flush));

      post_task(
        ThreadID::Calculate,
        &AggregatorMultyThread::do_stop,
        Addressee::Calculator);
    }

    is_read_stopped_ = true;
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
    LogHelper<LogTraits>::load(file_path, temp_collector);
    post_task(
      ThreadID::Calculate,
      &AggregatorMultyThread::do_merge,
      std::move(temp_collector),
      std::move(file_path));
    count_process_file_ += 1;
  }
  catch (const eh::Exception& exc)
  {
    Stream::Error stream;
    stream << FNS
           << "Can't process file="
           << file_path
           << " Reason: "
           << exc.what();
    logger_->error(stream.str(), Aspect::AGGREGATOR);

    post_task(ThreadID::Read, &AggregatorMultyThread::do_read);
  }
}

void AggregatorMultyThread::do_flush() noexcept
{
  using FlushPointer = void(AggregatorMultyThread::*)(const ProcessedFiles_var&);

  if (!active())
  {
    return;
  }

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
  if (!active())
  {
    return;
  }

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

    file_cleaner.remove_temp_files();
  }
  catch (const eh::Exception& exc)
  {
    Stream::Error stream;
    stream << "Error process files=[\n";
    for (const auto& original_file : *processed_files)
    {
      stream << original_file
             << '\n';
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
  if (!active())
  {
    return;
  }

  try
  {
    const auto count_added = merge(
      temp_collector,
      agg_collector_,
      priority_queue_);
    record_count_ += count_added;
    processed_files_->emplace_back(original_file);

    if (record_count_ < dump_max_size_)
    {
      post_task(
        ThreadID::Read,
        &AggregatorMultyThread::do_read);
      return;
    }

    bool need_add_read = true;
    while (record_count_ >= dump_max_size_)
    {
      if (agg_collector_.empty())
      {
        Stream::Error stream;
        stream << FNS
               << "Logic error: agg_collector is empty";
        throw Exception(stream);
      }

      if (priority_queue_.empty())
      {
        Stream::Error stream;
        stream << FNS
               << "Logic error: priority_queue is empty";
        throw Exception(stream);
      }

      const auto date = priority_queue_.top();
      priority_queue_.pop();
      const auto it = agg_collector_.find(KeyCollector(date));
      if (it == agg_collector_.end())
      {
        Stream::Error ostr;
        ostr << FNS
             << "Logic error. Not existing date="
             << date
             << " in agg_collector";
        throw Exception(ostr);
      }
      auto inner_collector = it->second;
      agg_collector_.erase(it);

      record_count_ -= inner_collector.size();

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
    Stream::Error stream;
    stream << FNS
           << "Critical error: "
           << exc.what();
    logger_->critical(stream.str(), Aspect::AGGREGATOR);

    deactivate_object();
  }
}

void AggregatorMultyThread::do_write(
  const CollectorInner& collector,
  const DayTimestamp& date,
  const bool need_add_read) noexcept
{
  if (!active())
  {
    return;
  }

  if (need_add_read)
  {
    post_task(
      ThreadID::Read,
      &AggregatorMultyThread::do_read);
  }

  if (collector.empty())
  {
    return;
  }

  try
  {
    auto generated_path = Utils::generate_file_path(
      output_dir_,
      prefix_agg_,
      date);
    const auto& temp_path = generated_path.first;

    const auto extension = LogHelper<LogInnerTraits>::save(
      temp_path,
      collector,
      archive_params_);

    generated_path.first += extension;
    generated_path.second += extension;

    result_files_.emplace_back(generated_path);
  }
  catch (const eh::Exception& exc)
  {
    Stream::Error stream;
    stream << FNS
           << "Critical error: "
           << exc.what();
    logger_->critical(stream.str(), Aspect::AGGREGATOR);

    deactivate_object();
  }
}

void AggregatorMultyThread::do_stop(const Addressee addresee) noexcept
{
  if (!active())
  {
    return;
  }

  if (addresee == Addressee::Calculator)
  {
    post_task(
      ThreadID::Write,
      &AggregatorMultyThread::do_stop,
      Addressee::Writer);
  }
  else if (addresee == Addressee::Writer)
  {
    is_success_completed_.store(true);
    deactivate_object();

    std::ostringstream stream;
    stream << FNS
           << "Aggregator completed successfully";
    logger_->info(stream.str(), Aspect::AGGREGATOR);
  }
}

std::size_t AggregatorMultyThread::merge(
  const Collector& temp_collector,
  Collector& collector,
  PriorityQueue& priority_queue)
{
  std::size_t count_added = 0;
  for (const auto& [temp_key, temp_inner_collector] : temp_collector)
  {
    if (temp_inner_collector.empty())
    {
      continue;
    }

    if (collector.find(temp_key) == std::end(collector))
    {
      priority_queue.emplace(temp_key.adv_sdate());
    }

    auto& inner_collector = collector.find_or_insert(temp_key);
    for (const auto& [k, v]: temp_inner_collector) {
      KeyCollectorInner new_k(
        k.tag_id(),
        std::string(),
        k.url(),
        k.cost(),
        0);

      auto it = inner_collector.find(new_k);
      if (it == inner_collector.end())
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

} // namespace PredictorSvcs::BidCostPredictor