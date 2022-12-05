#ifndef AD_SERVER_LOG_PROCESSING_SITE_REFERRER_STAT_HPP
#define AD_SERVER_LOG_PROCESSING_SITE_REFERRER_STAT_HPP


#include <LogCommons/LogCommons.hpp>
#include <LogCommons/StatCollector.hpp>

namespace AdServer {
namespace LogProcessing {

class SiteReferrerStatInnerData_V_1_1
{
public:
  explicit
  SiteReferrerStatInnerData_V_1_1(unsigned long requests = 0)
    /*throw(eh::Exception)*/
  :
    requests_(requests)
  {
  }

  bool operator==(const SiteReferrerStatInnerData_V_1_1& rhs) const
  {
    return requests_ == rhs.requests_;
  }

  SiteReferrerStatInnerData_V_1_1&
  operator+=(const SiteReferrerStatInnerData_V_1_1& rhs)
    /*throw(eh::Exception)*/
  {
    requests_ += rhs.requests_;
    return *this;
  }

  unsigned long requests() const
  {
    return requests_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is,
    SiteReferrerStatInnerData_V_1_1& data)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const SiteReferrerStatInnerData_V_1_1& data)
    /*throw(eh::Exception)*/;

private:
  unsigned long requests_;
};

class SiteReferrerStatInnerKey_V_3_1
{
public:
  SiteReferrerStatInnerKey_V_3_1()
  :
    user_status_(),
    tag_id_(),
    host_(),
    hash_()
  {
  }

  SiteReferrerStatInnerKey_V_3_1(
    char user_status,
    unsigned long tag_id,
    const String::SubString& host
  )
    /*throw(eh::Exception)*/
  :
    user_status_(user_status),
    tag_id_(tag_id),
    host_(host.str()),
    hash_()
  {
    calc_hash_();
  }

  bool operator==(const SiteReferrerStatInnerKey_V_3_1& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return user_status_ == rhs.user_status_ &&
      tag_id_ == rhs.tag_id_ &&
      host_.get() == rhs.host_.get();
  }

  char user_status() const
  {
    return user_status_;
  }

  unsigned long tag_id() const
  {
    return tag_id_;
  }

  const std::string& url() const
  {
    return host_.get();
  }

  size_t hash() const
  {
    return hash_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is,
    SiteReferrerStatInnerKey_V_3_1& key)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const SiteReferrerStatInnerKey_V_3_1& key)
    /*throw(eh::Exception)*/;

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    hash_add(hasher, tag_id_);
    hash_add(hasher, host_.get());
    hash_add(hasher, user_status_);
  }

  void invariant() const /*throw(eh::Exception)*/
  {
    if (!is_valid_user_status(user_status_))
    {
      Stream::Error es;
      es << "SiteReferrerStatInnerKey_V_3_1::invariant(): user_status_ "
         << "has invalid value '" << user_status_ << '\'';
      throw ConstraintViolation(es);
    }
  }

  char user_status_;
  unsigned long tag_id_;
  EmptyHolder<Aux_::StringIoWrapper> host_;
  size_t hash_;
};

class SiteReferrerStatInnerData_V_3_1
{
public:
  SiteReferrerStatInnerData_V_3_1()
  :
    requests_(),
    imps_(),
    clicks_()
  {
  }

  SiteReferrerStatInnerData_V_3_1(
    unsigned long requests,
    unsigned long imps,
    unsigned long clicks
  )
    /*throw(eh::Exception)*/
  :
    requests_(requests),
    imps_(imps),
    clicks_(clicks)
  {
  }

  bool operator==(const SiteReferrerStatInnerData_V_3_1& rhs) const
  {
    return requests_ == rhs.requests_ &&
      imps_ == rhs.imps_ &&
      clicks_ == rhs.clicks_;
  }

  SiteReferrerStatInnerData_V_3_1&
  operator+=(const SiteReferrerStatInnerData_V_3_1& rhs)
    /*throw(eh::Exception)*/
  {
    requests_ += rhs.requests_;
    imps_ += rhs.imps_;
    clicks_ += rhs.clicks_;
    return *this;
  }

  unsigned long requests() const
  {
    return requests_;
  }

  unsigned long imps() const
  {
    return imps_;
  }

  unsigned long clicks() const
  {
    return clicks_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is,
    SiteReferrerStatInnerData_V_3_1& data)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const SiteReferrerStatInnerData_V_3_1& data)
    /*throw(eh::Exception)*/;

private:
  unsigned long requests_;
  unsigned long imps_;
  unsigned long clicks_;
};

