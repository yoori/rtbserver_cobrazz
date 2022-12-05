#ifndef AD_SERVER_LOG_PROCESSING_CAMPAIGN_USER_STAT_HPP
#define AD_SERVER_LOG_PROCESSING_CAMPAIGN_USER_STAT_HPP


#include <iosfwd>
#include <ReferenceCounting/SmartPtr.hpp>
#include <LogCommons/CollectorBundle.hpp>
#include "LogCommons.hpp"
#include "StatCollector.hpp"

#include "CcgStat.hpp"

namespace AdServer {
namespace LogProcessing {

class CampaignUserStatInnerKey
{
public:
  CampaignUserStatInnerKey()
  :
    cmp_id_(),
    last_appearance_date_(),
    hash_()
  {
  }

  CampaignUserStatInnerKey(
    unsigned long cmp_id,
    const OptionalDayTimestamp& last_appearance_date
  )
  :
    cmp_id_(cmp_id),
    last_appearance_date_(last_appearance_date)
  {
    calc_hash_();
  }

  bool operator==(const CampaignUserStatInnerKey& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return cmp_id_ == rhs.cmp_id_ &&
      last_appearance_date_ == rhs.last_appearance_date_;
  }

  unsigned long cmp_id() const
  {
    return cmp_id_;
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
  operator>>(FixedBufStream<TabCategory>& is, CampaignUserStatInnerKey& key);

  friend
  std::ostream&
  operator<<(std::ostream& os, const CampaignUserStatInnerKey& key);

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    hash_add(hasher, cmp_id_);
    if (last_appearance_date_.present())
    {
      last_appearance_date_.get().hash_add(hasher);
    }
  }

  void invariant() const
  {
    if (!cmp_id_)
    {
      throw ConstraintViolation("CampaignUserStatInnerKey::invariant(): "
        "cmp_id_ must be > 0");
    }
  }

  unsigned long cmp_id_;
  OptionalDayTimestamp last_appearance_date_;
  size_t hash_;
};

class CampaignUserStatInnerData
{
public:
  CampaignUserStatInnerData()
  :
    unique_users_()
  {
  }

  CampaignUserStatInnerData(
    long unique_users
  )
  :
    unique_users_(unique_users)
  {
  }

  bool operator==(const CampaignUserStatInnerData& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return unique_users_ == rhs.unique_users_;
  }

  CampaignUserStatInnerData&
  operator+=(const CampaignUserStatInnerData& rhs)
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
  operator>>(FixedBufStream<TabCategory>& is, CampaignUserStatInnerData& data);

  friend
  std::ostream&
  operator<<(std::ostream& os, const CampaignUserStatInnerData& data);

private:
  long unique_users_;
};

struct CampaignUserStatKey
{
  CampaignUserStatKey(): adv_sdate_(), colo_id_(), hash_() {}

  CampaignUserStatKey(
    const DayTimestamp& adv_sdate,
    unsigned long colo_id
  )
  :
    adv_sdate_(adv_sdate),
    colo_id_(colo_id),
    hash_()
  {
    calc_hash_();
  }

  bool operator==(const CampaignUserStatKey& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return adv_sdate_ == rhs.adv_sdate_ &&
      colo_id_ == rhs.colo_id_;
  }

public:
  const DayTimestamp& adv_sdate() const
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

  friend
  std::istream&
  operator>>(std::istream& is, CampaignUserStatKey& key);

  friend
  std::ostream&
  operator<<(std::ostream& os, const CampaignUserStatKey& key);

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    hash_add(hasher, colo_id_);
    adv_sdate_.hash_add(hasher);
  }

  void invariant() const
  {
  }

  DayTimestamp adv_sdate_;
  unsigned long colo_id_;
  size_t hash_;
};

typedef StatCollector<
          CampaignUserStatInnerKey, CampaignUserStatInnerData, true, true
        > CampaignUserStatInnerCollector;

typedef CampaignUserStatInnerCollector CampaignUserStatData;

typedef StatCollector<CampaignUserStatKey, CampaignUserStatData, true>
  CampaignUserStatCollector;

typedef LogDefaultTraits<CampaignUserStatCollector> CampaignUserStatTraits;

template <typename OldCollector>
class CampaignStatToCampaignUserStatLoader: public LogLoader
{
public:
  typedef CampaignUserStatTraits Traits;
  typedef Traits::CollectorType CollectorT;
  typedef Traits::CollectorBundleType CollectorBundleT;
  typedef Traits::CollectorBundlePtrType CollectorBundlePtrT;
  typedef OldCollector OldCollectorT;

  explicit
  CampaignStatToCampaignUserStatLoader(
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
    /*throw(Exception)*/
  {
    try
    {
      CollectorT collector;
      typename OldCollectorT::KeyT old_key;
      typename OldCollectorT::DataT old_data;
      is >> old_key;
      read_eol(is);
      is >> old_data;
      if (is.eof())
      {
        CollectorT::KeyT key(old_key.sdate(), old_key.colo_id());
        CollectorT::DataT data;
        for (
          typename OldCollectorT::DataT::const_iterator it(old_data.begin());
          it != old_data.end(); ++it
        )
        {
          if (it->second.daily_reach() < it->second.monthly_reach())
          {
            Stream::Error es;
            es << __PRETTY_FUNCTION__ << ": CampaignStat::daily_reach is less "
              "than CampaignStat::monthly_reach";
            throw Exception(es);
          }
          unsigned long reach_diff =
            it->second.daily_reach() - it->second.monthly_reach();
          if (reach_diff)
          {
            CollectorT::DataT::KeyT
              inner_key(it->first.cmp_id(), old_key.sdate().time() -= ONE_DAY);
            data.add(inner_key, reach_diff);
          }
          if (it->second.monthly_reach() < it->second.total_reach())
          {
            Stream::Error es;
            es << __PRETTY_FUNCTION__ << ": CampaignStat::monthly_reach is "
              "less than CampaignStat::total_reach";
            throw Exception(es);
          }
          reach_diff = it->second.monthly_reach() - it->second.total_reach();
          if (reach_diff)
          {
            CollectorT::DataT::KeyT inner_key(it->first.cmp_id(),
              old_key.sdate().time() -= ONE_MONTH);
            data.add(inner_key, reach_diff);
          }
          if (it->second.total_reach())
          {
            CollectorT::DataT::KeyT
              inner_key(it->first.cmp_id(), OptionalDayTimestamp());
            data.add(inner_key, it->second.total_reach());
          }
        }
        collector.add(key, data);
      }
      bundle_->merge(collector, file_handle);
    }
    catch (const Exception&)
    {
      throw;
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error es;
      es << __PRETTY_FUNCTION__ << ": Caught eh::Exception. "
         << ": " << ex.what();
      throw Exception(es);
    }
    catch (...)
    {
      Stream::Error es;
      es << __PRETTY_FUNCTION__ << ": Caught unknown exception.";
      throw Exception(es);
    }
    if (!is.eof())
    {
      Stream::Error es;
      es << __PRETTY_FUNCTION__ << ": Error: Malformed log file "
         << "(extra data at the of file)";
      throw Exception(es);
    }
  }

private:
  virtual
  ~CampaignStatToCampaignUserStatLoader() noexcept {}

  CollectorBundlePtrT bundle_;
};

} // namespace LogProcessing
} // namespace AdServer

#endif // AD_SERVER_LOG_PROCESSING_CAMPAIGN_USER_STAT_HPP

