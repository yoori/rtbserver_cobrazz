#ifndef AD_SERVER_LOG_PROCESSING_CHANNEL_COUNT_STAT_HPP
#define AD_SERVER_LOG_PROCESSING_CHANNEL_COUNT_STAT_HPP


#include "LogCommons.hpp"
#include "StatCollector.hpp"

namespace AdServer {
namespace LogProcessing {

class ChannelCountStatInnerKey
{
public:
  ChannelCountStatInnerKey()
  :
    channel_type_(),
    channel_count_(),
    hash_()
  {
  }

  ChannelCountStatInnerKey(
    char channel_type,
    unsigned long channel_count
  )
  :
    channel_type_(channel_type),
    channel_count_(channel_count),
    hash_()
  {
    calc_hash_();
  }

  bool operator==(const ChannelCountStatInnerKey& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return channel_type_ == rhs.channel_type_ &&
      channel_count_ == rhs.channel_count_;
  }

  char channel_type() const
  {
    return channel_type_;
  }

  unsigned long channel_count() const
  {
    return channel_count_;
  }

  size_t hash() const
  {
    return hash_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, ChannelCountStatInnerKey& key)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const ChannelCountStatInnerKey& key)
    /*throw(eh::Exception)*/;

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    hash_add(hasher, channel_count_);
    hash_add(hasher, channel_type_);
  }

  bool channel_type_is_valid_() const
  {
    return channel_type_ == 'A' || channel_type_ == 'D';
  }

  void invariant() const /*throw(eh::Exception)*/
  {
    if (!channel_type_is_valid_())
    {
      Stream::Error es;
      es << "ChannelCountStatInnerKey::invariant(): channel_type_ "
         << "has invalid value '" << channel_type_ << '\'';
      throw ConstraintViolation(es);
    }
  }

  char channel_type_;
  unsigned long channel_count_;
  size_t hash_;
};

class ChannelCountStatInnerData
{
public:
  explicit
  ChannelCountStatInnerData(
    unsigned long users_count = 0
  )
  :
    users_count_(users_count)
  {
  }

  bool operator==(const ChannelCountStatInnerData& rhs) const
  {
    return users_count_ == rhs.users_count_;
  }

  ChannelCountStatInnerData&
  operator+=(const ChannelCountStatInnerData& rhs)
  {
    users_count_ += rhs.users_count_;
    return *this;
  }

  unsigned long users_count() const
  {
    return users_count_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, ChannelCountStatInnerData& data)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const ChannelCountStatInnerData& data)
    /*throw(eh::Exception)*/;

private:
  unsigned long users_count_;
};

struct ChannelCountStatKey
{
  ChannelCountStatKey(): isp_sdate_(), colo_id_(), hash_() {}

  ChannelCountStatKey(
    const DayTimestamp& isp_sdate,
    unsigned long colo_id
  )
  :
    isp_sdate_(isp_sdate),
    colo_id_(colo_id),
    hash_()
  {
    calc_hash_();
  }

  bool operator==(const ChannelCountStatKey& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return isp_sdate_ == rhs.isp_sdate_ && colo_id_ == rhs.colo_id_;
  }

public:
  const DayTimestamp& isp_sdate() const
  {
    return isp_sdate_;
  }

  unsigned long colo_id() const
  {
    return colo_id_;
  }

  size_t hash() const
  {
    return hash_;
  }

  friend
  std::istream&
  operator>>(std::istream& is, ChannelCountStatKey& key);

  friend
  std::ostream&
  operator<<(std::ostream& os, const ChannelCountStatKey& key)
    /*throw(eh::Exception)*/;

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    hash_add(hasher, colo_id_);
    isp_sdate_.hash_add(hasher);
  }

  DayTimestamp isp_sdate_;
  unsigned long colo_id_;
  size_t hash_;
};

typedef StatCollector<
          ChannelCountStatInnerKey, ChannelCountStatInnerData, false, true
        > ChannelCountStatInnerCollector;

typedef ChannelCountStatInnerCollector ChannelCountStatData;

typedef StatCollector<ChannelCountStatKey, ChannelCountStatData>
  ChannelCountStatCollector;

typedef LogDefaultTraits<ChannelCountStatCollector> ChannelCountStatTraits;

} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_CHANNEL_COUNT_STAT_HPP */