class SiteReferrerStatInnerKey
{
public:
  SiteReferrerStatInnerKey()
  :
    user_status_(),
    tag_id_(),
    ext_tag_id_(),
    host_(),
    hash_()
  {
  }

  SiteReferrerStatInnerKey(
    char user_status,
    unsigned long tag_id,
    const String::SubString& ext_tag_id,
    const String::SubString& host
  )
    /*throw(eh::Exception)*/
  :
    user_status_(user_status),
    tag_id_(tag_id),
    ext_tag_id_(ext_tag_id.str()),
    host_(host.str()),
    hash_()
  {
    calc_hash_();
  }

  SiteReferrerStatInnerKey(
    const SiteReferrerStatInnerKey_V_3_1& key
  )
    /*throw(eh::Exception)*/
  :
    user_status_(key.user_status()),
    tag_id_(key.tag_id()),
    ext_tag_id_(""),
    host_(key.url()),
    hash_()
  {
    calc_hash_();
  }

  bool operator==(const SiteReferrerStatInnerKey& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return user_status_ == rhs.user_status_ &&
      tag_id_ == rhs.tag_id_ &&
      ext_tag_id_.get() == rhs.ext_tag_id_.get() &&
      host_.get() == rhs.host_.get();
  }

  char user_status() const
  {
    return user_status_;
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
    return host_.get();
  }

  size_t hash() const
  {
    return hash_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, SiteReferrerStatInnerKey& key)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const SiteReferrerStatInnerKey& key)
    /*throw(eh::Exception)*/;

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    hash_add(hasher, tag_id_);
    hash_add(hasher, ext_tag_id_.get());
    hash_add(hasher, host_.get());
    hash_add(hasher, user_status_);
  }

  void invariant() const /*throw(eh::Exception)*/
  {
    if (!is_valid_user_status(user_status_))
    {
      Stream::Error es;
      es << "SiteReferrerStatInnerKey::invariant(): user_status_ "
        "has invalid value '" << user_status_ << '\'';
      throw ConstraintViolation(es);
    }
  }

  char user_status_;
  unsigned long tag_id_;
  EmptyHolder<Aux_::StringIoWrapper> ext_tag_id_;
  EmptyHolder<Aux_::StringIoWrapper> host_;
  size_t hash_;
};

class SiteReferrerStatInnerData_V_3_2
{
public:
  SiteReferrerStatInnerData_V_3_2()
  :
    requests_(),
    imps_(),
    clicks_(),
    passbacks_()
  {
  }

  SiteReferrerStatInnerData_V_3_2(
    unsigned long requests,
    unsigned long imps,
    unsigned long clicks,
    unsigned long passbacks
  )
    /*throw(eh::Exception)*/
  :
    requests_(requests),
    imps_(imps),
    clicks_(clicks),
    passbacks_(passbacks)
  {
  }

  bool operator==(const SiteReferrerStatInnerData_V_3_2& rhs) const
  {
    return requests_ == rhs.requests_ &&
      imps_ == rhs.imps_ &&
      clicks_ == rhs.clicks_ &&
      passbacks_ == rhs.passbacks_;
  }

  SiteReferrerStatInnerData_V_3_2&
  operator+=(const SiteReferrerStatInnerData_V_3_2& rhs)
    /*throw(eh::Exception)*/
  {
    requests_ += rhs.requests_;
    imps_ += rhs.imps_;
    clicks_ += rhs.clicks_;
    passbacks_ += rhs.passbacks_;
    return *this;
  }

  unsigned long requests() const
  {
    return requests_;
  }

  unsigned long imps() const
  {
    return imps_;
  }

  unsigned long clicks() const
  {
    return clicks_;
  }

  unsigned long passbacks() const
  {
    return passbacks_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is,
    SiteReferrerStatInnerData_V_3_2& data)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const SiteReferrerStatInnerData_V_3_2& data)
    /*throw(eh::Exception)*/;

private:
  unsigned long requests_;
  unsigned long imps_;
  unsigned long clicks_;
  unsigned long passbacks_;
};

class SiteReferrerStatInnerData
{
public:
  SiteReferrerStatInnerData()
  :
    requests_(),
    imps_(),
    clicks_(),
    passbacks_(),
    bids_won_count_(),
    bids_lost_count_(),
    no_bid_count_(),
    floor_won_cost_(FixedNumber::ZERO),
    floor_lost_cost_(FixedNumber::ZERO),
    floor_no_bid_cost_(FixedNumber::ZERO),
    bid_won_amount_(FixedNumber::ZERO),
    bid_lost_amount_(FixedNumber::ZERO),
    cost_(FixedNumber::ZERO)
  {
  }

