// STD
#include <filesystem>
#include <regex>

// UNIXCOMMONS
#include <Logger/Logger.hpp>

// THIS
#include <LogCommons/LogCommons.hpp>
#include "LogHelper.hpp"
#include "Persantage.hpp"
#include "Regenerator.hpp"
#include "Utils.hpp"

namespace Aspect
{

inline constexpr char REGENERATOR[] = "Regenerator";

} // namespace Aspect

namespace PredictorSvcs::BidCostPredictor
{

Regenerator::Regenerator(
  const std::string& input_dir,
  const std::string& output_dir,
  const std::optional<ArchiveParams>& archive_params,
  Logger* logger)
  : input_dir_(input_dir),
    output_dir_(output_dir),
    archive_params_(archive_params),
    prefix_(LogTraits::B::log_base_name()),
    logger_(ReferenceCounting::add_ref(logger))
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

std::string Regenerator::name() noexcept
{
  return "AggregatorSingleThread";
}

void Regenerator::activate_object_()
{
  thread_ = std::make_unique<std::jthread>([this] () {
    run();
  });
}

void Regenerator::wait_object_()
{
  thread_.reset();
}

void Regenerator::run() noexcept
{
  try
  {
    {
      std::ostringstream stream;
      stream << FNS
             << "Regenerator: started";
      logger_->info(stream.str(), Aspect::REGENERATOR);
    }

    const auto input_files = Utils::get_directory_files(
      input_dir_,
      prefix_);

    ProcessFiles process_files;
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

        process_files.emplace_back(date, file_path);
      }
    }

    regenerate(process_files, output_dir_, prefix_);

    {
      std::ostringstream stream;
      stream << FNS
             << "Regenerator completed successfully";
      logger_->info(stream.str(), Aspect::REGENERATOR);
    }
  }
  catch (const eh::Exception& exc)
  {
    clear_directory(output_dir_);

    Stream::Error stream;
    stream << FNS
           << "Regenerator is failed: "
           << exc.what();
    logger_->critical(stream.str(), Aspect::REGENERATOR);
  }
  catch (...)
  {
    clear_directory(output_dir_);

    Stream::Error stream;
    stream << FNS
           << "Regenerator is failed: Unknown error";
    logger_->critical(stream.str(), Aspect::REGENERATOR);
  }

  try
  {
    deactivate_object();
  }
  catch (...)
  {
  }
}

void Regenerator::remove_processed_files(
  const ProcessedFiles& processed_files) noexcept
{
  for (const auto& [temp_file_path, result_file_path] : processed_files)
  {
    std::remove(temp_file_path.c_str());
    std::remove(result_file_path.c_str());
  }
}

void Regenerator::clear_directory(
  const std::string& directory) noexcept
{
  try
  {
    for (const auto &entry: std::filesystem::directory_iterator(directory))
    {
      if (entry.is_regular_file())
      {
        std::filesystem::remove(entry.path());
      }
    }
  }
  catch (const eh::Exception& exc)
  {
    Stream::Error stream;
    stream << "Can't clear directory="
           << directory
           << ". Reason:"
           << exc.what();
    logger_->error(stream.str(), Aspect::REGENERATOR);
  }
}

void Regenerator::regenerate(
  const ProcessFiles& process_files,
  const std::string& output_dir,
  const std::string& prefix)
{
  Persantage persantage(logger_, Aspect::REGENERATOR, 5);
  persantage.set_total_number(process_files.size());

  Collector collector;
  for (const auto& file_info : process_files)
  {
    if (!active())
    {
      Stream::Error stream;
      stream << FNS
             << "Stop regenerate due to interruption";
      throw Exception(stream);
    }

    collector.clear();
    persantage.increase();

    const auto& date = file_info.first;
    const auto& path = file_info.second;

    ProcessedFiles processed_files;

    std::ifstream fstream(path);
    if (!fstream.is_open())
    {
      Stream::Error stream;
      stream << FNS
             << ": Can't open file="
             << path;
      logger_->error(stream.str(), Aspect::REGENERATOR);
      continue;
    }

    try
    {
      std::string line;
      line.reserve(1024);

      std::size_t line_num = 1;
      for (ValueT value; !fstream.eof(); ++line_num)
      {
        line.clear();
        LogProcessing::read_until_eol(fstream, line, false);
        if (fstream.eof())
        {
          Stream::Error stream;
          stream << FNS
                 << "Malformed file (file must end with an end-of-line character), line "
                 << line_num;
          logger_->error(stream.str(), Aspect::REGENERATOR);
          break;
        }

        if (!fstream.good())
        {
          std::stringstream stream;
          stream << FNS
                 << "Error occure, line "
                 << line_num;
          logger_->error(stream.str(), Aspect::REGENERATOR);
          break;
        }

        const auto pos = line.find('\t');
        if (pos == std::string::npos)
        {
          std::stringstream stream;
          stream << FNS
                 << "Bad file="
                 << path;
          logger_->error(stream.str(), Aspect::REGENERATOR);
          break;
        }
        line.insert(pos + 1, "-\t");

        LogProcessing::FixedBufStream<LogProcessing::TabCategory> fbs(line);
        if (!(fbs >> const_cast<Key&>(static_cast<const Key&>(value.first))))
        {
          Stream::Error stream;
          stream << FNS
                 << "Failed to read key from istream (line number = "
                 << line_num
                 << ")";
          logger_->error(stream.str(), Aspect::REGENERATOR);
          break;
        }
        if (!(fbs >> static_cast<Data&>(value.second)))
        {
          Stream::Error stream;
          stream << FNS
                 << "Failed to read data from istream (line number = "
                 << line_num
                 << ")";
          logger_->error(stream.str(), Aspect::REGENERATOR);
          break;
        }
        fbs.transfer_state(fstream);

        const auto& k = value.first;
        const auto& v = value.second;
        const auto it = collector.find(k);
        if (it == collector.end())
        {
          collector.add(k, v);
        }
        else
        {
          save_file(
            output_dir,
            prefix,
            date,
            collector,
            processed_files);
          collector.clear();
          collector.add(k, v);
        }

        char peek_sym = fstream.peek();
        if (peek_sym == '\n')
        {
          break;
        }
      }

      if (!collector.empty())
      {
        save_file(
          output_dir,
          prefix,
          date,
          collector,
          processed_files);
      }

      for (const auto& [temp_path, result_path] : processed_files)
      {
        if (std::rename(temp_path.c_str(), result_path.c_str()))
        {
          Stream::Error stream;
          stream << FNS
                 << "Can't rename from="
                 << temp_path
                 << ", to="
                 << result_path;
          throw Exception(stream);
        }
      }

      {
        std::ostringstream stream;
        stream << "File="
               << path
               << ", modificate on files="
               << processed_files.size();
        logger_->info(stream.str(), Aspect::REGENERATOR);
      }

      std::remove(path.c_str());
    }
    catch (const eh::Exception& exc)
    {
      remove_processed_files(processed_files);

      Stream::Error stream;
      stream << "Can't modificate file="
             << path
             << ", Reason: "
             << exc.what()
             << "\n";
      logger_->error(stream.str(),Aspect::REGENERATOR);
    }
  }
}

void Regenerator::save_file(
  const std::string& output_dir,
  const std::string& prefix,
  const DayTimestamp& date,
  Collector& collector,
  ProcessedFiles& processed_files)
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

  processed_files.emplace_back(generated_path);
}

} // namespace PredictorSvcs::BidCostPredictor