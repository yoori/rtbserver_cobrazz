#pragma once

#include <string>

#include <LogCommons/GenericLogIoImpl.hpp>
#include <LogCommons/LogCommons.hpp>
#include <LogCommons/StatCollector.hpp>

namespace AdServer::LogProcessing
{
  class PostClickActionData
  {
  public:
    PostClickActionData() = default;

    PostClickActionData(
      const Generics::Time& time,
      const AdServer::Commons::RequestId& request_id,
      std::string action_name,
      std::string action_value)
      : time_(time),
        request_id_(request_id),
        action_name_(std::move(action_name)),
        action_value_(std::move(action_value)),
        distribution_hash_(request_distribution_hash(request_id, AdServer::Commons::UserId()))
    {}

    const SecondsTimestamp& time() const noexcept
    {
      return time_;
    }

    const RequestId& request_id() const noexcept
    {
      return request_id_;
    }

    const std::string& action_name() const noexcept
    {
      return action_name_.get();
    }

    const std::string& action_value() const noexcept
    {
      return action_value_.get();
    }

    unsigned long distrib_hash() const noexcept
    {
      return distribution_hash_;
    }

  private:
    friend FixedBufStream<TabCategory>&
    operator>>(FixedBufStream<TabCategory>& is, PostClickActionData& data);

    friend BufferWriter&
    operator<<(BufferWriter& out, const PostClickActionData& data);

    SecondsTimestamp time_;
    RequestIdIoWrapper request_id_;
    EmptyHolder<Aux_::StringIoWrapper> action_name_;
    EmptyHolder<Aux_::StringIoWrapper> action_value_;
    unsigned long distribution_hash_ = 0;
  };

  using PostClickActionCollector = SeqCollector<PostClickActionData, true>;

  struct PostClickActionTraits:
    LogDefaultTraits<PostClickActionCollector, false, false, 1>
  {
    using IoHelperType = GenericLogIoHelperImpl<PostClickActionTraits>;
  };
}
