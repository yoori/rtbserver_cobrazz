#ifndef AD_SERVER_LOG_PROCESSING_USER_AGENT_STAT_HPP
#define AD_SERVER_LOG_PROCESSING_USER_AGENT_STAT_HPP


#include <iosfwd>
#include <sstream>
#include <Generics/Time.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Commons/StringHolder.hpp>
#include "LogCommons.hpp"
#include "StatCollector.hpp"

namespace AdServer {
namespace LogProcessing {

class UserAgentStatInnerKey
{
public:
  UserAgentStatInnerKey()
  :
    user_agent_(new AdServer::Commons::StringHolder("")),
    hash_()
  {
  }

  explicit
  UserAgentStatInnerKey(
    const AdServer::Commons::StringHolder_var& user_agent
  )
  :
    user_agent_(user_agent),
    hash_()
  {
    if (user_agent_->str().length() > max_user_agent_length_)
    {
      std::string tmp;
      trim(tmp, user_agent_->str(), max_user_agent_length_);
      user_agent_ = new AdServer::Commons::StringHolder(std::move(tmp));
    }
    calc_hash_();
  }

  bool operator==(const UserAgentStatInnerKey& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return user_agent_->str() == rhs.user_agent_->str();
  }

  const std::string& user_agent() const
  {
    return user_agent_->str();
  }

  size_t hash() const
  {
    return hash_;
  }

  static std::size_t max_user_agent_length()
  {
    return max_user_agent_length_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, UserAgentStatInnerKey& key)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const UserAgentStatInnerKey& key)
    /*throw(eh::Exception)*/;

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    hash_add(hasher, user_agent_->str());
  }

  void invariant() const /*throw(eh::Exception)*/
  {
    if (user_agent_->str().empty())
    {
      throw ConstraintViolation("UserAgentStatInnerKey::"
        "invariant(): user_agent_ must be non-empty");
    }
  }

  static const std::size_t max_user_agent_length_ = 2000;

  AdServer::Commons::StringHolder_var user_agent_;
  size_t hash_;
};

class UserAgentStatInnerData
{
public:
  UserAgentStatInnerData()
  :
    requests_(),
    holder_(new DataHolder())
  {}

  template <typename ChannelInteratorType, typename PlatformIteratorType>
  UserAgentStatInnerData(
    unsigned long requests,
    ChannelInteratorType channels_begin,
    ChannelInteratorType channels_end,
    PlatformIteratorType platforms_begin,
    PlatformIteratorType platforms_end
  )
  :
    requests_(requests),
    holder_(new DataHolder(channels_begin, channels_end,
      platforms_begin, platforms_end))
  {
  }

  bool operator==(const UserAgentStatInnerData& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return requests_ == rhs.requests_ &&
      (holder_.in() == rhs.holder_.in() || *holder_ == *rhs.holder_);
  }

  UserAgentStatInnerData&
  operator+=(const UserAgentStatInnerData& rhs)
  {
    requests_ += rhs.requests_;
    holder_ = rhs.holder_;
    return *this;
  }

  unsigned long requests() const
  {
    return requests_;
  }

  const NumberList& channels() const
  {
    return holder_->channels;
  }

  const NumberList& platforms() const
  {
    return holder_->platforms;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, UserAgentStatInnerData& data)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const UserAgentStatInnerData& data)
    /*throw(eh::Exception)*/;

private:
  struct DataHolder: public ReferenceCounting::AtomicImpl
  {
    DataHolder() {}

    template <typename ChannelIteratorType, typename PlatformIteratorType>
    DataHolder(
      ChannelIteratorType channels_begin,
      ChannelIteratorType channels_end,
      PlatformIteratorType platforms_begin,
      PlatformIteratorType platforms_end
    )
    :
      channels(channels_begin, channels_end),
      platforms(platforms_begin, platforms_end)
    {}

    bool operator==(const DataHolder& data) const
    {
      if (&data == this)
      {
        return true;
      }
      return channels == data.channels &&
        platforms == data.platforms;
    }

    void invariant() const /*throw(ConstraintViolation)*/
    {
    }

    NumberList channels;
    NumberList platforms;

  private:
    virtual ~DataHolder() noexcept {}
  };

  typedef ReferenceCounting::AssertPtr<DataHolder>::Ptr DataHolder_var;

  unsigned long requests_;
  DataHolder_var holder_;
};

typedef StatCollector<
          UserAgentStatInnerKey, UserAgentStatInnerData, false, true
        > UserAgentStatInnerCollector;

typedef DayTimestamp UserAgentStatKey;
typedef UserAgentStatInnerCollector UserAgentStatData;

typedef StatCollector<UserAgentStatKey, UserAgentStatData>
  UserAgentStatCollector;

typedef LogDefaultTraits<UserAgentStatCollector> UserAgentStatTraits;

} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_USER_AGENT_STAT_HPP */

