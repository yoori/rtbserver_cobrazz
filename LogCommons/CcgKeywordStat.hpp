#ifndef AD_SERVER_LOG_PROCESSING_CCG_KEYWORD_STAT_HPP
#define AD_SERVER_LOG_PROCESSING_CCG_KEYWORD_STAT_HPP


#include <iosfwd>
#include <istream>
#include <ostream>
#include <Generics/Time.hpp>
#include <Generics/SimpleDecimal.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include "LogCommons.hpp"
#include "StatCollector.hpp"


namespace AdServer {
namespace LogProcessing {

// CcgKeywordStat v1.4 - AdServer v2.0-2.2

// CcgKeywordStat v2.3 - AdServer v2.3 (and higher)

class CcgKeywordStatInnerKey
{
public:
  CcgKeywordStatInnerKey()
  :
    ccg_keyword_id_(),
    currency_exchange_id_(),
    cc_id_(),
    hash_()
  {
  }

  CcgKeywordStatInnerKey(
    unsigned long ccg_keyword_id,
    unsigned long currency_exchange_id,
    unsigned long cc_id
  )
  :
    ccg_keyword_id_(ccg_keyword_id),
    currency_exchange_id_(currency_exchange_id),
    cc_id_(cc_id),
    hash_()
  {
    calc_hash_();
  }

  bool operator==(const CcgKeywordStatInnerKey& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return ccg_keyword_id_ == rhs.ccg_keyword_id_ &&
      currency_exchange_id_ == rhs.currency_exchange_id_ &&
      cc_id_ == rhs.cc_id_;
  }

  unsigned long ccg_keyword_id() const
  {
    return ccg_keyword_id_;
  }

  unsigned long currency_exchange_id() const
  {
    return currency_exchange_id_;
  }

  unsigned long cc_id() const
  {
    return cc_id_;
  }

  size_t hash() const
  {
    return hash_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, CcgKeywordStatInnerKey& key)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const CcgKeywordStatInnerKey& key)
    /*throw(eh::Exception)*/;

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    hash_add(hasher, ccg_keyword_id_);
    hash_add(hasher, currency_exchange_id_);
    hash_add(hasher, cc_id_);
  }

  unsigned long ccg_keyword_id_;
  unsigned long currency_exchange_id_;
  unsigned long cc_id_;
  size_t hash_;
};

class CcgKeywordStatInnerData_V_1_4
{
public:
  typedef AdServer::LogProcessing::FixedNumber FixedNum;

  CcgKeywordStatInnerData_V_1_4()
  :
    imps_(),
    clicks_(),
    revenue_(FixedNum::ZERO)
  {
  }

  CcgKeywordStatInnerData_V_1_4(
    unsigned long imps,
    unsigned long clicks,
    const FixedNum& revenue
  )
  :
    imps_(imps),
    clicks_(clicks),
    revenue_(revenue)
  {
  }

  bool operator==(const CcgKeywordStatInnerData_V_1_4& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return imps_ == rhs.imps_ &&
      clicks_ == rhs.clicks_ &&
      revenue_ == rhs.revenue_;
  }

  CcgKeywordStatInnerData_V_1_4&
  operator+=(const CcgKeywordStatInnerData_V_1_4& rhs)
  {
    imps_ += rhs.imps_;
    clicks_ += rhs.clicks_;
    revenue_ += rhs.revenue_;
    return *this;
  }

  unsigned long imps() const
  {
    return imps_;
  }

  unsigned long clicks() const
  {
    return clicks_;
  }

  const FixedNum& revenue() const
  {
    return revenue_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is,
    CcgKeywordStatInnerData_V_1_4& data)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const CcgKeywordStatInnerData_V_1_4& data)
    /*throw(eh::Exception)*/;

private:
  void invariant() const /*throw(eh::Exception)*/
  {
    if (revenue_ < FixedNum::ZERO)
    {
      throw ConstraintViolation("CcgKeywordStatInnerData_V_1_4::invariant(): "
        "revenue_ must be >= 0");
    }
  }

  unsigned long imps_;
  unsigned long clicks_;
  FixedNum revenue_;
};

class CcgKeywordStatInnerData
{
public:
  typedef AdServer::LogProcessing::FixedNumber FixedNum;

  CcgKeywordStatInnerData()
  :
    imps_(),
    clicks_(),
    adv_revenue_(FixedNum::ZERO),
    adv_comm_revenue_(FixedNum::ZERO),
    pub_advcurrency_amount_(FixedNum::ZERO)
  {
  }

