
#include "CcgKeywordStat.hpp"
#include <LogCommons/LogCommons.ipp>

namespace AdServer {
namespace LogProcessing {

template <> const char* CcgKeywordStatTraits::B::base_name_ = "CCGKeywordStat";
template <> const char* CcgKeywordStatTraits::B::signature_ = "CCGKeywordStat";
template <> const char* CcgKeywordStatTraits::B::current_version_ = "2.3";

std::istream&
operator>>(std::istream& is, CcgKeywordStatKey& key)
{
  is >> key.sdate_;
  read_eol(is);
  is >> key.colo_id_;
  key.calc_hash_();
  return is;
}

std::ostream&
operator<<(std::ostream& os, const CcgKeywordStatKey& key)
  /*throw(eh::Exception)*/
{
  os << key.sdate_ << '\n' << key.colo_id_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, CcgKeywordStatInnerKey& key)
  /*throw(eh::Exception)*/
{
  is >> key.ccg_keyword_id_;
  is >> key.currency_exchange_id_;
  is >> key.cc_id_;
  key.calc_hash_();
  return is;
}

std::ostream&
operator<<(std::ostream& os, const CcgKeywordStatInnerKey& key)
  /*throw(eh::Exception)*/
{
  os << key.ccg_keyword_id_ << '\t';
  os << key.currency_exchange_id_ << '\t';
  os << key.cc_id_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, CcgKeywordStatInnerData& data)
  /*throw(eh::Exception)*/
{
  is >> data.imps_;
  is >> data.clicks_;
  is >> data.adv_revenue_;
  is >> data.adv_comm_revenue_;
  is >> data.pub_advcurrency_amount_;
  if (is)
  {
    data.invariant();
  }
  return is;
}

std::ostream&
operator<<(std::ostream& os, const CcgKeywordStatInnerData& data)
  /*throw(eh::Exception)*/
{
  data.invariant();
  os << data.imps_ << '\t';
  os << data.clicks_ << '\t';
  os << data.adv_revenue_ << '\t';
  os << data.adv_comm_revenue_ << '\t';
  os << data.pub_advcurrency_amount_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(
  FixedBufStream<TabCategory>& is,
  CcgKeywordStatInnerData_V_1_4& data
)
  /*throw(eh::Exception)*/
{
  is >> data.imps_;
  is >> data.clicks_;
  is >> data.revenue_;
  if (is)
  {
    data.invariant();
  }
  return is;
}

} // namespace LogProcessing
} // namespace AdServer

