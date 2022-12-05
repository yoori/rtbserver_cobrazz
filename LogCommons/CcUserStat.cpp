
#include "CcUserStat.hpp"
#include <LogCommons/LogCommons.ipp>

namespace AdServer {
namespace LogProcessing {

template <> const char* CcUserStatTraits::B::base_name_ = "CCUserStat";
template <> const char* CcUserStatTraits::B::signature_ = "CCUserStat";
template <> const char* CcUserStatTraits::B::current_version_ = "2.5";

std::istream&
operator>>(std::istream& is, CcUserStatKey& key)
{
  is >> key.adv_sdate_;
  read_eol(is);
  is >> key.colo_id_;
  if (is)
  {
    key.invariant();
    key.calc_hash_();
  }
  return is;
}

std::ostream&
operator<<(std::ostream& os, const CcUserStatKey& key)
  /*throw(eh::Exception)*/
{
  key.invariant();
  os << key.adv_sdate_ << '\n' << key.colo_id_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, CcUserStatInnerKey& key)
  /*throw(eh::Exception)*/
{
  is >> key.cc_id_;
  is >> key.last_appearance_date_;
  if (is)
  {
    key.invariant();
    key.calc_hash_();
  }
  return is;
}

std::ostream&
operator<<(std::ostream& os, const CcUserStatInnerKey& key)
  /*throw(eh::Exception)*/
{
  key.invariant();
  os << key.cc_id_ << '\t';
  os << key.last_appearance_date_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, CcUserStatInnerData& data)
  /*throw(eh::Exception)*/
{
  is >> data.unique_users_;
  return is;
}

std::ostream&
operator<<(std::ostream& os, const CcUserStatInnerData& data)
  /*throw(eh::Exception)*/
{
  os << data.unique_users_;
  return os;
}

} // namespace LogProcessing
} // namespace AdServer

