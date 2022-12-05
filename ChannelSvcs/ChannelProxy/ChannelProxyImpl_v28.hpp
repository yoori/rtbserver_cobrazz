#ifndef AD_SERVER_CHANNEL_PROXY_IMPL_V28_HPP_
#define AD_SERVER_CHANNEL_PROXY_IMPL_V28_HPP_

#include <set>
#include <vector>

#include <ReferenceCounting/ReferenceCounting.hpp>

#include <eh/Exception.hpp>

#include <Logger/Logger.hpp>
#include <Generics/ActiveObject.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/Time.hpp>

#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/ServantImpl.hpp>
#include <CORBACommons/ObjectPool.hpp>

#include <ChannelSvcs/ChannelProxy/ChannelProxy_v28_s.hpp>
#include "ChannelProxyImpl.hpp"


namespace AdServer
{
  namespace ChannelSvcs
  {
    /**
     * Implementation of common part ChannelProxy
     */
    class ChannelProxyImpl_v28:
      public virtual CORBACommons::ReferenceCounting::ServantImpl
       <POA_AdServer::ChannelSvcs::ChannelProxy_v28>
    {
    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      //typedef
      //  CORBACommons::ObjectPoolRefConfiguration LoadSessionPoolConfig;

      ChannelProxyImpl_v28(ChannelProxyImpl* impl) noexcept;

      virtual ~ChannelProxyImpl_v28() noexcept;

    public:
      //
      // IDL:AdServer/ChannelSvcs/ChannelUpdate_v28
      ///check:1.0
      //
      virtual void check(
      const ::AdServer::ChannelSvcs::ChannelUpdate_v28::CheckQuery& query,
      ::AdServer::ChannelSvcs::ChannelUpdate_v28::CheckData_out data) 
      /*throw(AdServer::ChannelSvcs::ImplementationException,
        AdServer::ChannelSvcs::NotConfigured)*/;

      //
      // IDL:AdServer/ChannelSvcs/ChannelServerUpdate/update_triggers:1.0
      //
      virtual void update_triggers(
        const ::AdServer::ChannelSvcs::ChannelIdSeq& ids,
        ::AdServer::ChannelSvcs::ChannelUpdate_v28::UpdateData_out result)
        /*throw(
          AdServer::ChannelSvcs::ImplementationException,
          AdServer::ChannelSvcs::NotConfigured)*/;

      virtual void update_all_ccg(
        const AdServer::ChannelSvcs::ChannelUpdate_v28::CCGQuery& in,
        AdServer::ChannelSvcs::ChannelUpdate_v28::PosCCGResult_out result)
        /*throw(AdServer::ChannelSvcs::ImplementationException,
          AdServer::ChannelSvcs::NotConfigured)*/;

      //
      // IDL:AdServer/ChannelSvcs/ChannelProxy/get_count_chunks:1.0
      //
      virtual ::CORBA::ULong get_count_chunks()
        /*throw(AdServer::ChannelSvcs::ImplementationException)*/;

      Logging::Logger* logger() const noexcept;
      
    protected:

      ChannelProxyImpl_var impl_;
    };

    typedef ReferenceCounting::SmartPtr<ChannelProxyImpl_v28>
        ChannelProxyImpl_v28_var;

  } /* ChannelSvcs */
} /* AdServer */

namespace AdServer
{
  namespace ChannelSvcs
  {
    inline
    Logging::Logger*
    ChannelProxyImpl_v28::logger() const noexcept
    {
      return impl_->logger();
    }
  } /* ChannelSvcs */
} /* AdServer */


#endif /*AD_SERVER_CHANNEL_PROXY_IMPL_V28_HPP_*/
