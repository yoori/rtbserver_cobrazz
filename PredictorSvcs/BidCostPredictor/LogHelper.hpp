#ifndef BIDCOSTPREDICTOR_LOGHELPER_HPP
#define BIDCOSTPREDICTOR_LOGHELPER_HPP

// STD
#include <fstream>

// THIS
#include <eh/Exception.hpp>
#include <LogCommons/BidCostStat.hpp>
#include <LogCommons/GenericLogIoImpl.hpp>
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

namespace detail
{
template<class, class = void>
struct is_collector : std::false_type
{
};

template<class T>
struct is_collector<T, std::void_t<typename T::CollectorTag>> : std::true_type
{
};

template<class T>
constexpr bool is_collector_v = is_collector<T>::value;
} // namespace detail

namespace LogProcessing = AdServer::LogProcessing;

template<class LogTraits>
struct LogHelper
{
  using BaseTraits = typename LogTraits::BaseTraits;
  using Collector = typename BaseTraits::CollectorType;
  using DataT = typename Collector::DataT;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

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

    if (fstream.eof() || fstream.peek() == EOF)
      return;

    LogProcessing::LogIoProxy<LogTraits>::load(collector, fstream);
  }

  static void save(
    const std::string& path,
    const Collector& collector)
  {
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
             << path
             << "]";
      throw Exception(stream);
    }

    if (collector.empty())
      return;

    if constexpr (detail::is_collector_v<DataT>)
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

#endif //BIDCOSTPREDICTOR_LOGHELPER_HPP