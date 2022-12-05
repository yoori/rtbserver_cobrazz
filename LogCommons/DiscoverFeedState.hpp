#ifndef AD_SERVER_LOG_PROCESSING_DISCOVER_FEED_STATE_HPP
#define AD_SERVER_LOG_PROCESSING_DISCOVER_FEED_STATE_HPP

#include <iosfwd>
#include <sstream>
#include <Generics/Time.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include "LogCommons.hpp"
#include "StatCollector.hpp"


namespace AdServer {
namespace LogProcessing {

class DiscoverFeedStateKey
{
public:
  DiscoverFeedStateKey(
    unsigned long feed_id = 0
  )
  :
    feed_id_(feed_id)
  {
  }

  bool operator==(const DiscoverFeedStateKey &rhs) const
  {
    return feed_id_ == rhs.feed_id_;
  }

  unsigned long feed_id() const
  {
    return feed_id_;
  }

  size_t hash() const
  {
    return feed_id_;
  }

  friend std::istream& operator>>(std::istream &is,
    DiscoverFeedStateKey &key);
  friend std::ostream& operator<<(std::ostream &os,
    const DiscoverFeedStateKey &key) /*throw(eh::Exception)*/;

private:
  unsigned long feed_id_;
};

class DiscoverFeedStateData
{
public:
  DiscoverFeedStateData()
  :
    time_(),
    total_news_()
  {
  }

  DiscoverFeedStateData(
    const SecondsTimestamp &time,
    unsigned long total_news
  )
  :
    time_(time),
    total_news_(total_news)
  {
  }

  bool operator==(const DiscoverFeedStateData &rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return time_ == rhs.time_ && total_news_ == rhs.total_news_;
  }

  DiscoverFeedStateData& operator+=(const DiscoverFeedStateData &rhs)
  {
    if (time_ < rhs.time_)
    {
      time_ = rhs.time_;
      total_news_ = rhs.total_news_;
    }
    return *this;
  }

  const SecondsTimestamp& time() const
  {
    return time_;
  }

  unsigned long total_news() const
  {
    return total_news_;
  }

  friend std::istream& operator>>(std::istream &is,
    DiscoverFeedStateData &data);
  friend std::ostream& operator<<(std::ostream &os,
    const DiscoverFeedStateData &data);

private:
  SecondsTimestamp time_;
  unsigned long total_news_;
};


typedef StatCollector<DiscoverFeedStateKey, DiscoverFeedStateData>
  DiscoverFeedStateCollector;

typedef LogDefaultTraits<DiscoverFeedStateCollector, false>
  DiscoverFeedStateTraits;



} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_DISCOVER_FEED_STATE_HPP */

