#ifndef AD_SERVER_LOG_PROCESSING_HISTORY_MATCH_CHANNELS_HPP
#define AD_SERVER_LOG_PROCESSING_HISTORY_MATCH_CHANNELS_HPP


#include <iosfwd>
#include <Generics/Time.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Commons/UserInfoManip.hpp>
#include <LogCommons/LogCommons.hpp>
#include <LogCommons/StatCollector.hpp>
#include <LogCommons/GenericLogIoImpl.hpp>

namespace AdServer {
namespace LogProcessing {

class HistoryMatchData
{
  friend
  FixedBufStream<TabCategory>& operator>>(
    FixedBufStream<TabCategory>& is,
    HistoryMatchData& data) /*throw(eh::Exception)*/;

  friend
  std::ostream& operator<<(
    std::ostream& os,
    const HistoryMatchData& data) /*throw(eh::Exception)*/;

public:
  HistoryMatchData()
  : time_(),
    user_id_(),
    temporary_(),
    last_colo_id_(),
    placement_colo_id_(),
    request_colo_id_(),
    search_channels_(),
    page_channels_(),
    url_channels_()
  {
  }

  HistoryMatchData(
    const SecondsTimestamp& time,
    const UserId& user_id,
    bool temporary,
    unsigned long last_colo_id,
    unsigned long placement_colo_id,
    unsigned long request_colo_id,
    NumberList search_channels,
    NumberList page_channels,
    NumberList url_channels)
  : time_(time),
    user_id_(user_id),
    temporary_(temporary),
    last_colo_id_(last_colo_id),
    placement_colo_id_(placement_colo_id),
    request_colo_id_(request_colo_id),
    search_channels_(search_channels),
    page_channels_(page_channels),
    url_channels_(url_channels)
  {
  }

  bool operator==(const HistoryMatchData& data) const
  {
    if (this == &data)
    {
      return true;
    }
    return time_ == data.time_ &&
      user_id_ == data.user_id_ &&
      temporary_ == data.temporary_ &&
      last_colo_id_ == data.last_colo_id_ &&
      placement_colo_id_ == data.placement_colo_id_ &&
      request_colo_id_ == data.request_colo_id_ &&
      search_channels_ == data.search_channels_ &&
      page_channels_ == data.page_channels_ &&
      url_channels_ == data.url_channels_;
  }

  const SecondsTimestamp& time() const
  {
    return time_;
  }

  bool temporary() const
  {
    return temporary_;
  }

  unsigned long last_colo_id() const
  {
    return last_colo_id_;
  }

  unsigned long placement_colo_id() const
  {
    return placement_colo_id_;
  }

  unsigned long request_colo_id() const
  {
    return request_colo_id_;
  }

  const NumberList& search_channels() const
  {
    return search_channels_;
  }

  const NumberList& page_channels() const
  {
    return page_channels_;
  }

  const NumberList& url_channels() const
  {
    return url_channels_;
  }

  unsigned long distrib_hash() const
  {
    return user_id_.hash();
  }

private:
  SecondsTimestamp time_;
  UserId user_id_; // currently used only for calculating distrib_hash()
  bool temporary_;
  unsigned long last_colo_id_;
  unsigned long placement_colo_id_;
  unsigned long request_colo_id_;
  NumberList search_channels_;
  NumberList page_channels_;
  NumberList url_channels_;
};

typedef SeqCollector<HistoryMatchData, true> HistoryMatchCollector;

typedef LogDefaultTraits<HistoryMatchCollector, false, false> HistoryMatchTraits;

} // namespace LogProcessing
} // namespace AdServer

#endif // AD_SERVER_LOG_PROCESSING_HISTORY_MATCH_CHANNELS_HPP
