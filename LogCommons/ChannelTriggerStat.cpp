
#include "ChannelTriggerStat.hpp"
#include <LogCommons/BufferWriter.hpp>
#include <LogCommons/LogCommons.ipp>

namespace AdServer::LogProcessing
{

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

  BufferWriter&
  operator<<(BufferWriter& out, const ChannelTriggerStatKey& key)
    /*throw(eh::Exception)*/
  {
    out << key.sdate_ << '\n' << key.colo_id_;
    return out;
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

  BufferWriter&
  operator<<(BufferWriter& out, const ChannelTriggerStatInnerKey& key)
    /*throw(eh::Exception)*/
  {
    key.invariant();
    out << key.channel_trigger_id_ << '\t';
    out << key.channel_id_ << '\t';
    out << key.type_;
    return out;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, ChannelTriggerStatInnerKey_V_2_4& key)
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

  BufferWriter&
  operator<<(BufferWriter& out, const ChannelTriggerStatInnerData& data)
    /*throw(eh::Exception)*/
  {
    data.invariant();
    out << data.hits_;
    return out;
  }

} // namespace AdServer::LogProcessing
