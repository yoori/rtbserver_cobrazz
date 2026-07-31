#include "GeoLogger.hpp"

#include <utility>

#include <LogCommons/CsvUtils.hpp>

namespace AdServer::LogProcessing
{
  template <> const char*
  GeoLoggerTraits::B::base_name_ = "GeoLogger";

  template <> const char*
  GeoLoggerTraits::B::signature_ = "<UNDEFINED>";

  template <> const char*
  GeoLoggerTraits::B::current_version_ = "<UNDEFINED>";

  GeoLoggerKey::GeoLoggerKey() = default;

  GeoLoggerKey::GeoLoggerKey(const GeoLoggerKey&) = default;

  GeoLoggerKey&
  GeoLoggerKey::operator=(const GeoLoggerKey&) = default;

  GeoLoggerKey::GeoLoggerKey(GeoLoggerKey&&) noexcept = default;

  GeoLoggerKey&
  GeoLoggerKey::operator=(GeoLoggerKey&&) noexcept = default;

  GeoLoggerKey::GeoLoggerKey(
    std::string&& ip_val,
    std::string&& source_val)
    : ip(std::move(ip_val)),
      source(std::move(source_val))
  {
    calc_hash_();
  }

  bool
  GeoLoggerKey::operator==(const GeoLoggerKey& rhs) const
  {
    return ip == rhs.ip && source == rhs.source;
  }

  size_t
  GeoLoggerKey::hash() const
  {
    return hash_;
  }

  void
  GeoLoggerKey::calc_hash_()
  {
    hash_ = 0;
    Generics::Murmur64Hash hasher(hash_);
    hash_add(hasher, ip);
    hash_add(hasher, source);
  }

  GeoLoggerData::GeoLoggerData() = default;

  GeoLoggerData::GeoLoggerData(const GeoLoggerData&) = default;

  GeoLoggerData&
  GeoLoggerData::operator=(const GeoLoggerData&) = default;

  GeoLoggerData::GeoLoggerData(GeoLoggerData&&) noexcept = default;

  GeoLoggerData&
  GeoLoggerData::operator=(GeoLoggerData&&) noexcept = default;

  GeoLoggerData&
  GeoLoggerData::operator+=(const GeoLoggerData&)
  {
    return *this;
  }

  const char*
  GeoLoggerTraits::csv_base_name()
  {
    return "GeoLogger";
  }

  const char*
  GeoLoggerTraits::csv_header()
  {
    return "IP,Source,Latitude,Longitude,Type,Country,Region,City";
  }

  std::ostream&
  GeoLoggerTraits::write_as_csv(
    std::ostream& os,
    const BaseTraits::CollectorType::KeyT& key,
    const BaseTraits::CollectorType::DataT& data)
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
} // namespace AdServer::LogProcessing
