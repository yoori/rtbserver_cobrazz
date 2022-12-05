#ifndef AD_SERVER_LOG_PROCESSING_CMP_STAT_HPP
#define AD_SERVER_LOG_PROCESSING_CMP_STAT_HPP


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

// CMPStat v2.3 - AdServer v2.3-3.2

class CmpStatInnerKey_V_2_3
{
  typedef StringIoWrapperOptional OptionalStringT;

public:
  typedef Generics::SimpleDecimal<uint32_t, 6, 5> DeliveryThresholdT;

  CmpStatInnerKey_V_2_3()
  :
    publisher_account_id_(),
    tag_id_(),
    country_code_(),
    currency_exchange_id_(),
    delivery_threshold_(DeliveryThresholdT::ZERO),
    adv_account_id_(),
    campaign_id_(),
    ccg_id_(),
    cc_id_(),
    channel_id_(),
    channel_rate_id_(),
    fraud_(),
    walled_garden_(),
    hash_()
  {
  }

  CmpStatInnerKey_V_2_3(
    unsigned long publisher_account_id,
    unsigned long tag_id,
    const std::string& country_code,
    unsigned long currency_exchange_id,
    const DeliveryThresholdT& delivery_threshold,
    unsigned long adv_account_id,
    unsigned long campaign_id,
    unsigned long ccg_id,
    unsigned long cc_id,
    unsigned long channel_id,
    unsigned long channel_rate_id,
    bool fraud,
    bool walled_garden
  )
  :
    publisher_account_id_(publisher_account_id),
    tag_id_(tag_id),
    country_code_(country_code),
    currency_exchange_id_(currency_exchange_id),
    delivery_threshold_(delivery_threshold),
    adv_account_id_(adv_account_id),
    campaign_id_(campaign_id),
    ccg_id_(ccg_id),
    cc_id_(cc_id),
    channel_id_(channel_id),
    channel_rate_id_(channel_rate_id),
    fraud_(fraud),
    walled_garden_(walled_garden)
  {
    calc_hash_();
  }

  bool operator==(const CmpStatInnerKey_V_2_3& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return publisher_account_id_ == rhs.publisher_account_id_ &&
      tag_id_ == rhs.tag_id_ &&
      country_code_ == rhs.country_code_ &&
      currency_exchange_id_ == rhs.currency_exchange_id_ &&
      delivery_threshold_ == rhs.delivery_threshold_ &&
      adv_account_id_ == rhs.adv_account_id_ &&
      campaign_id_ == rhs.campaign_id_ &&
      ccg_id_ == rhs.ccg_id_ &&
      cc_id_ == rhs.cc_id_ &&
      channel_id_ == rhs.channel_id_ &&
      channel_rate_id_ == rhs.channel_rate_id_ &&
      fraud_ == rhs.fraud_ &&
      walled_garden_ == rhs.walled_garden_;
  }

  unsigned long publisher_account_id() const
  {
    return publisher_account_id_;
  }

  unsigned long tag_id() const
  {
    return tag_id_;
  }

  const OptionalStringT& country_code() const
  {
    return country_code_;
  }

  unsigned long currency_exchange_id() const
  {
    return currency_exchange_id_;
  }

  const DeliveryThresholdT& delivery_threshold() const
  {
    return delivery_threshold_;
  }

  unsigned long adv_account_id() const
  {
    return adv_account_id_;
  }

  unsigned long campaign_id() const
  {
    return campaign_id_;
  }

  unsigned long ccg_id() const
  {
    return ccg_id_;
  }

  unsigned long cc_id() const
  {
    return cc_id_;
  }

  unsigned long channel_id() const
  {
    return channel_id_;
  }

  unsigned long channel_rate_id() const
  {
    return channel_rate_id_;
  }

  bool fraud() const
  {
    return fraud_;
  }

  bool walled_garden() const
  {
    return walled_garden_;
  }

  size_t hash() const
  {
    return hash_;
  }

