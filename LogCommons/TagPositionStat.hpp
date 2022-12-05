#ifndef AD_SERVER_LOG_PROCESSING_TAG_POSITION_STAT_HPP
#define AD_SERVER_LOG_PROCESSING_TAG_POSITION_STAT_HPP


#include <iosfwd>
#include <Generics/Time.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include "LogCommons.hpp"
#include "StatCollector.hpp"

namespace AdServer {
namespace LogProcessing {

class TagPositionStatInnerKey_V_2_7
{
public:
  typedef OptionalValue<unsigned long> OptionalUlong;

  TagPositionStatInnerKey_V_2_7()
  :
    tag_id_(),
    top_offset_(),
    left_offset_(),
    visibility_(),
    hash_()
  {
  }

  TagPositionStatInnerKey_V_2_7(
    unsigned long tag_id,
    const OptionalUlong& top_offset,
    const OptionalUlong& left_offset,
    const OptionalUlong& visibility
  )
  :
    tag_id_(tag_id),
    top_offset_(top_offset),
    left_offset_(left_offset),
    visibility_(visibility),
    hash_()
  {
    calc_hash_();
  }

  bool operator==(const TagPositionStatInnerKey_V_2_7& rhs) const
  {
    return tag_id_ == rhs.tag_id_ &&
      top_offset_ == rhs.top_offset_ &&
      left_offset_ == rhs.left_offset_ &&
      visibility_ == rhs.visibility_;
  }

  unsigned long tag_id() const
  {
    return tag_id_;
  }

  const OptionalUlong& top_offset() const
  {
    return top_offset_;
  }

  const OptionalUlong& left_offset() const
  {
    return left_offset_;
  }

  const OptionalUlong& visibility() const
  {
    return visibility_;
  }

  size_t hash() const
  {
    return hash_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is,
    TagPositionStatInnerKey_V_2_7& key)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const TagPositionStatInnerKey_V_2_7& key)
    /*throw(eh::Exception)*/;

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    hash_add(hasher, tag_id_);
    top_offset_.hash_add(hasher);
    left_offset_.hash_add(hasher);
    visibility_.hash_add(hasher);
  }

  unsigned long tag_id_;
  OptionalUlong top_offset_;
  OptionalUlong left_offset_;
  OptionalUlong visibility_;
  size_t hash_;
};

class TagPositionStatInnerKey
{
public:
  typedef OptionalValue<unsigned long> OptionalUlong;

  TagPositionStatInnerKey()
  :
    tag_id_(),
    top_offset_(),
    left_offset_(),
    visibility_(),
    test_(),
    hash_()
  {
  }

  TagPositionStatInnerKey(
    unsigned long tag_id,
    const OptionalUlong& top_offset,
    const OptionalUlong& left_offset,
    const OptionalUlong& visibility,
    bool test
  )
  :
    tag_id_(tag_id),
    top_offset_(top_offset),
    left_offset_(left_offset),
    visibility_(visibility),
    test_(test),
    hash_()
  {
    calc_hash_();
  }

  TagPositionStatInnerKey(
    const TagPositionStatInnerKey_V_2_7& key
  )
  :
    tag_id_(key.tag_id()),
    top_offset_(key.top_offset()),
    left_offset_(key.left_offset()),
    visibility_(key.visibility()),
    test_(),
    hash_()
  {
    calc_hash_();
  }

  bool operator==(const TagPositionStatInnerKey& rhs) const
  {
    return tag_id_ == rhs.tag_id_ &&
      top_offset_ == rhs.top_offset_ &&
      left_offset_ == rhs.left_offset_ &&
      visibility_ == rhs.visibility_ &&
      test_ == rhs.test_;
  }

  unsigned long tag_id() const
  {
    return tag_id_;
  }

  const OptionalUlong& top_offset() const
  {
    return top_offset_;
  }

  const OptionalUlong& left_offset() const
  {
    return left_offset_;
  }

  const OptionalUlong& visibility() const
  {
    return visibility_;
  }

  bool test() const
  {
    return test_;
  }

  size_t hash() const
  {
    return hash_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, TagPositionStatInnerKey& key)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const TagPositionStatInnerKey& key)
    /*throw(eh::Exception)*/;

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    hash_add(hasher, tag_id_);
    top_offset_.hash_add(hasher);
    left_offset_.hash_add(hasher);
    visibility_.hash_add(hasher);
    hash_add(hasher, test_);
  }

  unsigned long tag_id_;
  OptionalUlong top_offset_;
  OptionalUlong left_offset_;
  OptionalUlong visibility_;
  bool test_;
  size_t hash_;
};

class TagPositionStatInnerData
{
public:
  TagPositionStatInnerData()
  :
    requests_(),
    imps_(),
    clicks_()
  {
  }

  TagPositionStatInnerData(
    unsigned long requests,
    unsigned long imps,
    unsigned long clicks
  )
  :
    requests_(requests),
    imps_(imps),
    clicks_(clicks)
  {
  }

  bool operator==(const TagPositionStatInnerData& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return requests_ == rhs.requests_ &&
      imps_ == rhs.imps_ &&
      clicks_ == rhs.clicks_;
  }

  TagPositionStatInnerData&
  operator+=(const TagPositionStatInnerData& rhs)
  {
    requests_ += rhs.requests_;
    imps_ += rhs.imps_;
    clicks_ += rhs.clicks_;
    return *this;
  }

  unsigned long requests() const
  {
    return requests_;
  }

  unsigned long imps() const
  {
    return imps_;
  }

  unsigned long clicks() const
  {
    return clicks_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, TagPositionStatInnerData& data)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const TagPositionStatInnerData& data)
    /*throw(eh::Exception)*/;

private:
  unsigned long requests_;
  unsigned long imps_;
  unsigned long clicks_;
};

struct TagPositionStatKey
{
  TagPositionStatKey(): sdate_(), colo_id_(), hash_() {}

  TagPositionStatKey(
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

  bool operator==(const TagPositionStatKey& rhs) const
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
  operator>>(std::istream& is, TagPositionStatKey& key)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const TagPositionStatKey& key)
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

typedef TagPositionStatInnerData TagPositionStatInnerData_V_2_7;

typedef StatCollector<
          TagPositionStatInnerKey_V_2_7,
          TagPositionStatInnerData_V_2_7,
          false,
          true
        > TagPositionStatInnerCollector_V_2_7;

typedef TagPositionStatInnerCollector_V_2_7 TagPositionStatData_V_2_7;

typedef TagPositionStatKey TagPositionStatKey_V_2_7;

typedef StatCollector<TagPositionStatKey_V_2_7, TagPositionStatData_V_2_7>
  TagPositionStatCollector_V_2_7;

typedef StatCollector<
          TagPositionStatInnerKey, TagPositionStatInnerData, false, true
        > TagPositionStatInnerCollector;

typedef TagPositionStatInnerCollector TagPositionStatData;

typedef StatCollector<TagPositionStatKey, TagPositionStatData>
  TagPositionStatCollector;

struct TagPositionStatTraits: LogDefaultTraits<TagPositionStatCollector>
{
  template <typename Functor>
  static
  void
  for_each_old(Functor& f) /*throw(eh::Exception)*/
  {
    f.template operator()<TagPositionStatCollector_V_2_7>("2.7");
  }
};

} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_TAG_POSITION_STAT_HPP */

