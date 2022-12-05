
#include "DiscoverChannelState.hpp"
#include <LogCommons/LogCommons.ipp>

namespace AdServer {
namespace LogProcessing {

template <> const char* DiscoverChannelStateTraits::B::base_name_ =
  "DiscoverChannelState";
template <> const char* DiscoverChannelStateTraits::B::signature_ =
  "DiscoverChannelState";
template <> const char* DiscoverChannelStateTraits::B::current_version_ = "1.0";

std::istream&
operator>>(std::istream &is, DiscoverChannelStateKey &key)
{
  is >> key.channel_id_;
  return is;
}

std::ostream&
operator<<(std::ostream &os, const DiscoverChannelStateKey &key)
  /*throw(eh::Exception)*/
{
  os << key.channel_id_;
  return os;
}

std::istream&
operator>>(std::istream &is, DiscoverChannelStateData &data)
{
  is >> data.time_;
  read_tab(is);
  is >> data.total_news_;
  read_tab(is);
  is >> data.daily_news_;
  return is;
}

std::ostream&
operator<<(std::ostream &os, const DiscoverChannelStateData &data)
{
  os << data.time_ << '\t';
  os << data.total_news_ << '\t';
  os << data.daily_news_;
  return os;
}

} // namespace LogProcessing
} // namespace AdServer

