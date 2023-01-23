// STD
#include <regex>

// THIS
#include <LogCommons/LogCommons.hpp>
#include <Logger/Logger.hpp>
#include <Logger/StreamLogger.hpp>
#include "LogHelper.hpp"
#include "Persantage.hpp"
#include "Regenerator.hpp"
#include "Utils.hpp"

namespace Aspect
{
const char REGENERATOR[] = "Regenerator";
}

namespace PredictorSvcs
{
namespace BidCostPredictor
{

Regenerator::Regenerator(
  const std::string& input_dir,
  const std::string& output_dir,
  Logging::Logger* logger)
  : input_dir_(input_dir),
    output_dir_(output_dir),
    prefix_(LogTraits::B::log_base_name()),
    logger_(ReferenceCounting::add_ref(logger))
{
}

void Regenerator::start()
{
  try
  {
    if (!Utils::exist_directory(input_dir_))
    {
      Stream::Error ostr;
      ostr << "Not exist directory="
           << input_dir_;
      throw Exception(ostr);
    }

    if (!Utils::exist_directory(output_dir_))
    {
      Stream::Error ostr;
      ostr << "Not exist directory="
           << output_dir_;
      throw Exception(ostr);
    }

    logger_->info(
      std::string("Regenerator: started"),
      Aspect::REGENERATOR);

    auto input_files =
      Utils::get_directory_files(
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

        process_files.emplace_back(date, file_path);
      }
    }

    regenerate(process_files, output_dir_, prefix_);

    logger_->info(
      std::string("Regenerator: finished"),
      Aspect::REGENERATOR);
  }
  catch (const eh::Exception& ex)
  {
    Stream::Error ostr;
    ostr << __PRETTY_FUNCTION__
         << ": [Fatal error]: Regenerator is failed:"
         << ex.what();
    throw Exception(ostr);
  }
}

void Regenerator::stop() noexcept
{
}

void Regenerator::wait() noexcept
{
}

const char* Regenerator::name() noexcept
{
  return "AggregatorSingleThread";
}

void Regenerator::remove_processed_files(
  const ProcessedFiles& processed_files) noexcept
{
  for (const auto& [temp_file_path, result_file_path]: processed_files)
  {
    std::remove(temp_file_path.c_str());
    std::remove(result_file_path.c_str());
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
    collector.clear();
    persantage.increase();

    const auto& date = file_info.first;
    const auto& path = file_info.second;

    ProcessedFiles processed_files;

    std::ifstream fstream(path);
    if (!fstream.is_open())
    {
      std::stringstream stream;
      stream << __PRETTY_FUNCTION__
             << ": Can't open file="
             << path;
      logger_->error(
        stream.str(),
        Aspect::REGENERATOR);
      continue;
    }

    try
    {
      std::string line;
      line.reserve(1024);

      unsigned long line_num = 1;
      for (ValueT value; !fstream.eof(); ++line_num)
      {
        line.clear();
        LogProcessing::read_until_eol(fstream, line, false);
        if (fstream.eof())
        {
          std::stringstream stream;
          stream << __PRETTY_FUNCTION__
                 << ": Malformed file (file must end with an end-of-line character), line "
                 << line_num;
          logger_->error(stream.str(), Aspect::REGENERATOR);
          break;
        }

        if (!fstream.good())
        {
          std::stringstream stream;
          stream << __PRETTY_FUNCTION__
                 << ": Error occure, line "
                 << line_num;
          logger_->error(stream.str(), Aspect::REGENERATOR);
          break;
        }

        const auto pos = line.find('\t');
        if (pos == std::string::npos)
        {
          std::stringstream stream;
          stream << __PRETTY_FUNCTION__
                 << ": Bad file=" << path;
          logger_->error(stream.str(), Aspect::REGENERATOR);
          break;
        }
        line.insert(pos + 1, "-\t");

        LogProcessing::FixedBufStream<LogProcessing::TabCategory> fbs(line);
        if (!(fbs >> const_cast<Key&>(static_cast<const Key&>(value.first))))
        {
          std::stringstream stream;
          stream << __PRETTY_FUNCTION__
                 << ": Failed to read key from istream (line number = "
                 << line_num
                 << ")";
          logger_->error(stream.str(), Aspect::REGENERATOR);
          break;
        }
        if (!(fbs >> static_cast<Data&>(value.second)))
        {
          std::stringstream stream;
          stream << __PRETTY_FUNCTION__
                 << ": Failed to read data from istream (line number = " << line_num
                 << ")";
          logger_->error(stream.str(), Aspect::REGENERATOR);
          break;
        }
        fbs.transfer_state(fstream);

        const auto& k = value.first;
        const auto& v = value.second;

        if (!k.url().empty() && k.url()[0] == 'a')
        {
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
          prefix, date,
          collector,
          processed_files);
      }

      for (const auto& [temp_path, result_path]: processed_files)
      {
        if (std::rename(temp_path.c_str(), result_path.c_str()))
        {
          Stream::Error ostr;
          ostr << __PRETTY_FUNCTION__
               << ": Can't rename from="
               << temp_path
               << ", to="
               << result_path;
          throw Exception(ostr);
        }
      }

      std::stringstream stream;
      stream << "File="
             << path
             << ", modificate on files="
             << processed_files.size();
      logger_->info(stream.str(),Aspect::REGENERATOR);

      std::remove(path.c_str());
    }
    catch (const eh::Exception& exc)
    {
      std::stringstream stream;
      stream << "Can't modificate file="
             << path
             << ", Reason: "
             << exc.what()
             << "\n";
      logger_->error(stream.str(),Aspect::REGENERATOR);

      remove_processed_files(processed_files);
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
  const auto generated_path =
    Utils::generate_file_path(output_dir, prefix, date);
  const auto& temp_file_path = generated_path.first;
  LogHelper<LogTraits>::save(temp_file_path, collector);
  processed_files.emplace_back(generated_path);
}

} // namespace BidCostPredictor
} // namespace PredictorSvcs