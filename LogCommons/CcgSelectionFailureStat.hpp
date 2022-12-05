#ifndef AD_SERVER_LOG_PROCESSING_CCG_SELECTION_FAILURE_STAT_HPP
#define AD_SERVER_LOG_PROCESSING_CCG_SELECTION_FAILURE_STAT_HPP


#include <iosfwd>
#include <Generics/Time.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include "LogCommons.hpp"
#include "StatCollector.hpp"

namespace AdServer {
namespace LogProcessing {

class CcgSelectionFailureStatInnerKey
{
public:
  CcgSelectionFailureStatInnerKey()
  :
    ccg_id_(),
    combination_mask_(),
    hash_()
  {
  }

  CcgSelectionFailureStatInnerKey(
    unsigned long ccg_id,
    unsigned long combination_mask
  )
  :
    ccg_id_(ccg_id),
    combination_mask_(combination_mask),
    hash_()
  {
    calc_hash_();
  }

  bool operator==(const CcgSelectionFailureStatInnerKey& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return ccg_id_ == rhs.ccg_id_ &&
      combination_mask_ == rhs.combination_mask_;
  }

  unsigned long ccg_id() const
  {
    return ccg_id_;
  }

  unsigned long combination_mask() const
  {
    return combination_mask_;
  }

  size_t hash() const
  {
    return hash_;
  }

  friend FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is,
    CcgSelectionFailureStatInnerKey& key)
    /*throw(eh::Exception)*/;

  friend std::ostream&
  operator<<(std::ostream& os, const CcgSelectionFailureStatInnerKey& key)
    /*throw(eh::Exception)*/;

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    hash_add(hasher, ccg_id_);
    hash_add(hasher, combination_mask_);
  }

  unsigned long ccg_id_;
  unsigned long combination_mask_;
  size_t hash_;
};

class CcgSelectionFailureStatInnerData
{
public:
  CcgSelectionFailureStatInnerData()
  :
    requests_()
  {
  }

  CcgSelectionFailureStatInnerData(
    unsigned long requests
  )
  :
    requests_(requests)
  {
  }

  bool operator==(const CcgSelectionFailureStatInnerData& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return requests_ == rhs.requests_;
  }

  CcgSelectionFailureStatInnerData&
  operator+=(const CcgSelectionFailureStatInnerData& rhs)
  {
    requests_ += rhs.requests_;
    return *this;
  }

  unsigned long requests() const
  {
    return requests_;
  }

  friend FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is,
    CcgSelectionFailureStatInnerData& data)
    /*throw(eh::Exception)*/;

  friend std::ostream&
  operator<<(std::ostream& os, const CcgSelectionFailureStatInnerData& data)
    /*throw(eh::Exception)*/;

private:
  unsigned long requests_;
};

typedef StatCollector<
          CcgSelectionFailureStatInnerKey,
          CcgSelectionFailureStatInnerData,
          false,
          true
        > CcgSelectionFailureStatInnerCollector;

typedef DayTimestamp CcgSelectionFailureStatKey;
typedef CcgSelectionFailureStatInnerCollector CcgSelectionFailureStatData;

typedef StatCollector<CcgSelectionFailureStatKey, CcgSelectionFailureStatData>
  CcgSelectionFailureStatCollector;

typedef LogDefaultTraits<CcgSelectionFailureStatCollector>
  CcgSelectionFailureStatTraits;

} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_CCG_SELECTION_FAILURE_STAT_HPP */

