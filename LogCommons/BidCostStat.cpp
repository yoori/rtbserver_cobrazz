
#include "BidCostStat.hpp"
#include <LogCommons/LogCommons.ipp>

namespace AdServer {
namespace LogProcessing {

template <> const char* BidCostStatTraits::B::base_name_ = "BidCostStat";
template <> const char* BidCostStatTraits::B::signature_ = "BidCostStat";
template <> const char* BidCostStatTraits::B::current_version_ = "2.5";

std::istream&
operator>>(std::istream& is, BidCostStatKey& key)
{
  is >> key.adv_sdate_;
  if (is)
  {
    key.invariant();
    key.calc_hash_();
  }
  return is;
}

std::ostream&
operator<<(std::ostream& os, const BidCostStatKey& key)
  /*throw(eh::Exception)*/
{
  key.invariant();
  os << key.adv_sdate_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, BidCostStatInnerKey& key)
  /*throw(eh::Exception)*/
{
  TokenizerInputArchive<> ia(is);
  ia >> key;
  if (is)
  {
    key.invariant();
    key.calc_hash_();
  }
  return is;
}

std::ostream&
operator<<(std::ostream& os, const BidCostStatInnerKey& key)
  /*throw(eh::Exception)*/
{
  TabOutputArchive oa(os);
  oa << key;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, BidCostStatInnerData& data)
  /*throw(eh::Exception)*/
{
  TokenizerInputArchive<Aux_::NoInvariants> ia(is);
  ia >> data;
  return is;
}

std::ostream&
operator<<(std::ostream& os, const BidCostStatInnerData& data)
  /*throw(eh::Exception)*/
{
  SimpleTabOutputArchive oa(os);
  oa << data;
  return os;
}

} // namespace LogProcessing
} // namespace AdServer

