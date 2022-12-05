#ifndef AD_SERVER_LOG_COMMONS_AD_REQUEST_LOGGER_HPP
#define AD_SERVER_LOG_COMMONS_AD_REQUEST_LOGGER_HPP


#include <iosfwd>
#include <string>
#include <ostream>

#include <eh/Exception.hpp>
#include <Generics/Time.hpp>
#include <Commons/Containers.hpp>
#include <Commons/UserInfoManip.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <LogCommons/LogCommons.hpp>
#include <LogCommons/StatCollector.hpp>

#include <LogCommons/GenericLogIoImpl.hpp>
#include <LogCommons/CsvUtils.hpp>

namespace AdServer {
namespace LogProcessing {

class GenericAdRequestData
{
public:
  GenericAdRequestData(
    const Generics::Time& time,
    const AdServer::Commons::RequestId& request_id,
    const AdServer::Commons::Optional<unsigned long>& user_id_hash_mod
  ) : time_(time),
    request_id_(request_id),
    distribution_hash_(request_distribution_hash(
      request_id,
      user_id_hash_mod))
  {}

  GenericAdRequestData()
  :
    time_(),
    request_id_(),
    distribution_hash_()
  {}

  bool operator==(const GenericAdRequestData& data) const
  {
    if (this == &data)
    {
      return true;
    }
    return time_ == data.time_ && request_id_ == data.request_id_;
  }

  unsigned long distrib_hash() const
  {
    return distribution_hash_;
  }

  const SecondsTimestamp&
  time() const
  {
    return time_;
  }

  const RequestId&
  request_id() const
  {
    return request_id_;
  }

private:
  SecondsTimestamp time_;
  RequestId request_id_;
  unsigned long distribution_hash_;

  friend FixedBufStream<TabCategory>&
    operator>>(FixedBufStream<TabCategory>& is,
      GenericAdRequestData& data) /*throw(eh::Exception)*/;

  friend std::ostream& operator<<(std::ostream& os,
    const GenericAdRequestData& data) /*throw(eh::Exception)*/;
};

class AdvertiserActionData_V_3_3_1
{
  SecondsTimestamp time_;
  UserId user_id_;
  OptionalUlong action_id_;
  OptionalUlong device_channel_id_;
  OptionalValue<RequestId> action_request_id_;
  NumberList ccg_ids_;
  StringIoWrapperOptional referrer_;
  StringIoWrapperOptional order_id_;
  StringIoWrapperOptional ip_address_;
  FixedNumber cur_value_;

public:
  const SecondsTimestamp&
  time() const noexcept
  {
    return time_;
  }

  const UserId&
  user_id() const noexcept
  {
    return user_id_;
  }

  const OptionalUlong&
  action_id() const noexcept
  {
    return action_id_;
  }

  const OptionalUlong&
  device_channel_id() const noexcept
  {
    return device_channel_id_;
  }

  const OptionalValue<RequestId>&
  action_request_id() const noexcept
  {
    return action_request_id_;
  }

  const NumberList&
  ccg_ids() const noexcept
  {
    return ccg_ids_;
  }

  const StringIoWrapperOptional&
  referrer() const noexcept
  {
    return referrer_;
  }

  const StringIoWrapperOptional&
  order_id() const noexcept
  {
    return order_id_;
  }

  const StringIoWrapperOptional&
  ip_address() const noexcept
  {
    return ip_address_;
  }

  const FixedNumber&
  cur_value() const noexcept
  {
    return cur_value_;
  }

  AdvertiserActionData_V_3_3_1() noexcept : cur_value_(FixedNumber::ZERO) {}

  AdvertiserActionData_V_3_3_1(
    const SecondsTimestamp& time_val,
    const UserId& user_id_val,
    const OptionalUlong& action_id_val,
    const OptionalUlong& device_channel_id_val,
    const OptionalValue<RequestId>& action_request_id_val,
    const NumberList& ccg_ids_val,
    const StringIoWrapperOptional& referrer_val,
    const StringIoWrapperOptional& order_id_val,
    const StringIoWrapperOptional& ip_address_val,
    const FixedNumber& cur_value_val
  )
  :
    time_(time_val),
    user_id_(user_id_val),
    action_id_(action_id_val),
    device_channel_id_(device_channel_id_val),
    action_request_id_(action_request_id_val),
    ccg_ids_(ccg_ids_val),
    referrer_(referrer_val),
    order_id_(order_id_val),
    ip_address_(ip_address_val),
    cur_value_(cur_value_val)
  {
  }

