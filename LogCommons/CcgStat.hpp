#ifndef AD_SERVER_LOG_PROCESSING_CCG_STAT_HPP
#define AD_SERVER_LOG_PROCESSING_CCG_STAT_HPP


#include <iosfwd>
#include <Generics/Time.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include "LogCommons.hpp"
#include "StatCollector.hpp"

namespace AdServer {
namespace LogProcessing {

class ReachStatInnerKey
{
public:
  explicit
  ReachStatInnerKey(unsigned long id = 0)
  :
    id_(id)
  {
  }

  bool operator==(const ReachStatInnerKey& rhs) const
  {
    return id_ == rhs.id_;
  }

  unsigned long id() const noexcept
  {
    return id_;
  }

  unsigned long
  ccg_id() const noexcept
  {
    return id();
  }

  unsigned long
  cc_id() const noexcept
  {
    return id();
  }

  unsigned long
  cmp_id() const noexcept
  {
    return id();
  }

  unsigned long
  adv_account_id() const noexcept
  {
    return id();
  }

  size_t hash() const
  {
    return id_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, ReachStatInnerKey& key);

  friend
  std::ostream&
  operator<<(std::ostream& os, const ReachStatInnerKey& key);

private:
  unsigned long id_;
};

class ReachStatInnerData
{
public:
  ReachStatInnerData()
  :
    daily_reach_(),
    monthly_reach_(),
    total_reach_()
  {
  }

  ReachStatInnerData(
    unsigned long daily_reach,
    unsigned long monthly_reach,
    unsigned long total_reach
  )
  :
    daily_reach_(daily_reach),
    monthly_reach_(monthly_reach),
    total_reach_(total_reach)
  {
  }

  bool operator==(const ReachStatInnerData& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return daily_reach_ == rhs.daily_reach_ &&
      monthly_reach_ == rhs.monthly_reach_ &&
      total_reach_ == rhs.total_reach_;
  }

  ReachStatInnerData&
  operator+=(const ReachStatInnerData& rhs)
  {
    daily_reach_ += rhs.daily_reach_;
    monthly_reach_ += rhs.monthly_reach_;
    total_reach_ += rhs.total_reach_;
    return *this;
  }

  bool is_null() const
  {
    return !daily_reach_ && !monthly_reach_ && !total_reach_;
  }

  unsigned long daily_reach() const
  {
    return daily_reach_;
  }

  unsigned long monthly_reach() const
  {
    return monthly_reach_;
  }

  unsigned long total_reach() const
  {
    return total_reach_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, ReachStatInnerData& data);

private:
  unsigned long daily_reach_;
  unsigned long monthly_reach_;
  unsigned long total_reach_;
};

// Make DayTimestamp (ReachStatKey_V_1_0) compatible with new keys ReachStatKey
struct ReachStatKey_V_1_0: DayTimestamp
{
  const DayTimestamp&
  sdate() const noexcept
  {
    return *this;
  }

  unsigned long
  colo_id() const noexcept
  {
    return 0;
  }
};

struct ReachStatKey
{
  ReachStatKey(): sdate_(), colo_id_(), hash_() {}

  ReachStatKey(
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

  ReachStatKey(
    const DayTimestamp& timestamp
  )
  :
    sdate_(timestamp),
    colo_id_(),
    hash_()
  {
    calc_hash_();
  }

  bool operator==(const ReachStatKey& rhs) const
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
  operator>>(std::istream& is, ReachStatKey& key);

  friend
  std::ostream&
  operator<<(std::ostream& os, const ReachStatKey& key);

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    hash_add(hasher, colo_id_);
    sdate_.hash_add(hasher);
  }

  DayTimestamp sdate_;
  unsigned long colo_id_;
  size_t hash_;
};

typedef ReachStatInnerKey CcgStatInnerKey;

typedef ReachStatInnerData CcgStatInnerData_V_1_1;

class CcgStatInnerData_V_1_2: public ReachStatInnerData
{
public:
  CcgStatInnerData_V_1_2()
  :
    ReachStatInnerData(),
    auctions_lost_()
  {
  }

