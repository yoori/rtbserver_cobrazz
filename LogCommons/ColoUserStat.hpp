#ifndef AD_SERVER_LOG_PROCESSING_COLO_USER_STAT_HPP
#define AD_SERVER_LOG_PROCESSING_COLO_USER_STAT_HPP


#include <iosfwd>
#include <algorithm>
#include <ReferenceCounting/SmartPtr.hpp>
#include "LogCommons.hpp"
#include "StatCollector.hpp"

#include "ColoUsers.hpp"

namespace AdServer {
namespace LogProcessing {

class ColoUserStatInnerKey_V_2_5
{
public:
  ColoUserStatInnerKey_V_2_5()
  :
    create_date_(),
    last_appearance_date_(),
    hash_()
  {
  }

  ColoUserStatInnerKey_V_2_5(
    const DayTimestamp& create_date,
    const DayTimestamp& last_appearance_date
  )
  :
    create_date_(create_date),
    last_appearance_date_(last_appearance_date)
  {
    calc_hash_();
  }

  bool operator==(const ColoUserStatInnerKey_V_2_5& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return create_date_ == rhs.create_date_ &&
      last_appearance_date_ == rhs.last_appearance_date_;
  }

  const DayTimestamp& create_date() const
  {
    return create_date_;
  }

  const DayTimestamp& last_appearance_date() const
  {
    return last_appearance_date_;
  }

  size_t hash() const
  {
    return hash_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, ColoUserStatInnerKey_V_2_5& key)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const ColoUserStatInnerKey_V_2_5& key)
    /*throw(eh::Exception)*/;

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    create_date_.hash_add(hasher);
    last_appearance_date_.hash_add(hasher);
  }

  void invariant() const /*throw(eh::Exception)*/
  {
  }

  DayTimestamp create_date_;
  DayTimestamp last_appearance_date_;
  size_t hash_;
};

class ColoUserStatInnerKey
{
public:
  ColoUserStatInnerKey()
  :
    create_date_(),
    last_appearance_date_(),
    hash_()
  {
  }

  ColoUserStatInnerKey(
    const DayTimestamp& create_date,
    const OptionalDayTimestamp& last_appearance_date
  )
  :
    create_date_(create_date),
    last_appearance_date_(last_appearance_date)
  {
    calc_hash_();
  }

  ColoUserStatInnerKey(const ColoUserStatInnerKey_V_2_5& key)
  :
    create_date_(key.create_date()),
    last_appearance_date_(key.last_appearance_date())
  {
    if (last_appearance_date_.get().is_zero())
    {
      last_appearance_date_.reset();
    }
    calc_hash_();
  }

  bool operator==(const ColoUserStatInnerKey& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return create_date_ == rhs.create_date_ &&
      last_appearance_date_ == rhs.last_appearance_date_;
  }

  const DayTimestamp& create_date() const
  {
    return create_date_;
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
  operator>>(FixedBufStream<TabCategory>& is, ColoUserStatInnerKey& key)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const ColoUserStatInnerKey& key)
    /*throw(eh::Exception)*/;

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    create_date_.hash_add(hasher);
    if (last_appearance_date_.present())
    {
      last_appearance_date_.get().hash_add(hasher);
    }
  }

  void invariant() const /*throw(eh::Exception)*/
  {
  }

  DayTimestamp create_date_;
  OptionalDayTimestamp last_appearance_date_;
  size_t hash_;
};

class ColoUserStatInnerData_V_2_5
{
public:
  ColoUserStatInnerData_V_2_5()
  :
    unique_users_(),
    network_unique_users_()
  {
  }

  ColoUserStatInnerData_V_2_5(
    long unique_users,
    long network_unique_users
  )
  :
    unique_users_(unique_users),
    network_unique_users_(network_unique_users)
  {
  }

  bool operator==(const ColoUserStatInnerData_V_2_5& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return unique_users_ == rhs.unique_users_ &&
      network_unique_users_ == rhs.network_unique_users_;
  }

  ColoUserStatInnerData_V_2_5&
  operator+=(const ColoUserStatInnerData_V_2_5& rhs)
  {
    unique_users_ += rhs.unique_users_;
    network_unique_users_ += rhs.network_unique_users_;
    return *this;
  }

  bool is_null() const
  {
    return !unique_users_ && !network_unique_users_;
  }

  long unique_users() const
  {
    return unique_users_;
  }

  long network_unique_users() const
  {
    return network_unique_users_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is,
    ColoUserStatInnerData_V_2_5& data)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const ColoUserStatInnerData_V_2_5& data)
    /*throw(eh::Exception)*/;

private:
  long unique_users_;
  long network_unique_users_;
};

