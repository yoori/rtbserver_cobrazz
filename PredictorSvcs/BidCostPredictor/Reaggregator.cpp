// THIS
#include <Logger/Logger.hpp>
#include <Logger/StreamLogger.hpp>
#include "LogHelper.hpp"
#include "Reaggregator.hpp"
#include "Utils.hpp"

// STD
#include <regex>

namespace Aspect
{
const char REAGGREGATOR[] = "Reaggregator";
}

namespace PredictorSvcs
{
namespace BidCostPredictor
{

Reaggregator::Reaggregator(
        const std::string& input_dir,
        const std::string& output_dir,
        const Logging::Logger_var& logger)
        : input_dir_(input_dir),
          output_dir_(output_dir),
          prefix_(LogTraits::B::log_base_name()),
          logger_(logger)
{
}

void Reaggregator::start()
{
  try
  {
    logger_->info(
            std::string("Reaggregator: started"),
            Aspect::REAGGREGATOR);

    reaggregate(input_dir_, output_dir_, prefix_);

    logger_->info(
            std::string("Reaggregator: finished"),
            Aspect::REAGGREGATOR);
  }
  catch (const eh::Exception& ex)
  {
    Stream::Error ostr;
    ostr << __PRETTY_FUNCTION__
         << ": [Fatal error]: Reaggregate is failed:"
         << ex.what();

    logger_->error(ostr.str(), Aspect::REAGGREGATOR);
  }
}

void Reaggregator::stop() noexcept
{
}

void Reaggregator::wait() noexcept
{
}

const char* Reaggregator::name() noexcept
{
return "ReaggregatorSingleThread";
}

void Reaggregator::reaggregate(
        const std::string& input_dir,
        const std::string& output_dir,
        const std::string& prefix)
{
  auto input_files
      = Utils::GetDirectoryFiles(
              input_dir, prefix);

  AggregatedFiles aggregated_files;

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

      aggregated_files.emplace(date, file_path);
    }
  }

  removeUnique(aggregated_files);
  if (aggregated_files.empty())
  {
    logger_->info(
            std::string("Everything is already aggregated"),
            Aspect::REAGGREGATOR);
    return;
  }

  Collector collector;
  collector.prepare_adding(1000000);

  total_file_count_ = aggregated_files.size();
  logger_->info("Total file needed to process = "
                + std::to_string(aggregated_files.size()),
                Aspect::REAGGREGATOR);

  while (!aggregated_files.empty())
  {
    collector.clear();
    const auto& date = aggregated_files.begin()->first;

    ProcessedFiles processed_files;
    processDate(
            date,
            aggregated_files,
            collector,
            processed_files);

    ResultFile result_file;
    try
    {
      if (!collector.empty())
      {
        dumpFile(
                output_dir,
                prefix,
                date,
                collector,
                result_file);

        const auto& [temp_file_path, result_file_path] = result_file;
        if (std::rename(
                temp_file_path.c_str(),
                result_file_path.c_str()))
        {
          Stream::Error ostr;
          ostr << __PRETTY_FUNCTION__
               << ": Can't rename from="
               << temp_file_path
               << ", to="
               << result_file_path;
          throw Exception(ostr.str());
        }
      }
      std::for_each(std::begin(processed_files),
                    std::end(processed_files),
                    [] (const auto& path) {
        std::remove(path.c_str());
      });
    }
    catch (const eh::Exception& exc)
    {
      Stream::Error ostr;
      ostr << __PRETTY_FUNCTION__
           << ": Reason:"
           << "[date="
           << date
           << "] "
           << exc.what();
      logger_->error(ostr.str(), Aspect::REAGGREGATOR);

      const auto& [temp_file_path, result_file_path] = result_file;
      std::remove(temp_file_path.c_str());
      std::remove(result_file_path.c_str());
    }
  }
}

void Reaggregator::processDate(
        const DayTimestamp& date,
        AggregatedFiles& aggregated_files,
        Collector& collector,
        ProcessedFiles& processed_files) noexcept
{
  const auto [it_begin, it_end]
          = aggregated_files.equal_range(date);
  for (auto it = it_begin; it != it_end; ++it)
  {
    const std::size_t percentage
            = (current_file_number_ * 100) / total_file_count_;
    if (percentage >= counter_percentage_ * 5)
    {
      counter_percentage_ += 1;
      logger_->info(
              "Percentage of processed files = "
              + std::to_string(percentage),
              Aspect::REAGGREGATOR);
    }
    current_file_number_ += 1;

    const auto& file_path = it->second;
    try
    {
      Collector temp_collector;
      temp_collector.prepare_adding(500000);
      LogHelper<LogTraits>::load(file_path, temp_collector);
      collector += temp_collector;
      temp_collector.clear();
      processed_files.emplace_back(file_path);
    }
    catch (const eh::Exception& exc)
    {
      std::stringstream stream;
      stream << "Can't add file="
             << file_path
             << " to collector. Reason: "
             << exc.what();
      logger_->error(stream.str(), Aspect::REAGGREGATOR);
    }
  }

  aggregated_files.erase(it_begin, it_end);
}

void Reaggregator::removeUnique(AggregatedFiles& files)
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

void Reaggregator::dumpFile(
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

} // namespace BidCostPredictor
} // namespace PredictorSvcs