  unsigned long auctions_lost() const
  {
    return auctions_lost_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, CcgStatInnerData_V_1_2& data);

private:
  unsigned long auctions_lost_;
};

class CcgStatInnerData_V_2_3: public ReachStatInnerData
{
public:
  CcgStatInnerData_V_2_3()
  :
    ReachStatInnerData(),
    impops_(),
    auctions_lost_()
  {
  }

  CcgStatInnerData_V_2_3(
    const CcgStatInnerData_V_1_1& rhs
  )
  :
    ReachStatInnerData(rhs.daily_reach(), rhs.monthly_reach(),
      rhs.total_reach()),
    impops_(0),
    auctions_lost_()
  {
  }

  CcgStatInnerData_V_2_3(
    const CcgStatInnerData_V_1_2& rhs
  )
  :
    ReachStatInnerData(rhs.daily_reach(), rhs.monthly_reach(),
      rhs.total_reach()),
    impops_(0),
    auctions_lost_(rhs.auctions_lost())
  {
  }

  bool operator==(const CcgStatInnerData_V_2_3& rhs) const
  {
    return static_cast<const ReachStatInnerData&>(*this) ==
      static_cast<const ReachStatInnerData&>(rhs) &&
      impops_ == rhs.impops_ &&
      auctions_lost_ == rhs.auctions_lost_;
  }

  CcgStatInnerData_V_2_3&
  operator+=(const CcgStatInnerData_V_2_3& rhs)
  {
    static_cast<ReachStatInnerData&>(*this) +=
      static_cast<const ReachStatInnerData&>(rhs);
    impops_ += rhs.impops_;
    auctions_lost_ += rhs.auctions_lost_;
    return *this;
  }

  unsigned long impops() const
  {
    return impops_;
  }

  unsigned long auctions_lost() const
  {
    return auctions_lost_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, CcgStatInnerData_V_2_3& data);

private:
  unsigned long impops_;
  unsigned long auctions_lost_;
};

class CcgStatInnerData
{
public:
  CcgStatInnerData()
  :
    auctions_lost_()
  {
  }

  explicit
  CcgStatInnerData(
    unsigned long auctions_lost
  )
  :
    auctions_lost_(auctions_lost)
  {
  }

  CcgStatInnerData(
    const CcgStatInnerData_V_1_1&
  )
  :
    auctions_lost_()
  {
  }

  CcgStatInnerData(
    const CcgStatInnerData_V_1_2& rhs
  )
  :
    auctions_lost_(rhs.auctions_lost())
  {
  }

  CcgStatInnerData(
    const CcgStatInnerData_V_2_3& rhs
  )
  :
    auctions_lost_(rhs.auctions_lost())
  {
  }

  bool operator==(const CcgStatInnerData& rhs) const
  {
    return auctions_lost_ == rhs.auctions_lost_;
  }

  CcgStatInnerData&
  operator+=(const CcgStatInnerData& rhs)
  {
    auctions_lost_ += rhs.auctions_lost_;
    return *this;
  }

  unsigned long auctions_lost() const
  {
    return auctions_lost_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, CcgStatInnerData& data);

