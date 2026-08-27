#pragma once

#include <Commons/Coro/StartableAwaitable.hpp>

#include "InventoryActionProcessor.hpp"

namespace AdServer::RequestInfoSvcs
{
  class ConsiderInterface
  {
  public:
    virtual
    ~ConsiderInterface() noexcept
    {}

    virtual AdServer::Commons::StartableAwaitable<void>
    co_consider_click(
      const AdServer::Commons::UserId& user_id,
      const AdServer::Commons::RequestId& request_id,
      const Generics::Time& time) = 0;

    virtual AdServer::Commons::StartableAwaitable<void>
    co_consider_impression(
      const AdServer::Commons::UserId& user_id,
      const AdServer::Commons::RequestId& request_id,
      const Generics::Time& time,
      const ChannelIdSet& channels) = 0;
  };
}
