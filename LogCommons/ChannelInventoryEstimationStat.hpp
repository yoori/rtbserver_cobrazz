#ifndef AD_SERVER_LOG_PROCESSING_CHANNEL_INVENTORY_ESTIMATION_STAT_HPP
#define AD_SERVER_LOG_PROCESSING_CHANNEL_INVENTORY_ESTIMATION_STAT_HPP


#include <iosfwd>
#include <sstream>
#include <Generics/Time.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include "LogCommons.hpp"
#include "StatCollector.hpp"


namespace AdServer {
namespace LogProcessing {

// ChannelInventoryEstimationStat v1.0 - AdServer v2.1

// ChannelInventoryEstimationStat v1.1 - AdServer v2.2 (and higher)

class ChannelInventoryEstimationStatInnerKey
{
public:
  typedef Generics::SimpleDecimal<uint32_t, 2, 1> LevelT;

  ChannelInventoryEstimationStatInnerKey()
  :
    channel_id_(),
    level_(),
    hash_()
  {
  }

  ChannelInventoryEstimationStatInnerKey(
    unsigned long channel_id,
    const LevelT& level
  )
  :
    channel_id_(channel_id),
    level_(level),
    hash_()
  {
    calc_hash_();
  }

  bool operator==(const ChannelInventoryEstimationStatInnerKey& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return channel_id_ == rhs.channel_id_ && level_ == rhs.level_;
  }

  unsigned long channel_id() const
  {
    return channel_id_;
  }

  const LevelT& level() const
  {
    return level_;
  }

  size_t hash() const
  {
    return hash_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is,
    ChannelInventoryEstimationStatInnerKey& key);

  friend
  std::ostream&
  operator<<(std::ostream& os,
    const ChannelInventoryEstimationStatInnerKey& key);

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    hash_add(hasher, channel_id_);
    hash_add(hasher, level_);
  }

  void invariant() const /*throw(eh::Exception)*/
  {
    if (level_ < LevelT::ZERO || level_ > max_level_value_)
    {
      Stream::Error es;
      es << "ChannelInventoryEstimationStatInnerKey::invariant(): "
         << "level_ = " << level_ << " (must be 0.0 <= level_ <= "
         << max_level_value_ << ")";
      throw ConstraintViolation(es);
    }
  }

  static const LevelT max_level_value_;

  unsigned long channel_id_;
  LevelT level_;
  size_t hash_;
};

class ChannelInventoryEstimationStatInnerData
{
public:
  typedef AdServer::LogProcessing::FixedNumber FixedNum;

  ChannelInventoryEstimationStatInnerData()
  :
    users_regular_(FixedNum::ZERO),
    users_from_now_(FixedNum::ZERO)
  {
  }

  ChannelInventoryEstimationStatInnerData(
    const FixedNum& users_regular,
    const FixedNum& users_from_now
  )
  :
    users_regular_(users_regular),
    users_from_now_(users_from_now)
  {
  }

  bool operator==(const ChannelInventoryEstimationStatInnerData& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return users_regular_ == rhs.users_regular_ &&
      users_from_now_ == rhs.users_from_now_;
  }

  ChannelInventoryEstimationStatInnerData&
  operator+=(const ChannelInventoryEstimationStatInnerData& rhs)
  {
    users_regular_ += rhs.users_regular_;
    users_from_now_ += rhs.users_from_now_;
    return *this;
  }

  const FixedNum& users_regular() const
  {
    return users_regular_;
  }

  const FixedNum& users_from_now() const
  {
    return users_from_now_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is,
    ChannelInventoryEstimationStatInnerData& data);

  friend
  std::ostream&
  operator<<(std::ostream& os,
    const ChannelInventoryEstimationStatInnerData& data);

private:
  FixedNum users_regular_;
  FixedNum users_from_now_;
};

struct ChannelInventoryEstimationStatKey
{
  ChannelInventoryEstimationStatKey(): sdate_(), colo_id_(), hash_() {}

  ChannelInventoryEstimationStatKey(
    const DayTimestamp& sdate,
    unsigned long colo_id
  )
  :
    sdate_(sdate),
    colo_id_(colo_id),
    hash_()
  {
    calc_hash_();
  }

  bool operator==(const ChannelInventoryEstimationStatKey& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return sdate_ == rhs.sdate_ && colo_id_ == rhs.colo_id_;
  }

  const DayTimestamp& sdate() const
  {
    return sdate_;
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
  operator>>(std::istream& is, ChannelInventoryEstimationStatKey& key);

  friend
  std::ostream&
  operator<<(std::ostream& os, const ChannelInventoryEstimationStatKey& key)
    /*throw(eh::Exception)*/;

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    sdate_.hash_add(hasher);
    hash_add(hasher, colo_id_);
  }

  DayTimestamp sdate_;
  unsigned long colo_id_;
  size_t hash_;
};

typedef ChannelInventoryEstimationStatInnerKey
  ChannelInventoryEstimationStatInnerKey_V_1_0;

typedef ChannelInventoryEstimationStatInnerData
  ChannelInventoryEstimationStatInnerData_V_1_0;

typedef ChannelInventoryEstimationStatKey
  ChannelInventoryEstimationStatKey_V_1_0;

typedef StatCollector<
          ChannelInventoryEstimationStatInnerKey_V_1_0,
          ChannelInventoryEstimationStatInnerData_V_1_0,
          false, // Don't exclude null values
          true
        > ChannelInventoryEstimationStatInnerCollector_V_1_0;

typedef ChannelInventoryEstimationStatInnerCollector_V_1_0
  ChannelInventoryEstimationStatData_V_1_0;

typedef StatCollector<
          ChannelInventoryEstimationStatKey_V_1_0,
          ChannelInventoryEstimationStatData_V_1_0
        > ChannelInventoryEstimationStatCollector_V_1_0;

typedef StatCollector<
          ChannelInventoryEstimationStatInnerKey,
          ChannelInventoryEstimationStatInnerData,
          false, // Don't exclude null values
          true
        > ChannelInventoryEstimationStatInnerCollector;

typedef ChannelInventoryEstimationStatInnerCollector
  ChannelInventoryEstimationStatData;

typedef StatCollector<
          ChannelInventoryEstimationStatKey,
          ChannelInventoryEstimationStatData
        > ChannelInventoryEstimationStatCollector;


struct ChannelInventoryEstimationStatTraits :
  LogDefaultTraits<ChannelInventoryEstimationStatCollector>
{
  template <typename Functor>
  static void
  for_each_old(Functor& f) /*throw(eh::Exception)*/
  {
    typedef ChannelInventoryEstimationStatCollector_V_1_0 Collector_V_1_0;
    f.template operator()<Collector_V_1_0>("1.0");
  }
};


} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_CHANNEL_INVENTORY_ESTIMATION_STAT_HPP */

