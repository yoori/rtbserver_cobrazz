// THIS
#include "LogCommons/LogCommons.ipp"
#include "BidCostCollector.hpp"

namespace AdServer::LogProcessing
{

template<> const char* BidCostTraits::B::base_name_ = "BidCost";
template<> const char* BidCostTraits::B::signature_ = "BidCost";
template<> const char* BidCostTraits::B::current_version_ = "2.5";

FixedBufStream<TabCategory>& operator>>(
  FixedBufStream<TabCategory>& is,
  BidCostKey& key)
{
  TokenizerInputArchive<> ia(is);
  ia >> key;
  if (is)
  {
    key.invariant();
    key.calc_hash();
  }

  return is;
}

std::ostream& operator<<(
  std::ostream& os,
  const BidCostKey& key)
{
  TabOutputArchive oa(os);
  oa << key;

  return os;
}

FixedBufStream<TabCategory>& operator>>(
  FixedBufStream<TabCategory>& is,
  BidCostData& data)
{
  using NoInvariants = Aux_::NoInvariants;
  TokenizerInputArchive<NoInvariants> ia(is);
  ia >> data;

  return is;
}

std::ostream& operator<<(
  std::ostream& os,
  const BidCostData& data)
{
  SimpleTabOutputArchive oa(os);
  oa << data;

  return os;
}

} // namespace AdServer::LogProcessing