#include "PostClickAction.hpp"

#include <LogCommons/BufferWriter.hpp>
#include <LogCommons/LogCommons.ipp>

namespace AdServer::LogProcessing
{
  template <> const char* PostClickActionTraits::B::base_name_ = "PostClickAction";
  template <> const char* PostClickActionTraits::B::signature_ = "PostClickAction";
  template <> const char* PostClickActionTraits::B::current_version_ = "1.0";

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, PostClickActionData& data)
  {
    is >> data.time_;
    is >> data.request_id_;
    is >> data.action_name_;
    is >> data.action_value_;
    data.distribution_hash_ = request_distribution_hash(
      data.request_id_,
      AdServer::Commons::UserId());
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const PostClickActionData& data)
  {
    return out << data.time_ << '\t' << data.request_id_ << '\t' <<
      data.action_name_ << '\t' << data.action_value_;
  }
}
