
#include "SiteReferrerStat.hpp"
#include <LogCommons/LogCommons.ipp>

namespace AdServer {
namespace LogProcessing {

template <> const char* SiteReferrerStatTraits::B::base_name_ =
  "SiteReferrerStat";
template <> const char* SiteReferrerStatTraits::B::signature_ =
  "SiteReferrerStat";
template <> const char* SiteReferrerStatTraits::B::current_version_ =
  "3.3";

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

std::ostream&
operator<<(std::ostream& os, const SiteReferrerStatKey& key)
  /*throw(eh::Exception)*/
{
  os << key.sdate_ << '\n' << key.colo_id_;
  return os;
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

std::ostream&
operator<<(std::ostream& os, const SiteReferrerStatInnerKey& key)
  /*throw(eh::Exception)*/
{
  key.invariant();
  os << key.user_status_ << '\t';
  os << key.tag_id_ << '\t';
  os << key.ext_tag_id_ << '\t';
  os << key.host_;
  return os;
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

std::ostream&
operator<<(std::ostream& os, const SiteReferrerStatInnerData& data)
  /*throw(eh::Exception)*/
{
  os << data.requests_ << '\t';
  os << data.imps_ << '\t';
  os << data.clicks_ << '\t';
  os << data.passbacks_ << '\t';
  os << data.bids_won_count_ << '\t';
  os << data.bids_lost_count_ << '\t';
  os << data.no_bid_count_ << '\t';
  os << data.floor_won_cost_ << '\t';
  os << data.floor_lost_cost_ << '\t';
  os << data.floor_no_bid_cost_ << '\t';
  os << data.bid_won_amount_ << '\t';
  os << data.bid_lost_amount_ << '\t';
  os << data.cost_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(
  FixedBufStream<TabCategory>& is,
  SiteReferrerStatInnerData_V_1_1& data
)
  /*throw(eh::Exception)*/
{
  is >> data.requests_;
  return is;
}

FixedBufStream<TabCategory>&
operator>>(
  FixedBufStream<TabCategory>& is,
  SiteReferrerStatInnerKey_V_3_1& key
)
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
operator>>(
  FixedBufStream<TabCategory>& is,
  SiteReferrerStatInnerData_V_3_1& data
)
  /*throw(eh::Exception)*/
{
  is >> data.requests_;
  is >> data.imps_;
  is >> data.clicks_;
  return is;
}

FixedBufStream<TabCategory>&
operator>>(
  FixedBufStream<TabCategory>& is,
  SiteReferrerStatInnerData_V_3_2& data
)
  /*throw(eh::Exception)*/
{
  is >> data.requests_;
  is >> data.imps_;
  is >> data.clicks_;
  is >> data.passbacks_;
  return is;
}

} // namespace LogProcessing
} // namespace AdServer

