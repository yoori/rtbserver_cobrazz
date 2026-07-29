#pragma once

#include <LogCommons/LogCommons.hpp>
#include <LogCommons/StatCollector.hpp>
#include <LogCommons/CsvUtils.hpp>

namespace AdServer::LogProcessing
{
  struct GeoLoggerKey
  {
    std::string ip;
    std::string source;

    bool
    operator==(const GeoLoggerKey& rhs) const
    {
      return ip == rhs.ip && source == rhs.source;
    }

    size_t
    hash() const
    {
      size_t hash_value = 0;
      Generics::Murmur64Hash hasher(hash_value);
      hash_add(hasher, ip);
      hash_add(hasher, source);
      return hash_value;
    }
  };

  struct GeoLoggerData
  {
    FixedNumber lat = FixedNumber::ZERO;
    FixedNumber lon = FixedNumber::ZERO;
    std::string type;
    std::string country;
    std::string region;
    std::string city;

    GeoLoggerData&
    operator+=(const GeoLoggerData&)
    {
      return *this;
    }
  };

  typedef StatCollector<GeoLoggerKey, GeoLoggerData> GeoLoggerCollector;

  struct GeoLoggerTraits:
    LogDefaultTraits<GeoLoggerCollector, false, true>
  {
    static const char* csv_base_name() { return "GeoLogger"; }

    static const char* csv_header()
    {
      return "IP,Source,Latitude,Longitude,Type,Country,Region,City";
    }

    static std::ostream&
    write_as_csv(
      std::ostream& os,
      const BaseTraits::CollectorType::KeyT& key,
      const BaseTraits::CollectorType::DataT& data
    )
    {
      write_string_as_csv(os, key.ip) << ',';
      write_string_as_csv(os, key.source) << ',';
      os << data.lat << ',';
      os << data.lon << ',';
      write_string_as_csv(os, data.type) << ',';
      write_string_as_csv(os, data.country) << ',';
      write_string_as_csv(os, data.region) << ',';
      write_string_as_csv(os, data.city);
      return os;
    }
  };

} // namespace AdServer::LogProcessing
