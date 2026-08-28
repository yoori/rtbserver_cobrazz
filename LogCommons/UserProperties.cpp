#include "UserProperties.hpp"
#include <LogCommons/BufferWriter.hpp>
#include <LogCommons/LogCommons.ipp>

namespace AdServer::LogProcessing
{
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

  BufferWriter&
  operator<<(BufferWriter& out, const UserPropertiesKey& key)
    /*throw(eh::Exception)*/
  {
    key.invariant();
    out << key.sdate_ << '\t';
    out << key.isp_sdate_ << '\t';
    out << key.colo_id_ << '\t';
    out << key.user_status_ << '\t';
    out << Aux_::StringIoWrapper(key.property_name_.text()) << '\t';
    if (key.property_value_.empty())
    {
      out << '-';
    }
    else
    {
      out << Aux_::StringIoWrapper(key.property_value_);
    }
    return out;
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

  BufferWriter&
  operator<<(BufferWriter& out, const UserPropertiesData& data)
  {
    out << data.profiling_requests_ << '\t';
    out << data.requests_ << '\t';
    out << data.imps_unverified_ << '\t';
    out << data.imps_verified_ << '\t';
    out << data.clicks_ << '\t';
    out << data.actions_;
    return out;
  }
} // namespace AdServer::LogProcessing
