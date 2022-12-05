
#include "ChannelHitStat.hpp"
#include <LogCommons/LogCommons.ipp>

namespace AdServer {
namespace LogProcessing {

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

std::ostream&
operator<<(std::ostream& os, const ChannelHitStatKey& key)
  /*throw(eh::Exception)*/
{
  os << key.sdate_ << '\n' << key.colo_id_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, ChannelHitStatInnerKey& key)
{
  is >> key.channel_id_;
  return is;
}

std::ostream&
operator<<(std::ostream& os, const ChannelHitStatInnerKey& key)
{
  os << key.channel_id_;
  return os;
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

std::ostream&
operator<<(std::ostream& os, const ChannelHitStatInnerData& data)
{
  os << data.hits_ << '\t';
  os << data.hits_urls_ << '\t';
  os << data.hits_kws_ << '\t';
  os << data.hits_search_kws_ << '\t';
  os << data.hits_url_kws_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(
  FixedBufStream<TabCategory>& is,
  ChannelHitStatInnerData_V_1_0& data
)
{
  is >> data.hits_;
  is >> data.hits_urls_;
  is >> data.hits_kws_;
  is >> data.hits_search_kws_;
  is >> data.hits_repeat_kws_;
  return is;
}

} // namespace LogProcessing
} // namespace AdServer