  friend
  std::ostream&
  operator<<(std::ostream& os, const CcgStatInnerData& data);

private:
  unsigned long auctions_lost_;
};

typedef CcgStatInnerKey CcgStatInnerKey_V_1_1;

typedef StatCollector<
          CcgStatInnerKey_V_1_1, CcgStatInnerData_V_1_1, false, true
        > CcgStatInnerCollector_V_1_1;

typedef ReachStatKey_V_1_0 CcgStatKey_V_1_1;
typedef CcgStatInnerCollector_V_1_1 CcgStatData_V_1_1;

typedef StatCollector<CcgStatKey_V_1_1, CcgStatData_V_1_1>
  CcgStatCollector_V_1_1;

typedef CcgStatInnerKey CcgStatInnerKey_V_1_2;

typedef StatCollector<
          CcgStatInnerKey_V_1_2, CcgStatInnerData_V_1_2, false, true
        > CcgStatInnerCollector_V_1_2;

typedef ReachStatKey CcgStatKey_V_1_2;
typedef CcgStatInnerCollector_V_1_2 CcgStatData_V_1_2;

typedef StatCollector<CcgStatKey_V_1_2, CcgStatData_V_1_2>
  CcgStatCollector_V_1_2;

typedef CcgStatInnerKey CcgStatInnerKey_V_2_3;

typedef StatCollector<
          CcgStatInnerKey_V_2_3, CcgStatInnerData_V_2_3, false, true
        > CcgStatInnerCollector_V_2_3;

typedef ReachStatKey CcgStatKey_V_2_3;
typedef CcgStatInnerCollector_V_2_3 CcgStatData_V_2_3;

typedef StatCollector<CcgStatKey_V_2_3, CcgStatData_V_2_3>
  CcgStatCollector_V_2_3;

typedef StatCollector<CcgStatInnerKey, CcgStatInnerData, false, true>
  CcgStatInnerCollector;

typedef ReachStatKey CcgStatKey;
typedef CcgStatInnerCollector CcgStatData;

typedef StatCollector<CcgStatKey, CcgStatData> CcgStatCollector;

struct CcgStatTraits: LogDefaultTraits<CcgStatCollector>
{
  template <typename Functor>
  static void
  for_each_old(Functor& f) /*throw(eh::Exception)*/
  {
    f.template operator ()<CcgStatCollector_V_1_1>("1.1");
    f.template operator ()<CcgStatCollector_V_1_2>("1.2");
    f.template operator ()<CcgStatCollector_V_2_3>("2.3");
  }
};

typedef ReachStatInnerKey CcStatInnerKey;
typedef ReachStatInnerData CcStatInnerData_V_1_1;

class CcStatInnerData_V_1_2: public ReachStatInnerData
{
public:
  CcStatInnerData_V_1_2()
  :
    ReachStatInnerData(),
    auctions_lost_()
  {
  }

  unsigned long auctions_lost() const
  {
    return auctions_lost_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, CcStatInnerData_V_1_2& data);

private:
  unsigned long auctions_lost_;
};

class CcStatInnerData_V_2_3: public ReachStatInnerData
{
public:
  CcStatInnerData_V_2_3()
  :
    ReachStatInnerData(),
    impops_(),
    auctions_lost_()
  {
  }

  CcStatInnerData_V_2_3(
    const CcStatInnerData_V_1_1& rhs
  )
  :
    ReachStatInnerData(rhs.daily_reach(), rhs.monthly_reach(),
      rhs.total_reach()),
    impops_(),
    auctions_lost_()
  {
  }

  CcStatInnerData_V_2_3(
    const CcStatInnerData_V_1_2& rhs
  )
  :
    ReachStatInnerData(rhs.daily_reach(), rhs.monthly_reach(),
      rhs.total_reach()),
    impops_(),
    auctions_lost_(rhs.auctions_lost())
  {
  }

  bool operator==(const CcStatInnerData_V_2_3& rhs) const
  {
    return static_cast<const ReachStatInnerData&>(*this) ==
      static_cast<const ReachStatInnerData&>(rhs) &&
      impops_ == rhs.impops_ &&
      auctions_lost_ == rhs.auctions_lost_;
  }

  CcStatInnerData_V_2_3&
  operator+=(const CcStatInnerData_V_2_3& rhs)
  {
    static_cast<ReachStatInnerData&>(*this) +=
      static_cast<const ReachStatInnerData&>(rhs);
    impops_ += rhs.impops_;
    auctions_lost_ += rhs.auctions_lost_;
    return *this;
  }

  unsigned long auctions_lost() const
  {
    return auctions_lost_;
  }

  unsigned long impops() const
  {
    return impops_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, CcStatInnerData_V_2_3& data);

private:
  unsigned long impops_;
  unsigned long auctions_lost_;
};

class CcStatInnerData
{
public:
  CcStatInnerData()
  :
    auctions_lost_()
  {
  }

  explicit
  CcStatInnerData(
    unsigned long auctions_lost
  )
  :
    auctions_lost_(auctions_lost)
  {
  }

  CcStatInnerData(
    const CcStatInnerData_V_1_1&
  )
  :
    auctions_lost_()
  {
  }

  CcStatInnerData(
    const CcStatInnerData_V_1_2& rhs
  )
  :
    auctions_lost_(rhs.auctions_lost())
  {
  }

