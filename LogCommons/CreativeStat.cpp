
#include "CreativeStat.hpp"
#include <LogCommons/LogCommons.ipp>

namespace AdServer {
namespace LogProcessing {

template <> const char* CreativeStatTraits::B::base_name_ =
  "CreativeStat";
template <> const char* CreativeStatTraits::B::signature_ =
  "CreativeStat";
template <> const char* CreativeStatTraits::B::current_version_ =
  "3.5";

const CreativeStatInnerKey::DeliveryThresholdT
  CreativeStatInnerKey::max_delivery_threshold_value_("1.0");

const CreativeStatInnerKey_V_3_0::DeliveryThresholdT
  CreativeStatInnerKey_V_3_0::max_delivery_threshold_value_("1.0");

const CreativeStatInnerKey_V_3_3::DeliveryThresholdT
  CreativeStatInnerKey_V_3_3::max_delivery_threshold_value_("1.0");

std::istream&
operator>>(std::istream& is, CreativeStatKey& key)
{
  is >> key.sdate_;
  read_eol(is);
  is >> key.adv_sdate_;
  key.calc_hash_();
  return is;
}

std::ostream&
operator<<(std::ostream& os, const CreativeStatKey& key)
  /*throw(eh::Exception)*/
{
  os << key.sdate_ << '\n' << key.adv_sdate_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, CreativeStatInnerKey& key)
{
  TokenizerInputArchive<> ia(is);
  ia >> key;
  key.calc_hash_();
  return is;
}

std::ostream&
operator<<(std::ostream& os, const CreativeStatInnerKey& key)
  /*throw(eh::Exception)*/
{
  TabOutputArchive oa(os);
  oa << key;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, CreativeStatInnerData& data)
  /*throw(eh::Exception)*/
{
  TokenizerInputArchive<Aux_::NoInvariants> ia(is);
  ia >> data;
  return is;
}

std::ostream&
operator<<(std::ostream& os, const CreativeStatInnerData& data)
  /*throw(eh::Exception)*/
{
  SimpleTabOutputArchive oa(os);
  oa << data;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, CreativeStatInnerKey_V_3_0& key)
{
  TokenizerInputArchive<> ia(is);
  ia >> key;
  key.calc_hash_();
  return is;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, CreativeStatInnerKey_V_3_3& key)
{
  TokenizerInputArchive<> ia(is);
  ia >> key;
  key.calc_hash_();
  return is;
}

} // namespace LogProcessing
} // namespace AdServer