  bool operator==(const AdvertiserActionData_V_3_3_1& data) const
  {
    if (this == &data)
    {
      return true;
    }
    return time_ == data.time_ &&
      user_id_ == data.user_id_ &&
      action_id_ == data.action_id_ &&
      device_channel_id_ == data.device_channel_id_ &&
      action_request_id_ == data.action_request_id_ &&
      ccg_ids_ == data.ccg_ids_ &&
      referrer_ == data.referrer_ &&
      order_id_ == data.order_id_ &&
      ip_address_ == data.ip_address_ &&
      cur_value_ == data.cur_value_;
  }

  unsigned long distrib_hash() const
  {
    return user_id_distribution_hash(user_id_);
  }

  friend FixedBufStream<TabCategory>& operator>>
    (FixedBufStream<TabCategory>& is, AdvertiserActionData_V_3_3_1& data)
    /*throw(eh::Exception)*/;
};

class AdvertiserActionData
{
  SecondsTimestamp time_;
  UserIdIoWrapper user_id_;
  RequestIdIoWrapper request_id_;
  OptionalUlong action_id_;
  OptionalUlong device_channel_id_;
  OptionalValue<RequestId> action_request_id_;
  NumberList ccg_ids_;
  StringIoWrapperOptional referrer_;
  StringIoWrapperOptional order_id_;
  StringIoWrapperOptional ip_address_;
  FixedNumber cur_value_;

public:
  const SecondsTimestamp&
  time() const noexcept
  {
    return time_;
  }

  const UserId&
  user_id() const noexcept
  {
    return user_id_;
  }

  const RequestId&
  request_id() const noexcept
  {
    return request_id_;
  }

  const OptionalUlong&
  action_id() const noexcept
  {
    return action_id_;
  }

  const OptionalUlong&
  device_channel_id() const noexcept
  {
    return device_channel_id_;
  }

  const OptionalValue<RequestId>&
  action_request_id() const noexcept
  {
    return action_request_id_;
  }

  const NumberList&
  ccg_ids() const noexcept
  {
    return ccg_ids_;
  }

  const StringIoWrapperOptional&
  referrer() const noexcept
  {
    return referrer_;
  }

  const StringIoWrapperOptional&
  order_id() const noexcept
  {
    return order_id_;
  }

  const StringIoWrapperOptional&
  ip_address() const noexcept
  {
    return ip_address_;
  }

  const FixedNumber&
  cur_value() const noexcept
  {
    return cur_value_;
  }

  AdvertiserActionData() noexcept : cur_value_(FixedNumber::ZERO) {}

  AdvertiserActionData(
    const SecondsTimestamp& time_val,
    const UserId& user_id_val,
    const RequestId& request_id_val,
    const OptionalUlong& action_id_val,
    const OptionalUlong& device_channel_id_val,
    const OptionalValue<RequestId>& action_request_id_val,
    const NumberList& ccg_ids_val,
    const StringIoWrapperOptional& referrer_val,
    const StringIoWrapperOptional& order_id_val,
    const StringIoWrapperOptional& ip_address_val,
    const FixedNumber& cur_value_val
  )
  :
    time_(time_val),
    user_id_(user_id_val),
    request_id_(request_id_val),
    action_id_(action_id_val),
    device_channel_id_(device_channel_id_val),
    action_request_id_(action_request_id_val),
    ccg_ids_(ccg_ids_val),
    referrer_(referrer_val),
    order_id_(order_id_val),
    ip_address_(ip_address_val),
    cur_value_(cur_value_val)
  {
  }

  AdvertiserActionData(
    const AdvertiserActionData_V_3_3_1& data
  )
  :
    time_(data.time()),
    user_id_(data.user_id()),
    request_id_(),
    action_id_(data.action_id()),
    device_channel_id_(data.device_channel_id()),
    action_request_id_(data.action_request_id()),
    ccg_ids_(data.ccg_ids()),
    referrer_(data.referrer()),
    order_id_(data.order_id()),
    ip_address_(data.ip_address()),
    cur_value_(data.cur_value())
  {
  }

  bool operator==(const AdvertiserActionData& data) const
  {
    if (this == &data)
    {
      return true;
    }
    return time_ == data.time_ &&
      user_id_ == data.user_id_ &&
      request_id_ == data.request_id_ &&
      action_id_ == data.action_id_ &&
      device_channel_id_ == data.device_channel_id_ &&
      action_request_id_ == data.action_request_id_ &&
      ccg_ids_ == data.ccg_ids_ &&
      referrer_ == data.referrer_ &&
      order_id_ == data.order_id_ &&
      ip_address_ == data.ip_address_ &&
      cur_value_ == data.cur_value_;
  }

