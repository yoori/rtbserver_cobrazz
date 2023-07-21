
#include "RequestStatsHourlyExtStat.hpp"
#include <LogCommons/LogCommons.ipp>

namespace AdServer {
namespace LogProcessing {

template <> const char* RequestStatsHourlyExtStatTraits::B::base_name_ =
  "RequestStatsHourlyExtStat";
template <> const char* RequestStatsHourlyExtStatTraits::B::signature_ =
  "RequestStatsHourlyExtStat";
template <> const char* RequestStatsHourlyExtStatTraits::B::current_version_ =
  "1.0";

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

std::ostream&
operator<<(std::ostream& os, const RequestStatsHourlyExtKey& key)
  /*throw(eh::Exception)*/
{
  os << key.sdate_ << '\n' << key.adv_sdate_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, RequestStatsHourlyExtInnerKey& key)
{
  TokenizerInputArchive<> ia(is);
  ia >> key;
  key.calc_hash_();
  return is;
}

std::ostream&
operator<<(std::ostream& os, const RequestStatsHourlyExtInnerKey& key)
  /*throw(eh::Exception)*/
{
  TabOutputArchive oa(os);
  oa << key;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, RequestStatsHourlyExtInnerData& data)
  /*throw(eh::Exception)*/
{
  TokenizerInputArchive<Aux_::NoInvariants> ia(is);
  ia >> data;
  return is;
}

std::ostream&
operator<<(std::ostream& os, const RequestStatsHourlyExtInnerData& data)
  /*throw(eh::Exception)*/
{
  SimpleTabOutputArchive oa(os);
  oa << data;
  return os;
}

} // namespace LogProcessing
} // namespace AdServer


