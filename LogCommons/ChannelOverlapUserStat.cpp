
#include "ChannelOverlapUserStat.hpp"
#include <LogCommons/LogCommons.ipp>

namespace AdServer {
namespace LogProcessing {

template <> const char*
ChannelOverlapUserStatTraits::B::base_name_ = "ChannelOverlapUserStat";

template <> const char*
ChannelOverlapUserStatTraits::B::signature_ = "ChannelOverlapUserStat";

template <> const char*
ChannelOverlapUserStatTraits::B::current_version_ = "2.5";

FixedBufStream<TabCategory>&
operator>>(
  FixedBufStream<TabCategory>& is,
  ChannelOverlapUserStatInnerKey& key
)
  /*throw(eh::Exception)*/
{
  is >> key.channel1_id_;
  is >> key.channel2_id_;
  key.calc_hash_();
  return is;
}

std::ostream&
operator<<(std::ostream& os, const ChannelOverlapUserStatInnerKey& key)
  /*throw(eh::Exception)*/
{
  os << key.channel1_id_ << '\t';
  os << key.channel2_id_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(
  FixedBufStream<TabCategory>& is,
  ChannelOverlapUserStatInnerData& data
)
  /*throw(eh::Exception)*/
{
  is >> data.unique_users_;
  return is;
}

std::ostream&
operator<<(std::ostream& os, const ChannelOverlapUserStatInnerData& data)
  /*throw(eh::Exception)*/
{
  os << data.unique_users_;
  return os;
}

} // namespace LogProcessing
} // namespace AdServer

