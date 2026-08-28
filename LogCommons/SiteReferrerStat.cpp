
#include "SiteReferrerStat.hpp"
#include <LogCommons/BufferWriter.hpp>
#include <LogCommons/LogCommons.ipp>

namespace AdServer::LogProcessing
{

  template <> const char* SiteReferrerStatTraits::B::base_name_ = "SiteReferrerStat";
  template <> const char* SiteReferrerStatTraits::B::signature_ = "SiteReferrerStat";
  template <> const char* SiteReferrerStatTraits::B::current_version_ = "3.3";

  std::istream&
  operator>>(std::istream& is, SiteReferrerStatKey& key)
    /*throw(eh::Exception)*/
  {
    is >> key.sdate_;
    read_eol(is);
    is >> key.colo_id_;
    key.calc_hash_();
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const SiteReferrerStatKey& key)
    /*throw(eh::Exception)*/
  {
    out << key.sdate_ << '\n' << key.colo_id_;
    return out;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, SiteReferrerStatInnerKey& key)
    /*throw(eh::Exception)*/
  {
    is >> key.user_status_;
    is >> key.tag_id_;
    is >> key.ext_tag_id_;
    is >> key.host_;
    if (is)
    {
      key.invariant();
      key.calc_hash_();
    }
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const SiteReferrerStatInnerKey& key)
    /*throw(eh::Exception)*/
  {
    key.invariant();
    out << key.user_status_ << '\t';
    out << key.tag_id_ << '\t';
    out << key.ext_tag_id_ << '\t';
    out << key.host_;
    return out;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, SiteReferrerStatInnerData& data)
    /*throw(eh::Exception)*/
  {
    is >> data.requests_;
    is >> data.imps_;
    is >> data.clicks_;
    is >> data.passbacks_;
    is >> data.bids_won_count_;
    is >> data.bids_lost_count_;
    is >> data.no_bid_count_;
    is >> data.floor_won_cost_;
    is >> data.floor_lost_cost_;
    is >> data.floor_no_bid_cost_;
    is >> data.bid_won_amount_;
    is >> data.bid_lost_amount_;
    is >> data.cost_;
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const SiteReferrerStatInnerData& data)
    /*throw(eh::Exception)*/
  {
    out << data.requests_ << '\t';
    out << data.imps_ << '\t';
    out << data.clicks_ << '\t';
    out << data.passbacks_ << '\t';
    out << data.bids_won_count_ << '\t';
    out << data.bids_lost_count_ << '\t';
    out << data.no_bid_count_ << '\t';
    out << data.floor_won_cost_ << '\t';
    out << data.floor_lost_cost_ << '\t';
    out << data.floor_no_bid_cost_ << '\t';
    out << data.bid_won_amount_ << '\t';
    out << data.bid_lost_amount_ << '\t';
    out << data.cost_;
    return out;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, SiteReferrerStatInnerKey_V_3_1& key)
    /*throw(eh::Exception)*/
  {
    is >> key.user_status_;
    is >> key.tag_id_;
    is >> key.host_;
    if (is)
    {
      key.invariant();
      key.calc_hash_();
    }
    return is;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, SiteReferrerStatInnerData_V_3_1& data)
    /*throw(eh::Exception)*/
  {
    is >> data.requests_;
    is >> data.imps_;
    is >> data.clicks_;
    return is;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, SiteReferrerStatInnerData_V_3_2& data)
    /*throw(eh::Exception)*/
  {
    is >> data.requests_;
    is >> data.imps_;
    is >> data.clicks_;
    is >> data.passbacks_;
    return is;
  }

} // namespace AdServer::LogProcessing
