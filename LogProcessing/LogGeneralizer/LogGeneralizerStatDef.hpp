#ifndef AD_SERVER_LOG_PROCESSING_LOG_GENERALIZER_STAT_DEF_HPP
#define AD_SERVER_LOG_PROCESSING_LOG_GENERALIZER_STAT_DEF_HPP


#include <tr1/unordered_map>
#include <Generics/Time.hpp>
#include <Generics/GnuHashTable.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <Commons/CorbaTypes.hpp>
#include <LogCommons/LogCommons.hpp>

namespace AdServer {
namespace LogProcessing {

struct FixedNumberWrapper: public FixedNumber
{
  FixedNumberWrapper(): FixedNumber(FixedNumber::ZERO) {}
  FixedNumberWrapper(const FixedNumber &val): FixedNumber(val) {}
  FixedNumberWrapper(const String::SubString &val): FixedNumber(val) {}
};

class CampaignStatKeyDef
{
public:
  CampaignStatKeyDef()
  :
    sdate_(),
    adv_sdate_(),
    adv_account_id_(),
    campaign_id_(),
    ccg_id_(),
    hash_()
  {
  }

  CampaignStatKeyDef(
    const Generics::Time &sdate,
    const Generics::Time &adv_sdate,
    unsigned long adv_account_id,
    unsigned long campaign_id,
    unsigned long ccg_id
  )
  :
    sdate_(sdate),
    adv_sdate_(adv_sdate),
    adv_account_id_(adv_account_id),
    campaign_id_(campaign_id),
    ccg_id_(ccg_id),
    hash_()
  {
    calc_hash();
  }

  bool operator==(const CampaignStatKeyDef &rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return sdate_ == rhs.sdate_ &&
      adv_sdate_ == rhs.adv_sdate_ &&
      adv_account_id_ == rhs.adv_account_id_ &&
      campaign_id_ == rhs.campaign_id_ &&
      ccg_id_ == rhs.ccg_id_;
  }

  const Generics::Time& sdate() const
  {
    return sdate_;
  }

  const Generics::Time& adv_sdate() const
  {
    return adv_sdate_;
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

  size_t hash() const
  {
    return hash_;
  }

private:
  void calc_hash()
  {
    unsigned tmp = sdate_.tv_sec;
    hash_ = Generics::CRC::quick(0, &tmp, sizeof(tmp));
    tmp = adv_sdate_.tv_sec;
    hash_ = Generics::CRC::quick(hash_, &tmp, sizeof(tmp));
    hash_ = Generics::CRC::quick(hash_, &adv_account_id_,
      sizeof(adv_account_id_));
    hash_ = Generics::CRC::quick(hash_, &campaign_id_, sizeof(campaign_id_));
    hash_ = Generics::CRC::quick(hash_, &ccg_id_, sizeof(ccg_id_));
  }

  Generics::Time sdate_;
  Generics::Time adv_sdate_;
  unsigned long adv_account_id_;
  unsigned long campaign_id_;
  unsigned long ccg_id_;
  size_t hash_;
};

class CreativeStatValueDef
{
public:
  CreativeStatValueDef()
  :
    requests_(),
    imps_(),
    clicks_(),
    actions_()
  {
  }

  CreativeStatValueDef(
    long requests,
    long imps,
    long clicks,
    long actions
  )
  :
    requests_(requests),
    imps_(imps),
    clicks_(clicks),
    actions_(actions)
  {
  }

  CreativeStatValueDef& operator+=(const CreativeStatValueDef &rhs)
  {
    requests_ += rhs.requests_;
    imps_ += rhs.imps_;
    clicks_ += rhs.clicks_;
    actions_ += rhs.actions_;
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

private:
  long requests_; // same as unverified_imps in CreativeStat
  long imps_;
  long clicks_;
  long actions_;
};

class CampaignTagStatValueDef
{
public:
  typedef FixedNumberWrapper DecimalT;

  CampaignTagStatValueDef()
  :
    pub_isp_amount_(),
    adv_amount_(),
    adv_comm_amount_()
  {
  }

  CampaignTagStatValueDef(
    DecimalT pub_isp_amount,
    DecimalT adv_amount,
    DecimalT adv_comm_amount
  )
  :
    pub_isp_amount_(pub_isp_amount),
    adv_amount_(adv_amount),
    adv_comm_amount_(adv_comm_amount)
  {
  }

  CampaignTagStatValueDef& operator+=(const CampaignTagStatValueDef &rhs)
  {
    pub_isp_amount_ += rhs.pub_isp_amount_;
    adv_amount_ += rhs.adv_amount_;
    adv_comm_amount_ += rhs.adv_comm_amount_;
    return *this;
  }