  unsigned long distrib_hash() const
  {
    return request_distribution_hash(request_id_, user_id_);
  }

  friend FixedBufStream<TabCategory>&
    operator>>(FixedBufStream<TabCategory>& is, AdvertiserActionData& data)
    /*throw(eh::Exception)*/;

  friend std::ostream&
  operator<<(std::ostream& os, const AdvertiserActionData& data)
    /*throw(eh::Exception)*/;
};

typedef SeqCollector<AdvertiserActionData_V_3_3_1, true>
  AdvertiserActionCollector_V_3_3_1;

typedef SeqCollector<AdvertiserActionData, true> AdvertiserActionCollector;

struct AdvertiserActionTraits:
  LogDefaultTraits<AdvertiserActionCollector, false, false>
{
  template <class FUNCTOR_>
  static void for_each_old(FUNCTOR_& f) /*throw(eh::Exception)*/
  {
    f.template operator()<AdvertiserActionCollector_V_3_3_1>("3.3.1");
  }

  typedef GenericLogIoHelperImpl<AdvertiserActionTraits> IoHelperType;
};

struct ActionOpportunityData
{
  SecondsTimestamp time;
  UserId user_id;
  unsigned long cid;
  RequestId request_id;

  bool operator==(const ActionOpportunityData& data) const
  {
    if (this == &data)
    {
      return true;
    }
    return time == data.time &&
      user_id == data.user_id &&
      cid == data.cid &&
      request_id == data.request_id;
  }

  unsigned long distrib_hash() const
  {
    return user_id_distribution_hash(user_id);
  }

  friend FixedBufStream<TabCategory>&
    operator>>(FixedBufStream<TabCategory>& is, ActionOpportunityData& data)
    /*throw(eh::Exception)*/;

  friend std::ostream& operator<<(std::ostream& os,
    const ActionOpportunityData& data) /*throw(eh::Exception)*/;
};

typedef SeqCollector<ActionOpportunityData, true> ActionOpportunityCollector;

typedef LogDefaultTraits<ActionOpportunityCollector, false, false> ActionOpportunityTraits;

class PassbackOpportunityData
{
public:
  PassbackOpportunityData(
    const Generics::Time& time,
    unsigned long colo_id,
    unsigned long tag_id,
    const AdServer::Commons::RequestId& request_id,
    char user_status,
    const AdServer::Commons::UserId& user_id
  ) : time_(time),
    colo_id_(colo_id),
    tag_id_(tag_id),
    request_id_(request_id),
    user_status_(user_status),
    distribution_hash_(request_distribution_hash(
      request_id,
      user_id))
  {}

  PassbackOpportunityData()
  {}

  bool operator==(const PassbackOpportunityData& data) const
  {
    if (this == &data)
    {
      return true;
    }
    return time_ == data.time_ &&
      colo_id_ == data.colo_id_ &&
      tag_id_ == data.tag_id_ &&
      request_id_ == data.request_id_ &&
      user_status_ == data.user_status_;
  }

  unsigned long distrib_hash() const
  {
    return distribution_hash_;
  }

  const SecondsTimestamp&
  time() const
  {
    return time_;
  }

  unsigned long
  colo_id() const
  {
    return colo_id_;
  }

  unsigned long
  tag_id() const
  {
    return tag_id_;
  }

  const RequestId&
  request_id() const
  {
    return request_id_;
  }

  char
  user_status() const
  {
    return user_status_;
  }

  template <typename Archive>
  void
  serialize(Archive& ar)
  {
    ar & time_;
    ar & colo_id_;
    ar & tag_id_;
    ar & request_id_;
    ar ^ user_status_;
  }

  void invariant() const /*throw(eh::Exception)*/
  {
    if (!is_valid_user_status(user_status_))
    {
      Stream::Error ostr;
      ostr << "PassbackOpportunityData::invariant_(): user_status "
        "has invalid value '" << user_status_ << '\'';
      throw ConstraintViolation(ostr);
    }
  }

private:
  SecondsTimestamp time_;
  unsigned long colo_id_;
  unsigned long tag_id_;
  RequestId request_id_;
  char user_status_;
  unsigned long distribution_hash_;