  SiteReferrerStatInnerData(
    unsigned long requests,
    unsigned long imps,
    unsigned long clicks,
    unsigned long passbacks,
    unsigned long bids_won_count,
    long bids_lost_count,
    unsigned long no_bid_count,
    const FixedNumber& floor_won_cost,
    const FixedNumber& floor_lost_cost,
    const FixedNumber& floor_no_bid_cost,
    const FixedNumber& bid_won_amount,
    const FixedNumber& bid_lost_amount,
    const FixedNumber& cost
  )
    /*throw(eh::Exception)*/
  :
    requests_(requests),
    imps_(imps),
    clicks_(clicks),
    passbacks_(passbacks),
    bids_won_count_(bids_won_count),
    bids_lost_count_(bids_lost_count),
    no_bid_count_(no_bid_count),
    floor_won_cost_(floor_won_cost),
    floor_lost_cost_(floor_lost_cost),
    floor_no_bid_cost_(floor_no_bid_cost),
    bid_won_amount_(bid_won_amount),
    bid_lost_amount_(bid_lost_amount),
    cost_(cost)
  {
  }

  SiteReferrerStatInnerData(
    const SiteReferrerStatInnerData_V_1_1& data
  )
    /*throw(eh::Exception)*/
  :
    requests_(data.requests()),
    imps_(),
    clicks_(),
    passbacks_(),
    bids_won_count_(),
    bids_lost_count_(),
    no_bid_count_(),
    floor_won_cost_(FixedNumber::ZERO),
    floor_lost_cost_(FixedNumber::ZERO),
    floor_no_bid_cost_(FixedNumber::ZERO),
    bid_won_amount_(FixedNumber::ZERO),
    bid_lost_amount_(FixedNumber::ZERO),
    cost_(FixedNumber::ZERO)
  {
  }

  SiteReferrerStatInnerData(
    const SiteReferrerStatInnerData_V_3_1& data
  )
    /*throw(eh::Exception)*/
  :
    requests_(data.requests()),
    imps_(data.imps()),
    clicks_(data.clicks()),
    passbacks_(),
    bids_won_count_(),
    bids_lost_count_(),
    no_bid_count_(),
    floor_won_cost_(FixedNumber::ZERO),
    floor_lost_cost_(FixedNumber::ZERO),
    floor_no_bid_cost_(FixedNumber::ZERO),
    bid_won_amount_(FixedNumber::ZERO),
    bid_lost_amount_(FixedNumber::ZERO),
    cost_(FixedNumber::ZERO)
  {
  }

  SiteReferrerStatInnerData(
    const SiteReferrerStatInnerData_V_3_2& data
  )
    /*throw(eh::Exception)*/
  :
    requests_(data.requests()),
    imps_(data.imps()),
    clicks_(data.clicks()),
    passbacks_(data.passbacks()),
    bids_won_count_(),
    bids_lost_count_(),
    no_bid_count_(),
    floor_won_cost_(FixedNumber::ZERO),
    floor_lost_cost_(FixedNumber::ZERO),
    floor_no_bid_cost_(FixedNumber::ZERO),
    bid_won_amount_(FixedNumber::ZERO),
    bid_lost_amount_(FixedNumber::ZERO),
    cost_(FixedNumber::ZERO)
  {
  }

  bool operator==(const SiteReferrerStatInnerData& rhs) const
  {
    return requests_ == rhs.requests_ &&
      imps_ == rhs.imps_ &&
      clicks_ == rhs.clicks_ &&
      passbacks_ == rhs.passbacks_ &&
      bids_won_count_ == rhs.bids_won_count_ &&
      bids_lost_count_ == rhs.bids_lost_count_ &&
      no_bid_count_ == rhs.no_bid_count_ &&
      floor_won_cost_ == rhs.floor_won_cost_ &&
      floor_lost_cost_ == rhs.floor_lost_cost_ &&
      floor_no_bid_cost_ == rhs.floor_no_bid_cost_ &&
      bid_won_amount_ == rhs.bid_won_amount_ &&
      bid_lost_amount_ == rhs.bid_lost_amount_ &&
      cost_ == rhs.cost_;
  }

  SiteReferrerStatInnerData&
  operator+=(const SiteReferrerStatInnerData& rhs)
    /*throw(eh::Exception)*/
  {
    requests_ += rhs.requests_;
    imps_ += rhs.imps_;
    clicks_ += rhs.clicks_;
    passbacks_ += rhs.passbacks_;
    bids_won_count_ += rhs.bids_won_count_;
    bids_lost_count_ += rhs.bids_lost_count_;
    no_bid_count_ += rhs.no_bid_count_;
    floor_won_cost_ += rhs.floor_won_cost_;
    floor_lost_cost_ += rhs.floor_lost_cost_;
    floor_no_bid_cost_ += rhs.floor_no_bid_cost_;
    bid_won_amount_ += rhs.bid_won_amount_;
    bid_lost_amount_ += rhs.bid_lost_amount_;
    cost_ += rhs.cost_;
    return *this;
  }

