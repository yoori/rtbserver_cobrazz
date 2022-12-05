
#include "DiscoverFeedState.hpp"
#include <LogCommons/LogCommons.ipp>

namespace AdServer {
namespace LogProcessing {

template <> const char* DiscoverFeedStateTraits::B::base_name_ = "DiscoverFeedState";
template <> const char* DiscoverFeedStateTraits::B::signature_ = "DiscoverFeedState";
template <> const char* DiscoverFeedStateTraits::B::current_version_ = "1.0";

std::istream&
operator>>(std::istream &is, DiscoverFeedStateKey &key)
{
  is >> key.feed_id_;
  return is;
}

std::ostream&
operator<<(std::ostream &os, const DiscoverFeedStateKey &key)
  /*throw(eh::Exception)*/
{
  os << key.feed_id_;
  return os;
}

std::istream&
operator>>(std::istream &is, DiscoverFeedStateData &data)
{
  is >> data.time_;
  read_tab(is);
  is >> data.total_news_;
  return is;
}

std::ostream&
operator<<(std::ostream &os, const DiscoverFeedStateData &data)
{
  os << data.time_ << '\t';
  os << data.total_news_;
  return os;
}

} // namespace LogProcessing
} // namespace AdServer

