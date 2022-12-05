#ifndef AD_SERVER_CHANNEL_SERVER_CONTROL_IMPL_HPP_
#define AD_SERVER_CHANNEL_SERVER_CONTROL_IMPL_HPP_


#include <ReferenceCounting/DefaultImpl.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <eh/Exception.hpp>
#include <Logger/Logger.hpp>
#include <Generics/ActiveObject.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/Time.hpp>
#include <ChannelSvcs/ChannelCommons/ChannelServer_s.hpp>
#include "ChannelServerImpl.hpp"

namespace AdServer
{
namespace ChannelSvcs
{
  /**
   * Implementation of common part ChannelServer
   */

  class ChannelServerControlImpl:
    public virtual CORBACommons::ReferenceCounting::ServantImpl
      <POA_AdServer::ChannelSvcs::ChannelServerControl>
  {

  public:

    ChannelServerControlImpl(ChannelServerCustomImpl* custom) noexcept;

  protected:
    virtual ~ChannelServerControlImpl() noexcept;
  public:

    //
    // IDL:AdServer/ChannelSvcs/ChannelServerControl/set_sources:1.0
    //
    virtual void set_sources(const ::AdServer::ChannelSvcs::
      ChannelServerControl::DBSourceInfo& db_info,
      const ::AdServer::ChannelSvcs::ChunkKeySeq& sources)
      /*throw(AdServer::ChannelSvcs::ImplementationException)*/;

    //
    // IDL:AdServer/ChannelSvcs/ChannelServerControl/set_proxy_sources:1.0
    //
    virtual void set_proxy_sources(const ::AdServer::ChannelSvcs::
      ChannelServerControl::ProxySourceInfo& poxy_info,
      const ::AdServer::ChannelSvcs::ChunkKeySeq& sources)
      /*throw(AdServer::ChannelSvcs::ImplementationException)*/;

    //
    // IDL:AdServer/ChannelSvcs/ChannelServerControl/get_queries_counter:1.0
    //
    virtual ::CORBA::ULong check_configuration() noexcept;

  protected:

    typedef Sync::PosixRWLock Mutex_;
    typedef Sync::PosixRGuard ReadGuard_;
    typedef Sync::PosixWGuard WriteGuard_;

    mutable Mutex_ lock_;
  private:

    ChannelServerCustomImpl_var custom_impl_;
  };

  typedef ReferenceCounting::SmartPtr<ChannelServerControlImpl>
    ChannelServerControlImpl_var;

} /* ChannelSvcs */
} /* AdServer */

#endif /*AD_SERVER_CHANNEL_SERVER_CONTROL_IMPL_HPP_*/

