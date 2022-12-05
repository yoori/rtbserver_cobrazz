
#include "ChannelInventoryEstimationStat.hpp"
#include <LogCommons/LogCommons.ipp>

namespace AdServer {
namespace LogProcessing {

template <>
const char* ChannelInventoryEstimationStatTraits::B::base_name_ =
  "ChannelInventoryEstimationStat";
template <>
const char* ChannelInventoryEstimationStatTraits::B::signature_ =
  "ChannelInventoryEstimationStat";
template <>
const char* ChannelInventoryEstimationStatTraits::B::current_version_ =
  "1.1"; // Last change: AdServer v2.2

const ChannelInventoryEstimationStatInnerKey::LevelT
  ChannelInventoryEstimationStatInnerKey::max_level_value_("2.0");

std::istream&
operator>>(std::istream& is, ChannelInventoryEstimationStatKey& key)
{
  is >> key.sdate_;
  read_eol(is);
  is >> key.colo_id_;
  key.calc_hash_();
  return is;
}

std::ostream&
operator<<(std::ostream& os, const ChannelInventoryEstimationStatKey& key)
  /*throw(eh::Exception)*/
{
  os << key.sdate_ << '\n' << key.colo_id_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(
  FixedBufStream<TabCategory>& is,
  ChannelInventoryEstimationStatInnerKey& key
)
{
  is >> key.channel_id_;
  is >> key.level_;
  if (is)
  {
    key.invariant();
    key.calc_hash_();
  }
  return is;
}

std::ostream&
operator<<(std::ostream& os, const ChannelInventoryEstimationStatInnerKey& key)
{
  key.invariant();
  os << key.channel_id_ << '\t';
  os << key.level_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(
  FixedBufStream<TabCategory>& is,
  ChannelInventoryEstimationStatInnerData& data
)
{
  is >> data.users_regular_;
  is >> data.users_from_now_;
  return is;
}

std::ostream&
operator<<(
  std::ostream& os,
  const ChannelInventoryEstimationStatInnerData& data
)
{
  os << data.users_regular_ << '\t';
  os << data.users_from_now_;
  return os;
}

} // namespace LogProcessing
} // namespace AdServer