  friend FixedBufStream<TabCategory>&
    operator >>(FixedBufStream<TabCategory>& is, PassbackOpportunityData& data)
    /*throw(eh::Exception)*/;

  friend std::ostream&
  operator <<(std::ostream& os, const PassbackOpportunityData& data)
    /*throw(eh::Exception)*/;
};

typedef SeqCollector<PassbackOpportunityData, true> PassbackOpportunityCollector;

typedef LogDefaultTraits<PassbackOpportunityCollector, false, false> PassbackOpportunityTraits;

typedef GenericAdRequestData ClickData_V_1_0;

class ClickData
{
public:
  ClickData()
  {
  }

  ClickData(
    const Generics::Time& time,
    const AdServer::Commons::RequestId& request_id,
    const StringIoWrapperOptional& referrer,
    const AdServer::Commons::Optional<unsigned long>& user_id_hash_mod
  )
  :
    time_(time),
    request_id_(request_id),
    referrer_(referrer),
    distribution_hash_(request_distribution_hash(request_id, user_id_hash_mod))
  {
  }

  ClickData(
    const ClickData_V_1_0& data
  )
  :
    time_(data.time()),
    request_id_(data.request_id()),
    referrer_(),
    distribution_hash_(data.distrib_hash())
  {
  }

  bool operator==(const ClickData& data) const
  {
    if (this == &data)
    {
      return true;
    }
    return time_ == data.time_ &&
      request_id_ == data.request_id_ &&
      referrer_ == data.referrer_;
  }

  const SecondsTimestamp& time() const
  {
    return time_;
  }

  const RequestId& request_id() const
  {
    return request_id_;
  }

  const StringIoWrapperOptional& referrer() const
  {
    return referrer_;
  }

  unsigned long distrib_hash() const
  {
    return distribution_hash_;
  }

private:
  SecondsTimestamp time_;
  RequestId request_id_;
  StringIoWrapperOptional referrer_;
  unsigned long distribution_hash_;

  friend FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, ClickData& data)
    /*throw(eh::Exception)*/;

  friend std::ostream&
  operator<<(std::ostream& os, const ClickData& data)
    /*throw(eh::Exception)*/;
};

typedef SeqCollector<ClickData_V_1_0, true> ClickCollector_V_1_0;

typedef SeqCollector<ClickData, true> ClickCollector;

struct ClickTraits: LogDefaultTraits<ClickCollector, false, false>
{
  template <class FUNCTOR_>
  static void for_each_old(FUNCTOR_& f) /*throw(eh::Exception)*/
  {
    f.template operator()<ClickCollector_V_1_0>("1.0");
  }

  typedef GenericLogIoHelperImpl<ClickTraits> IoHelperType;
};

typedef GenericAdRequestData ImpressionData_V_1_0;

class ImpressionData_V_3_1
{
public:
  ImpressionData_V_3_1(
    const Generics::Time& time,
    const AdServer::Commons::RequestId& request_id,
    const OptionalFixedNumber& pub_revenue,
    const OptionalFixedNumber& pub_sys_revenue,
    const AdServer::Commons::Optional<unsigned long>& user_id_hash_mod
  )
  :
    time_(time),
    request_id_(request_id),
    pub_revenue_(pub_revenue),
    pub_sys_revenue_(pub_sys_revenue),
    distribution_hash_(request_distribution_hash(request_id, user_id_hash_mod))
  {
  }

  ImpressionData_V_3_1()
  {
  }

  bool operator==(const ImpressionData_V_3_1& data) const
  {
    if (this == &data)
    {
      return true;
    }
    return time_ == data.time_ &&
      pub_revenue_ == data.pub_revenue_ &&
      pub_sys_revenue_ == data.pub_sys_revenue_ &&
      request_id_ == data.request_id_;
  }

  const SecondsTimestamp& time() const
  {
    return time_;
  }

  const RequestId& request_id() const
  {
    return request_id_;
  }

  const OptionalFixedNumber& pub_revenue() const
  {
    return pub_revenue_;
  }

  const OptionalFixedNumber& pub_sys_revenue() const
  {
    return pub_sys_revenue_;
  }

  unsigned long distrib_hash() const
  {
    return distribution_hash_;
  }

private:
  SecondsTimestamp time_;
  RequestId request_id_;
  OptionalFixedNumber pub_revenue_;
  OptionalFixedNumber pub_sys_revenue_;
  unsigned long distribution_hash_;

  friend FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, ImpressionData_V_3_1& data)
    /*throw(eh::Exception)*/;
};

