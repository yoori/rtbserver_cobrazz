// STD
#include <filesystem>
#include <regex>

// THIS
#include <Logger/StreamLogger.hpp>
#include "LogHelper.hpp"
#include "ReaggregatorMultyThread.hpp"
#include "Utils.hpp"

namespace Aspect
{

inline constexpr char REAGGREGATOR[] = "REAGGREGATOR";

} // namespace Aspect

namespace PredictorSvcs::BidCostPredictor
{

ReaggregatorMultyThread::ReaggregatorMultyThread(
  const std::string& input_dir,
  const std::string& output_dir,
  Logging::Logger* logger)
  : input_dir_(input_dir),
    output_dir_(output_dir),
    prefix_(LogTraits::B::log_base_name()),
    logger_(ReferenceCounting::add_ref(logger)),
    processed_files_(std::make_shared<ProcessedFiles>()),
    persantage_(logger_, Aspect::REAGGREGATOR, 5)
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

  observer_ = new ActiveObjectObserver(
    [this] (
      const Severity severity,
      const String::SubString& description,
      const char* error_code) {
      if (severity == Severity::CRITICAL_ERROR || severity == Severity::ERROR)
      {
        Stream::Error stream;
        stream << FNS
               << "ReaggregatorMultyThread stopped due to incorrect operation of queues."
               << " Reason: "
               << description;
        logger_->critical(
          stream.str(),
          Aspect::REAGGREGATOR,
          error_code);

        deactivate_object();
      }
    });

  const auto size = static_cast<std::uint8_t>(ThreadID::MAX_NUMBER);
  for (std::uint8_t i = 0; i < size; ++i)
  {
    task_runners_.emplace_back(new Generics::TaskRunner(observer_, 1));
  }

  collector_ = pool_collector_.get_collector();
}

ReaggregatorMultyThread::~ReaggregatorMultyThread()
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
bool ReaggregatorMultyThread::post_task(
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
      deactivate_object();

      Stream::Error stream;
      stream << FNS
             << "Can't enqueue_task"
             << " Reason: "
             << exc.what();
      logger_->critical(stream.str(), Aspect::REAGGREGATOR);
    }
    catch (...)
    {
      deactivate_object();

      Stream::Error stream;
      stream << FNS
             << "Can't enqueue_task"
             << " Reason: Unknown error";
      logger_->critical(stream.str(), Aspect::REAGGREGATOR);
    }

    return false;
  }

void ReaggregatorMultyThread::activate_object_()
{
  {
    std::ostringstream stream;
    stream << FNS
           << "Reaggregator: started";
    logger_->info(stream.str(), Aspect::REAGGREGATOR);
  }

  try
  {
    for (auto& task_runner: task_runners_)
    {
      task_runner->activate_object();
    }
  }
  catch (const eh::Exception &exc)
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
      reaggregate();
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
             << "Reaggregator is failed. Reason: "
             << exc.what();
      logger_->critical(stream.str(), Aspect::REAGGREGATOR);
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
             << "Reaggregator is failed. Reason: Unknown error";
      logger_->critical(stream.str(), Aspect::REAGGREGATOR);
    }
  });
}

void ReaggregatorMultyThread::wait_object_()
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
    logger_->info(stream.str(), Aspect::REAGGREGATOR);
  }
}

void ReaggregatorMultyThread::deactivate_object_()
{
  if (!is_success_completed_.load())
  {
    Stream::Error stream;
    stream << FNS
           << "Reaggregator was aborted...";
    logger_->critical(stream.str(), Aspect::REAGGREGATOR);
  }
}

std::string ReaggregatorMultyThread::name() noexcept
{
  return "Reaggregator";
}