  friend FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, CmpStatInnerKey_V_2_3& key)
    /*throw(eh::Exception)*/;

  friend std::ostream&
  operator<<(std::ostream& os, const CmpStatInnerKey_V_2_3& key)
    /*throw(eh::Exception)*/;

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    hash_add(hasher, publisher_account_id_);
    hash_add(hasher, tag_id_);
    hash_add(hasher, currency_exchange_id_);
    hash_add(hasher, adv_account_id_);
    hash_add(hasher, campaign_id_);
    hash_add(hasher, ccg_id_);
    hash_add(hasher, cc_id_);
    hash_add(hasher, channel_id_);
    hash_add(hasher, channel_rate_id_);
    hash_add(hasher, fraud_);
    hash_add(hasher, walled_garden_);
    hash_add(hasher, delivery_threshold_);
    country_code_.hash_add(hasher);
  }

  bool delivery_threshold_is_valid_() const
  {
    return delivery_threshold_ >= DeliveryThresholdT::ZERO &&
      delivery_threshold_ <= max_delivery_threshold_value_;
  }

  void invariant_() const /*throw(eh::Exception)*/
  {
    if (!delivery_threshold_is_valid_())
    {
      Stream::Error es;
      es << "CmpStatInnerKey_V_2_3::invariant_(): delivery_threshold_ "
         << "has invalid value '" << delivery_threshold_ << '\'';
      throw ConstraintViolation(es);
    }
  }

  static const DeliveryThresholdT max_delivery_threshold_value_;

  unsigned long publisher_account_id_;
  unsigned long tag_id_;
  OptionalStringT country_code_;
  unsigned long currency_exchange_id_;
  DeliveryThresholdT delivery_threshold_;
  unsigned long adv_account_id_;
  unsigned long campaign_id_;
  unsigned long ccg_id_;
  unsigned long cc_id_;
  unsigned long channel_id_;
  unsigned long channel_rate_id_;
  bool fraud_;
  bool walled_garden_;
  size_t hash_;
};

class CmpStatInnerKey
{
  typedef StringIoWrapperOptional OptionalStringT;

public:
  typedef Generics::SimpleDecimal<uint32_t, 6, 5> DeliveryThresholdT;

  CmpStatInnerKey()
  :
    publisher_account_id_(),
    tag_id_(),
    size_id_(),
    country_code_(),
    currency_exchange_id_(),
    delivery_threshold_(DeliveryThresholdT::ZERO),
    adv_account_id_(),
    campaign_id_(),
    ccg_id_(),
    cc_id_(),
    channel_id_(),
    channel_rate_id_(),
    fraud_(),
    walled_garden_(),
    hash_()
  {
  }

  CmpStatInnerKey(
    unsigned long publisher_account_id,
    unsigned long tag_id,
    const OptionalUlong& size_id,
    const std::string& country_code,
    unsigned long currency_exchange_id,
    const DeliveryThresholdT& delivery_threshold,
    unsigned long adv_account_id,
    unsigned long campaign_id,
    unsigned long ccg_id,
    unsigned long cc_id,
    unsigned long channel_id,
    unsigned long channel_rate_id,
    bool fraud,
    bool walled_garden
  )
  :
    publisher_account_id_(publisher_account_id),
    tag_id_(tag_id),
    size_id_(size_id),
    country_code_(country_code),
    currency_exchange_id_(currency_exchange_id),
    delivery_threshold_(delivery_threshold),
    adv_account_id_(adv_account_id),
    campaign_id_(campaign_id),
    ccg_id_(ccg_id),
    cc_id_(cc_id),
    channel_id_(channel_id),
    channel_rate_id_(channel_rate_id),
    fraud_(fraud),
    walled_garden_(walled_garden)
  {
    calc_hash_();
  }

  CmpStatInnerKey(
    const CmpStatInnerKey_V_2_3& key
  )
  :
    publisher_account_id_(key.publisher_account_id()),
    tag_id_(key.tag_id()),
    size_id_(),
    country_code_(key.country_code()),
    currency_exchange_id_(key.currency_exchange_id()),
    delivery_threshold_(key.delivery_threshold()),
    adv_account_id_(key.adv_account_id()),
    campaign_id_(key.campaign_id()),
    ccg_id_(key.ccg_id()),
    cc_id_(key.cc_id()),
    channel_id_(key.channel_id()),
    channel_rate_id_(key.channel_rate_id()),
    fraud_(key.fraud()),
    walled_garden_(key.walled_garden())
  {
    calc_hash_();
  }