  CcStatInnerData(const CcStatInnerData_V_2_3& rhs) noexcept
  :
    auctions_lost_(rhs.auctions_lost())
  {
  }

  bool operator==(const CcStatInnerData& rhs) const
  {
    return auctions_lost_ == rhs.auctions_lost_;
  }

  CcStatInnerData&
  operator+=(const CcStatInnerData& rhs)
  {
    auctions_lost_ += rhs.auctions_lost_;
    return *this;
  }

  unsigned long auctions_lost() const
  {
    return auctions_lost_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, CcStatInnerData& data);

  friend
  std::ostream&
  operator<<(std::ostream& os, const CcStatInnerData& data);

private:
  unsigned long auctions_lost_;
};

typedef ReachStatInnerKey CcStatInnerKey_V_1_0;

typedef ReachStatInnerData CcStatInnerData_V_1_0;

typedef StatCollector<CcStatInnerKey_V_1_0, CcStatInnerData_V_1_0, false, true>
  CcStatInnerCollector_V_1_0;

typedef ReachStatKey_V_1_0 CcStatKey_V_1_0;
typedef CcStatInnerCollector_V_1_0 CcStatData_V_1_0;

typedef StatCollector<CcStatKey_V_1_0, CcStatData_V_1_0> CcStatCollector_V_1_0;

typedef ReachStatInnerKey CcStatInnerKey_V_1_1;

typedef StatCollector<CcStatInnerKey_V_1_1, CcStatInnerData_V_1_1, false, true>
  CcStatInnerCollector_V_1_1;

typedef ReachStatKey CcStatKey_V_1_1;
typedef CcStatInnerCollector_V_1_1 CcStatData_V_1_1;

typedef StatCollector<CcStatKey_V_1_1, CcStatData_V_1_1>
  CcStatCollector_V_1_1;

typedef ReachStatInnerKey CcStatInnerKey_V_1_2;

typedef StatCollector<CcStatInnerKey_V_1_2, CcStatInnerData_V_1_2, false, true>
  CcStatInnerCollector_V_1_2;

typedef ReachStatKey CcStatKey_V_1_2;
typedef CcStatInnerCollector_V_1_2 CcStatData_V_1_2;

typedef StatCollector<CcStatKey_V_1_2, CcStatData_V_1_2>
  CcStatCollector_V_1_2;

typedef ReachStatInnerKey CcStatInnerKey_V_2_3;

typedef StatCollector<CcStatInnerKey_V_2_3, CcStatInnerData_V_2_3, false, true>
  CcStatInnerCollector_V_2_3;

typedef ReachStatKey CcStatKey_V_2_3;
typedef CcStatInnerCollector_V_2_3 CcStatData_V_2_3;

typedef StatCollector<CcStatKey_V_2_3, CcStatData_V_2_3>
  CcStatCollector_V_2_3;

typedef StatCollector<ReachStatInnerKey, CcStatInnerData, false, true>
  CcStatInnerCollector;

typedef ReachStatKey CcStatKey;
typedef CcStatInnerCollector CcStatData;

typedef StatCollector<CcStatKey, CcStatData> CcStatCollector;

struct CcStatTraits: LogDefaultTraits<CcStatCollector>
{
  template <typename Functor>
  static void
  for_each_old(Functor& f) /*throw(eh::Exception)*/
  {
    f.template operator ()<CcStatCollector_V_1_0>("1.0");
    f.template operator ()<CcStatCollector_V_1_1>("1.1");
    f.template operator ()<CcStatCollector_V_1_2>("1.2");
    f.template operator ()<CcStatCollector_V_2_3>("2.3");
  }
};

typedef ReachStatInnerKey CampaignStatInnerKey;
typedef ReachStatInnerData CampaignStatInnerData;

typedef StatCollector<CampaignStatInnerKey, CampaignStatInnerData, false, true>
  CampaignStatInnerCollector;

typedef ReachStatKey CampaignStatKey;
typedef CampaignStatInnerCollector CampaignStatData;

typedef StatCollector<CampaignStatKey, CampaignStatData>
  CampaignStatCollector;

typedef ReachStatInnerKey CampaignStatInnerKey_V_1_0;
typedef ReachStatInnerData CampaignStatInnerData_V_1_0;

typedef StatCollector<
          CampaignStatInnerKey_V_1_0, CampaignStatInnerData_V_1_0, false, true
        > CampaignStatInnerCollector_V_1_0;

typedef ReachStatKey_V_1_0 CampaignStatKey_V_1_0;
typedef CampaignStatInnerCollector_V_1_0 CampaignStatData_V_1_0;

typedef StatCollector<CampaignStatKey_V_1_0, CampaignStatData_V_1_0>
  CampaignStatCollector_V_1_0;

struct CampaignStatTraits: LogDefaultTraits<CampaignStatCollector>
{
  template <typename Functor>
  static void
  for_each_old(Functor& f) /*throw(eh::Exception)*/
  {
    f.template operator ()<CampaignStatCollector_V_1_0>("1.0");
  }
};

class AdvertiserUserStatInnerData_V_1_0
{
public:
  typedef ReachStatInnerData ReachRecord;

