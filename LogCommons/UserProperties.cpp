#include "UserProperties.hpp"
#include <LogCommons/LogCommons.ipp>

namespace AdServer {
namespace LogProcessing {

template <> const char* UserPropertiesTraits::B::base_name_ = "UserProperties";
template <> const char* UserPropertiesTraits::B::signature_ = "UserProperties";
template <> const char* UserPropertiesTraits::B::current_version_ = "1.2";

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, UserPropertiesKey& key)
{
  is >> key.sdate_;
  is >> key.isp_sdate_;
  is >> key.colo_id_;
  is >> key.user_status_;
  Aux_::StringIoWrapper property_name_wrapper;
  is >> property_name_wrapper;
  key.property_name_ = property_name_wrapper;
  Aux_::StringIoWrapper property_value_wrapper;
  is >> property_value_wrapper;
  key.property_value_ = property_value_wrapper;
  if (key.property_value_ == "-")
  {
    key.property_value_.clear();
  }
  key.invariant();
  key.calc_hash_();
  return is;
}

std::ostream&
operator<<(std::ostream& os, const UserPropertiesKey& key)
  /*throw(eh::Exception)*/
{
  key.invariant();
  os << key.sdate_ << '\t';
  os << key.isp_sdate_ << '\t';
  os << key.colo_id_ << '\t';
  os << key.user_status_ << '\t';
  os << Aux_::StringIoWrapper(key.property_name_.text()) << '\t';
  if (key.property_value_.empty())
  {
    os << '-';
  }
  else
  {
    os << Aux_::StringIoWrapper(key.property_value_);
  }
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, UserPropertiesData& data)
{
  is >> data.profiling_requests_;
  is >> data.requests_;
  is >> data.imps_unverified_;
  is >> data.imps_verified_;
  is >> data.clicks_;
  is >> data.actions_;
  return is;
}

std::ostream&
operator<<(std::ostream& os, const UserPropertiesData& data)
{
  os << data.profiling_requests_ << '\t';
  os << data.requests_ << '\t';
  os << data.imps_unverified_ << '\t';
  os << data.imps_verified_ << '\t';
  os << data.clicks_ << '\t';
  os << data.actions_;
  return os;
}

} // namespace LogProcessing
} // namespace AdServer

