#ifndef AD_SERVER_LOG_PROCESSING_SSP_GEO_STAT_HPP
#define AD_SERVER_LOG_PROCESSING_SSP_GEO_STAT_HPP

#include <LogCommons/LogCommons.hpp>
#include <LogCommons/StatCollector.hpp>
#include <LogCommons/CsvUtils.hpp>
#include <Commons/StringHolder.hpp>

namespace AdServer {
namespace LogProcessing {

struct SSPGeoData
{
  std::string country;
  std::string region;
  std::string city;
};

typedef SeqCollector<SSPGeoData> SSPGeoCollector;

struct SSPGeoTraits:
  LogDefaultTraits<SSPGeoCollector, false, false>
{
  static const char* csv_base_name() { return "SSPGeo"; }

  static const char* csv_header()
  {
    return "Сountry,Region,City";
  }

  static std::ostream&
  write_data_as_csv(
    std::ostream& os,
    const BaseTraits::CollectorType::DataT& data
  )
  {
    write_string_as_csv(os, data.country) << ',';
    write_string_as_csv(os, data.region) << ',';
    write_string_as_csv(os, data.city);
    return os;
  }
};

} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_SSP_GEO_STAT_HPP */
