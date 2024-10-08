#ifndef BIDCOSTPREDICTOR_LOGHELPER_HPP
#define BIDCOSTPREDICTOR_LOGHELPER_HPP

// STD
#include <fstream>

// UNIXCOMMONS
#include <eh/Exception.hpp>

// THIS
#include <LogCommons/ArchiveIfstream.hpp>
#include <LogCommons/ArchiveOfstream.hpp>
#include <LogCommons/BidCostStat.hpp>
#include <LogCommons/GenericLogIoImpl.hpp>
#include <LogCommons/LogCommons.hpp>

namespace AdServer::LogProcessing
{

using BidCostStatInnerTraits = LogDefaultTraits<BidCostStatInnerCollector, false>;

} // namespace AdServer::LogProcessing

namespace PredictorSvcs::BidCostPredictor
{

namespace Detail
{

template<class, class = void>
struct IsCollector : std::false_type
{
};

template<class T>
struct IsCollector<T, std::void_t<typename T::CollectorTag>> : std::true_type
{
};

template<class T>
constexpr bool IsCollectorV = IsCollector<T>::value;

} // namespace Detail

namespace LogProcessing = AdServer::LogProcessing;

template<class LogTraits>
struct LogHelper
{
  using BaseTraits = typename LogTraits::BaseTraits;
  using Collector = typename BaseTraits::CollectorType;
  using DataT = typename Collector::DataT;
  using ArchiveIfstream = LogProcessing::ArchiveIfstream;
  using ArchiveOfstream = LogProcessing::ArchiveOfstream;
  using ArchiveParams = AdServer::LogProcessing::ArchiveParams;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  static void load(
    const std::string& file_path,
    Collector& collector)
  {
    std::unique_ptr<std::istream> istream;
    if (LogProcessing::ArchiveIfstream::is_archive(file_path))
    {
      istream = std::make_unique<ArchiveIfstream>(file_path);
    }
    else
    {
      istream = std::make_unique<std::ifstream>(file_path);
    }
    auto& ref_istream = *istream;

    if (!ref_istream)
    {
      Stream::Error stream;
      stream << FNS
             << "Can't open file="
             << file_path;
      throw Exception(stream);
    }

    using LogHeader = typename LogTraits::HeaderType;
    LogHeader log_header;

    if (!(ref_istream >> log_header))
    {
      Stream::Error stream;
      stream << FNS
             << "Failed to read log header [file="
             << file_path
             << "]";
      throw Exception(stream);
    }

    if (log_header.version() != LogTraits::current_version())
    {
      Stream::Error stream;
      stream << FNS
             << "Invalid log header version [file="
             << file_path
             << "]";
      throw Exception(stream);
    }

    if (ref_istream.eof() || ref_istream.peek() == EOF)
    {
      return;
    }

    LogProcessing::LogIoProxy<LogTraits>::load(collector, ref_istream);
  }

  static std::string save(
    const std::string& path,
    const Collector& collector,
    const std::optional<ArchiveParams>& archive_params)
  {
    std::string extension;

    std::unique_ptr<std::ostream> ostream;
    if (archive_params)
    {
      auto archive_ofstream = std::make_unique<ArchiveOfstream>(
        path,
        *archive_params);
      extension = archive_ofstream->file_extension();
      ostream = std::move(archive_ofstream);
    }
    else
    {
      ostream = std::make_unique<std::ofstream>(path);
    }
    auto& ref_ostream = *ostream;

    if (!ref_ostream)
    {
      Stream::Error stream;
      stream << FNS
             << "Can't create file="
             << path;
      throw Exception(stream);
    }

    using LogHeader = typename LogTraits::HeaderType;
    LogHeader log_header(LogTraits::current_version());

    if (!(ref_ostream << log_header))
    {
      std::remove(path.c_str());

      Stream::Error stream;
      stream << FNS
             << "Failed to write log header [file="
             << path
             << "]";
      throw Exception(stream);
    }

    if (collector.empty())
    {
      return extension;
    }

    try
    {
      if constexpr (Detail::IsCollectorV<DataT>)
      {
        LogProcessing::PackedSaveStrategy<LogTraits>().save(ref_ostream, collector);
      }
      else
      {
        LogProcessing::DefaultSaveStrategy<LogTraits>().save(ref_ostream, collector);
      }
    }
    catch (const eh::Exception& exc)
    {
      std::remove(path.c_str());

      Stream::Error stream;
      stream << FNS
             << "Failed to write collector [file="
             << path
             << "] Reason: "
             << exc.what();
      throw Exception(stream);
    }
    catch (...)
    {
      std::remove(path.c_str());

      Stream::Error stream;
      stream << FNS
             << "Failed to write collector [file="
             << path
             << "] Reason: Unknown error";
      throw Exception(stream);
    }

    return extension;
  }
};

} // namespace PredictorSvcs::BidCostPredictor

#endif //BIDCOSTPREDICTOR_LOGHELPER_HPP