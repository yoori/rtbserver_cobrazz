
#include "AdRequestLogger.hpp"
#include <LogCommons/LogCommons.ipp>

namespace AdServer {
namespace LogProcessing {

template <> const char*
AdvertiserActionTraits::B::base_name_ = "AdvertiserAction";
template <> const char*
AdvertiserActionTraits::B::signature_ = "AdvertiserAction";
template <> const char*
AdvertiserActionTraits::B::current_version_ = "3.6";

template <> const char*
ActionOpportunityTraits::B::base_name_ = "ActionOpportunity";
template <> const char*
ActionOpportunityTraits::B::signature_ = "ActionOpportunity";
template <> const char*
ActionOpportunityTraits::B::current_version_ = "1.0";

template <> const char* ClickTraits::B::base_name_ = "Click";
template <> const char* ClickTraits::B::signature_ = "Click";
template <> const char* ClickTraits::B::current_version_ = "3.3.1";

// For research logs next fields not used.
template <> const char*
ImpressionTraits::B::base_name_ = "Impression";
template <> const char*
ImpressionTraits::B::signature_ = "Impression";
template <> const char*
ImpressionTraits::B::current_version_ = "3.5";

template <> const char*
PassbackImpressionTraits::B::base_name_ = "PassbackImpression";
template <> const char*
PassbackImpressionTraits::B::signature_ = "PassbackImpression";
template <> const char*
PassbackImpressionTraits::B::current_version_ = "1.0";

template <> const char*
PassbackOpportunityTraits::B::base_name_ = "PassbackOpportunity";
template <> const char*
PassbackOpportunityTraits::B::signature_ = "PassbackOpportunity";
template <> const char*
PassbackOpportunityTraits::B::current_version_ = "1.0";

FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is,
    GenericAdRequestData& data)
  /*throw(eh::Exception)*/
{
  is >> data.time_;
  is >> data.request_id_;
  return is;
}

std::ostream&
operator<<(std::ostream& os, const GenericAdRequestData& data)
  /*throw(eh::Exception)*/
{
  return os << data.time_ << '\t' << data.request_id_;
}

FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is,
    AdvertiserActionData_V_3_3_1& data)
  /*throw(eh::Exception)*/
{
  is >> data.time_;
  is >> data.user_id_;
  is >> data.action_id_;
  is >> data.device_channel_id_;
  is >> data.action_request_id_;
  is >> data.ccg_ids_;
  is >> data.referrer_;
  is >> data.order_id_;
  is >> data.ip_address_;
  is >> data.cur_value_;
  return is;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, AdvertiserActionData& data)
  /*throw(eh::Exception)*/
{
  is >> data.time_;
  is >> data.user_id_;
  is >> data.request_id_;
  is >> data.action_id_;
  is >> data.device_channel_id_;
  is >> data.action_request_id_;
  is >> data.ccg_ids_;
  is >> data.referrer_;
  is >> data.order_id_;
  is >> data.ip_address_;
  is >> data.cur_value_;
  return is;
}

std::ostream&
operator<<(std::ostream& os, const AdvertiserActionData& data)
  /*throw(eh::Exception)*/
{
  os << data.time_ << '\t';
  os << data.user_id_ << '\t';
  os << data.request_id_ << '\t';
  os << data.action_id_ << '\t';
  os << data.device_channel_id_ << '\t';
  os << data.action_request_id_ << '\t';
  os << data.ccg_ids_ << '\t';
  os << data.referrer_ << '\t';
  os << data.order_id_ << '\t';
  os << data.ip_address_ << '\t';
  os << data.cur_value_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, ActionOpportunityData& data)
  /*throw(eh::Exception)*/
{
  is >> data.time;
  is >> data.user_id;
  is >> data.cid;
  is >> data.request_id;
  return is;
}

std::ostream&
operator<<(std::ostream& os, const ActionOpportunityData& data)
  /*throw(eh::Exception)*/
{
  return os << data.time << '\t'
            << data.user_id << '\t'
            << data.cid << '\t'
            << data.request_id;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, PassbackOpportunityData& data)
  /*throw(eh::Exception)*/
{
  TokenizerInputArchive<> ia(is);
  ia >> data;
  return is;
}

std::ostream&
operator<<(std::ostream& os, const PassbackOpportunityData& data)
  /*throw(eh::Exception)*/
{
  TabOutputArchive oa(os);
  oa << data;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, ClickData& data)
  /*throw(eh::Exception)*/
{
  is >> data.time_;
  is >> data.request_id_;
  is >> data.referrer_;
  return is;
}

std::ostream&
operator<<(std::ostream& os, const ClickData& data)
  /*throw(eh::Exception)*/
{
  return os << data.time_ << '\t'
            << data.request_id_ << '\t'
            << data.referrer_;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, ImpressionData_V_3_1& data)
  /*throw(eh::Exception)*/
{
  is >> data.time_;
  is >> data.request_id_;
  is >> data.pub_revenue_;
  is >> data.pub_sys_revenue_;
  return is;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, ImpressionData_V_3_2& data)
  /*throw(eh::Exception)*/
{
  is >> data.time_;
  is >> data.request_id_;
  is >> data.pub_revenue_;
  is >> data.pub_sys_revenue_;
  is >> data.pub_revenue_type_;
  data.invariant();
  return is;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, ImpressionData_V_3_3& data)
  /*throw(eh::Exception)*/
{
  is >> data.time_;
  is >> data.request_id_;
  is >> data.user_id_;
  is >> data.pub_revenue_;
  is >> data.pub_sys_revenue_;
  is >> data.pub_revenue_type_;
  is >> data.request_type_;
  data.invariant();
  return is;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, ImpressionData_V_3_3_1& data)
  /*throw(eh::Exception)*/
{
  is >> data.time_;
  is >> data.request_id_;
  is >> data.user_id_;
  is >> data.referrer_;
  is >> data.pub_revenue_;
  is >> data.pub_sys_revenue_;
  is >> data.pub_revenue_type_;
  is >> data.request_type_;
  is >> data.action_name_;
  data.invariant();
  return is;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, ImpressionData& data)
  /*throw(eh::Exception)*/
{
  is >> data.time_;
  is >> data.request_id_;
  is >> data.user_id_;
  is >> data.referrer_;
  is >> data.pub_revenue_;
  is >> data.pub_sys_revenue_;
  is >> data.pub_revenue_type_;
  is >> data.request_type_;
  is >> data.viewability_;
  is >> data.action_name_;
  data.invariant();
  return is;
}

std::ostream&
operator<<(std::ostream& os, const ImpressionData& data)
  /*throw(eh::Exception)*/
{
  data.invariant();
  return os << data.time_ << '\t' <<
    data.request_id_ << '\t' <<
    data.user_id_ << '\t' <<
    data.referrer_ << '\t' <<
    data.pub_revenue_ << '\t' <<
    data.pub_sys_revenue_ << '\t' <<
    data.pub_revenue_type_ << '\t' <<
    data.request_type_ << '\t' <<
    data.viewability_ << '\t' <<
    data.action_name_;
}

} // namespace LogProcessing
} // namespace AdServer

