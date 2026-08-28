
#include "ExpressionPerformance.hpp"
#include <LogCommons/BufferWriter.hpp>
#include <LogCommons/LogCommons.ipp>

namespace AdServer::LogProcessing
{

  template <> const char* ExpressionPerformanceTraits::B::base_name_ = "ExpressionPerformance";
  template <> const char* ExpressionPerformanceTraits::B::signature_ = "ExpressionPerformance";
  template <> const char* ExpressionPerformanceTraits::B::current_version_ = "1.1";

  std::istream&
  operator>>(std::istream& is, ExpressionPerformanceKey& key)
  {
    is >> key.sdate_;
    read_eol(is);
    is >> key.colo_id_;
    key.calc_hash_();
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const ExpressionPerformanceKey& key)
    /*throw(eh::Exception)*/
  {
    out << key.sdate_ << '\n' << key.colo_id_;
    return out;
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

  BufferWriter&
  operator<<(BufferWriter& out, const ExpressionPerformanceInnerKey& key)
    /*throw(eh::Exception)*/
  {
    key.invariant();
    out << key.cc_id_ << '\t';
    out << Aux_::StringIoWrapper(key.expression_);
    return out;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, ExpressionPerformanceInnerData& data)
  {
    is >> data.imps_verified_;
    is >> data.clicks_;
    is >> data.actions_;
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const ExpressionPerformanceInnerData& data)
  {
    out << data.imps_verified_ << '\t';
    out << data.clicks_ << '\t';
    out << data.actions_;
    return out;
  }

} // namespace AdServer::LogProcessing
