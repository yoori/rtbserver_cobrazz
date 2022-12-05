/// @file TagAuctionStat.hpp

#ifndef AD_SERVER_LOG_PROCESSING_TAG_AUCTION_STAT_HPP
#define AD_SERVER_LOG_PROCESSING_TAG_AUCTION_STAT_HPP

#include <LogCommons/LogCommons.hpp>
#include <LogCommons/StatCollector.hpp>

namespace AdServer {
namespace LogProcessing {

class TagAuctionStatInnerKey
{
public:
  TagAuctionStatInnerKey()
  :
    tag_id_(),
    auction_ccg_count_(),
    hash_()
  {
  }

  TagAuctionStatInnerKey(
    unsigned long tag_id,
    unsigned long auction_ccg_count
  )
  :
    tag_id_(tag_id),
    auction_ccg_count_(auction_ccg_count),
    hash_()
  {
    calc_hash_();
  }

  bool
  operator==(const TagAuctionStatInnerKey& right) const
  {
    return &right == this ||
      (tag_id_ == right.tag_id_ &&
      auction_ccg_count_ == right.auction_ccg_count_);
  }

  unsigned long
  tag_id() const
  {
    return tag_id_;
  }

  unsigned long
  auction_ccg_count() const
  {
    return auction_ccg_count_;
  }

  size_t
  hash() const
  {
    return hash_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, TagAuctionStatInnerKey& key)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const TagAuctionStatInnerKey& key)
    /*throw(eh::Exception)*/;

private:
  void
  calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    hash_add(hasher, tag_id_);
    hash_add(hasher, auction_ccg_count_);
  }

  unsigned long tag_id_;
  unsigned long auction_ccg_count_;
  size_t hash_;
};

class TagAuctionStatInnerData
{
public:
  TagAuctionStatInnerData(): requests_() {}

  explicit
  TagAuctionStatInnerData(unsigned long requests): requests_(requests) {}

  bool
  operator==(const TagAuctionStatInnerData& right) const
  {
    return requests_ == right.requests_;
  }

  TagAuctionStatInnerData&
  operator+=(const TagAuctionStatInnerData& right)
  {
    requests_ += right.requests_;
    return *this;
  }

  unsigned long
  requests() const
  {
    return requests_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, TagAuctionStatInnerData& data)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const TagAuctionStatInnerData& data)
    /*throw(eh::Exception)*/;

private:
  unsigned long requests_;
};

struct TagAuctionStatKey
{
  TagAuctionStatKey(): pub_sdate_(), colo_id_(), hash_() {}

  TagAuctionStatKey(
    const DayTimestamp& pub_sdate,
    unsigned long colo_id
  )
  :
    pub_sdate_(pub_sdate),
    colo_id_(colo_id),
    hash_()
  {
    calc_hash_();
  }

  bool
  operator==(const TagAuctionStatKey& right) const
  {
    return &right == this ||
      (pub_sdate_ == right.pub_sdate_ && colo_id_ == right.colo_id_);
  }

public:
  const DayTimestamp&
  pub_sdate() const
  {
    return pub_sdate_;
  }

  unsigned long
  colo_id() const
  {
    return colo_id_;
  }

  size_t
  hash() const
  {
    return hash_;
  }

  friend
  std::istream&
  operator>>(std::istream& is, TagAuctionStatKey& key) /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const TagAuctionStatKey& key)
    /*throw(eh::Exception)*/;

private:
  void
  calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    pub_sdate_.hash_add(hasher);
    hash_add(hasher, colo_id_);
  }

  DayTimestamp pub_sdate_;
  unsigned long colo_id_;
  size_t hash_;
};

typedef StatCollector<
          TagAuctionStatInnerKey, TagAuctionStatInnerData, false, true
        > TagAuctionStatInnerCollector;

typedef TagAuctionStatInnerCollector TagAuctionStatData;

typedef StatCollector<TagAuctionStatKey, TagAuctionStatData>
  TagAuctionStatCollector;

typedef LogDefaultTraits<TagAuctionStatCollector> TagAuctionStatTraits;

} // namespace LogProcessing
} // namespace AdServer

#endif // AD_SERVER_LOG_PROCESSING_TAG_AUCTION_STAT_HPP

