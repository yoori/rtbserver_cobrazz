
#include "CampaignUserStat.hpp"
#include <LogCommons/LogCommons.ipp>

namespace AdServer {
namespace LogProcessing {

template <> const char*
CampaignUserStatTraits::B::base_name_ = "CampaignUserStat";

template <> const char*
CampaignUserStatTraits::B::signature_ = "CampaignUserStat";

template <> const char*
CampaignUserStatTraits::B::current_version_ = "2.5";

std::istream&
operator>>(std::istream& is, CampaignUserStatKey& key)
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
operator<<(std::ostream& os, const CampaignUserStatKey& key)
{
  key.invariant();
  os << key.adv_sdate_ << '\n' << key.colo_id_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, CampaignUserStatInnerKey& key)
{
  is >> key.cmp_id_;
  is >> key.last_appearance_date_;
  if (is)
  {
    key.invariant();
    key.calc_hash_();
  }
  return is;
}

std::ostream&
operator<<(std::ostream& os, const CampaignUserStatInnerKey& key)
{
  key.invariant();
  os << key.cmp_id_ << '\t';
  os << key.last_appearance_date_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, CampaignUserStatInnerData& data)
{
  is >> data.unique_users_;
  return is;
}

std::ostream&
operator<<(std::ostream& os, const CampaignUserStatInnerData& data)
{
  os << data.unique_users_;
  return os;
}

} // namespace LogProcessing
} // namespace AdServer

