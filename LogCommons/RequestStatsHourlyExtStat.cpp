
#include "RequestStatsHourlyExtStat.hpp"
#include <LogCommons/BufferWriter.hpp>
#include <LogCommons/LogCommons.ipp>

namespace AdServer::LogProcessing
{

  template <> const char* RequestStatsHourlyExtStatTraits::B::base_name_ =
    "RequestStatsHourlyExtStat";
  template <> const char* RequestStatsHourlyExtStatTraits::B::signature_ =
    "RequestStatsHourlyExtStat";
  template <> const char* RequestStatsHourlyExtStatTraits::B::current_version_ = "1.0";

  const RequestStatsHourlyExtInnerKey::DeliveryThresholdT
    RequestStatsHourlyExtInnerKey::max_delivery_threshold_value_("1.0");

  std::istream&
  operator>>(std::istream& is, RequestStatsHourlyExtKey& key)
  {
    is >> key.sdate_;
    read_eol(is);
    is >> key.adv_sdate_;
    key.calc_hash_();
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const RequestStatsHourlyExtKey& key)
    /*throw(eh::Exception)*/
  {
    out << key.sdate_ << '\n' << key.adv_sdate_;
    return out;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, RequestStatsHourlyExtInnerKey& key)
  {
    TokenizerInputArchive<> ia(is);
    ia >> key;
    key.calc_hash_();
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const RequestStatsHourlyExtInnerKey& key)
    /*throw(eh::Exception)*/
  {
    BufferTabOutputArchive archive(out);
    archive << key;
    return out;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, RequestStatsHourlyExtInnerData& data)
    /*throw(eh::Exception)*/
  {
    TokenizerInputArchive<Aux_::NoInvariants> ia(is);
    ia >> data;
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const RequestStatsHourlyExtInnerData& data)
    /*throw(eh::Exception)*/
  {
    SimpleBufferTabOutputArchive archive(out);
    archive << data;
    return out;
  }

} // namespace AdServer::LogProcessing
