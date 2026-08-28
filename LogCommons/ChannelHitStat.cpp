
#include "ChannelHitStat.hpp"
#include <LogCommons/BufferWriter.hpp>
#include <LogCommons/LogCommons.ipp>

namespace AdServer::LogProcessing
{

  template <> const char*
  ChannelHitStatTraits::B::base_name_ = "ChannelHitStat";

  template <> const char*
  ChannelHitStatTraits::B::signature_ = "ChannelHitStat";

  template <> const char*
  ChannelHitStatTraits::B::current_version_ = "3.3";

  std::istream&
  operator>>(std::istream& is, ChannelHitStatKey& key)
  {
    is >> key.sdate_;
    read_eol(is);
    is >> key.colo_id_;
    key.calc_hash_();
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const ChannelHitStatKey& key)
    /*throw(eh::Exception)*/
  {
    out << key.sdate_ << '\n' << key.colo_id_;
    return out;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, ChannelHitStatInnerKey& key)
  {
    is >> key.channel_id_;
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const ChannelHitStatInnerKey& key)
  {
    out << key.channel_id_;
    return out;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, ChannelHitStatInnerData& data)
  {
    is >> data.hits_;
    is >> data.hits_urls_;
    is >> data.hits_kws_;
    is >> data.hits_search_kws_;
    is >> data.hits_url_kws_;
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const ChannelHitStatInnerData& data)
  {
    out << data.hits_ << '\t';
    out << data.hits_urls_ << '\t';
    out << data.hits_kws_ << '\t';
    out << data.hits_search_kws_ << '\t';
    out << data.hits_url_kws_;
    return out;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, ChannelHitStatInnerData_V_1_0& data)
  {
    is >> data.hits_;
    is >> data.hits_urls_;
    is >> data.hits_kws_;
    is >> data.hits_search_kws_;
    is >> data.hits_repeat_kws_;
    return is;
  }

} // namespace AdServer::LogProcessing