  AdvertiserUserStatInnerData_V_1_0()
  :
    reach_(),
    text_reach_(),
    display_reach_()
  {
  }

  AdvertiserUserStatInnerData_V_1_0(
    const ReachRecord& reach,
    const ReachRecord& text_reach,
    const ReachRecord& display_reach
  )
  :
    reach_(reach),
    text_reach_(text_reach),
    display_reach_(display_reach)
  {
  }

  AdvertiserUserStatInnerData_V_1_0(
    const AdvertiserUserStatInnerData_V_1_0& data
  )
  :
    reach_(data.reach()),
    text_reach_(data.text_reach()),
    display_reach_(data.display_reach())
  {
  }

  bool operator==(const AdvertiserUserStatInnerData_V_1_0& data) const
  {
    return reach_ == data.reach_ &&
      text_reach_ == data.text_reach_ &&
      display_reach_ == data.display_reach_;
  }

  AdvertiserUserStatInnerData_V_1_0&
  operator+=(const AdvertiserUserStatInnerData_V_1_0& data)
  {
    reach_ += data.reach_;
    text_reach_ += data.text_reach_;
    display_reach_ += data.display_reach_;
    return *this;
  }

  bool is_null() const
  {
    return reach_.is_null() && text_reach_.is_null() &&
      display_reach_.is_null();
  }

  const ReachRecord& reach() const
  {
    return reach_;
  }

  const ReachRecord& text_reach() const
  {
    return text_reach_;
  }

  const ReachRecord& display_reach() const
  {
    return display_reach_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is,
    AdvertiserUserStatInnerData_V_1_0& data);

private:
  ReachStatInnerData reach_;
  ReachStatInnerData text_reach_;
  ReachStatInnerData display_reach_;
};

class AdvertiserUserStatInnerKey
{
public:
  AdvertiserUserStatInnerKey()
  :
    adv_account_id_(),
    last_appearance_date_(),
    hash_()
  {
  }

  AdvertiserUserStatInnerKey(
    unsigned long adv_account_id,
    const OptionalDayTimestamp& last_appearance_date
  )
  :
    adv_account_id_(adv_account_id),
    last_appearance_date_(last_appearance_date)
  {
    calc_hash_();
  }

  bool operator==(const AdvertiserUserStatInnerKey& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return adv_account_id_ == rhs.adv_account_id_ &&
      last_appearance_date_ == rhs.last_appearance_date_;
  }

  unsigned long adv_account_id() const
  {
    return adv_account_id_;
  }

  const OptionalDayTimestamp& last_appearance_date() const
  {
    return last_appearance_date_;
  }

  size_t hash() const
  {
    return hash_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, AdvertiserUserStatInnerKey& key);

  friend
  std::ostream&
  operator<<(std::ostream& os, const AdvertiserUserStatInnerKey& key);

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    hash_add(hasher, adv_account_id_);
    if (last_appearance_date_.present())
    {
      last_appearance_date_.get().hash_add(hasher);
    }
  }

  unsigned long adv_account_id_;
  OptionalDayTimestamp last_appearance_date_;
  size_t hash_;
};

class AdvertiserUserStatInnerData
{
public:
  AdvertiserUserStatInnerData()
  :
    unique_users_(),
    text_unique_users_(),
    display_unique_users_()
  {
  }