class ColoUserStatInnerData_V_2_6
{
public:
  ColoUserStatInnerData_V_2_6()
  :
    unique_users_(),
    network_unique_users_(),
    unique_hids_()
  {
  }

  ColoUserStatInnerData_V_2_6(
    long unique_users,
    long network_unique_users,
    long unique_hids
  )
  :
    unique_users_(unique_users),
    network_unique_users_(network_unique_users),
    unique_hids_(unique_hids)
  {
  }

  bool operator==(const ColoUserStatInnerData_V_2_6& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return unique_users_ == rhs.unique_users_ &&
      network_unique_users_ == rhs.network_unique_users_ &&
      unique_hids_ == rhs.unique_hids_;
  }

  ColoUserStatInnerData_V_2_6&
  operator+=(const ColoUserStatInnerData_V_2_6& rhs)
  {
    unique_users_ += rhs.unique_users_;
    network_unique_users_ += rhs.network_unique_users_;
    unique_hids_ += rhs.unique_hids_;
    return *this;
  }

  bool is_null() const
  {
    return !unique_users_ && !network_unique_users_ && !unique_hids_;
  }

  long unique_users() const
  {
    return unique_users_;
  }

  long network_unique_users() const
  {
    return network_unique_users_;
  }

  long unique_hids() const
  {
    return unique_hids_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is,
    ColoUserStatInnerData_V_2_6& data)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const ColoUserStatInnerData_V_2_6& data)
    /*throw(eh::Exception)*/;

private:
  long unique_users_;
  long network_unique_users_;
  long unique_hids_;
};

class ColoUserStatInnerData
{
public:
  ColoUserStatInnerData()
  :
    unique_users_(),
    network_unique_users_(),
    profiling_unique_users_(),
    unique_hids_()
  {
  }

  ColoUserStatInnerData(
    long unique_users,
    long network_unique_users,
    long profiling_unique_users,
    long unique_hids
  )
  :
    unique_users_(unique_users),
    network_unique_users_(network_unique_users),
    profiling_unique_users_(profiling_unique_users),
    unique_hids_(unique_hids)
  {
  }

  ColoUserStatInnerData(const ColoUserStatInnerData_V_2_5& data)
  :
    unique_users_(data.unique_users()),
    network_unique_users_(data.network_unique_users()),
    profiling_unique_users_(),
    unique_hids_()
  {
  }

  ColoUserStatInnerData(const ColoUserStatInnerData_V_2_6& data)
  :
    unique_users_(data.unique_users()),
    network_unique_users_(data.network_unique_users()),
    profiling_unique_users_(),
    unique_hids_(data.unique_hids())
  {
  }

  bool operator==(const ColoUserStatInnerData& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return unique_users_ == rhs.unique_users_ &&
      network_unique_users_ == rhs.network_unique_users_ &&
      profiling_unique_users_ == rhs.profiling_unique_users_ &&
      unique_hids_ == rhs.unique_hids_;
  }

  ColoUserStatInnerData&
  operator+=(const ColoUserStatInnerData& rhs)
  {
    unique_users_ += rhs.unique_users_;
    network_unique_users_ += rhs.network_unique_users_;
    profiling_unique_users_ += rhs.profiling_unique_users_;
    unique_hids_ += rhs.unique_hids_;
    return *this;
  }

  bool is_null() const
  {
    return !unique_users_ && !network_unique_users_ &&
      !profiling_unique_users_ && !unique_hids_;
  }

  long unique_users() const
  {
    return unique_users_;
  }

  long network_unique_users() const
  {
    return network_unique_users_;
  }

  long profiling_unique_users() const
  {
    return profiling_unique_users_;
  }

  long unique_hids() const
  {
    return unique_hids_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, ColoUserStatInnerData& data)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const ColoUserStatInnerData& data)
    /*throw(eh::Exception)*/;

private:
  long unique_users_;
  long network_unique_users_;
  long profiling_unique_users_;
  long unique_hids_;
};

struct ColoUserStatKey
{
  ColoUserStatKey(): sdate_(), colo_id_(), hash_() {}

  ColoUserStatKey(
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

  bool operator==(const ColoUserStatKey& rhs) const
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
  operator>>(std::istream& is, ColoUserStatKey& key);

  friend
  std::ostream&
  operator<<(std::ostream& os, const ColoUserStatKey& key)
    /*throw(eh::Exception)*/;

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    hash_add(hasher, colo_id_);
    sdate_.hash_add(hasher);
  }

  void invariant() const /*throw(eh::Exception)*/
  {
  }

  DayTimestamp sdate_;
  unsigned long colo_id_;
  size_t hash_;
};

typedef ColoUserStatKey ColoUserStatKey_V_2_5;

typedef StatCollector<
          ColoUserStatInnerKey_V_2_5, ColoUserStatInnerData_V_2_5, true, true
        > ColoUserStatInnerCollector_V_2_5;