  CcgKeywordStatInnerData(
    unsigned long imps,
    unsigned long clicks,
    const FixedNum& adv_revenue,
    const FixedNum& adv_comm_revenue,
    const FixedNum& pub_advcurrency_amount
  )
  :
    imps_(imps),
    clicks_(clicks),
    adv_revenue_(adv_revenue),
    adv_comm_revenue_(adv_comm_revenue),
    pub_advcurrency_amount_(pub_advcurrency_amount)
  {
  }

  CcgKeywordStatInnerData(
    const CcgKeywordStatInnerData_V_1_4& data
  )
  :
    imps_(data.imps()),
    clicks_(data.clicks()),
    adv_revenue_(data.revenue()),
    adv_comm_revenue_(FixedNum::ZERO),
    pub_advcurrency_amount_(FixedNum::ZERO)
  {
  }

  bool operator==(const CcgKeywordStatInnerData& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return imps_ == rhs.imps_ &&
      clicks_ == rhs.clicks_ &&
      adv_revenue_ == rhs.adv_revenue_ &&
      adv_comm_revenue_ == rhs.adv_comm_revenue_ &&
      pub_advcurrency_amount_ == rhs.pub_advcurrency_amount_;
  }

  CcgKeywordStatInnerData&
  operator+=(const CcgKeywordStatInnerData& rhs)
  {
    imps_ += rhs.imps_;
    clicks_ += rhs.clicks_;
    adv_revenue_ += rhs.adv_revenue_;
    adv_comm_revenue_ += rhs.adv_comm_revenue_;
    pub_advcurrency_amount_ += rhs.pub_advcurrency_amount_;
    return *this;
  }

  unsigned long imps() const
  {
    return imps_;
  }

  unsigned long clicks() const
  {
    return clicks_;
  }

  const FixedNum& adv_revenue() const
  {
    return adv_revenue_;
  }

  const FixedNum& adv_comm_revenue() const
  {
    return adv_comm_revenue_;
  }

  const FixedNum& pub_advcurrency_amount() const
  {
    return pub_advcurrency_amount_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, CcgKeywordStatInnerData& data)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const CcgKeywordStatInnerData& data)
    /*throw(eh::Exception)*/;

private:
  void invariant() const /*throw(eh::Exception)*/
  {
  }

  unsigned long imps_;
  unsigned long clicks_;
  FixedNum adv_revenue_;
  FixedNum adv_comm_revenue_;
  FixedNum pub_advcurrency_amount_;
};


struct CcgKeywordStatKey
{
  CcgKeywordStatKey(): sdate_(), colo_id_(), hash_() {}

  CcgKeywordStatKey(
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

  bool operator==(const CcgKeywordStatKey& rhs) const
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
  const DayHourTimestamp& sdate() const
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

  friend
  std::istream&
  operator>>(std::istream& is, CcgKeywordStatKey& key);

  friend
  std::ostream&
  operator<<(std::ostream& os, const CcgKeywordStatKey& key)
    /*throw(eh::Exception)*/;

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    sdate_.hash_add(hasher);
    colo_id_.hash_add(hasher);
  }

  DayHourTimestamp sdate_;
  ColoIdT colo_id_;
  size_t hash_;
};


typedef CcgKeywordStatInnerKey CcgKeywordStatInnerKey_V_1_4;

typedef StatCollector<
          CcgKeywordStatInnerKey_V_1_4,
          CcgKeywordStatInnerData_V_1_4,
          false,
          true
        > CcgKeywordStatInnerCollector_V_1_4;

typedef CcgKeywordStatKey CcgKeywordStatKey_V_1_4;
typedef CcgKeywordStatInnerCollector_V_1_4 CcgKeywordStatData_V_1_4;

typedef StatCollector<CcgKeywordStatKey_V_1_4, CcgKeywordStatData_V_1_4>
  CcgKeywordStatCollector_V_1_4;

typedef StatCollector<
          CcgKeywordStatInnerKey, CcgKeywordStatInnerData, false, true
        > CcgKeywordStatInnerCollector;

typedef CcgKeywordStatInnerCollector CcgKeywordStatData;

typedef StatCollector<CcgKeywordStatKey, CcgKeywordStatData>
  CcgKeywordStatCollector;

struct CcgKeywordStatTraits: LogDefaultTraits<CcgKeywordStatCollector>
{
  template <typename Functor>
  static void
  for_each_old(Functor& f) /*throw(eh::Exception)*/
  {
    f.template operator ()<CcgKeywordStatCollector_V_1_4>("1.4");
  }
};


} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_CCG_KEYWORD_STAT_HPP */

