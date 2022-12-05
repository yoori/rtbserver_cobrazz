
#include "SiteStat.hpp"
#include <LogCommons/LogCommons.ipp>

namespace AdServer {
namespace LogProcessing {

template <> const char* SiteStatTraits::B::base_name_ =
  "SiteStat";
template <> const char* SiteStatTraits::B::signature_ =
  "SiteStat";
template <> const char* SiteStatTraits::B::current_version_ =
  "1.0"; // Last change: AdServer v2.0

std::istream&
operator>>(std::istream& is, SiteStatKey& key)
{
  is >> key.isp_sdate_;
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
operator<<(std::ostream& os, const SiteStatKey& key)
  /*throw(eh::Exception)*/
{
  key.invariant();
  os << key.isp_sdate_ << '\n' << key.colo_id_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, SiteStatInnerKey& key)
  /*throw(eh::Exception)*/
{
  is >> key.site_id_;
  if (is)
  {
    key.invariant();
  }
  return is;
}

std::ostream&
operator<<(std::ostream& os, const SiteStatInnerKey& key)
  /*throw(eh::Exception)*/
{
  key.invariant();
  os << key.site_id_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, SiteStatInnerData& data)
  /*throw(eh::Exception)*/
{
  is >> data.daily_reach_;
  is >> data.monthly_reach_;
  return is;
}

std::ostream&
operator<<(std::ostream& os, const SiteStatInnerData& data)
  /*throw(eh::Exception)*/
{
  os << data.daily_reach_ << '\t';
  os << data.monthly_reach_;
  return os;
}

} // namespace LogProcessing
} // namespace AdServer

