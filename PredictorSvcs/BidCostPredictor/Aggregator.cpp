// THIS
#include <LogCommons/BidCostStat.hpp>
#include <Logger/Logger.hpp>
#include <Logger/StreamLogger.hpp>
#include "Aggregator.hpp"
#include "FileCleaner.hpp"
#include "LogHelper.hpp"
#include "Remover.hpp"
#include "Utils.hpp"

namespace Aspect
{
const char AGGREGATOR[] = "Aggregator";
}

namespace PredictorSvcs
{
namespace BidCostPredictor
{

Aggregator::Aggregator(
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
          logger_(logger)
{
}

void Aggregator::start()
{
  try
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

    aggregate();

    logger_->info(
            std::string("Aggregate: finished"),
            Aspect::AGGREGATOR);
  }
  catch(const eh::Exception& ex)
  {
    Stream::Error ostr;
    ostr << __PRETTY_FUNCTION__
         << ": [Fatal error]: Aggregate is failed: "
         << ex.what();
    logger_->critical(ostr.str(), Aspect::AGGREGATOR);
  }
}

void Aggregator::aggregate()
{
  auto input_files = Utils::GetDirectoryFiles(
          input_dir_,
          prefix_stat_);
  total_file_number_ = input_files.size();

  while(!input_files.empty())
  {
    const auto it_begin = std::begin(input_files);
    auto it_end = std::end(input_files);
    if (input_files.size() > max_process_files_)
    {
      it_end = it_begin;
      std::advance(it_end, max_process_files_);
    }

    ProcessFiles process_files;
    process_files.splice(
            std::end(process_files),
            input_files,
            it_begin,
            it_end);
    try
    {
      processFiles(process_files);
    }
    catch (const eh::Exception& exc)
    {
      std::stringstream stream;
      stream << "Can't process files=[";
      std::for_each(
              std::begin(process_files),
              std::end(process_files),
              [&stream](const auto& path){
                stream << path << ";\n";
              });
      stream << "]\n"
             << "Reason: "
             << exc.what();
      logger_->error(stream.str(), Aspect::AGGREGATOR);
    }
  }
}

void Aggregator::stop() noexcept
{
}

void Aggregator::wait() noexcept
{
}

const char* Aggregator::name() noexcept
{
  return "AggregatorSingleThread";
}

void Aggregator::processFiles(const ProcessFiles& files)
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
    const std::size_t percentage
            = (current_file_number_ * 100) / total_file_number_;
    if (percentage >= counter_percentage_ * 5)
    {
      counter_percentage_ += 1;
      logger_->info(
              "Percentage of processed files = " + std::to_string(percentage),
              Aspect::AGGREGATOR);
    }
    current_file_number_ += 1;

    try
    {
        LogHelper<LogTraits>::load(file_path, temp_collector);
    }
    catch (const eh::Exception &exc)
    {
      Stream::Error ostr;
      ostr << __PRETTY_FUNCTION__
           << ": Can't process file="
           << file_path
           << ". Reason: "
           << exc.what();
      logger_->error(ostr.str(), Aspect::AGGREGATOR);
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
        std::cout << record_count << std::endl;
        throw Exception("Logic error. Collector is empty");
      }

      if (priority_queue.empty())
        throw Exception("Logic error. Priority_queue is empty");

      const auto date = priority_queue.top();
      priority_queue.pop();
      const auto it = collector.find(KeyCollector(date));
      if (it == collector.end())
      {
        Stream::Error ostr;
        ostr << ": Logic error. Not existing date="
             << date
             << " in collector";
        throw Exception(ostr);
      }
      Remover remover(collector, it);

      dumpFile(output_dir_,
               prefix_agg_,
               date,
               it->second,
               result_files);
      record_count -= it->second.size();
    }
  }

  for (const auto& [k, v]: collector)
  {
    dumpFile(output_dir_,
             prefix_agg_,
             k.adv_sdate(),
             v,
             result_files);
  }

  for (const auto& [temp_path, result_path]: result_files)
  {
    if (std::rename(temp_path.c_str(), result_path.c_str()))
    {
      Stream::Error ostr;
      ostr << __PRETTY_FUNCTION__
           << ": Can't rename file="
           << temp_path
           << " to"
           << result_path;
      throw Exception(ostr);
    }
  }

  file_cleaner.clearTemp();

  for (const auto& original_path : processed_files)
  {
    std::remove(original_path.c_str());
  }
}

void Aggregator::dumpFile(
        const Path& output_dir,
        const std::string& prefix,
        const DayTimestamp& date,
        const CollectorInner& collector,
        ResultFiles& result_files)
{
  const auto generated_path =
          Utils::GenerateFilePath(output_dir, prefix, date);
  const auto& temp_path =
          generated_path.first;

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
      continue;

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

} // namespace PredictorSvcs
} // namespace BidCostPredictor
