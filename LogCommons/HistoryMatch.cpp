
#include "HistoryMatch.hpp"
#include <LogCommons/LogCommons.ipp>

namespace AdServer {
namespace LogProcessing {

template <> const char* HistoryMatchTraits::B::base_name_ = "HistoryMatch";
template <> const char* HistoryMatchTraits::B::signature_ = "HistoryMatch";
template <> const char* HistoryMatchTraits::B::current_version_ = "1.0";

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is,
  HistoryMatchData& data) /*throw(eh::Exception)*/
{
  is >> data.time_;
  is >> data.user_id_;
  is >> data.temporary_;
  is >> data.last_colo_id_;
  is >> data.placement_colo_id_;
  is >> data.request_colo_id_;
  is >> data.search_channels_;
  is >> data.page_channels_;
  is >> data.url_channels_;
  return is;
}

std::ostream&
operator<<(std::ostream& os,
  const HistoryMatchData& data) /*throw(eh::Exception)*/
{
  return os
    << data.time_ << '\t'
    << data.user_id_ << '\t'
    << data.temporary_ << '\t'
    << data.last_colo_id_ << '\t'
    << data.placement_colo_id_ << '\t'
    << data.request_colo_id_ << '\t'
    << data.search_channels_ << '\t'
    << data.page_channels_ << '\t'
    << data.url_channels_;
}

} // namespace LogProcessing
} // namespace AdServer