  AdvertiserUserStatInnerData(
    long unique_users,
    long text_unique_users,
    long display_unique_users
  )
  :
    unique_users_(unique_users),
    text_unique_users_(text_unique_users),
    display_unique_users_(display_unique_users)
  {
  }

  bool operator==(const AdvertiserUserStatInnerData& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return unique_users_ == rhs.unique_users_ &&
      text_unique_users_ == rhs.text_unique_users_ &&
      display_unique_users_ == rhs.display_unique_users_;
  }

  AdvertiserUserStatInnerData&
  operator+=(const AdvertiserUserStatInnerData& rhs)
  {
    unique_users_ += rhs.unique_users_;
    text_unique_users_ += rhs.text_unique_users_;
    display_unique_users_ += rhs.display_unique_users_;
    return *this;
  }

  bool is_null() const
  {
    return !unique_users_ && !text_unique_users_ && !display_unique_users_;
  }

  long unique_users() const
  {
    return unique_users_;
  }

  long text_unique_users() const
  {
    return text_unique_users_;
  }

  long display_unique_users() const
  {
    return display_unique_users_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is,
    AdvertiserUserStatInnerData& data);

  friend
  std::ostream&
  operator<<(std::ostream& os, const AdvertiserUserStatInnerData& data);

private:
  long unique_users_;
  long text_unique_users_;
  long display_unique_users_;
};

typedef StatCollector<
          ReachStatInnerKey, AdvertiserUserStatInnerData_V_1_0, false, true
        > AdvertiserUserStatInnerCollector_V_1_0;

typedef AdvertiserUserStatInnerCollector_V_1_0 AdvertiserUserStatData_V_1_0;

typedef StatCollector<DayTimestamp, AdvertiserUserStatData_V_1_0, true>
  AdvertiserUserStatCollector_V_1_0;

typedef StatCollector<
          AdvertiserUserStatInnerKey, AdvertiserUserStatInnerData, true, true
        > AdvertiserUserStatInnerCollector;

typedef DayTimestamp AdvertiserUserStatKey;
typedef AdvertiserUserStatInnerCollector AdvertiserUserStatData;

typedef StatCollector<AdvertiserUserStatKey, AdvertiserUserStatData, true>
  AdvertiserUserStatCollector;

typedef LogDefaultTraits<AdvertiserUserStatCollector> AdvertiserUserStatTraits;

// NOTE: Old versions load policies doesn't support different StatCollectors in
// versions history
struct AdvertiserUserStatVersionsTraits:
  LogDefaultTraits<AdvertiserUserStatCollector>
{
  template <typename Functor>
  static void
  for_each_old(Functor& f) /*throw(eh::Exception)*/
  {
    f.template operator()<AdvertiserUserStatCollector_V_1_0>("1.0");
  }
};

class AdvertiserUserStat_V_1_0_To_CurrentLoader: public LogLoader
{
public:
  typedef AdvertiserUserStatTraits Traits;
  typedef Traits::CollectorType CollectorT;
  typedef Traits::CollectorBundleType CollectorBundleT;
  typedef Traits::CollectorBundlePtrType CollectorBundlePtrT;
  typedef AdvertiserUserStatCollector_V_1_0 OldCollectorT;

  explicit
  AdvertiserUserStat_V_1_0_To_CurrentLoader(
    const CollectorBundlePtrT& bundle
  )
  :
    bundle_(bundle)
  {
  }

  virtual
  void
  load(
    std::istream& is,
    const CollectorBundleFileGuard_var& file_handle =
      CollectorBundleFileGuard_var()
  )
    /*throw(Exception)*/;

private:
  virtual
  ~AdvertiserUserStat_V_1_0_To_CurrentLoader() noexcept {}

  AdvertiserUserStat_V_1_0_To_CurrentLoader(
    const AdvertiserUserStat_V_1_0_To_CurrentLoader&
  );

  AdvertiserUserStat_V_1_0_To_CurrentLoader&
  operator=(const AdvertiserUserStat_V_1_0_To_CurrentLoader&);

  CollectorBundlePtrT bundle_;
};

} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_CCG_STAT_HPP */

