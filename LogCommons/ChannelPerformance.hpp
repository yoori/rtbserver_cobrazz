#ifndef AD_SERVER_LOG_PROCESSING_CHANNEL_PERFORMANCE_HPP
#define AD_SERVER_LOG_PROCESSING_CHANNEL_PERFORMANCE_HPP


#include <iosfwd>
#include <istream>
#include <ostream>
#include <numeric>
#include <Generics/Time.hpp>
#include <Generics/SimpleDecimal.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include "LogCommons.hpp"
#include "StatCollector.hpp"
#include <LogCommons/GenericLogIoImpl.hpp>
#include <Commons/StringHolder.hpp>

namespace AdServer {
namespace LogProcessing {

class ChannelPerformanceInnerKey
{
public:
  ChannelPerformanceInnerKey()
  :
    channel_id_(),
    ccg_id_(),
    tag_size_(),
    hash_()
  {
  }

  ChannelPerformanceInnerKey(
    unsigned long channel_id,
    unsigned long ccg_id,
    const Commons::ImmutableString& tag_size
  )
  :
    channel_id_(channel_id),
    ccg_id_(ccg_id),
    tag_size_(tag_size),
    hash_()
  {
    calc_hash_();
  }

  bool operator==(const ChannelPerformanceInnerKey& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return channel_id_ == rhs.channel_id_ &&
      ccg_id_ == rhs.ccg_id_ &&
      tag_size_ == rhs.tag_size_;
  }

  unsigned long channel_id() const
  {
    return channel_id_;
  }

  unsigned long ccg_id() const
  {
    return ccg_id_;
  }

  const std::string& tag_size() const
  {
    return tag_size_.str();
  }

  size_t hash() const
  {
    return hash_;
  }

  friend FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, ChannelPerformanceInnerKey& key);

  friend std::istream&
  operator>>(std::istream& is, ChannelPerformanceInnerKey& key);

  friend std::ostream&
  operator<<(std::ostream& os, const ChannelPerformanceInnerKey& key);

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    hash_add(hasher, channel_id_);
    hash_add(hasher, ccg_id_);
    hash_add(hasher, tag_size_.str());
  }

  unsigned long channel_id_;
  unsigned long ccg_id_;
  Commons::ImmutableString tag_size_;
  size_t hash_;
};

class ChannelPerformanceInnerData
{
public:
  typedef AdServer::LogProcessing::FixedNumber FixedNum;

  ChannelPerformanceInnerData()
  :
    requests_(),
    imps_(),
    clicks_(),
    actions_(),
    revenue_(FixedNum::ZERO)
  {
  }

  ChannelPerformanceInnerData(
    long requests,
    long imps,
    long clicks,
    long actions,
    const FixedNum& revenue
  )
  :
    requests_(requests),
    imps_(imps),
    clicks_(clicks),
    actions_(actions),
    revenue_(revenue)
  {
  }

  bool operator==(const ChannelPerformanceInnerData& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return requests_ == rhs.requests_ &&
      imps_ == rhs.imps_ &&
      clicks_ == rhs.clicks_ &&
      actions_ == rhs.actions_ &&
      revenue_ == rhs.revenue_;
  }

  ChannelPerformanceInnerData&
  operator+=(const ChannelPerformanceInnerData& rhs)
  {
    requests_ += rhs.requests_;
    imps_ += rhs.imps_;
    clicks_ += rhs.clicks_;
    actions_ += rhs.actions_;
    revenue_ += rhs.revenue_;
    return *this;
  }

  long requests() const
  {
    return requests_;
  }

  long imps() const
  {
    return imps_;
  }

  long clicks() const
  {
    return clicks_;
  }

  long actions() const
  {
    return actions_;
  }

  const FixedNum& revenue() const
  {
    return revenue_;
  }

  friend FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is,
    ChannelPerformanceInnerData& data);

  friend std::istream&
  operator>>(std::istream& is, ChannelPerformanceInnerData& data);

  friend std::ostream&
  operator<<(std::ostream& os, const ChannelPerformanceInnerData& data);

public:
  //
  // Mediators
  //

  /*
   * StatRequestOne
   */
  struct StatRequestOne
  {
    long requests;

    StatRequestOne(long requests_val)
      : requests(requests_val)
    {}
  };


  explicit
  ChannelPerformanceInnerData(const StatRequestOne& rhs)
  :
    requests_(rhs.requests),
    imps_(),
    clicks_(),
    actions_(),
    revenue_(FixedNum::ZERO)
  {
  }

  ChannelPerformanceInnerData&
  operator+= (const StatRequestOne& rhs)
  {
    requests_ += rhs.requests;
    return *this;
  }

private:
  void invariant() const /*throw(eh::Exception)*/
  {
    if (revenue_ < FixedNum::ZERO)
    {
      throw ConstraintViolation("ChannelPerformanceInnerData::invariant(): "
        "revenue_ must be >= 0");
    }
  }

  long requests_;
  long imps_;
  long clicks_;
  long actions_;
  FixedNum revenue_;
};

struct ChannelPerformanceKey
{
  ChannelPerformanceKey(): sdate_(), colo_id_(), hash_() {}

  ChannelPerformanceKey(
    const DayHourTimestamp& sdate,
    unsigned long colo_id
  )
  :
    sdate_(sdate),
    colo_id_(colo_id),
    hash_()
  {
    calc_hash_();
  }

  bool operator==(const ChannelPerformanceKey& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return sdate_ == rhs.sdate_ && colo_id_ == rhs.colo_id_;
  }

  const DayHourTimestamp& sdate() const
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

  friend FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, ChannelPerformanceKey& key);

  friend std::istream&
  operator>>(std::istream& is, ChannelPerformanceKey& key);

  friend std::ostream&
  operator<<(std::ostream& os, const ChannelPerformanceKey& key)
    /*throw(eh::Exception)*/;

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    sdate_.hash_add(hasher);
    hash_add(hasher, colo_id_);
  }

  DayHourTimestamp sdate_;
  unsigned long colo_id_;
  size_t hash_;
};

typedef StatCollector<
          ChannelPerformanceInnerKey,
          ChannelPerformanceInnerData,
          false,
          true, // Use FixedBufStream
          true,
          false
        > ChannelPerformanceInnerCollector;

typedef ChannelPerformanceInnerCollector ChannelPerformanceData;

typedef StatCollector<
          ChannelPerformanceKey,
          ChannelPerformanceData,
          false,
          false, // Don't use FixedBufStream
          true,
          false
        > ChannelPerformanceCollector;

typedef LogDefaultTraits<ChannelPerformanceCollector> ChannelPerformanceTraits;

} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_CHANNEL_PERFORMANCE_HPP */

