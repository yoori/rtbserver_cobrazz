
#include "DeviceChannelCountStat.hpp"
#include <LogCommons/BufferWriter.hpp>
#include <LogCommons/LogCommons.ipp>

namespace AdServer::LogProcessing
{

  template <> const char* DeviceChannelCountStatTraits::B::base_name_ = "DeviceChannelCountStat";
  template <> const char* DeviceChannelCountStatTraits::B::signature_ = "DeviceChannelCountStat";
  template <> const char* DeviceChannelCountStatTraits::B::current_version_ = "2.6";

  std::istream&
  operator>>(std::istream& is, DeviceChannelCountStatKey& key)
  {
    is >> key.isp_sdate_;
    read_eol(is);
    is >> key.colo_id_;
    key.calc_hash_();
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const DeviceChannelCountStatKey& key)
    /*throw(eh::Exception)*/
  {
    out << key.isp_sdate_ << '\n' << key.colo_id_;
    return out;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, DeviceChannelCountStatInnerKey& key)
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

  BufferWriter&
  operator<<(BufferWriter& out, const DeviceChannelCountStatInnerKey& key)
    /*throw(eh::Exception)*/
  {
    key.invariant();
    out << key.device_channel_id_ << '\t';
    out << key.channel_type_ << '\t';
    out << key.channel_count_;
    return out;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, DeviceChannelCountStatInnerData& data)
    /*throw(eh::Exception)*/
  {
    is >> data.users_count_;
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const DeviceChannelCountStatInnerData& data)
    /*throw(eh::Exception)*/
  {
    out << data.users_count_;
    return out;
  }

} // namespace AdServer::LogProcessing
