
#include "ChannelTriggerImpStat.hpp"
#include <LogCommons/BufferWriter.hpp>
#include <LogCommons/LogCommons.ipp>

namespace AdServer::LogProcessing
{

  template <> const char*
  ChannelTriggerImpStatTraits::B::base_name_ = "ChannelTriggerImpStat";

  template <> const char*
  ChannelTriggerImpStatTraits::B::signature_ = "ChannelTriggerImpStat";

  template <> const char*
  ChannelTriggerImpStatTraits::B::current_version_ = "3.5";

  std::istream&
  operator>>(std::istream& is, ChannelTriggerImpStatKey& key)
  {
    is >> key.sdate_;
    read_eol(is);
    is >> key.colo_id_;
    key.calc_hash_();
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const ChannelTriggerImpStatKey& key)
    /*throw(eh::Exception)*/
  {
    out << key.sdate_ << '\n' << key.colo_id_;
    return out;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, ChannelTriggerImpStatInnerKey& key)
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
  operator<<(BufferWriter& out, const ChannelTriggerImpStatInnerKey& key)
    /*throw(eh::Exception)*/
  {
    key.invariant();
    out << key.channel_trigger_id_ << '\t';
    out << key.channel_id_ << '\t';
    out << key.type_;
    return out;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, ChannelTriggerImpStatInnerKey_V_2_3& key)
    /*throw(eh::Exception)*/
  {
    is >> key.channel_trigger_id_;
    is >> key.type_;
    if (is)
    {
      key.invariant();
      key.calc_hash_();
    }
    return is;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, ChannelTriggerImpStatInnerData& data)
    /*throw(eh::Exception)*/
  {
    is >> data.approximated_imps_;
    is >> data.approximated_clicks_;
    if (is)
    {
      data.invariant();
    }
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const ChannelTriggerImpStatInnerData& data)
    /*throw(eh::Exception)*/
  {
    data.invariant();
    out << data.approximated_imps_ << '\t';
    out << data.approximated_clicks_;
    return out;
  }

} // namespace AdServer::LogProcessing
