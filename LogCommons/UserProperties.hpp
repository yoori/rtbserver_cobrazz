#ifndef AD_SERVER_LOG_PROCESSING_USER_PROPERTIES_HPP
#define AD_SERVER_LOG_PROCESSING_USER_PROPERTIES_HPP

#include <iosfwd>
#include <sstream>
#include <Generics/Time.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include "LogCommons.hpp"
#include "StatCollector.hpp"

namespace AdServer {
namespace LogProcessing {

class UserPropertiesKey
{
public:
  UserPropertiesKey()
  :
    sdate_(),
    isp_sdate_(),
    colo_id_(),
    user_status_(),
    property_name_(),
    property_value_(),
    hash_()
  {
  }

  UserPropertiesKey(const UserPropertiesKey& init)
    : sdate_(init.sdate_),
      isp_sdate_(init.isp_sdate_),
      colo_id_(init.colo_id_),
      user_status_(init.user_status_),
      property_name_(init.property_name_),
      property_value_(init.property_value_)
  {}

  UserPropertiesKey(UserPropertiesKey&& init)
    : sdate_(init.sdate_),
      isp_sdate_(init.isp_sdate_),
      colo_id_(init.colo_id_),
      user_status_(init.user_status_),
      property_name_(std::move(init.property_name_)),
      property_value_(std::move(init.property_value_))
  {}

  UserPropertiesKey(
    const DayHourTimestamp& sdate,
    const DayHourTimestamp& isp_sdate,
    unsigned long colo_id,
    char user_status,
    const Generics::StringHashAdapter& property_name,
    const std::string& property_value
  )
  :
    sdate_(sdate),
    isp_sdate_(isp_sdate),
    colo_id_(colo_id),
    user_status_(user_status),
    property_name_(property_name),
    property_value_(property_value),
    hash_()
  {
    calc_hash_();
  }

  bool operator==(const UserPropertiesKey& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return sdate_ == rhs.sdate_ &&
      isp_sdate_ == rhs.isp_sdate_ &&
      colo_id_ == rhs.colo_id_ &&
      user_status_ == rhs.user_status_ &&
      property_name_ == rhs.property_name_ &&
      property_value_ == rhs.property_value_;
  }

  const DayHourTimestamp& sdate() const
  {
    return sdate_;
  }

  const DayHourTimestamp& isp_sdate() const
  {
    return isp_sdate_;
  }

  unsigned long colo_id() const
  {
    return colo_id_;
  }

  char user_status() const
  {
    return user_status_;
  }

  const std::string& property_name() const
  {
    return property_name_.text();
  }

  const std::string& property_value() const
  {
    return property_value_;
  }

  size_t hash() const
  {
    return hash_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, UserPropertiesKey& key);

  friend
  std::ostream&
  operator<<(std::ostream& os, const UserPropertiesKey& key)
    /*throw(eh::Exception)*/;

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_, property_name_.hash());
    sdate_.hash_add(hasher);
    isp_sdate_.hash_add(hasher);
    hash_add(hasher, colo_id_);
    hash_add(hasher, property_value_);
    hash_add(hasher, user_status_);
  }

  void invariant() const /*throw(eh::Exception)*/
  {
    if (property_name_.text().empty())
    {
      throw ConstraintViolation("UserPropertiesKey::invariant(): "
        "property_name_ must be non-empty");
    }
    if (!is_valid_user_status(user_status_))
    {
      Stream::Error es;
      es << "UserPropertiesKey::invariant(): user_status_ "
        "has invalid value '" << user_status_ << '\'';
      throw ConstraintViolation(es);
    }
  }

  DayHourTimestamp sdate_;
  DayHourTimestamp isp_sdate_;
  unsigned long colo_id_;
  char user_status_;
  Generics::StringHashAdapter property_name_;
  std::string property_value_;
  size_t hash_;
};

class UserPropertiesData
{
public:
  UserPropertiesData()
  :
    profiling_requests_(),
    requests_(),
    imps_unverified_(),
    imps_verified_(),
    clicks_(),
    actions_()
  {
  }

  UserPropertiesData(
    long profiling_requests,
    long requests,
    long imps_unverified,
    long imps_verified,
    long clicks,
    long actions
  )
  :
    profiling_requests_(profiling_requests),
    requests_(requests),
    imps_unverified_(imps_unverified),
    imps_verified_(imps_verified),
    clicks_(clicks),
    actions_(actions)
  {
  }

  bool operator==(const UserPropertiesData& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return profiling_requests_ == rhs.profiling_requests_ &&
      requests_ == rhs.requests_ &&
      imps_unverified_ == rhs.imps_unverified_ &&
      imps_verified_ == rhs.imps_verified_ &&
      clicks_ == rhs.clicks_ &&
      actions_ == rhs.actions_;
  }

  UserPropertiesData& operator+=(const UserPropertiesData& rhs)
  {
    profiling_requests_ += rhs.profiling_requests_;
    requests_ += rhs.requests_;
    imps_unverified_ += rhs.imps_unverified_;
    imps_verified_ += rhs.imps_verified_;
    clicks_ += rhs.clicks_;
    actions_ += rhs.actions_;
    return *this;
  }

  long profiling_requests() const
  {
    return profiling_requests_;
  }

  long requests() const
  {
    return requests_;
  }

  long imps_unverified() const
  {
    return imps_unverified_;
  }

  long imps_verified() const
  {
    return imps_verified_;
  }

  long clicks() const
  {
    return clicks_;
  }

  long actions() const
  {
    return actions_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, UserPropertiesData& data);

  friend
  std::ostream&
  operator<<(std::ostream& os, const UserPropertiesData& data);

private:
  long profiling_requests_;
  long requests_;
  long imps_unverified_;
  long imps_verified_;
  long clicks_;
  long actions_;
};

typedef StatCollector<UserPropertiesKey, UserPropertiesData, false, true>
  UserPropertiesCollector;

typedef LogDefaultTraits<UserPropertiesCollector, false> UserPropertiesTraits;

} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_USER_PROPERTIES_HPP */