class ImpressionData_V_3_2
{
public:
  ImpressionData_V_3_2(
    const Generics::Time& time,
    const AdServer::Commons::RequestId& request_id,
    const OptionalFixedNumber& pub_revenue,
    const OptionalFixedNumber& pub_sys_revenue,
    char pub_revenue_type,
    const AdServer::Commons::Optional<unsigned long>& user_id_hash_mod
  )
  :
    time_(time),
    request_id_(request_id),
    pub_revenue_(pub_revenue),
    pub_sys_revenue_(pub_sys_revenue),
    pub_revenue_type_(pub_revenue_type),
    distribution_hash_(request_distribution_hash(request_id, user_id_hash_mod))
  {
  }

  ImpressionData_V_3_2()
  {
  }

  bool operator==(const ImpressionData_V_3_2& data) const
  {
    if (this == &data)
    {
      return true;
    }
    return time_ == data.time_ &&
      pub_revenue_ == data.pub_revenue_ &&
      pub_sys_revenue_ == data.pub_sys_revenue_ &&
      pub_revenue_type_ == data.pub_revenue_type_ &&
      request_id_ == data.request_id_;
  }

  const SecondsTimestamp& time() const
  {
    return time_;
  }

  const RequestId& request_id() const
  {
    return request_id_;
  }

  const OptionalFixedNumber& pub_revenue() const
  {
    return pub_revenue_;
  }

  const OptionalFixedNumber& pub_sys_revenue() const
  {
    return pub_sys_revenue_;
  }

  char pub_revenue_type() const
  {
    return pub_revenue_type_;
  }

  unsigned long distrib_hash() const
  {
    return distribution_hash_;
  }

  void invariant() const /*throw(ConstraintViolation)*/
  {
    if (!pub_revenue_type_is_valid_())
    {
      Stream::Error es;
      es << "ImpressionData_V_3_2::invariant(): pub_revenue_type_ "
        "has invalid value '" << pub_revenue_type_ << '\'';
      throw ConstraintViolation(es);
    }
  }

private:
  bool pub_revenue_type_is_valid_() const
  {
    return pub_revenue_type_ == 'A' || pub_revenue_type_ == 'P';
  }

  friend FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, ImpressionData_V_3_2& data)
    /*throw(eh::Exception)*/;

  SecondsTimestamp time_;
  RequestId request_id_;
  OptionalFixedNumber pub_revenue_;
  OptionalFixedNumber pub_sys_revenue_;
  char pub_revenue_type_;
  unsigned long distribution_hash_;
};

class ImpressionData_V_3_3
{
public:
  ImpressionData_V_3_3()
  {
  }

  ImpressionData_V_3_3(
    const Generics::Time& time,
    const AdServer::Commons::RequestId& request_id,
    const AdServer::Commons::UserId& user_id,
    const OptionalFixedNumber& pub_revenue,
    const OptionalFixedNumber& pub_sys_revenue,
    char pub_revenue_type,
    char request_type,
    const AdServer::Commons::Optional<unsigned long>& user_id_hash_mod
  )
  :
    time_(time),
    request_id_(request_id),
    user_id_(user_id),
    pub_revenue_(pub_revenue),
    pub_sys_revenue_(pub_sys_revenue),
    pub_revenue_type_(pub_revenue_type),
    request_type_(request_type),
    distribution_hash_(request_distribution_hash(request_id, user_id_hash_mod))
  {
  }

  bool operator==(const ImpressionData_V_3_3& data) const
  {
    if (this == &data)
    {
      return true;
    }
    return time_ == data.time_ &&
      request_id_ == data.request_id_ &&
      user_id_ == data.user_id_ &&
      pub_revenue_ == data.pub_revenue_ &&
      pub_sys_revenue_ == data.pub_sys_revenue_ &&
      pub_revenue_type_ == data.pub_revenue_type_ &&
      request_type_ == data.request_type_;
  }

  const SecondsTimestamp& time() const
  {
    return time_;
  }

  const RequestId& request_id() const
  {
    return request_id_;
  }

  const UserId& user_id() const
  {
    return user_id_;
  }

  const OptionalFixedNumber& pub_revenue() const
  {
    return pub_revenue_;
  }

  const OptionalFixedNumber& pub_sys_revenue() const
  {
    return pub_sys_revenue_;
  }

  char pub_revenue_type() const
  {
    return pub_revenue_type_;
  }

  char request_type() const
  {
    return request_type_;
  }

