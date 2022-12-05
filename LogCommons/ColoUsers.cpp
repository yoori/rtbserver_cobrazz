
#include "ColoUsers.hpp"
#include <LogCommons/LogCommons.ipp>

namespace AdServer {
namespace LogProcessing {

template <> const char* ColoUsersTraits::B::base_name_ = "ColoUsers";
template <> const char* ColoUsersTraits::B::signature_ = "ColoUsers";
template <> const char* ColoUsersTraits::B::current_version_ = "1.2";

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, ColoUsersKey& key)
{
  is >> key.sdate_;
  is >> key.isp_sdate_;
  is >> key.colo_id_;
  is >> key.created_;
  if (is)
  {
    key.invariant();
    key.calc_hash_();
  }
  return is;
}

std::ostream&
operator<<(std::ostream& os, const ColoUsersKey& key)
  /*throw(eh::Exception)*/
{
  key.invariant();
  os << key.sdate_ << '\t';
  os << key.isp_sdate_ << '\t';
  os << key.colo_id_ << '\t';
  os << key.created_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, ColoUsersData& data)
{
  is >> data.users_count_;
  is >> data.weekly_users_count_;
  is >> data.monthly_users_count_;
  is >> data.daily_network_users_count_;
  is >> data.monthly_network_users_count_;
  return is;
}

std::ostream&
operator<<(std::ostream& os, const ColoUsersData& data)
{
  os << data.users_count_ << '\t';
  os << data.weekly_users_count_ << '\t';
  os << data.monthly_users_count_ << '\t';
  os << data.daily_network_users_count_ << '\t';
  os << data.monthly_network_users_count_;
  return os;
}

} // namespace LogProcessing
} // namespace AdServer

