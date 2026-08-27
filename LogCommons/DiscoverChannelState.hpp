#pragma once

#include <iosfwd>
#include <Generics/Time.hpp>
#include "LogCommons.hpp"
#include "StatCollector.hpp"

namespace AdServer::LogProcessing
{

  class DiscoverChannelStateKey
  {
  public:

    DiscoverChannelStateKey(std::uint32_t channel_id = 0)
    :
      channel_id_(channel_id)
    {
    }

    bool operator==(const DiscoverChannelStateKey &rhs) const
    {
      return channel_id_ == rhs.channel_id_;
    }

    std::uint32_t channel_id() const
    {
      return channel_id_;
    }

    size_t hash() const
    {
      return channel_id_;
    }

    friend std::istream& operator>>(std::istream &is, DiscoverChannelStateKey &key);
    friend std::ostream& operator<<(std::ostream &os,
      const DiscoverChannelStateKey &key) /*throw(eh::Exception)*/;

  private:
    std::uint32_t channel_id_;
  };

  class DiscoverChannelStateData
  {
  public:
    DiscoverChannelStateData()
    :
      time_(),
      total_news_(),
      daily_news_()
    {
    }

    DiscoverChannelStateData(
      const SecondsTimestamp &time,
      unsigned long total_news,
      unsigned long daily_news
    )
    :
      time_(time),
      total_news_(total_news),
      daily_news_(daily_news)
    {
    }

    bool operator==(const DiscoverChannelStateData &rhs) const
    {
      if (&rhs == this)
      {
        return true;
      }
      return time_ == rhs.time_ && total_news_ == rhs.total_news_ && daily_news_ == rhs.daily_news_;
    }

    DiscoverChannelStateData& operator+=(const DiscoverChannelStateData &rhs)
    {
      if (time_ < rhs.time_)
      {
        time_ = rhs.time_;
        total_news_ = rhs.total_news_;
        daily_news_ = rhs.daily_news_;
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

    unsigned long daily_news() const
    {
      return daily_news_;
    }

    friend std::istream& operator>>(std::istream &is, DiscoverChannelStateData &data);
    friend std::ostream& operator<<(std::ostream &os, const DiscoverChannelStateData &data);

  private:
    SecondsTimestamp time_;
    unsigned long total_news_;
    unsigned long daily_news_;
  };

  typedef StatCollector<DiscoverChannelStateKey, DiscoverChannelStateData>
    DiscoverChannelStateCollector;

  typedef LogDefaultTraits<DiscoverChannelStateCollector, false>
    DiscoverChannelStateTraits;


} // namespace AdServer::LogProcessing