  unsigned long distrib_hash() const
  {
    return distribution_hash_;
  }

  void invariant() const /*throw(ConstraintViolation)*/
  {
    if (!pub_revenue_type_is_valid_())
    {
      Stream::Error es;
      es << "ImpressionData_V_3_3::invariant(): pub_revenue_type_ "
        "has invalid value '" << pub_revenue_type_ << '\'';
      throw ConstraintViolation(es);
    }
    if (!request_type_is_valid_())
    {
      Stream::Error es;
      es << "ImpressionData_V_3_3::invariant(): request_type_ "
        "has invalid value '" << request_type_ << '\'';
      throw ConstraintViolation(es);
    }
  }

private:
  bool pub_revenue_type_is_valid_() const
  {
    return pub_revenue_type_ == 'A' || pub_revenue_type_ == 'P';
  }

  bool request_type_is_valid_() const
  {
    return request_type_ == 'N' || request_type_ == 'T';
  }

  friend FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, ImpressionData_V_3_3& data)
    /*throw(eh::Exception)*/;

  SecondsTimestamp time_;
  RequestId request_id_;
  UserIdIoWrapper user_id_;
  OptionalFixedNumber pub_revenue_;
  OptionalFixedNumber pub_sys_revenue_;
  char pub_revenue_type_;
  char request_type_;
  unsigned long distribution_hash_;
};

class ImpressionData_V_3_3_1
{
public:
  ImpressionData_V_3_3_1()
  {
  }

  ImpressionData_V_3_3_1(
    const Generics::Time& time,
    const AdServer::Commons::RequestId& request_id,
    const AdServer::Commons::UserId& user_id,
    const StringIoWrapperOptional& referrer,
    const OptionalFixedNumber& pub_revenue,
    const OptionalFixedNumber& pub_sys_revenue,
    char pub_revenue_type,
    char request_type,
    const String::SubString& action_name,
    const AdServer::Commons::Optional<unsigned long>& user_id_hash_mod
  )
  :
    time_(time),
    request_id_(request_id),
    user_id_(user_id),
    referrer_(referrer),
    pub_revenue_(pub_revenue),
    pub_sys_revenue_(pub_sys_revenue),
    pub_revenue_type_(pub_revenue_type),
    request_type_(request_type),
    action_name_(action_name.str()),
    distribution_hash_(request_distribution_hash(request_id, user_id_hash_mod))
  {
  }

  bool operator==(const ImpressionData_V_3_3_1& data) const
  {
    if (this == &data)
    {
      return true;
    }
    return time_ == data.time_ &&
      request_id_ == data.request_id_ &&
      user_id_ == data.user_id_ &&
      referrer_ == data.referrer_ &&
      pub_revenue_ == data.pub_revenue_ &&
      pub_sys_revenue_ == data.pub_sys_revenue_ &&
      pub_revenue_type_ == data.pub_revenue_type_ &&
      request_type_ == data.request_type_ &&
      action_name_.get() == data.action_name_.get();
  }

  const SecondsTimestamp& time() const
  {
    return time_;
  }

  const RequestId& request_id() const
  {
    return request_id_;
  }

  const UserId& user_id() const
  {
    return user_id_;
  }

  const StringIoWrapperOptional& referrer() const
  {
    return referrer_;
  }

  const OptionalFixedNumber& pub_revenue() const
  {
    return pub_revenue_;
  }

  const OptionalFixedNumber& pub_sys_revenue() const
  {
    return pub_sys_revenue_;
  }

  char pub_revenue_type() const
  {
    return pub_revenue_type_;
  }

  char request_type() const
  {
    return request_type_;
  }

  unsigned long distrib_hash() const
  {
    return distribution_hash_;
  }

  void invariant() const /*throw(ConstraintViolation)*/
  {
    if (!pub_revenue_type_is_valid_())
    {
      Stream::Error es;
      es << "ImpressionData_V_3_3_1::invariant(): pub_revenue_type_ "
        "has invalid value '" << pub_revenue_type_ << '\'';
      throw ConstraintViolation(es);
    }
    if (!request_type_is_valid_())
    {
      Stream::Error es;
      es << "ImpressionData_V_3_3_1::invariant(): request_type_ "
        "has invalid value '" << request_type_ << '\'';
      throw ConstraintViolation(es);
    }
  }

private:
  bool pub_revenue_type_is_valid_() const
  {
    return pub_revenue_type_ == 'A' || pub_revenue_type_ == 'P';
  }

