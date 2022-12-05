#ifndef AD_SERVER_LOG_PROCESSING_PAGE_LOADS_DAILY_STAT_HPP
#define AD_SERVER_LOG_PROCESSING_PAGE_LOADS_DAILY_STAT_HPP


#include <iosfwd>
#include <ReferenceCounting/SmartPtr.hpp>
#include "LogCommons.hpp"
#include "StatCollector.hpp"

#include <LogCommons/GenericLogIoImpl.hpp>

namespace AdServer {
namespace LogProcessing {

struct PageLoadsDailyStatKey_V_1_0
{
  PageLoadsDailyStatKey_V_1_0(): sdate_(), colo_id_(), hash_() {}

  PageLoadsDailyStatKey_V_1_0(
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

  bool operator==(const PageLoadsDailyStatKey_V_1_0& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return sdate_ == rhs.sdate_ &&
      colo_id_ == rhs.colo_id_;
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
  operator>>(std::istream& is, PageLoadsDailyStatKey_V_1_0& key);

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    sdate_.hash_add(hasher);
    hash_add(hasher, colo_id_);
  }

  void invariant() const /*throw(eh::Exception)*/
  {
    if (!colo_id_)
    {
      throw ConstraintViolation("PageLoadsDailyStatKey_V_1_0::invariant(): "
        "colo_id_ must be > 0");
    }
  }

  DayTimestamp sdate_;
  unsigned long colo_id_;
  size_t hash_;
};

class PageLoadsDailyStatInnerKey
{
public:
  PageLoadsDailyStatInnerKey() {}

  template <class ITERATOR_TYPE_>
  PageLoadsDailyStatInnerKey(
    unsigned long site_id,
    const std::string& country,
    const ITERATOR_TYPE_& tags_begin,
    const ITERATOR_TYPE_& tags_end
  )
  :
    holder_(new DataHolder(site_id, country, tags_begin, tags_end))
  {
    calc_hash_();
  }

  bool operator==(const PageLoadsDailyStatInnerKey& rhs) const
  {
    if (&rhs == this || holder_ == rhs.holder_)
    {
      return true;
    }
    return *holder_ == *rhs.holder_;
  }

  unsigned long site_id() const
  {
    return holder_->site_id;
  }

  const std::string& country() const
  {
    return holder_->country.get();
  }

  const NumberList& tags() const
  {
    return holder_->tags;
  }

  size_t hash() const
  {
    return holder_->hash;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, PageLoadsDailyStatInnerKey& key)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const PageLoadsDailyStatInnerKey& key)
    /*throw(eh::Exception)*/;

private:
  typedef OptionalValue<Aux_::StringIoWrapper,
    Aux_::ClearableOptionalValueTraits<Aux_::StringIoWrapper> >
      OptionalStringT;

  class DataHolder: public ReferenceCounting::AtomicImpl
  {
  public:
    DataHolder(): site_id(), country(), tags() {}

    template <class ITERATOR_TYPE_>
    DataHolder(
      unsigned long site_id_val,
      const std::string& country_val,
      const ITERATOR_TYPE_& tags_begin,
      const ITERATOR_TYPE_& tags_end
    )
    :
      site_id(site_id_val),
      country(country_val)
    {
      tags.assign(tags_begin, tags_end);
    }

    bool operator==(const DataHolder& rhs) const
    {
      if (&rhs == this)
      {
        return true;
      }
      return site_id == rhs.site_id &&
        country == rhs.country &&
        tags == rhs.tags;
    }

    void calc_hash_()
    {
      Generics::Murmur64Hash hasher(hash);
      hash_add(hasher, site_id);
      for (NumberList::const_iterator it = tags.begin();
        it != tags.end(); ++it)
      {
        hash_add(hasher, *it);
      }
      country.hash_add(hasher);
    }

    void invariant() const /*throw(eh::Exception)*/
    {
      if (!site_id)
      {
        throw ConstraintViolation("PageLoadsDailyStatInnerKey::invariant(): "
          "site_id must be > 0");
      }
      if (tags.empty())
      {
        throw ConstraintViolation("PageLoadsDailyStatInnerKey::invariant(): "
          "tags must be non-empty");
      }
    }

    unsigned long site_id;
    OptionalStringT country;
    NumberList tags;
    size_t hash;

  protected:
    virtual
    ~DataHolder() noexcept = default;
  };

  typedef ReferenceCounting::SmartPtr<DataHolder> DataHolder_var;

  void calc_hash_()
  {
    holder_->calc_hash_();
  }

