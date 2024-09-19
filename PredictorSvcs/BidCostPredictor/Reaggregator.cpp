// STD
#include <filesystem>
#include <regex>

// THIS
#include <Logger/Logger.hpp>
#include "LogHelper.hpp"
#include "Reaggregator.hpp"
#include "Utils.hpp"

namespace Aspect
{

inline constexpr char REAGGREGATOR[] = "Reaggregator";

} // namespace Aspect

namespace PredictorSvcs::BidCostPredictor
{

Reaggregator::Reaggregator(
  const std::string& input_dir,
  const std::string& output_dir,
  const std::optional<ArchiveParams>& archive_params,
  Logger* logger)
  : input_dir_(input_dir),
    output_dir_(output_dir),
    archive_params_(archive_params),
    prefix_(LogTraits::B::log_base_name()),
    logger_(ReferenceCounting::add_ref(logger)),
    persantage_(logger_, Aspect::REAGGREGATOR, 5)
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

Reaggregator::~Reaggregator()
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

std::string Reaggregator::name() noexcept
{
  return "Reaggregator";
}

void Reaggregator::activate_object_()
{
  thread_ = std::make_unique<std::jthread>([this] () {
    run();
  });
}

void Reaggregator::wait_object_()
{
  thread_.reset();
}

void Reaggregator::run() noexcept
{
  try
  {
    {
      std::ostringstream stream;
      stream << FNS
             << "Reaggregator: started";
      logger_->info(stream.str(), Aspect::REAGGREGATOR);
    }

    reaggregate(input_dir_, output_dir_, prefix_);

    {
      std::ostringstream stream;
      stream << FNS
             << "Reaggregator completed successfully";
      logger_->info(stream.str(), Aspect::REAGGREGATOR);
    }
  }
  catch (const eh::Exception& exc)
  {
    Stream::Error stream;
    stream << FNS
           << "Reaggregate is failed : "
           << exc.what();
    logger_->critical(stream.str(), Aspect::REAGGREGATOR);
  }

  try
  {
    deactivate_object();
  }
  catch (...)
  {
  }
}

void Reaggregator::reaggregate(
  const std::string& input_dir,
  const std::string& output_dir,
  const std::string& prefix)
{
  auto input_files = Utils::get_directory_files(
    input_dir,
    prefix);

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

      if (!stream)
      {
        Stream::Error stream;
        stream << FNS
               << "Not correct name of file";
        throw Exception(stream);
      }

      aggregated_files.emplace(date, file_path);
    }
  }

  remove_unique(aggregated_files);

  if (aggregated_files.empty())
  {
    std::ostringstream stream;
    stream << FNS
           << "Everything is already aggregated";
    logger_->info(stream.str(), Aspect::REAGGREGATOR);
    return;
  }

  Collector collector;
  collector.prepare_adding(1000000);

  persantage_.set_total_number(aggregated_files.size());

  {
    std::ostringstream stream;
    stream << FNS
           << "Total file needed to process = "
           << std::to_string(aggregated_files.size());
    logger_->info(stream.str(), Aspect::REAGGREGATOR);
  }

  while (!aggregated_files.empty())
  {
    if (!active())
    {
      Stream::Error stream;
      stream << FNS
             << "Stop reaggregate due to interruption";
      throw Exception(stream);
    }

    collector.clear();
    const auto& date = aggregated_files.begin()->first;

    ProcessedFiles processed_files;
    process_date(
      date,
      aggregated_files,
      collector,
      processed_files);

    ResultFile result_file;
    try
    {
      if (!collector.empty())
      {
        dump_file(
          output_dir,
          prefix,
          date,
          collector,
          result_file);

        const auto& [temp_file_path, result_file_path] = result_file;
        if (std::rename(temp_file_path.c_str(), result_file_path.c_str()))
        {
          Stream::Error stream;
          stream << FNS
                 << "Can't rename from="
                 << temp_file_path
                 << ", to="
                 << result_file_path;
          throw Exception(stream);
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
      Stream::Error stream;
      stream << FNS
             << ": Reason:"
             << "[date="
             << date
             << "]: "
             << exc.what();
      logger_->error(stream.str(), Aspect::REAGGREGATOR);

      const auto& [temp_file_path, result_file_path] = result_file;
      std::remove(temp_file_path.c_str());
      std::remove(result_file_path.c_str());
    }
  }
}

void Reaggregator::process_date(
  const DayTimestamp& date,
  AggregatedFiles& aggregated_files,
  Collector& collector,
  ProcessedFiles& processed_files) noexcept
{
  Collector temp_collector;
  temp_collector.prepare_adding(500000);

  const auto [it_begin, it_end] = aggregated_files.equal_range(date);
  for (auto it = it_begin; it != it_end; ++it)
  {
    persantage_.increase();
    const auto& file_path = it->second;
    try
    {
      LogHelper<LogTraits>::load(file_path, temp_collector);
      collector += temp_collector;
      temp_collector.clear();
      processed_files.emplace_back(file_path);
    }
    catch (const eh::Exception& exc)
    {
      Stream::Error stream;
      stream << "Can't add file="
             << file_path
             << " to collector. Reason: "
             << exc.what();
      logger_->error(stream.str(), Aspect::REAGGREGATOR);
    }
  }

  aggregated_files.erase(it_begin, it_end);
}

void Reaggregator::remove_unique(AggregatedFiles& files)
{
  if (files.empty())
  {
    return;
  }

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

void Reaggregator::dump_file(
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
  const auto extension = LogHelper<LogTraits>::save(
    temp_file_path,
    collector,
    archive_params_);

  generated_path.first += extension;
  generated_path.second += extension;

  result_file = std::move(generated_path);
}

} // namespace PredictorSvcs::BidCostPredictor