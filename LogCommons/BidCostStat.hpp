#ifndef AD_SERVER_LOG_PROCESSING_BID_COST_STAT_HPP
#define AD_SERVER_LOG_PROCESSING_BID_COST_STAT_HPP


#include <iosfwd>
#include <ReferenceCounting/SmartPtr.hpp>
#include "LogCommons.hpp"
#include "StatCollector.hpp"
#include "CollectorBundle.hpp"

namespace AdServer {
namespace LogProcessing {

class BidCostStatInnerKey
{
public:
  typedef AdServer::LogProcessing::FixedNumber FixedNum;

  BidCostStatInnerKey()
  :
    tag_id_(),
    ext_tag_id_(),
    url_(),
    cost_(FixedNum::ZERO),
    cc_id_(),
    hash_()
  {
  }

  BidCostStatInnerKey(
    const unsigned long tag_id,
    const String::SubString& ext_tag_id,
    const String::SubString& url,
    const FixedNum& cost,
    const unsigned long cc_id
  )
  :
    tag_id_(tag_id),
    ext_tag_id_(ext_tag_id.str()),
    url_(url.str()),
    cost_(cost),
    cc_id_(cc_id)
  {
    calc_hash_();
  }

  bool operator==(const BidCostStatInnerKey& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return tag_id_ == rhs.tag_id_ &&
      ext_tag_id_.get() == rhs.ext_tag_id_.get() &&
      url_.get() == rhs.url_.get() &&
      cost_ == rhs.cost_ && cc_id_ == rhs.cc_id_;
  }

  unsigned long tag_id() const
  {
    return tag_id_;
  }

  const std::string& ext_tag_id() const
  {
    return ext_tag_id_.get();
  }

  const std::string& url() const
  {
    return url_.get();
  }

  const FixedNum& cost() const
  {
    return cost_;
  }

  unsigned long cc_id() const
  {
    return cc_id_;
  }

  size_t hash() const
  {
    return hash_;
  }

  void invariant() const /*throw(eh::Exception)*/
  {
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, BidCostStatInnerKey& key)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const BidCostStatInnerKey& key)
    /*throw(eh::Exception)*/;

  template <class ARCHIVE_>
  void serialize(ARCHIVE_& ar)
  {

    (ar
      & tag_id_
      & ext_tag_id_
      & url_
      & cost_) ^ cc_id_;
  }
  
private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    hash_add(hasher, tag_id_);
    hash_add(hasher, ext_tag_id_.get());
    hash_add(hasher, url_.get());
    hash_add(hasher, cost_);
    hash_add(hasher, cc_id_);
  }

  unsigned long tag_id_;
  EmptyHolder<Aux_::StringIoWrapper> ext_tag_id_;
  EmptyHolder<Aux_::StringIoWrapper> url_;
  FixedNum cost_;
  unsigned long cc_id_;

  size_t hash_;
};

class BidCostStatInnerData
{
public:
  BidCostStatInnerData()
    : unverified_imps_(),
      imps_(),
      clicks_()
  {
  }

  explicit
  BidCostStatInnerData(
    long unverified_imps,
    long imps,
    long clicks)
    : unverified_imps_(unverified_imps),
      imps_(imps),
      clicks_(clicks)
  {}

  bool operator==(const BidCostStatInnerData& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return unverified_imps_ == rhs.unverified_imps_ &&
      imps_ == rhs.imps_ &&
      clicks_ == rhs.clicks_;
  }

  BidCostStatInnerData&
  operator+=(const BidCostStatInnerData& rhs)
  {
    unverified_imps_ += rhs.unverified_imps_;
    imps_ += rhs.imps_;
    clicks_ += rhs.clicks_;
    return *this;
  }

  bool is_null() const
  {
    return unverified_imps_ == 0 && imps_ == 0 && clicks_ == 0;
  }

  long unverified_imps() const
  {
    return unverified_imps_;
  }

  long imps() const
  {
    return imps_;
  }

  long clicks() const
  {
    return clicks_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, BidCostStatInnerData& data)
    /*throw(eh::Exception)*/;

  friend
  std::ostream& operator<<(std::ostream& os, const BidCostStatInnerData& data)
    /*throw(eh::Exception)*/;

  template <class ARCHIVE_>
  void serialize(ARCHIVE_& ar)
  {

    (ar
      & unverified_imps_
      & imps_)
      ^ clicks_;
  }

private:
  long unverified_imps_;
  long imps_;
  long clicks_;
};

struct BidCostStatKey
{
  BidCostStatKey(): adv_sdate_(), hash_() {}

  BidCostStatKey(
    const DayTimestamp& adv_sdate
  )
  :
    adv_sdate_(adv_sdate),
    hash_()
  {
    calc_hash_();
  }

  bool operator==(const BidCostStatKey& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return adv_sdate_ == rhs.adv_sdate_;
  }

public:
  const DayTimestamp& adv_sdate() const
  {
    return adv_sdate_;
  }

  size_t hash() const
  {
    return hash_;
  }

  void invariant() const /*throw(eh::Exception)*/
  {
  }

  friend
  std::istream&
  operator>>(std::istream& is, BidCostStatKey& key);

  friend
  std::ostream&
  operator<<(std::ostream& os, const BidCostStatKey& key) /*throw(eh::Exception)*/;

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    adv_sdate_.hash_add(hasher);
  }

  DayTimestamp adv_sdate_;
  size_t hash_;
};

typedef StatCollector<BidCostStatInnerKey, BidCostStatInnerData, true, true>
  BidCostStatInnerCollector;

typedef BidCostStatInnerCollector BidCostStatData;

typedef StatCollector<BidCostStatKey, BidCostStatData, true>
  BidCostStatCollector;

typedef LogDefaultTraits<BidCostStatCollector> BidCostStatTraits;

} // namespace LogProcessing
} // namespace AdServer

#endif // AD_SERVER_LOG_PROCESSING_BID_COST_STAT_HPP

