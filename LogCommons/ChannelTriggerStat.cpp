
#include "ChannelTriggerStat.hpp"
#include <LogCommons/LogCommons.ipp>

namespace AdServer {
namespace LogProcessing {

template <> const char*
ChannelTriggerStatTraits::B::base_name_ = "ChannelTriggerStat";

template <> const char*
ChannelTriggerStatTraits::B::signature_ = "ChannelTriggerStat";

template <> const char*
ChannelTriggerStatTraits::B::current_version_ = "3.5";

std::istream&
operator>>(std::istream& is, ChannelTriggerStatKey& key)
{
  is >> key.sdate_;
  read_eol(is);
  is >> key.colo_id_;
  key.calc_hash_();
  return is;
}

std::ostream&
operator<<(std::ostream& os, const ChannelTriggerStatKey& key)
  /*throw(eh::Exception)*/
{
  os << key.sdate_ << '\n' << key.colo_id_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, ChannelTriggerStatInnerKey& key)
  /*throw(eh::Exception)*/
{
  is >> key.channel_trigger_id_;
  is >> key.channel_id_;
  is >> key.type_;
  if (is)
  {
    key.invariant();
    key.calc_hash_();
  }
  return is;
}

std::ostream&
operator<<(std::ostream& os, const ChannelTriggerStatInnerKey& key)
  /*throw(eh::Exception)*/
{
  key.invariant();
  os << key.channel_trigger_id_ << '\t';
  os << key.channel_id_ << '\t';
  os << key.type_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(
  FixedBufStream<TabCategory>& is,
  ChannelTriggerStatInnerKey_V_1_2& key
)
  /*throw(eh::Exception)*/
{
  is >> key.channel_id_;
  is >> key.type_;
  Aux_::StringIoWrapper trigger_wrapper;
  is >> trigger_wrapper;
  key.trigger_ =
    new AdServer::Commons::StringHolder(std::move(trigger_wrapper));
  key.invariant();
  key.calc_hash_();
  return is;
}

std::ostream&
operator<<(std::ostream& os, const ChannelTriggerStatInnerKey_V_1_2& key)
  /*throw(eh::Exception)*/
{
  key.invariant();
  os << key.channel_id_ << '\t';
  os << key.type_ << '\t';
  os << Aux_::StringIoWrapper(key.trigger_->str());
  return os;
}

FixedBufStream<TabCategory>&
operator>>(
  FixedBufStream<TabCategory>& is,
  ChannelTriggerStatInnerKey_V_2_4& key
)
  /*throw(eh::Exception)*/
{
  is >> key.type_;
  is >> key.channel_trigger_id_;
  if (is)
  {
    key.invariant();
    key.calc_hash_();
  }
  return is;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, ChannelTriggerStatInnerData& data)
  /*throw(eh::Exception)*/
{
  is >> data.hits_;
  if (is)
  {
    data.invariant();
  }
  return is;
}

std::ostream&
operator<<(std::ostream& os, const ChannelTriggerStatInnerData& data)
  /*throw(eh::Exception)*/
{
  data.invariant();
  os << data.hits_;
  return os;
}

} // namespace LogProcessing
} // namespace AdServer