  bool request_type_is_valid_() const
  {
    return request_type_ == 'N' ||
      request_type_ == 'T' ||
      request_type_ == 'C';
  }

  friend FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, ImpressionData_V_3_3_1& data)
    /*throw(eh::Exception)*/;

  friend std::ostream&
  operator<<(std::ostream& os, const ImpressionData_V_3_3_1& data)
    /*throw(eh::Exception)*/;

  SecondsTimestamp time_;
  RequestId request_id_;
  UserIdIoWrapper user_id_;
  StringIoWrapperOptional referrer_;
  OptionalFixedNumber pub_revenue_;
  OptionalFixedNumber pub_sys_revenue_;
  char pub_revenue_type_;
  char request_type_;
  EmptyHolder<Aux_::StringIoWrapper> action_name_;
  unsigned long distribution_hash_;
};

class ImpressionData
{
public:
  ImpressionData()
  {
  }

  ImpressionData(
    const Generics::Time& time,
    const AdServer::Commons::RequestId& request_id,
    const AdServer::Commons::UserId& user_id,
    const StringIoWrapperOptional& referrer,
    const OptionalFixedNumber& pub_revenue,
    const OptionalFixedNumber& pub_sys_revenue,
    char pub_revenue_type,
    char request_type,
    long viewability,
    const String::SubString& action_name,
    const AdServer::Commons::Optional<unsigned long>& user_id_hash_mod)
  :
    time_(time),
    request_id_(request_id),
    user_id_(user_id),
    referrer_(referrer),
    pub_revenue_(pub_revenue),
    pub_sys_revenue_(pub_sys_revenue),
    pub_revenue_type_(pub_revenue_type),
    request_type_(request_type),
    viewability_(viewability),
    action_name_(action_name.str()),
    distribution_hash_(request_distribution_hash(request_id, user_id_hash_mod))
  {}

  ImpressionData(
    const ImpressionData_V_1_0& data
  )
  :
    time_(data.time()),
    request_id_(data.request_id()),
    user_id_(),
    referrer_(), // NULL
    pub_revenue_(), // NULL
    pub_sys_revenue_(), // NULL
    pub_revenue_type_('A'),
    request_type_('T'),
    viewability_(-1),
    action_name_(),
    distribution_hash_(data.distrib_hash())
  {
  }

  ImpressionData(
    const ImpressionData_V_3_1& data
  )
  :
    time_(data.time()),
    request_id_(data.request_id()),
    user_id_(),
    referrer_(), // NULL
    pub_revenue_(data.pub_revenue()),
    pub_sys_revenue_(data.pub_sys_revenue()),
    pub_revenue_type_('A'),
    request_type_('T'),
    viewability_(-1),
    action_name_(),
    distribution_hash_(data.distrib_hash())
  {
  }

  ImpressionData(
    const ImpressionData_V_3_2& data
  )
  :
    time_(data.time()),
    request_id_(data.request_id()),
    user_id_(),
    referrer_(), // NULL
    pub_revenue_(data.pub_revenue()),
    pub_sys_revenue_(data.pub_sys_revenue()),
    pub_revenue_type_(data.pub_revenue_type()),
    request_type_('T'),
    viewability_(-1),
    action_name_(),
    distribution_hash_(data.distrib_hash())
  {
  }

  ImpressionData(
    const ImpressionData_V_3_3& data
  )
  :
    time_(data.time()),
    request_id_(data.request_id()),
    user_id_(data.user_id()),
    referrer_(), // NULL
    pub_revenue_(data.pub_revenue()),
    pub_sys_revenue_(data.pub_sys_revenue()),
    pub_revenue_type_(data.pub_revenue_type()),
    request_type_(data.request_type()),
    viewability_(-1),
    action_name_(),
    distribution_hash_(data.distrib_hash())
  {
  }

  ImpressionData(
    const ImpressionData_V_3_3_1& data
  )
  :
    time_(data.time()),
    request_id_(data.request_id()),
    user_id_(data.user_id()),
    referrer_(data.referrer()),
    pub_revenue_(data.pub_revenue()),
    pub_sys_revenue_(data.pub_sys_revenue()),
    pub_revenue_type_(data.pub_revenue_type()),
    request_type_(data.request_type()),
    viewability_(-1),
    distribution_hash_(data.distrib_hash())
  {
  }