  bool operator==(const CmpStatInnerKey& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return publisher_account_id_ == rhs.publisher_account_id_ &&
      tag_id_ == rhs.tag_id_ &&
      size_id_ == rhs.size_id_ &&
      country_code_ == rhs.country_code_ &&
      currency_exchange_id_ == rhs.currency_exchange_id_ &&
      delivery_threshold_ == rhs.delivery_threshold_ &&
      adv_account_id_ == rhs.adv_account_id_ &&
      campaign_id_ == rhs.campaign_id_ &&
      ccg_id_ == rhs.ccg_id_ &&
      cc_id_ == rhs.cc_id_ &&
      channel_id_ == rhs.channel_id_ &&
      channel_rate_id_ == rhs.channel_rate_id_ &&
      fraud_ == rhs.fraud_ &&
      walled_garden_ == rhs.walled_garden_;
  }

  unsigned long publisher_account_id() const
  {
    return publisher_account_id_;
  }

  unsigned long tag_id() const
  {
    return tag_id_;
  }

  const OptionalUlong& size_id() const
  {
    return size_id_;
  }

  const OptionalStringT& country_code() const
  {
    return country_code_;
  }

  unsigned long currency_exchange_id() const
  {
    return currency_exchange_id_;
  }

  const DeliveryThresholdT& delivery_threshold() const
  {
    return delivery_threshold_;
  }

  unsigned long adv_account_id() const
  {
    return adv_account_id_;
  }

  unsigned long campaign_id() const
  {
    return campaign_id_;
  }

  unsigned long ccg_id() const
  {
    return ccg_id_;
  }

  unsigned long cc_id() const
  {
    return cc_id_;
  }

  unsigned long channel_id() const
  {
    return channel_id_;
  }

  unsigned long channel_rate_id() const
  {
    return channel_rate_id_;
  }

  bool fraud() const
  {
    return fraud_;
  }

  bool walled_garden() const
  {
    return walled_garden_;
  }

  size_t hash() const
  {
    return hash_;
  }

  friend FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, CmpStatInnerKey& key)
    /*throw(eh::Exception)*/;

  friend std::ostream&
  operator<<(std::ostream& os, const CmpStatInnerKey& key)
    /*throw(eh::Exception)*/;

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    hash_add(hasher, publisher_account_id_);
    hash_add(hasher, tag_id_);
    size_id_.hash_add(hasher);
    hash_add(hasher, currency_exchange_id_);
    hash_add(hasher, adv_account_id_);
    hash_add(hasher, campaign_id_);
    hash_add(hasher, ccg_id_);
    hash_add(hasher, cc_id_);
    hash_add(hasher, channel_id_);
    hash_add(hasher, channel_rate_id_);
    hash_add(hasher, fraud_);
    hash_add(hasher, walled_garden_);
    hash_add(hasher, delivery_threshold_);
    country_code_.hash_add(hasher);
  }

  bool delivery_threshold_is_valid_() const
  {
    return delivery_threshold_ >= DeliveryThresholdT::ZERO &&
      delivery_threshold_ <= max_delivery_threshold_value_;
  }

  void invariant_() const /*throw(eh::Exception)*/
  {
    if (!delivery_threshold_is_valid_())
    {
      Stream::Error es;
      es << "CmpStatInnerKey::invariant_(): delivery_threshold_ "
         << "has invalid value '" << delivery_threshold_ << '\'';
      throw ConstraintViolation(es);
    }
  }

  static const DeliveryThresholdT max_delivery_threshold_value_;

  unsigned long publisher_account_id_;
  unsigned long tag_id_;
  OptionalUlong size_id_;
  OptionalStringT country_code_;
  unsigned long currency_exchange_id_;
  DeliveryThresholdT delivery_threshold_;
  unsigned long adv_account_id_;
  unsigned long campaign_id_;
  unsigned long ccg_id_;
  unsigned long cc_id_;
  unsigned long channel_id_;
  unsigned long channel_rate_id_;
  bool fraud_;
  bool walled_garden_;
  size_t hash_;
};

class CmpStatInnerData
{
public:
  typedef FixedNumber FixedNum;

public:
  CmpStatInnerData()
  :
    imps_(),
    clicks_(),
    cmp_amount_(FixedNum::ZERO),
    adv_amount_cmp_(FixedNum::ZERO),
    cmp_sys_amount_(FixedNum::ZERO)
  {
  }

  CmpStatInnerData(
    long imps,
    long clicks,
    const FixedNum& cmp_amount,
    const FixedNum& adv_amount_cmp,
    const FixedNum& cmp_sys_amount
  )
  :
    imps_(imps),
    clicks_(clicks),
    cmp_amount_(cmp_amount),
    adv_amount_cmp_(adv_amount_cmp),
    cmp_sys_amount_(cmp_sys_amount)
  {
  }

