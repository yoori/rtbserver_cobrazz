
#ifndef REQUESTINFOSVCS_EXPRESSIONMATCHER_CONSIDERREVENUEINTERFACE_HPP_
#define REQUESTINFOSVCS_EXPRESSIONMATCHER_CONSIDERREVENUEINTERFACE_HPP_

#include "InventoryActionProcessor.hpp"

namespace AdServer
{
  namespace RequestInfoSvcs
  {
    class ConsiderRevenueInterface
    {
    public:
      virtual
      ~ConsiderRevenueInterface() noexcept
      {}

      virtual void
      consider_action(
        const AdServer::Commons::UserId& user_id,
        const Generics::Time& time,
        const RevenueDecimal& revenue)
        noexcept = 0;

      virtual void
      consider_click(
        const AdServer::Commons::UserId& user_id,
        const AdServer::Commons::RequestId& request_id,
        const Generics::Time& time,
        const RevenueDecimal& revenue)
        noexcept = 0;

      virtual void
      consider_impression(
        const AdServer::Commons::UserId& user_id,
        const AdServer::Commons::RequestId& request_id,
        const Generics::Time& time,
        const RevenueDecimal& revenue,
        const ChannelIdSet& channels)
        noexcept = 0;

      virtual void
      consider_request(
        const AdServer::Commons::UserId& user_id,
        const Generics::Time& time)
        noexcept = 0;
    };
  }
}

#endif /* REQUESTINFOSVCS_EXPRESSIONMATCHER_CONSIDERREVENUEINTERFACE_HPP_ */
