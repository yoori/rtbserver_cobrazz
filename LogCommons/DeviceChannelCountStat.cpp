
#include "DeviceChannelCountStat.hpp"
#include <LogCommons/LogCommons.ipp>

namespace AdServer {
namespace LogProcessing {

template <> const char* DeviceChannelCountStatTraits::B::base_name_ =
  "DeviceChannelCountStat";
template <> const char* DeviceChannelCountStatTraits::B::signature_ =
  "DeviceChannelCountStat";
template <> const char* DeviceChannelCountStatTraits::B::current_version_ =
  "2.6";

std::istream&
operator>>(std::istream& is, DeviceChannelCountStatKey& key)
{
  is >> key.isp_sdate_;
  read_eol(is);
  is >> key.colo_id_;
  key.calc_hash_();
  return is;
}

std::ostream&
operator<<(std::ostream& os, const DeviceChannelCountStatKey& key)
  /*throw(eh::Exception)*/
{
  os << key.isp_sdate_ << '\n' << key.colo_id_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(
  FixedBufStream<TabCategory>& is,
  DeviceChannelCountStatInnerKey& key
)
  /*throw(eh::Exception)*/
{
  is >> key.device_channel_id_;
  is >> key.channel_type_;
  is >> key.channel_count_;
  if (is)
  {
    key.invariant();
    key.calc_hash_();
  }
  return is;
}

std::ostream&
operator<<(std::ostream& os, const DeviceChannelCountStatInnerKey& key)
  /*throw(eh::Exception)*/
{
  key.invariant();
  os << key.device_channel_id_ << '\t';
  os << key.channel_type_ << '\t';
  os << key.channel_count_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(
  FixedBufStream<TabCategory>& is,
  DeviceChannelCountStatInnerData& data
)
  /*throw(eh::Exception)*/
{
  is >> data.users_count_;
  return is;
}

std::ostream&
operator<<(std::ostream& os, const DeviceChannelCountStatInnerData& data)
  /*throw(eh::Exception)*/
{
  os << data.users_count_;
  return os;
}

} // namespace LogProcessing
} // namespace AdServer