  void invariant() const /*throw(eh::Exception)*/
  {
    holder_->invariant();
  }

  DataHolder_var holder_;
};

class PageLoadsDailyStatInnerData
{
public:
  PageLoadsDailyStatInnerData()
  :
    page_loads_(),
    utilized_page_loads_()
  {
  }

  PageLoadsDailyStatInnerData(
    long page_loads,
    long utilized_page_loads
  )
  :
    page_loads_(page_loads),
    utilized_page_loads_(utilized_page_loads)
  {
  }

  bool operator==(const PageLoadsDailyStatInnerData& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return page_loads_ == rhs.page_loads_ &&
      utilized_page_loads_ == rhs.utilized_page_loads_;
  }

  PageLoadsDailyStatInnerData&
  operator+=(const PageLoadsDailyStatInnerData& rhs)
  {
    page_loads_ += rhs.page_loads_;
    utilized_page_loads_ += rhs.utilized_page_loads_;
    return *this;
  }

  bool is_null() const
  {
    return !page_loads_ && !utilized_page_loads_;
  }

  long page_loads() const
  {
    return page_loads_;
  }

  long utilized_page_loads() const
  {
    return utilized_page_loads_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is,
    PageLoadsDailyStatInnerData& data)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const PageLoadsDailyStatInnerData& data)
    /*throw(eh::Exception)*/;

private:
  long page_loads_;
  long utilized_page_loads_;
};

struct PageLoadsDailyStatKey
{
  PageLoadsDailyStatKey(): sdate_(), colo_id_(), hash_() {}

  PageLoadsDailyStatKey(
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

  PageLoadsDailyStatKey(
    const PageLoadsDailyStatKey_V_1_0& key)
  :
    sdate_(key.sdate().time()),
    colo_id_(key.colo_id()),
    hash_()
  {
    calc_hash_();
  }

  bool operator==(const PageLoadsDailyStatKey& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return sdate_ == rhs.sdate_ &&
      colo_id_ == rhs.colo_id_;
  }

public:
  const DayHourTimestamp& sdate() const
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
  operator>>(std::istream& is, PageLoadsDailyStatKey& key);

  friend
  std::ostream&
  operator<<(std::ostream& os, const PageLoadsDailyStatKey& key)
    /*throw(eh::Exception)*/;

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    sdate_.hash_add(hasher);
    hash_add(hasher, colo_id_);
  }

  void invariant() const /*throw(eh::Exception)*/
  {
    if (!colo_id_)
    {
      throw ConstraintViolation("PageLoadsDailyStatKey::invariant(): "
        "colo_id_ must be > 0");
    }
  }

  DayHourTimestamp sdate_;
  unsigned long colo_id_;
  size_t hash_;
};

typedef PageLoadsDailyStatInnerKey PageLoadsDailyStatInnerKey_V_1_0;
typedef PageLoadsDailyStatInnerData PageLoadsDailyStatInnerData_V_1_0;

typedef StatCollector<
          PageLoadsDailyStatInnerKey_V_1_0,
          PageLoadsDailyStatInnerData_V_1_0,
          true, // Exclude null values
          true
        > PageLoadsDailyStatInnerCollector_V_1_0;

typedef PageLoadsDailyStatInnerCollector_V_1_0 PageLoadsDailyStatData_V_1_0;

typedef StatCollector<
          PageLoadsDailyStatKey_V_1_0, PageLoadsDailyStatData_V_1_0, true
        > PageLoadsDailyStatCollector_V_1_0;

typedef StatCollector<
          PageLoadsDailyStatInnerKey,
          PageLoadsDailyStatInnerData,
          true, // Exclude null values
          true
        > PageLoadsDailyStatInnerCollector;

typedef PageLoadsDailyStatInnerCollector PageLoadsDailyStatData;

typedef StatCollector<PageLoadsDailyStatKey, PageLoadsDailyStatData, true>
  PageLoadsDailyStatCollector;

struct PageLoadsDailyStatTraits: LogDefaultTraits<PageLoadsDailyStatCollector>
{
  template <class FUNCTOR_>
  static
  void
  for_each_old(FUNCTOR_& f) /*throw(eh::Exception)*/
  {
    f.template operator()<PageLoadsDailyStatCollector_V_1_0, true>("1.0");
  }

  typedef GenericLogIoHelperImpl<PageLoadsDailyStatTraits> IoHelperType;
};

} // namespace LogProcessing
} // namespace AdServer

#endif // AD_SERVER_LOG_PROCESSING_PAGE_LOADS_DAILY_STAT_HPP