  bool operator==(const ImpressionData& data) const
  {
    if (this == &data)
    {
      return true;
    }
    return time_ == data.time_ &&
      request_id_ == data.request_id_ &&
      user_id_ == data.user_id_ &&
      referrer_ == data.referrer_ &&
      pub_revenue_ == data.pub_revenue_ &&
      pub_sys_revenue_ == data.pub_sys_revenue_ &&
      pub_revenue_type_ == data.pub_revenue_type_ &&
      request_type_ == data.request_type_ &&
      action_name_.get() == data.action_name_.get();
  }

  const SecondsTimestamp& time() const
  {
    return time_;
  }

  const RequestId& request_id() const
  {
    return request_id_;
  }

  const UserId& user_id() const
  {
    return user_id_;
  }

  const StringIoWrapperOptional& referrer() const
  {
    return referrer_;
  }

  const OptionalFixedNumber& pub_revenue() const
  {
    return pub_revenue_;
  }

  const OptionalFixedNumber& pub_sys_revenue() const
  {
    return pub_sys_revenue_;
  }

  char pub_revenue_type() const
  {
    return pub_revenue_type_;
  }

  char request_type() const
  {
    return request_type_;
  }

  long viewability() const
  {
    return viewability_;
  }

  const std::string&
  action_name() const
  {
    return action_name_.get();
  }

  unsigned long distrib_hash() const
  {
    return distribution_hash_;
  }

  void invariant() const /*throw(ConstraintViolation)*/
  {
    if (!pub_revenue_type_is_valid_())
    {
      Stream::Error es;
      es << "ImpressionData::invariant(): pub_revenue_type_ "
        "has invalid value '" << pub_revenue_type_ << '\'';
      throw ConstraintViolation(es);
    }
    if (!request_type_is_valid_())
    {
      Stream::Error es;
      es << "ImpressionData::invariant(): request_type_ "
        "has invalid value '" << request_type_ << '\'';
      throw ConstraintViolation(es);
    }
  }

private:
  bool pub_revenue_type_is_valid_() const
  {
    return pub_revenue_type_ == 'A' || pub_revenue_type_ == 'P';
  }

  bool request_type_is_valid_() const
  {
    return request_type_ == 'N' ||
      request_type_ == 'T' ||
      request_type_ == 'C';
  }

  friend FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, ImpressionData& data)
    /*throw(eh::Exception)*/;

  friend std::ostream&
  operator<<(std::ostream& os, const ImpressionData& data)
    /*throw(eh::Exception)*/;

  SecondsTimestamp time_;
  RequestId request_id_;
  UserIdIoWrapper user_id_;
  StringIoWrapperOptional referrer_;
  OptionalFixedNumber pub_revenue_;
  OptionalFixedNumber pub_sys_revenue_;
  char pub_revenue_type_;
  char request_type_;
  long viewability_;
  EmptyHolder<Aux_::StringIoWrapper> action_name_;
  unsigned long distribution_hash_;
};

typedef SeqCollector<ImpressionData_V_1_0, true> ImpressionCollector_V_1_0;

typedef SeqCollector<ImpressionData_V_3_1, true> ImpressionCollector_V_3_1;

typedef SeqCollector<ImpressionData_V_3_2, true> ImpressionCollector_V_3_2;

typedef SeqCollector<ImpressionData_V_3_3, true> ImpressionCollector_V_3_3;

typedef SeqCollector<ImpressionData_V_3_3_1, true> ImpressionCollector_V_3_3_1;

typedef SeqCollector<ImpressionData, true> ImpressionCollector;

struct ImpressionTraits: LogDefaultTraits<ImpressionCollector, false, false, 1>
{
  template <class FUNCTOR_>
  static void for_each_old(FUNCTOR_& f) /*throw(eh::Exception)*/
  {
    f.template operator()<ImpressionCollector_V_1_0>("1.0");
    f.template operator()<ImpressionCollector_V_3_1>("3.1");
    f.template operator()<ImpressionCollector_V_3_2>("3.2");
    f.template operator()<ImpressionCollector_V_3_3>("3.3");
    f.template operator()<ImpressionCollector_V_3_3_1>("3.3.1");
  }

  typedef GenericLogIoHelperImpl<ImpressionTraits> IoHelperType;
};

typedef GenericAdRequestData PassbackImpressionData;

typedef SeqCollector<PassbackImpressionData, true> PassbackImpressionCollector;

struct PassbackImpressionTraits :
  LogDefaultTraits<PassbackImpressionCollector, false, false, 2>
{
};

} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_COMMONS_AD_REQUEST_LOGGER_HPP */

