#ifndef AD_SERVER_LOG_PROCESSING_SITE_STAT_HPP
#define AD_SERVER_LOG_PROCESSING_SITE_STAT_HPP


#include <iosfwd>
#include <ReferenceCounting/SmartPtr.hpp>
#include "LogCommons.hpp"
#include "StatCollector.hpp"

namespace AdServer {
namespace LogProcessing {

class SiteStatInnerKey
{
public:
  explicit
  SiteStatInnerKey(
    unsigned long site_id = 0
  )
  :
    site_id_(site_id)
  {
  }

  bool operator==(const SiteStatInnerKey& rhs) const
  {
    return site_id_ == rhs.site_id_;
  }

  unsigned long site_id() const
  {
    return site_id_;
  }

  size_t hash() const
  {
    return site_id_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, SiteStatInnerKey& key)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const SiteStatInnerKey& key)
    /*throw(eh::Exception)*/;

private:
  void invariant() const /*throw(eh::Exception)*/
  {
    if (!site_id_)
    {
      throw ConstraintViolation("SiteStatInnerKey::invariant(): "
        "site_id_ must be > 0");
    }
  }

  unsigned long site_id_;
};

class SiteStatInnerData
{
public:
  SiteStatInnerData()
  :
    daily_reach_(),
    monthly_reach_()
  {
  }

  SiteStatInnerData(
    unsigned long daily_reach,
    unsigned long monthly_reach
  )
  :
    daily_reach_(daily_reach),
    monthly_reach_(monthly_reach)
  {
  }

  bool operator==(const SiteStatInnerData& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return daily_reach_ == rhs.daily_reach_ &&
      monthly_reach_ == rhs.monthly_reach_;
  }

  SiteStatInnerData&
  operator+=(const SiteStatInnerData& rhs)
  {
    daily_reach_ += rhs.daily_reach_;
    monthly_reach_ += rhs.monthly_reach_;
    return *this;
  }

  unsigned long daily_reach() const
  {
    return daily_reach_;
  }

  unsigned long monthly_reach() const
  {
    return monthly_reach_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, SiteStatInnerData& data)
    /*throw(eh::Exception)*/;

  friend
  std::ostream& operator<<(std::ostream& os, const SiteStatInnerData& data)
    /*throw(eh::Exception)*/;

private:
  unsigned long daily_reach_;
  unsigned long monthly_reach_;
};

struct SiteStatKey
{
  SiteStatKey(): isp_sdate_(), colo_id_(), hash_() {}

  SiteStatKey(
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

  bool operator==(const SiteStatKey& rhs) const
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
  operator>>(std::istream& is, SiteStatKey& key);

  friend
  std::ostream&
  operator<<(std::ostream& os, const SiteStatKey& key)
    /*throw(eh::Exception)*/;

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    isp_sdate_.hash_add(hasher);
    hash_add(hasher, colo_id_);
  }

  void invariant() const /*throw(eh::Exception)*/
  {
    if (!colo_id_)
    {
      throw ConstraintViolation("SiteStatKey::invariant(): "
        "colo_id_ must be > 0");
    }
  }

  DayTimestamp isp_sdate_;
  unsigned long colo_id_;
  size_t hash_;
};

typedef StatCollector<SiteStatInnerKey, SiteStatInnerData, false, true>
  SiteStatInnerCollector;

typedef SiteStatInnerCollector SiteStatData;

typedef StatCollector<SiteStatKey, SiteStatData> SiteStatCollector;

typedef ReferenceCounting::SmartPtr<SiteStatCollector> SiteStatCollector_var;

typedef LogDefaultTraits<SiteStatCollector> SiteStatTraits;

} // namespace LogProcessing
} // namespace AdServer

#endif // AD_SERVER_LOG_PROCESSING_SITE_STAT_HPP