  bool operator==(const CmpStatInnerData& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return imps_ == rhs.imps_ &&
      clicks_ == rhs.clicks_ &&
      cmp_amount_ == rhs.cmp_amount_ &&
      adv_amount_cmp_ == rhs.adv_amount_cmp_ &&
      cmp_sys_amount_ == rhs.cmp_sys_amount_;
  }

  CmpStatInnerData&
  operator+=(const CmpStatInnerData& rhs)
  {
    imps_ += rhs.imps_;
    clicks_ += rhs.clicks_;
    cmp_amount_ += rhs.cmp_amount_;
    adv_amount_cmp_ += rhs.adv_amount_cmp_;
    cmp_sys_amount_ += rhs.cmp_sys_amount_;
    return *this;
  }

  long imps() const
  {
    return imps_;
  }

  long clicks() const
  {
    return clicks_;
  }

  const FixedNum& cmp_amount() const
  {
    return cmp_amount_;
  }

  const FixedNum& adv_amount_cmp() const
  {
    return adv_amount_cmp_;
  }

  const FixedNum& cmp_sys_amount() const
  {
    return cmp_sys_amount_;
  }

  friend FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, CmpStatInnerData& data)
    /*throw(eh::Exception)*/;

  friend std::ostream&
  operator<<(std::ostream& os, const CmpStatInnerData& data)
    /*throw(eh::Exception)*/;

private:
  void invariant_() const /*throw(eh::Exception)*/ {}

  long imps_;
  long clicks_;
  FixedNum cmp_amount_;
  FixedNum adv_amount_cmp_;
  FixedNum cmp_sys_amount_;
};

struct CmpStatKey
{
  CmpStatKey(): sdate_(), adv_sdate_(), colo_id_(), hash_() {}

  CmpStatKey(
    const DayHourTimestamp& sdate,
    const DayHourTimestamp& adv_sdate,
    unsigned long colo_id
  )
  :
    sdate_(sdate),
    adv_sdate_(adv_sdate),
    colo_id_(colo_id),
    hash_()
  {
    calc_hash_();
  }

  bool operator==(const CmpStatKey& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return sdate_ == rhs.sdate_ &&
      adv_sdate_ == rhs.adv_sdate_ &&
      colo_id_ == rhs.colo_id_;
  }

public:
  const DayHourTimestamp& sdate() const
  {
    return sdate_;
  }

  const DayHourTimestamp& adv_sdate() const
  {
    return adv_sdate_;
  }

  unsigned long colo_id() const
  {
    return colo_id_;
  }

  size_t hash() const
  {
    return hash_;
  }

  friend std::istream&
  operator>>(std::istream& is, CmpStatKey& key);

  friend std::ostream&
  operator<<(std::ostream& os, const CmpStatKey& key) /*throw(eh::Exception)*/;

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    sdate_.hash_add(hasher);
    adv_sdate_.hash_add(hasher);
    hash_add(hasher, colo_id_);
  }

  DayHourTimestamp sdate_;
  DayHourTimestamp adv_sdate_;
  unsigned long colo_id_;
  size_t hash_;
};

typedef CmpStatInnerData CmpStatInnerData_V_2_3;

typedef StatCollector<
          CmpStatInnerKey_V_2_3, CmpStatInnerData_V_2_3, false, true
        > CmpStatInnerCollector_V_2_3;

typedef CmpStatInnerCollector_V_2_3 CmpStatData_V_2_3;

typedef CmpStatKey CmpStatKey_V_2_3;

typedef StatCollector<CmpStatKey_V_2_3, CmpStatData_V_2_3>
  CmpStatCollector_V_2_3;

typedef StatCollector<CmpStatInnerKey, CmpStatInnerData, false, true>
  CmpStatInnerCollector;

typedef CmpStatInnerCollector CmpStatData;

typedef StatCollector<CmpStatKey, CmpStatData> CmpStatCollector;

typedef ReferenceCounting::SmartPtr<CmpStatCollector> CmpStatCollector_var;

struct CmpStatTraits: LogDefaultTraits<CmpStatCollector>
{
  template <class FUNCTOR_>
  static
  void
  for_each_old(FUNCTOR_& f) /*throw(eh::Exception)*/
  {
    // V2.3 may be packed
    f.template operator()<CmpStatCollector_V_2_3, true>("2.3");
  }
};

} // namespace LogProcessing
} // namespace AdServer

#endif // AD_SERVER_LOG_PROCESSING_CMP_STAT_HPP

