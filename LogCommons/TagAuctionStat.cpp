/// @file  TagAuctionStat.cpp

#include "TagAuctionStat.hpp"
#include <LogCommons/LogCommons.ipp>

namespace AdServer {
namespace LogProcessing {

template <> const char* TagAuctionStatTraits::B::base_name_ =
  "TagAuctionStat";
template <> const char* TagAuctionStatTraits::B::signature_ =
  "TagAuctionStat";
template <> const char* TagAuctionStatTraits::B::current_version_ =
  "1.0"; // Last change: AdServer v2.3

std::istream&
operator>>(std::istream& is, TagAuctionStatKey& key)
  /*throw(eh::Exception)*/
{
  is >> key.pub_sdate_;
  read_eol(is);
  is >> key.colo_id_;
  key.calc_hash_();
  return is;
}

std::ostream&
operator<<(std::ostream& os, const TagAuctionStatKey& key)
  /*throw(eh::Exception)*/
{
  os << key.pub_sdate_ << '\n' << key.colo_id_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, TagAuctionStatInnerKey& key)
  /*throw(eh::Exception)*/
{
  is >> key.tag_id_;
  is >> key.auction_ccg_count_;
  key.calc_hash_();
  return is;
}

std::ostream&
operator<<(std::ostream& os, const TagAuctionStatInnerKey& key)
  /*throw(eh::Exception)*/
{
  os << key.tag_id_ << '\t';
  os << key.auction_ccg_count_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, TagAuctionStatInnerData& data)
  /*throw(eh::Exception)*/
{
  is >> data.requests_;
  return is;
}

std::ostream&
operator<<(std::ostream& os, const TagAuctionStatInnerData& data)
  /*throw(eh::Exception)*/
{
  os << data.requests_;
  return os;
}

} // namespace LogProcessing
} // namespace AdServer

