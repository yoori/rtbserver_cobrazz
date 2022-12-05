
#include "ChannelPerformance.hpp"
#include <LogCommons/LogCommons.ipp>

namespace AdServer {
namespace LogProcessing {

template <> const char*
ChannelPerformanceTraits::B::base_name_ = "ChannelPerformance";

template <> const char*
ChannelPerformanceTraits::B::signature_ = "ChannelPerformance";

template <> const char*
ChannelPerformanceTraits::B::current_version_ = "3.0";

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, ChannelPerformanceKey& key)
{
  is >> key.sdate_;
  is >> key.colo_id_;
  key.calc_hash_();
  return is;
}

std::istream&
operator>>(std::istream& is, ChannelPerformanceKey& key)
{
  is >> key.sdate_;
  read_eol(is);
  is >> key.colo_id_;
  key.calc_hash_();
  return is;
}

std::ostream&
operator<<(std::ostream& os, const ChannelPerformanceKey& key)
  /*throw(eh::Exception)*/
{
  os << key.sdate_ << '\n' << key.colo_id_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, ChannelPerformanceInnerKey& key)
{
  is >> key.channel_id_;
  is >> key.ccg_id_;

  StringIoWrapperOptional tag_size;
  is >> tag_size;
  key.tag_size_ = std::move(tag_size.get());

  key.calc_hash_();
  return is;
}

std::istream&
operator>>(std::istream& is, ChannelPerformanceInnerKey& key)
{
  is >> key.channel_id_;
  read_tab(is);
  is >> key.ccg_id_;
  read_tab(is);

  StringIoWrapperOptional tag_size;
  is >> tag_size;
  key.tag_size_ = std::move(tag_size.get());

  key.calc_hash_();
  return is;
}

std::ostream&
operator<<(std::ostream& os, const ChannelPerformanceInnerKey& key)
{
  os << key.channel_id_ << '\t';
  os << key.ccg_id_ << '\t';
  os << StringIoWrapperOptional(key.tag_size_.str());
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, ChannelPerformanceInnerData& data)
{
  is >> data.requests_;
  is >> data.imps_;
  is >> data.clicks_;
  is >> data.actions_;
  is >> data.revenue_;
  if (is)
  {
    data.invariant();
  }
  return is;
}

std::istream&
operator>>(std::istream& is, ChannelPerformanceInnerData& data)
{
  is >> data.requests_;
  read_tab(is);
  is >> data.imps_;
  read_tab(is);
  is >> data.clicks_;
  read_tab(is);
  is >> data.actions_;
  read_tab(is);
  is >> data.revenue_;
  if (is)
  {
    data.invariant();
  }
  return is;
}

std::ostream&
operator<<(std::ostream& os, const ChannelPerformanceInnerData& data)
{
  data.invariant();
  os << data.requests_ << '\t';
  os << data.imps_ << '\t';
  os << data.clicks_ << '\t';
  os << data.actions_ << '\t';
  os << data.revenue_;
  return os;
}

} // namespace LogProcessing
} // namespace AdServer