typedef ColoUserStatInnerCollector_V_2_5 ColoUserStatData_V_2_5;

typedef StatCollector<ColoUserStatKey_V_2_5, ColoUserStatData_V_2_5, true>
  ColoUserStatCollector_V_2_5;

typedef ColoUserStatInnerKey_V_2_5 ColoUserStatInnerKey_V_2_6;

typedef ColoUserStatKey ColoUserStatKey_V_2_6;

typedef StatCollector<
          ColoUserStatInnerKey_V_2_6, ColoUserStatInnerData_V_2_6, true, true
        > ColoUserStatInnerCollector_V_2_6;

typedef ColoUserStatInnerCollector_V_2_6 ColoUserStatData_V_2_6;

typedef StatCollector<ColoUserStatKey_V_2_6, ColoUserStatData_V_2_6, true>
  ColoUserStatCollector_V_2_6;

typedef StatCollector<
          ColoUserStatInnerKey, ColoUserStatInnerData, true, true
        > ColoUserStatInnerCollector;

typedef ColoUserStatInnerCollector ColoUserStatData;

typedef StatCollector<ColoUserStatKey, ColoUserStatData, true>
  ColoUserStatCollector;

struct ColoUserStatTraits: LogDefaultTraits<ColoUserStatCollector>
{
  template <typename Functor>
  static void
  for_each_old(Functor& f) /*throw(eh::Exception)*/
  {
    f.template operator()<ColoUserStatCollector_V_2_5>("2.5");
    f.template operator()<ColoUserStatCollector_V_2_6>("2.6");
  }
};

typedef ColoUserStatInnerKey_V_2_5 GlobalColoUserStatInnerKey_V_2_5;

typedef ColoUserStatInnerData_V_2_5 GlobalColoUserStatInnerData_V_2_5;

typedef ColoUserStatKey_V_2_5 GlobalColoUserStatKey_V_2_5;

typedef StatCollector<
          GlobalColoUserStatInnerKey_V_2_5,
          GlobalColoUserStatInnerData_V_2_5,
          true,
          true
        > GlobalColoUserStatInnerCollector_V_2_5;

typedef GlobalColoUserStatInnerCollector_V_2_5 GlobalColoUserStatData_V_2_5;

typedef StatCollector<
          GlobalColoUserStatKey_V_2_5,
          GlobalColoUserStatData_V_2_5,
          true
        > GlobalColoUserStatCollector_V_2_5;

typedef ColoUserStatInnerKey_V_2_6 GlobalColoUserStatInnerKey_V_2_6;

typedef ColoUserStatInnerData_V_2_6 GlobalColoUserStatInnerData_V_2_6;

typedef ColoUserStatKey_V_2_6 GlobalColoUserStatKey_V_2_6;

typedef StatCollector<
          GlobalColoUserStatInnerKey_V_2_6,
          GlobalColoUserStatInnerData_V_2_6,
          true,
          true
        > GlobalColoUserStatInnerCollector_V_2_6;

typedef GlobalColoUserStatInnerCollector_V_2_6 GlobalColoUserStatData_V_2_6;

typedef StatCollector<
          GlobalColoUserStatKey_V_2_6,
          GlobalColoUserStatData_V_2_6,
          true
        > GlobalColoUserStatCollector_V_2_6;

typedef ColoUserStatInnerKey GlobalColoUserStatInnerKey;

typedef ColoUserStatInnerData GlobalColoUserStatInnerData;

typedef ColoUserStatKey GlobalColoUserStatKey;

typedef StatCollector<
          GlobalColoUserStatInnerKey, GlobalColoUserStatInnerData, true, true
        > GlobalColoUserStatInnerCollector;

typedef GlobalColoUserStatInnerCollector GlobalColoUserStatData;

typedef StatCollector<GlobalColoUserStatKey, GlobalColoUserStatData, true>
  GlobalColoUserStatCollector;

struct GlobalColoUserStatTraits :
  LogDefaultTraits<GlobalColoUserStatCollector, true, true, 1>
{
  template <typename Functor>
  static void
  for_each_old(Functor& f) /*throw(eh::Exception)*/
  {
    f.template operator()<GlobalColoUserStatCollector_V_2_5>("2.5");
    f.template operator()<GlobalColoUserStatCollector_V_2_6>("2.6");
  }
};