  unsigned long requests() const
  {
    return requests_;
  }

  unsigned long imps() const
  {
    return imps_;
  }

  unsigned long clicks() const
  {
    return clicks_;
  }

  unsigned long passbacks() const
  {
    return passbacks_;
  }

  unsigned long bids_won_count() const
  {
    return bids_won_count_;
  }

  long bids_lost_count() const
  {
    return bids_lost_count_;
  }

  unsigned long no_bid_count() const
  {
    return no_bid_count_;
  }

  const FixedNumber& floor_won_cost() const
  {
    return floor_won_cost_;
  }

  const FixedNumber& floor_lost_cost() const
  {
    return floor_lost_cost_;
  }

  const FixedNumber& floor_no_bid_cost() const
  {
    return floor_no_bid_cost_;
  }

  const FixedNumber& bid_won_amount() const
  {
    return bid_won_amount_;
  }

  const FixedNumber& bid_lost_amount() const
  {
    return bid_lost_amount_;
  }

  const FixedNumber& cost() const
  {
    return cost_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, SiteReferrerStatInnerData& data)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const SiteReferrerStatInnerData& data)
    /*throw(eh::Exception)*/;

private:
  unsigned long requests_;
  unsigned long imps_;
  unsigned long clicks_;
  unsigned long passbacks_;
  unsigned long bids_won_count_;
  long bids_lost_count_;
  unsigned long no_bid_count_;
  FixedNumber floor_won_cost_;
  FixedNumber floor_lost_cost_;
  FixedNumber floor_no_bid_cost_;
  FixedNumber bid_won_amount_;
  FixedNumber bid_lost_amount_;
  FixedNumber cost_;
};

struct SiteReferrerStatKey
{
  SiteReferrerStatKey(): sdate_(), colo_id_(), hash_() {}

  SiteReferrerStatKey(
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

  bool operator==(const SiteReferrerStatKey& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return sdate_ == rhs.sdate_ && colo_id_ == rhs.colo_id_;
  }

public:
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
  operator>>(std::istream& is, SiteReferrerStatKey& key)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const SiteReferrerStatKey& key)
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

typedef SiteReferrerStatInnerKey_V_3_1 SiteReferrerStatInnerKey_V_1_1;
typedef SiteReferrerStatKey SiteReferrerStatKey_V_1_1;

typedef StatCollector<
          SiteReferrerStatKey_V_1_1,
          StatCollector<
            SiteReferrerStatInnerKey_V_1_1,
            SiteReferrerStatInnerData_V_1_1,
            false,
            true
          >
        > SiteReferrerStatCollector_V_1_1;

typedef SiteReferrerStatKey SiteReferrerStatKey_V_3_1;

typedef StatCollector<
          SiteReferrerStatKey_V_3_1,
          StatCollector<
            SiteReferrerStatInnerKey_V_3_1,
            SiteReferrerStatInnerData_V_3_1,
            false,
            true
          >
        > SiteReferrerStatCollector_V_3_1;

typedef SiteReferrerStatKey SiteReferrerStatKey_V_3_2;
typedef SiteReferrerStatInnerKey SiteReferrerStatInnerKey_V_3_2;

typedef StatCollector<
          SiteReferrerStatKey_V_3_2,
          StatCollector<
            SiteReferrerStatInnerKey_V_3_2,
            SiteReferrerStatInnerData_V_3_2,
            false,
            true
          >
        > SiteReferrerStatCollector_V_3_2;

typedef StatCollector<
          SiteReferrerStatKey,
          StatCollector<
            SiteReferrerStatInnerKey,
            SiteReferrerStatInnerData,
            false,
            true
          >
        > SiteReferrerStatCollector;

struct SiteReferrerStatTraits: LogDefaultTraits<SiteReferrerStatCollector>
{
  template <typename Functor>
  static
  void
  for_each_old(Functor& f) /*throw(eh::Exception)*/
  {
    f.template operator()<SiteReferrerStatCollector_V_1_1>("1.1");
    f.template operator()<SiteReferrerStatCollector_V_3_1>("3.1");
    // V3.2 is packed
    f.template operator()<SiteReferrerStatCollector_V_3_2, true>("3.2");
  }
};

} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_SITE_REFERRER_STAT_HPP */

