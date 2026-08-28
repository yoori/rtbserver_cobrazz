
#include "ChannelOverlapUserStat.hpp"
#include <LogCommons/BufferWriter.hpp>
#include <LogCommons/LogCommons.ipp>

namespace AdServer::LogProcessing
{

  template <> const char*
  ChannelOverlapUserStatTraits::B::base_name_ = "ChannelOverlapUserStat";

  template <> const char*
  ChannelOverlapUserStatTraits::B::signature_ = "ChannelOverlapUserStat";

  template <> const char*
  ChannelOverlapUserStatTraits::B::current_version_ = "2.5";

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, ChannelOverlapUserStatInnerKey& key)
    /*throw(eh::Exception)*/
  {
    is >> key.channel1_id_;
    is >> key.channel2_id_;
    key.calc_hash_();
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const ChannelOverlapUserStatInnerKey& key)
    /*throw(eh::Exception)*/
  {
    out << key.channel1_id_ << '\t';
    out << key.channel2_id_;
    return out;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, ChannelOverlapUserStatInnerData& data)
    /*throw(eh::Exception)*/
  {
    is >> data.unique_users_;
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const ChannelOverlapUserStatInnerData& data)
    /*throw(eh::Exception)*/
  {
    out << data.unique_users_;
    return out;
  }

} // namespace AdServer::LogProcessing
