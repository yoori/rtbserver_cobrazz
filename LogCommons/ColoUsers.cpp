
#include "ColoUsers.hpp"
#include <LogCommons/BufferWriter.hpp>
#include <LogCommons/LogCommons.ipp>

namespace AdServer::LogProcessing
{

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

  BufferWriter&
  operator<<(BufferWriter& out, const ColoUsersKey& key)
    /*throw(eh::Exception)*/
  {
    key.invariant();
    out << key.sdate_ << '\t';
    out << key.isp_sdate_ << '\t';
    out << key.colo_id_ << '\t';
    out << key.created_;
    return out;
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

  BufferWriter&
  operator<<(BufferWriter& out, const ColoUsersData& data)
  {
    out << data.users_count_ << '\t';
    out << data.weekly_users_count_ << '\t';
    out << data.monthly_users_count_ << '\t';
    out << data.daily_network_users_count_ << '\t';
    out << data.monthly_network_users_count_;
    return out;
  }

} // namespace AdServer::LogProcessing
