/// @file  TagAuctionStat.cpp

#include "TagAuctionStat.hpp"
#include <LogCommons/BufferWriter.hpp>
#include <LogCommons/LogCommons.ipp>

namespace AdServer::LogProcessing
{

  template <> const char* TagAuctionStatTraits::B::base_name_ = "TagAuctionStat";
  template <> const char* TagAuctionStatTraits::B::signature_ = "TagAuctionStat";
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

  BufferWriter&
  operator<<(BufferWriter& out, const TagAuctionStatKey& key)
    /*throw(eh::Exception)*/
  {
    out << key.pub_sdate_ << '\n' << key.colo_id_;
    return out;
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

  BufferWriter&
  operator<<(BufferWriter& out, const TagAuctionStatInnerKey& key)
    /*throw(eh::Exception)*/
  {
    out << key.tag_id_ << '\t' << key.auction_ccg_count_;
    return out;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, TagAuctionStatInnerData& data)
    /*throw(eh::Exception)*/
  {
    is >> data.requests_;
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const TagAuctionStatInnerData& data)
    /*throw(eh::Exception)*/
  {
    out << data.requests_;
    return out;
  }

} // namespace AdServer::LogProcessing