void ReaggregatorMultyThread::reaggregate()
{
  const auto input_files = Utils::get_directory_files(
    input_dir_,
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

      DayTimestamp date;
      stream >> date;

      if (!stream)
      {
        Stream::Error stream;
        stream << FNS
               << "Not correct name of file";
        throw Exception(stream);
      }

      aggregated_files_.emplace(date, file_path);
    }
  }

  remove_unique(aggregated_files_);

  if (aggregated_files_.empty())
  {
    is_success_completed_.store(true);

    std::ostringstream stream;
    stream << FNS
           << "Everything is already reaggregated";
    logger_->info(stream.str(), Aspect::REAGGREGATOR);

    deactivate_object();
    return;
  }

  persantage_.set_total_number(aggregated_files_.size());

  {
    std::ostringstream stream;
    stream << FNS
           << "Total file needed to process = "
           << aggregated_files_.size();
    logger_->info(stream.str(), Aspect::REAGGREGATOR);
  }

  current_date_ = aggregated_files_.begin()->first;

  for (int i = 1; i <= 3; i += 1)
  {
    if (!post_task(
      ThreadID::Read,
      &ReaggregatorMultyThread::do_read))
    {
      Stream::Error stream;
      stream << FNS
             << "Fatal error: initial read is failed";
      throw Exception(stream);
    }
  }
}

void ReaggregatorMultyThread::remove_unique(
  AggregatedFiles& files)
{
  if (files.empty())
  {
    return;
  }

  std::list<DayTimestamp> need_remove;
  auto it = files.begin();
  auto it_next = it;
  const auto it_end = files.end();
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
      {
        need_remove.emplace_back(it->first);
      }
      it = it_next;
      count = 1;
    }
  }

  if (count == 1)
  {
    need_remove.emplace_back(it->first);
  }

  for (const auto& date : need_remove)
  {
    files.erase(date);
  }
}

void ReaggregatorMultyThread::do_clean(
  Collector& collector,
  const PoolType poolType) noexcept
{
  if (!active())
  {
    return;
  }

  try
  {
    if (poolType == PoolType::Merge)
    {
      pool_collector_.add_collector(std::move(collector));
    }
    else if (poolType == PoolType::Temp)
    {
      pool_temp_collector_.add_collector(std::move(collector));
    }
  }
  catch (...)
  {
  }
}

