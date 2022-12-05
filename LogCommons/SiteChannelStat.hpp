#ifndef AD_SERVER_LOG_PROCESSING_SITE_CHANNEL_STAT_HPP
#define AD_SERVER_LOG_PROCESSING_SITE_CHANNEL_STAT_HPP


#include <iosfwd>
#include <sstream>
#include <Generics/Time.hpp>
#include <Generics/SimpleDecimal.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include "LogCommons.hpp"
#include "StatCollector.hpp"


namespace AdServer {
namespace LogProcessing {

class SiteChannelStatInnerKey
{
public:
  SiteChannelStatInnerKey()
  :
    tag_id_(),
    channel_id_(),
    hash_()
  {
  }

  SiteChannelStatInnerKey(
    unsigned long tag_id,
    unsigned long channel_id
  )
  :
    tag_id_(tag_id),
    channel_id_(channel_id),
    hash_()
  {
    calc_hash();
  }

  bool operator==(const SiteChannelStatInnerKey &rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return tag_id_ == rhs.tag_id_ && channel_id_ == rhs.channel_id_;
  }

  unsigned long tag_id() const
  {
    return tag_id_;
  }

  unsigned long channel_id() const
  {
    return channel_id_;
  }

  size_t hash() const
  {
    return hash_;
  }

  friend std::istream& operator>>(std::istream &is,
    SiteChannelStatInnerKey &key) /*throw(eh::Exception)*/;
  friend std::ostream& operator<<(std::ostream &os,
    const SiteChannelStatInnerKey &key) /*throw(eh::Exception)*/;

private:
  void calc_hash()
  {
    Generics::Murmur64Hash hasher(hash_);
    hash_add(hasher, tag_id_);
    hash_add(hasher, channel_id_);
  }

  unsigned long tag_id_;
  unsigned long channel_id_;
  size_t hash_;
};

class SiteChannelStatInnerData
{
public:
  typedef AdServer::LogProcessing::FixedNumber FixedNum;

  SiteChannelStatInnerData()
  :
    imps_(),
    adv_revenue_(FixedNum::ZERO),
    pub_revenue_(FixedNum::ZERO)
  {
  }

  SiteChannelStatInnerData(
    unsigned long imps,
    const FixedNum &adv_revenue,
    const FixedNum &pub_revenue
  )
  :
    imps_(imps),
    adv_revenue_(adv_revenue),
    pub_revenue_(pub_revenue)
  {
  }

  bool operator==(const SiteChannelStatInnerData &rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return imps_ == rhs.imps_ &&
      adv_revenue_ == rhs.adv_revenue_ &&
      pub_revenue_ == rhs.pub_revenue_;
  }

  SiteChannelStatInnerData&
  operator+=(const SiteChannelStatInnerData &rhs)
  {
    imps_ += rhs.imps_;
    adv_revenue_ += rhs.adv_revenue_;
    pub_revenue_ += rhs.pub_revenue_;
    return *this;
  }

  unsigned long imps() const
  {
    return imps_;
  }

  const FixedNum& adv_revenue() const
  {
    return adv_revenue_;
  }

  const FixedNum& pub_revenue() const
  {
    return pub_revenue_;
  }

  friend std::istream& operator>>(std::istream &is,
    SiteChannelStatInnerData &data) /*throw(eh::Exception)*/;
  friend std::ostream& operator<<(std::ostream &os,
    const SiteChannelStatInnerData &data) /*throw(eh::Exception)*/;

private:
  void invariant() const /*throw(eh::Exception)*/
  {
    if (adv_revenue_ < FixedNum::ZERO)
    {
      throw ConstraintViolation("SiteChannelStatInnerData::invariant(): "
        "adv_revenue_ must be >= 0");
    }
    if (pub_revenue_ < FixedNum::ZERO)
    {
      throw ConstraintViolation("SiteChannelStatInnerData::invariant(): "
        "pub_revenue_ must be >= 0");
    }
  }

  unsigned long imps_;
  FixedNum adv_revenue_;
  FixedNum pub_revenue_;
};


struct SiteChannelStatKey
{
  SiteChannelStatKey(): sdate_(), colo_id_(), hash_() {}

  SiteChannelStatKey(
    const DayTimestamp &sdate,
    unsigned long colo_id
  )
  :
    sdate_(sdate),
    colo_id_(colo_id),
    hash_()
  {
    calc_hash();
  }

  bool operator==(const SiteChannelStatKey &rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return sdate_ == rhs.sdate_ && colo_id_ == rhs.colo_id_;
  }

private:
  typedef OptionalValue<unsigned long> ColoIdT;

public:
  const DayTimestamp& sdate() const
  {
    return sdate_;
  }

  const ColoIdT& colo_id() const
  {
    return colo_id_;
  }

  size_t hash() const
  {
    return hash_;
  }

  friend std::istream& operator>>(std::istream &is,
    SiteChannelStatKey &key);
  friend std::ostream& operator<<(std::ostream &os,
    const SiteChannelStatKey &key) /*throw(eh::Exception)*/;

private:
  void calc_hash()
  {
    Generics::Murmur64Hash hasher(hash_);
    sdate_.hash_add(hasher);
    colo_id_.hash_add(hasher);
  }

  DayTimestamp sdate_;
  ColoIdT colo_id_;
  size_t hash_;
};


typedef StatCollector<SiteChannelStatInnerKey, SiteChannelStatInnerData>
  SiteChannelStatInnerCollector;

typedef SiteChannelStatInnerCollector SiteChannelStatData;

typedef StatCollector<SiteChannelStatKey, SiteChannelStatData>
  SiteChannelStatCollector;

typedef LogDefaultTraits<SiteChannelStatCollector> SiteChannelStatTraits;



} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_SITE_CHANNEL_STAT_HPP */

