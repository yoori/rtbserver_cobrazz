#ifndef RTBSERVER_COBRAZZ_BIDCOSTPREDICTOR_LOGHELPER_HPP
#define RTBSERVER_COBRAZZ_BIDCOSTPREDICTOR_LOGHELPER_HPP

// STD
#include <fstream>

// THIS
#include <eh/Exception.hpp>
#include <LogCommons/GenericLogIoImpl.hpp>
#include <LogCommons/BidCostStat.hpp>
#include <LogCommons/LogCommons.hpp>

namespace AdServer
{
namespace LogProcessing
{
using BidCostStatInnerTraits = LogDefaultTraits<BidCostStatInnerCollector, false>;
} // namespace LogProcessing
} // namespace AdServer

namespace PredictorSvcs
{
namespace BidCostPredictor
{

namespace LogProcessing = AdServer::LogProcessing;

template<class LogTraits>
struct LogHelper
{
  using BaseTraits = typename LogTraits::BaseTraits;
  using Collector = typename BaseTraits::CollectorType;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  static_assert(std::is_same_v<Collector, LogProcessing::BidCostStatCollector>
             || std::is_same_v<Collector, LogProcessing::BidCostStatInnerCollector>,
                "Type must be BidCostStatCollector or BidCostStatInnerCollector");

  static void load(const std::string& file_path, Collector& collector)
  {
    std::ifstream fstream(file_path);
    if (!fstream.is_open())
    {
      Stream::Error stream;
      stream << __PRETTY_FUNCTION__
             << ": Can't open file="
             << file_path;
      throw Exception(stream);
    }

    using LogHeader = typename LogTraits::HeaderType;
    LogHeader log_header;

    if (!(fstream >> log_header))
    {
      Stream::Error stream;
      stream << __PRETTY_FUNCTION__
             << ": Failed to read log header [file="
             << file_path << "]";
      throw Exception(stream);
    }

    if (log_header.version() != LogTraits::current_version())
    {
      Stream::Error stream;
      stream << __PRETTY_FUNCTION__
             << ": Invalid log header version [file="
             << file_path << "]";
      throw Exception(stream);
    }

    LogProcessing::LogIoProxy<LogTraits>::load(collector, fstream);
  }

  static void save(const std::string& path,
                   const Collector& collector)
  {
    if (collector.empty())
      return;

    std::ofstream fstream(path);
    if (!fstream.is_open())
    {
      Stream::Error stream;
      stream << __PRETTY_FUNCTION__
             << ": Can't open file="
             << path;
      throw Exception(stream);
    }

    using LogHeader = typename LogTraits::HeaderType;
    LogHeader log_header(LogTraits::current_version());

    if (!(fstream << log_header))
    {
      Stream::Error stream;
      stream << __PRETTY_FUNCTION__
             << ": Failed to write log header [file="
             << path << "]";
      throw Exception(stream);
    }

    if constexpr (std::is_same_v<Collector, LogProcessing::BidCostStatCollector>)
    {
      LogProcessing::PackedSaveStrategy<LogTraits>().save(fstream, collector);
    }
    else
    {
      LogProcessing::DefaultSaveStrategy<LogTraits>().save(fstream, collector);
    }
  }
};

} // namespace BidCostPredictor
} // namespace PredictorSvcs

#endif //RTBSERVER_COBRAZZ_BIDCOSTPREDICTOR_LOGHELPER_HPP
