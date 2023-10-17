// THIS
#include "LogCommons/LogCommons.ipp"
#include "CtrCollector.hpp"

namespace AdServer
{
namespace LogProcessing
{

template<> const char *CtrTraits::B::base_name_ = "CtrStat";
template<> const char *CtrTraits::B::signature_ = "CtrStat";
template<> const char *CtrTraits::B::current_version_ = "2.5";

FixedBufStream<TabCategory>& operator>>(
  FixedBufStream<TabCategory>& is,
  CtrKey& key)
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
  const CtrKey& key)
{
  TabOutputArchive oa(os);
  oa << key;
  return os;
}

FixedBufStream<TabCategory>& operator>>(
  FixedBufStream<TabCategory>& is,
  CtrData& data)
{
  using NoInvariants = Aux_::NoInvariants;
  TokenizerInputArchive<NoInvariants> ia(is);
  ia >> data;
  return is;
}

std::ostream& operator<<(
  std::ostream& os,
  const CtrData& data)
{
  SimpleTabOutputArchive oa(os);
  oa << data;
  return os;
}

} // namespace LogProcessing
} // namespace AdServer