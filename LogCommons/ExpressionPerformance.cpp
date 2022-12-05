
#include "ExpressionPerformance.hpp"
#include <LogCommons/LogCommons.ipp>

namespace AdServer {
namespace LogProcessing {

template <> const char* ExpressionPerformanceTraits::B::base_name_ =
  "ExpressionPerformance";
template <> const char* ExpressionPerformanceTraits::B::signature_ =
  "ExpressionPerformance";
template <> const char* ExpressionPerformanceTraits::B::current_version_ =
  "1.1";

std::istream&
operator>>(std::istream& is, ExpressionPerformanceKey& key)
{
  is >> key.sdate_;
  read_eol(is);
  is >> key.colo_id_;
  key.calc_hash_();
  return is;
}

std::ostream&
operator<<(std::ostream& os, const ExpressionPerformanceKey& key)
  /*throw(eh::Exception)*/
{
  os << key.sdate_ << '\n' << key.colo_id_;
  return os;
}

FixedBufStream<TabCategory>&
operator>>(FixedBufStream<TabCategory>& is, ExpressionPerformanceInnerKey& key)
{
  is >> key.cc_id_;
  Aux_::StringIoWrapper expression_wrapper;
  is >> expression_wrapper;
  key.expression_ = expression_wrapper;
  key.invariant();
  key.calc_hash_();
  return is;
}

std::ostream&
operator<<(std::ostream& os, const ExpressionPerformanceInnerKey& key)
  /*throw(eh::Exception)*/
{
  key.invariant();
  os << key.cc_id_ << '\t';
  os << Aux_::StringIoWrapper(key.expression_);
  return os;
}

FixedBufStream<TabCategory>&
operator>>(
  FixedBufStream<TabCategory>& is,
  ExpressionPerformanceInnerData& data
)
{
  is >> data.imps_verified_;
  is >> data.clicks_;
  is >> data.actions_;
  return is;
}

std::ostream&
operator<<(std::ostream& os, const ExpressionPerformanceInnerData& data)
{
  os << data.imps_verified_ << '\t';
  os << data.clicks_ << '\t';
  os << data.actions_;
  return os;
}

} // namespace LogProcessing
} // namespace AdServer

