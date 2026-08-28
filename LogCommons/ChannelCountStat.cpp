
#include "ChannelCountStat.hpp"
#include <LogCommons/BufferWriter.hpp>
#include <LogCommons/LogCommons.ipp>

namespace AdServer::LogProcessing
{

  template <> const char* ChannelCountStatTraits::B::base_name_ = "ChannelCountStat";
  template <> const char* ChannelCountStatTraits::B::signature_ = "ChannelCountStat";
  template <> const char* ChannelCountStatTraits::B::current_version_ = "1.0";

  std::istream&
  operator>>(std::istream& is, ChannelCountStatKey& key)
  {
    is >> key.isp_sdate_;
    read_eol(is);
    is >> key.colo_id_;
    key.calc_hash_();
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const ChannelCountStatKey& key)
    /*throw(eh::Exception)*/
  {
    out << key.isp_sdate_ << '\n' << key.colo_id_;
    return out;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, ChannelCountStatInnerKey& key)
    /*throw(eh::Exception)*/
  {
    is >> key.channel_type_;
    is >> key.channel_count_;
    if (is)
    {
      key.invariant();
      key.calc_hash_();
    }
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const ChannelCountStatInnerKey& key)
    /*throw(eh::Exception)*/
  {
    key.invariant();
    out << key.channel_type_ << '\t';
    out << key.channel_count_;
    return out;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, ChannelCountStatInnerData& data)
    /*throw(eh::Exception)*/
  {
    is >> data.users_count_;
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const ChannelCountStatInnerData& data)
    /*throw(eh::Exception)*/
  {
    out << data.users_count_;
    return out;
  }

} // namespace AdServer::LogProcessing
