
#include "ChannelInventory.hpp"
#include <LogCommons/LogCommons.ipp>

namespace AdServer {
namespace LogProcessing {

template <> const char*
ChannelInventoryTraits::B::base_name_ = "ChannelInventory";

template <> const char*
ChannelInventoryTraits::B::signature_ = "ChannelInventory";

template <> const char*
ChannelInventoryTraits::B::current_version_ = "1.3";

std::istream&
operator>>(std::istream& is, ChannelInventoryKey& key)
{
  is >> key.sdate_;
  read_eol(is);
  is >> key.colo_id_;
  key.calc_hash_();
  return is;
}

std::ostream&
operator<<(std::ostream& os, const ChannelInventoryKey& key)
  /*throw(eh::Exception)*/
{
  os << key.sdate_ << '\n' << key.colo_id_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, ChannelInventoryInnerKey& key)
  /*throw(eh::Exception)*/
{
  is >> key.channel_id_;
  return is;
}

std::ostream&
operator<<(std::ostream& os, const ChannelInventoryInnerKey& key)
  /*throw(eh::Exception)*/
{
  os << key.channel_id_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, ChannelInventoryInnerData& data)
  /*throw(eh::Exception)*/
{
  is >> data.sum_user_ecpm_;
  is >> data.active_user_count_;
  is >> data.total_user_count_;
  data.meaningful_data_ = true;
  return is;
}

std::ostream&
operator<<(std::ostream& os, const ChannelInventoryInnerData& data)
  /*throw(eh::Exception)*/
{
  os << data.sum_user_ecpm_ << '\t';
  os << data.active_user_count_ << '\t';
  os << data.total_user_count_;
  return os;
}

} // namespace LogProcessing
} // namespace AdServer

