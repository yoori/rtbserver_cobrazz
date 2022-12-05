#ifndef AD_SERVER_LOG_PROCESSING_SITE_USER_STAT_HPP
#define AD_SERVER_LOG_PROCESSING_SITE_USER_STAT_HPP


#include <iosfwd>
#include <ReferenceCounting/SmartPtr.hpp>
#include "LogCommons.hpp"
#include "StatCollector.hpp"

#include "SiteStat.hpp"

namespace AdServer {
namespace LogProcessing {

class SiteUserStatInnerKey
{
public:
  SiteUserStatInnerKey()
  :
    site_id_(),
    last_appearance_date_(),
    hash_()
  {
  }

  SiteUserStatInnerKey(
    unsigned long site_id,
    const DayTimestamp& last_appearance_date
  )
  :
    site_id_(site_id),
    last_appearance_date_(last_appearance_date)
  {
    calc_hash_();
  }

  bool operator==(const SiteUserStatInnerKey& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return site_id_ == rhs.site_id_ &&
      last_appearance_date_ == rhs.last_appearance_date_;
  }

  unsigned long site_id() const
  {
    return site_id_;
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
  operator>>(FixedBufStream<TabCategory>& is, SiteUserStatInnerKey& key);

  friend
  std::ostream&
  operator<<(std::ostream& os, const SiteUserStatInnerKey& key);

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    hash_add(hasher, site_id_);
    last_appearance_date_.hash_add(hasher);
  }

  void invariant() const
  {
    if (!site_id_)
    {
      throw ConstraintViolation("SiteUserStatInnerKey::invariant(): "
        "site_id_ must be > 0");
    }
  }

  unsigned long site_id_;
  // FIXME: Change type of last_appearance_date_ to OptionalDayTimestamp
  DayTimestamp last_appearance_date_;
  size_t hash_;
};

class SiteUserStatInnerData
{
public:
  SiteUserStatInnerData()
  :
    unique_users_()
  {
  }

  explicit
  SiteUserStatInnerData(
    long unique_users
  )
  :
    unique_users_(unique_users)
  {
  }

  bool operator==(const SiteUserStatInnerData& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return unique_users_ == rhs.unique_users_;
  }

  SiteUserStatInnerData&
  operator+=(const SiteUserStatInnerData& rhs)
  {
    unique_users_ += rhs.unique_users_;
    return *this;
  }

  bool is_null() const
  {
    return !unique_users_;
  }

  long unique_users() const
  {
    return unique_users_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, SiteUserStatInnerData& data);

  friend
  std::ostream&
  operator<<(std::ostream& os, const SiteUserStatInnerData& data);

private:
  long unique_users_;
};

struct SiteUserStatKey
{
  SiteUserStatKey(): isp_sdate_(), colo_id_(), hash_() {}

  SiteUserStatKey(
    const DayTimestamp& isp_sdate,
    unsigned long colo_id
  )
  :
    isp_sdate_(isp_sdate),
    colo_id_(colo_id),
    hash_()
  {
    calc_hash_();
  }

  bool operator==(const SiteUserStatKey& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return isp_sdate_ == rhs.isp_sdate_ &&
      colo_id_ == rhs.colo_id_;
  }

public:
  const DayTimestamp& isp_sdate() const
  {
    return isp_sdate_;
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
  operator>>(std::istream& is, SiteUserStatKey& key);

  friend
  std::ostream&
  operator<<(std::ostream& os, const SiteUserStatKey& key);

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    isp_sdate_.hash_add(hasher);
    hash_add(hasher, colo_id_);
  }

  void invariant() const
  {
    if (!colo_id_)
    {
      throw ConstraintViolation("SiteUserStatKey::invariant(): "
        "colo_id_ must be > 0");
    }
  }

  DayTimestamp isp_sdate_;
  unsigned long colo_id_;
  size_t hash_;
};

typedef StatCollector<SiteUserStatInnerKey, SiteUserStatInnerData, true, true>
  SiteUserStatInnerCollector;

typedef SiteUserStatInnerCollector SiteUserStatData;

typedef StatCollector<SiteUserStatKey, SiteUserStatData, true>
  SiteUserStatCollector;

typedef ReferenceCounting::SmartPtr<SiteUserStatCollector>
  SiteUserStatCollector_var;

typedef LogDefaultTraits<SiteUserStatCollector> SiteUserStatTraits;

class SiteStatToSiteUserStatLoader: public LogLoader
{
public:
  typedef SiteUserStatTraits Traits;
  typedef Traits::CollectorType CollectorT;
  typedef Traits::CollectorBundleType CollectorBundleT;
  typedef Traits::CollectorBundlePtrType CollectorBundlePtrT;
  typedef SiteStatCollector OldCollectorT;

  explicit
  SiteStatToSiteUserStatLoader(
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
  ~SiteStatToSiteUserStatLoader() noexcept {}

  SiteStatToSiteUserStatLoader(const SiteStatToSiteUserStatLoader&);

  SiteStatToSiteUserStatLoader&
  operator=(const SiteStatToSiteUserStatLoader&);

  CollectorBundlePtrT bundle_;
};

} // namespace LogProcessing
} // namespace AdServer

#endif // AD_SERVER_LOG_PROCESSING_SITE_USER_STAT_HPP