  const DecimalT& pub_isp_amount() const
  {
    return pub_isp_amount_;
  }

  const DecimalT& adv_amount() const
  {
    return adv_amount_;
  }

  const DecimalT& adv_comm_amount() const
  {
    return adv_comm_amount_;
  }

private:
  DecimalT pub_isp_amount_;
  DecimalT adv_amount_;
  DecimalT adv_comm_amount_;
};

class CtrResetStatValueDef
{
public:
  CtrResetStatValueDef(unsigned long imps = 0)
    : imps_(imps)
  {}

  CtrResetStatValueDef&
  operator+=(const CtrResetStatValueDef& rhs)
  {
    imps_ += rhs.imps_;
    return *this;
  }

  unsigned long
  imps() const noexcept
  {
    return imps_;
  }

private:
  unsigned long imps_;
};

struct CampaignStatValueDef
{
  typedef FixedNumberWrapper DecimalT;

  // cc_id -> CreativeStatValueDef
  typedef std::tr1::unordered_map<unsigned long, CreativeStatValueDef>
    CreativeStatMap;

  // publisher_account_id -> DecimalT
  typedef std::tr1::unordered_map<unsigned long, DecimalT> PublisherAmountMap;

  // tag_id -> CampaignTagStatValueDef
  typedef std::tr1::unordered_map<unsigned long, CampaignTagStatValueDef>
    TagAmountMap;

  // ctr_reset_id -> imps
  typedef std::tr1::unordered_map<unsigned long, CtrResetStatValueDef>
    CtrResetStatMap;

  CampaignStatValueDef& operator+=(const CampaignStatValueDef &val)
  {
    adv_account_amount += val.adv_account_amount;
    adv_amount += val.adv_amount;
    adv_comm_amount += val.adv_comm_amount;
    adv_payable_comm_amount += val.adv_payable_comm_amount;
    for (CreativeStatMap::const_iterator it = val.creative_stats.begin();
      it != val.creative_stats.end(); ++it)
    {
      creative_stats[it->first] += it->second;
    }
    for (PublisherAmountMap::const_iterator it = val.publisher_amounts.begin();
      it != val.publisher_amounts.end(); ++it)
    {
      publisher_amounts[it->first] += it->second;
    }
    for (TagAmountMap::const_iterator it = val.tag_amounts.begin();
      it != val.tag_amounts.end(); ++it)
    {
      tag_amounts[it->first] += it->second;
    }
    for(CtrResetStatMap::const_iterator it = val.ctr_reset_stats.begin();
        it != val.ctr_reset_stats.end(); ++it)
    {
      ctr_reset_stats[it->first] += it->second;
    }
    return *this;
  }

  DecimalT adv_account_amount;
  DecimalT adv_amount;
  DecimalT adv_comm_amount;
  DecimalT adv_payable_comm_amount;
  CreativeStatMap creative_stats;
  PublisherAmountMap publisher_amounts;
  TagAmountMap tag_amounts;
  CtrResetStatMap ctr_reset_stats;
};

typedef Generics::GnuHashTable<CampaignStatKeyDef, CampaignStatValueDef>
  CampaignStatMap;

struct LogGeneralizerStatValue: public ReferenceCounting::AtomicImpl
{
  typedef Sync::PosixMutex LockT;
  typedef Sync::PosixGuard GuardT;

  LogGeneralizerStatValue() noexcept : upload_stopped(true) {}

  CampaignStatMap map;
  Generics::Time start_clear_timestamp;
  bool upload_stopped;
  mutable LockT lock;

private:
  virtual ~LogGeneralizerStatValue() noexcept {}
};

typedef ReferenceCounting::SmartPtr<LogGeneralizerStatValue>
  LogGeneralizerStatValue_var;

typedef std::tr1::unordered_map<CORBA::ULong, LogGeneralizerStatValue_var>
  LogGeneralizerStatMap;

struct LogGeneralizerStatMapBundle: public ReferenceCounting::AtomicImpl
{
  typedef Sync::PosixRWLock RWLockT;
  typedef Sync::PosixWGuard ReadGuardT;
  typedef Sync::PosixRGuard WriteGuardT;

  LogGeneralizerStatMap map;
  mutable RWLockT lock;

private:
  virtual ~LogGeneralizerStatMapBundle() noexcept {}
};

typedef ReferenceCounting::SmartPtr<LogGeneralizerStatMapBundle>
  LogGeneralizerStatMapBundle_var;

} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_LOG_GENERALIZER_STAT_DEF_HPP */