void ReaggregatorMultyThread::do_write(
  const ProcessedFiles_var& processed_files,
  Collector& collector,
  const LogProcessing::DayTimestamp& date) noexcept
{
  if (!active())
  {
    return;
  }

  post_task(
    ThreadID::Read,
    &ReaggregatorMultyThread::do_read);

  if (collector.empty())
  {
    return;
  }

  ResultFile result_file;

  try
  {
    dump_file(
      output_dir_,
      prefix_,
      date,
      collector,
      result_file);

    post_task(
      ThreadID::Clean,
      &ReaggregatorMultyThread::do_clean,
      std::move(collector),
      PoolType::Merge);

    const auto& [temp_path, result_path] = result_file;
    if (std::rename(
      temp_path.c_str(),
      result_path.c_str()))
    {
      Stream::Error ostr;
      ostr << FNS
           << "Can't rename from="
           << temp_path
           << ", to="
           << result_path;
      throw Exception(ostr.str());
    }

    std::for_each(
      std::begin(*processed_files),
      std::end(*processed_files),
      [](const auto &path) {
        std::remove(path.c_str());
      });
  }
  catch (const eh::Exception& exc)
  {
    Stream::Error stream;
    stream << FNS
           << "Reason:"
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

void ReaggregatorMultyThread::do_read() noexcept
{
  if (!active())
  {
    return;
  }

  if (aggregated_files_.empty())
  {
    if (!is_read_stoped_)
    {
      post_task(
        ThreadID::Calculate,
        &ReaggregatorMultyThread::do_flush,
        current_date_);

      post_task(
        ThreadID::Calculate,
        &ReaggregatorMultyThread::do_stop,
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

  auto temp_collector = pool_temp_collector_.get_collector();
  try
  {
    LogHelper<LogTraits>::load(file_path, temp_collector);
  }
  catch (const eh::Exception& exc)
  {
    Stream::Error stream;
    stream << FNS
           << "Can't add file="
           << file_path
           << " to collector. Reason: "
           << exc.what();
    logger_->error(stream.str(), Aspect::REAGGREGATOR);

    post_task(
      ThreadID::Read,
      &ReaggregatorMultyThread::do_read);
    return;
  }

  bool need_task_read_in_merge = false;
  const bool need_flush = (date != current_date_);
  if (need_flush)
  {
    post_task(
      ThreadID::Calculate,
      &ReaggregatorMultyThread::do_flush,
      current_date_);

    current_date_ = date;
  }
  else
  {
    need_task_read_in_merge = true;
  }

  post_task(
    ThreadID::Calculate,
    &ReaggregatorMultyThread::do_merge,
    std::move(temp_collector),
    file_path,
    need_task_read_in_merge);
}

void ReaggregatorMultyThread::do_flush(
  const LogProcessing::DayTimestamp& date) noexcept
{
  if (!active())
  {
    return;
  }

  try
  {
    Collector save_collector(std::move(collector_));
    collector_ = pool_collector_.get_collector();

    post_task(
      ThreadID::Write,
      &ReaggregatorMultyThread::do_write,
      std::move(processed_files_),
      std::move(save_collector),
      date);

    processed_files_ = std::make_shared<ProcessedFiles>();
  }
  catch (const eh::Exception& exc)
  {
    Stream::Error stream;
    stream << FNS
           << "Can't Flush to collector. Reason: "
           << exc.what() << '\n';

    for (const auto& path_file : *processed_files_)
    {
      stream << "File="
             << path_file
             << " not reaggregated";
    }
    logger_->error(stream.str(), Aspect::REAGGREGATOR);
  }
}

void ReaggregatorMultyThread::do_merge(
  Collector& temp_collector,
  const std::string& file_path,
  const bool need_read_task) noexcept
{
  if (!active())
  {
    return;
  }

  if (need_read_task)
  {
    post_task(
      ThreadID::Read,
      &ReaggregatorMultyThread::do_read);
  }

  try
  {
    collector_ += temp_collector;
    processed_files_->emplace_back(file_path);

    post_task(
      ThreadID::Clean,
      &ReaggregatorMultyThread::do_clean,
      std::move(temp_collector),
      PoolType::Temp);
  }
  catch (const eh::Exception& exc)
  {
    std::stringstream stream;
    stream << FNS
           << "Can't add file="
           << file_path
           << " to collector. Reason: "
           << exc.what() << "\n"
           << "File=" << file_path
           <<  " not aggreagted";
    logger_->error(stream.str(), Aspect::REAGGREGATOR);
  }
}

void ReaggregatorMultyThread::do_stop(
  const Addressee addresee) noexcept
{
  if (!active())
  {
    return;
  }

  if (addresee == Addressee::Calculator)
  {
    post_task(
      ThreadID::Write,
      &ReaggregatorMultyThread::do_stop,
      Addressee::Writer);
  }
  else if (addresee == Addressee::Writer)
  {
    is_success_completed_.store(true);
    deactivate_object();

    std::ostringstream stream;
    stream << FNS
           << "Reaggregator completed successfully";
    logger_->info(stream.str(), Aspect::REAGGREGATOR);
  }
}

void ReaggregatorMultyThread::dump_file(
  const Path& output_dir,
  const std::string& prefix,
  const DayTimestamp& date,
  Collector& collector,
  ResultFile& result_file)
{
  auto generated_path = Utils::generate_file_path(
    output_dir,
    prefix,
    date);
  const auto& temp_file_path = generated_path.first;

  LogHelper<LogTraits>::save(temp_file_path, collector);
  result_file = std::move(generated_path);
}

} // namespace PredictorSvcs::BidCostPredictor