#ifndef REQUESTINFOSVCS_EXPRESSIONMATCHER_CONSIDERINTERFACE_HPP_
#define REQUESTINFOSVCS_EXPRESSIONMATCHER_CONSIDERINTERFACE_HPP_

#include "InventoryActionProcessor.hpp"

namespace AdServer
{
  namespace RequestInfoSvcs
  {
    class ConsiderInterface
    {
    public:
      virtual
      ~ConsiderInterface() noexcept
      {}

      virtual void
      consider_click(
        const AdServer::Commons::RequestId& request_id,
        const Generics::Time& time)
        noexcept = 0;

      virtual void
      consider_impression(
        const AdServer::Commons::UserId& user_id,
        const AdServer::Commons::RequestId& request_id,
        const Generics::Time& time,
        const ChannelIdSet& channels)
        noexcept = 0;
    };
  }
}

#endif /* REQUESTINFOSVCS_EXPRESSIONMATCHER_CONSIDERINTERFACE_HPP_ */
