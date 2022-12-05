#ifndef AD_SERVER_LOG_PROCESSING_SEARCH_TERM_STAT_HPP
#define AD_SERVER_LOG_PROCESSING_SEARCH_TERM_STAT_HPP


#include <LogCommons/LogCommons.hpp>
#include <LogCommons/StatCollector.hpp>
#include <LogCommons/GenericLogIoImpl.hpp>

namespace AdServer {
namespace LogProcessing {

class SearchTermStatInnerKey
{
  typedef Aux_::StringIoWrapper String_;

public:
  SearchTermStatInnerKey()
  :
    search_term_(),
    hash_()
  {
  }

  explicit
  SearchTermStatInnerKey(
    const std::string& search_term
  )
    /*throw(eh::Exception)*/
  :
    search_term_(search_term),
    hash_()
  {
    calc_hash_();
  }

  bool operator==(const SearchTermStatInnerKey& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return search_term_ == rhs.search_term_;
  }

  const std::string& search_term() const
  {
    return search_term_;
  }

  size_t hash() const
  {
    return hash_;
  }

  size_t distrib_hash() const
  {
    return hash_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, SearchTermStatInnerKey& key)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const SearchTermStatInnerKey& key)
    /*throw(eh::Exception)*/;

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    hash_add(hasher, search_term_);
  }

  void invariant_() const /*throw(eh::Exception)*/
  {
    if (search_term_.empty())
    {
      throw ConstraintViolation("SearchTermStatInnerKey::"
        "invariant_(): search_term_ must be non-empty");
    }
  }

  String_ search_term_;
  size_t hash_;
};

class SearchTermStatInnerData
{
public:
  SearchTermStatInnerData()
  :
    hits_()
  {
  }

  explicit
  SearchTermStatInnerData(unsigned long hits)
    /*throw(eh::Exception)*/
  :
    hits_(hits)
  {
  }

  bool operator==(const SearchTermStatInnerData& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return hits_ == rhs.hits_;
  }

  SearchTermStatInnerData&
  operator+=(const SearchTermStatInnerData& rhs)
    /*throw(eh::Exception)*/
  {
    hits_ += rhs.hits_;
    return *this;
  }

  unsigned long hits() const
  {
    return hits_;
  }

  void set_hits(unsigned long hits)
  {
    hits_ = hits;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, SearchTermStatInnerData& data)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const SearchTermStatInnerData& data)
    /*throw(eh::Exception)*/;

private:
  unsigned long hits_;
};

struct SearchTermStatKey
{
  SearchTermStatKey(): sdate_(), colo_id_(), hash_() {}

  SearchTermStatKey(
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

  bool operator==(const SearchTermStatKey& rhs) const
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
  operator>>(std::istream& is, SearchTermStatKey& key)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const SearchTermStatKey& key)
    /*throw(eh::Exception)*/;

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

typedef StatCollector<
          SearchTermStatInnerKey, SearchTermStatInnerData, false, true
        > SearchTermStatInnerCollector;

typedef SearchTermStatInnerCollector SearchTermStatData;

typedef StatCollector<SearchTermStatKey, SearchTermStatData>
  SearchTermStatCollector;

struct SearchTermStatTraits:
  LogDefaultTraits<SearchTermStatCollector, true, true>
{
  typedef CollectorHitsFilter<CollectorType> CollectorFilterType;
};

} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_SEARCH_TERM_STAT_HPP */

