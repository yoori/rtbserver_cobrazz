// STD
#include <filesystem>

// THIS
#include <LogCommons/BidCostStat.hpp>
#include <Logger/Logger.hpp>
#include <Logger/StreamLogger.hpp>
#include "Aggregator.hpp"
#include "FileCleaner.hpp"
#include "LogHelper.hpp"
#include "Utils.hpp"

namespace Aspect
{

inline constexpr char AGGREGATOR[] = "Aggregator";

} // namespace Aspect

namespace PredictorSvcs::BidCostPredictor
{

Aggregator::Aggregator(
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
    persantage_(logger_, Aspect::AGGREGATOR, 5)
{
  if (!std::filesystem::is_directory(input_dir_))
  {
    Stream::Error stream;
    stream << FNS
           << "Not exist directory="
           << input_dir_;
    throw Exception(stream);
  }

  if (!std::filesystem::is_directory(output_dir_))
  {
    Stream::Error stream;
    stream << FNS
           << "Not exist directory="
           << output_dir_;
    throw Exception(stream);
  }
}

Aggregator::~Aggregator()
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

std::string Aggregator::name() noexcept
{
  return "AggregatorSingleThread";
}

void Aggregator::activate_object_()
{
  thread_ = std::make_unique<std::jthread>([this] () {
    run();
  });
}

void Aggregator::wait_object_()
{
  thread_.reset();
}

void Aggregator::run() noexcept
{
  try
  {
    {
      std::ostringstream stream;
      stream << FNS
             << "Aggregate: started";
      logger_->info(stream.str(), Aspect::AGGREGATOR);
    }

    aggregate();

    {
      std::ostringstream stream;
      stream << FNS
             << "Aggregator completed successfully";
      logger_->info(stream.str(), Aspect::AGGREGATOR);
    }
  }
  catch(const eh::Exception& exc)
  {
    Stream::Error stream;
    stream << FNS
           << "Aggregate is failed: "
           << exc.what();
    logger_->critical(stream.str(), Aspect::AGGREGATOR);
  }

  try
  {
    deactivate_object();
  }
  catch (...)
  {
  }
}

void Aggregator::aggregate()
{
  auto input_files = Utils::get_directory_files(
    input_dir_,
    prefix_stat_);
  persantage_.set_total_number(input_files.size());

  while(!input_files.empty())
  {
    const auto it_begin = std::begin(input_files);
    auto it_end = std::end(input_files);
    if (input_files.size() > max_process_files_)
    {
      it_end = it_begin;
      std::advance(it_end, max_process_files_);
    }

    ProcessFiles files;
    files.splice(
      std::end(files),
      input_files,
      it_begin,
      it_end);
    try
    {
      Aggregator::process_files(files);
    }
    catch (const eh::Exception& exc)
    {
      Stream::Error stream;
      stream << "Can't process files=[";
      std::for_each(
        std::begin(files),
        std::end(files),
        [&stream](const auto& path) {
          stream << path << ";\n";
      });
      stream << "]\n"
             << "Reason: "
             << exc.what();
      logger_->error(stream.str(), Aspect::AGGREGATOR);
    }
  }
}

void Aggregator::process_files(const ProcessFiles& files)
{
  Collector collector;
  PriorityQueue priority_queue;

  ProcessedFiles processed_files;
  ResultFiles result_files;
  FileCleaner file_cleaner(result_files);
  std::size_t record_count = 0;

  Collector temp_collector;
  for (const auto& file_path : files)
  {
    persantage_.increase();
    try
    {
      LogHelper<LogTraits>::load(file_path, temp_collector);
    }
    catch (const eh::Exception &exc)
    {
      Stream::Error stream;
      stream << FNS
             << "Can't process file="
             << file_path
             << ". Reason: "
             << exc.what();
      logger_->error(stream.str(), Aspect::AGGREGATOR);
      continue;
    }

    record_count += merge(
      collector,
      temp_collector,
      priority_queue);

    temp_collector.clear();
    processed_files.emplace_back(file_path);

    while (record_count >= dump_max_size_)
    {
      if (collector.empty())
      {
        Stream::Error stream;
        stream << FNS
               << "Logic error. Collector is empty";
        throw Exception(stream);
      }

      if (priority_queue.empty())
      {
        Stream::Error stream;
        stream << FNS
               << "Logic error. Priority_queue is empty";
        throw Exception(stream);
      }

      const auto date = priority_queue.top();
      priority_queue.pop();
      const auto it = collector.find(KeyCollector(date));
      if (it == collector.end())
      {
        Stream::Error stream;
        stream << FNS
               << "Logic error. Not existing date="
               << date
               << " in collector";
        throw Exception(stream);
      }

      const auto collector_inner = it->second;
      collector.erase(it);

      dump_file(
        output_dir_,
        prefix_agg_,
        date,
        collector_inner,
        result_files);
      record_count -= collector_inner.size();
    }
  }

  for (const auto& [k, v] : collector)
  {
    dump_file(
      output_dir_,
      prefix_agg_,
      k.adv_sdate(),
      v,
      result_files);
  }

  for (const auto& [temp_path, result_path] : result_files)
  {
    if (std::rename(temp_path.c_str(), result_path.c_str()))
    {
      Stream::Error stream;
      stream << FNS
             << "Can't rename file="
             << temp_path
             << " to"
             << result_path;
      throw Exception(stream);
    }
  }

  file_cleaner.remove_temp_files();

  for (const auto& original_path : processed_files)
  {
    std::remove(original_path.c_str());
  }
}

void Aggregator::dump_file(
  const Path& output_dir,
  const std::string& prefix,
  const DayTimestamp& date,
  const CollectorInner& collector,
  ResultFiles& result_files)
{
  const auto generated_path = Utils::generate_file_path(
    output_dir,
    prefix,
    date);
  const auto& temp_path = generated_path.first;

  LogHelper<LogInnerTraits>::save(temp_path, collector);
  result_files.emplace_back(std::move(generated_path));
}

std::size_t Aggregator::merge(
  Collector& collector,
  Collector& temp_collector,
  PriorityQueue& priority_queue)
{
  std::size_t count_added = 0;
  for (auto& [temp_key, temp_inner_collector]: temp_collector)
  {
    if (temp_inner_collector.empty())
    {
      continue;
    }

    if (collector.find(temp_key) == collector.end())
    {
      priority_queue.emplace(temp_key.adv_sdate());
    }

    auto& inner_collector = collector.find_or_insert(temp_key);
    for (const auto& [k, v]: temp_inner_collector)
    {
      KeyCollectorInner new_k(
        k.tag_id(),
        std::string(),
        k.url(),
        k.cost(),
        0);

      if (auto it = inner_collector.find(new_k); it == inner_collector.end())
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