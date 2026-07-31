#pragma once

#include <string>
#include <utility>

#include <LogCommons/LogCommons.hpp>
#include <LogCommons/StatCollector.hpp>
#include <LogCommons/CsvUtils.hpp>

namespace AdServer::LogProcessing
{
  struct GeoLoggerKey
  {
    GeoLoggerKey() = default;
    GeoLoggerKey(const GeoLoggerKey&) = default;
    GeoLoggerKey& operator=(const GeoLoggerKey&) = default;
    GeoLoggerKey(GeoLoggerKey&&) noexcept = default;
    GeoLoggerKey& operator=(GeoLoggerKey&&) noexcept = default;

    GeoLoggerKey(
      std::string&& ip_val,
      std::string&& source_val)
      : ip(std::move(ip_val)),
        source(std::move(source_val))
    {
      calc_hash_();
    }

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
      return hash_;
    }

  private:
    void
    calc_hash_()
    {
      hash_ = 0;
      Generics::Murmur64Hash hasher(hash_);
      hash_add(hasher, ip);
      hash_add(hasher, source);
    }

    size_t hash_ = 0;
  };

  struct GeoLoggerData
  {
    GeoLoggerData() = default;
    GeoLoggerData(const GeoLoggerData&) = default;
    GeoLoggerData& operator=(const GeoLoggerData&) = default;
    GeoLoggerData(GeoLoggerData&&) noexcept = default;
    GeoLoggerData& operator=(GeoLoggerData&&) noexcept = default;

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

  struct GeoLoggerTraits: LogDefaultTraits<GeoLoggerCollector, false, true>
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
