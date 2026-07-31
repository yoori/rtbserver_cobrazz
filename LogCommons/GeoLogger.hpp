#pragma once

#include <iosfwd>
#include <string>

#include <LogCommons/LogCommons.hpp>
#include <LogCommons/StatCollector.hpp>

namespace AdServer::LogProcessing
{
  struct GeoLoggerKey
  {
    GeoLoggerKey();
    GeoLoggerKey(const GeoLoggerKey&);
    GeoLoggerKey& operator=(const GeoLoggerKey&);
    GeoLoggerKey(GeoLoggerKey&&) noexcept;
    GeoLoggerKey& operator=(GeoLoggerKey&&) noexcept;

    GeoLoggerKey(
      std::string&& ip_val,
      std::string&& source_val);

    std::string ip;
    std::string source;

    bool
    operator==(const GeoLoggerKey& rhs) const;

    size_t
    hash() const;

  private:
    void
    calc_hash_();

    size_t hash_ = 0;
  };

  struct GeoLoggerData
  {
    GeoLoggerData();
    GeoLoggerData(const GeoLoggerData&);
    GeoLoggerData& operator=(const GeoLoggerData&);
    GeoLoggerData(GeoLoggerData&&) noexcept;
    GeoLoggerData& operator=(GeoLoggerData&&) noexcept;

    FixedNumber lat = FixedNumber::ZERO;
    FixedNumber lon = FixedNumber::ZERO;
    std::string type;
    std::string country;
    std::string region;
    std::string city;

    GeoLoggerData&
    operator+=(const GeoLoggerData&);
  };

  typedef StatCollector<GeoLoggerKey, GeoLoggerData> GeoLoggerCollector;

  struct GeoLoggerTraits: LogDefaultTraits<GeoLoggerCollector, false, true>
  {
    static const char*
    csv_base_name();

    static const char*
    csv_header();

    static std::ostream&
    write_as_csv(
      std::ostream& os,
      const BaseTraits::CollectorType::KeyT& key,
      const BaseTraits::CollectorType::DataT& data);
  };

} // namespace AdServer::LogProcessing