class ColoUsers_To_Colo_Or_GlobalColo_UserStatLoader: public LogLoader
{
public:
  typedef ColoUserStatTraits C_U_S_Traits;
  typedef C_U_S_Traits::CollectorType C_U_S_CollectorT;
  typedef C_U_S_Traits::CollectorBundleType C_U_S_CollectorBundleT;
  typedef C_U_S_Traits::CollectorBundlePtrType C_U_S_CollectorBundlePtrT;
  typedef GlobalColoUserStatTraits G_C_U_S_Traits;
  typedef G_C_U_S_Traits::CollectorType G_C_U_S_CollectorT;
  typedef G_C_U_S_Traits::CollectorBundleType G_C_U_S_CollectorBundleT;
  typedef G_C_U_S_Traits::CollectorBundlePtrType G_C_U_S_CollectorBundlePtrT;
  typedef ColoUsersCollector OldCollectorT;

  ColoUsers_To_Colo_Or_GlobalColo_UserStatLoader() {}

  ~ColoUsers_To_Colo_Or_GlobalColo_UserStatLoader() noexcept {}

  void set_bundle(const C_U_S_CollectorBundlePtrT& bundle)
  {
    cus_bundle_ = bundle;
  }

  void
  set_bundle(const G_C_U_S_CollectorBundlePtrT& bundle)
  {
    gcus_bundle_ = bundle;
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
  ColoUsers_To_Colo_Or_GlobalColo_UserStatLoader(
    const ColoUsers_To_Colo_Or_GlobalColo_UserStatLoader&
  );

  ColoUsers_To_Colo_Or_GlobalColo_UserStatLoader&
  operator=(const ColoUsers_To_Colo_Or_GlobalColo_UserStatLoader&);

  template <class COLLECTOR_TYPE_>
  void
  add_colo_user_stat_data_(
    const DayTimestamp& sdate,
    unsigned long colo_id,
    unsigned long created,
    const OldCollectorT::DataT& colo_users_data,
    COLLECTOR_TYPE_& collector
  );

  C_U_S_CollectorBundlePtrT cus_bundle_;
  G_C_U_S_CollectorBundlePtrT gcus_bundle_;
};

template <class COLLECTOR_TYPE_>
inline
void
ColoUsers_To_Colo_Or_GlobalColo_UserStatLoader::add_colo_user_stat_data_(
  const DayTimestamp& sdate,
  unsigned long colo_id,
  unsigned long created,
  const OldCollectorT::DataT& colo_users_data,
  COLLECTOR_TYPE_& collector
)
{
  static const time_t ONE_DAY = 24 * 60 * 60;
  static const time_t ONE_MONTH = 31 * 24 * 60 * 60;

  typename COLLECTOR_TYPE_::KeyT key(sdate, colo_id);

  typename COLLECTOR_TYPE_::DataT data;

  typedef typename COLLECTOR_TYPE_::DataT::KeyT InnerKeyT;
  typedef typename COLLECTOR_TYPE_::DataT::DataT InnerDataT;

  const DayTimestamp create_date = sdate.time() - created * ONE_DAY;

  long unique_users_count = 0;
  long network_unique_users_count = 0;

  unsigned long weekly_or_monthly_users_count =
    std::max(colo_users_data.weekly_users_count(),
      colo_users_data.monthly_users_count());

  if (colo_users_data.users_count() < weekly_or_monthly_users_count)
  {
    Stream::Error es;
    es << __PRETTY_FUNCTION__ << ": ColoUsers::users_count is less than "
      "MAX(ColoUsers::weekly_users_count, ColoUsers::monthly_users_count)";
    throw Exception(es);
  }

  if (colo_users_data.daily_network_users_count() <
    colo_users_data.monthly_network_users_count())
  {
    Stream::Error es;
    es << __PRETTY_FUNCTION__ << ": ColoUsers::daily_network_users_count "
      "is less than ColoUsers::monthly_network_users_count";
    throw Exception(es);
  }

  unique_users_count = colo_users_data.users_count() -
    weekly_or_monthly_users_count;

  network_unique_users_count = colo_users_data.daily_network_users_count() -
    colo_users_data.monthly_network_users_count();

  data.add(InnerKeyT(create_date, sdate.time() -= ONE_DAY),
    InnerDataT(unique_users_count, network_unique_users_count, 0, 0));

  if (colo_users_data.weekly_users_count() >
    colo_users_data.monthly_users_count())
  {
    unique_users_count = colo_users_data.weekly_users_count() -
      colo_users_data.monthly_users_count();

    data.add(InnerKeyT(create_date, sdate.time() -= 7 * ONE_DAY),
      InnerDataT(unique_users_count, 0, 0, 0));
  }

  unique_users_count = colo_users_data.monthly_users_count();

  network_unique_users_count = colo_users_data.monthly_network_users_count();

  data.add(InnerKeyT(create_date, sdate.time() -= ONE_MONTH),
    InnerDataT(unique_users_count, network_unique_users_count, 0, 0));

  collector.add(key, data);
}

} // namespace LogProcessing
} // namespace AdServer

#endif // AD_SERVER_LOG_PROCESSING_COLO_USER_STAT_HPP

