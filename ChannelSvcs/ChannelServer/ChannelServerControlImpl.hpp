#pragma once


#include <memory>
#include <mutex>
#include <shared_mutex>

#include <ReferenceCounting/DefaultImpl.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <eh/Exception.hpp>
#include <Logger/Logger.hpp>
#include <Generics/ActiveObject.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/Time.hpp>
#include <ChannelSvcs/ChannelCommons/ChannelServer_s.hpp>
#include "ChannelServerCore.hpp"

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

    ChannelServerControlImpl(ChannelServerCorePtr custom) noexcept;

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

    typedef std::shared_mutex Mutex_;
    typedef std::shared_lock<Mutex_> ReadGuard_;
    typedef std::unique_lock<Mutex_> WriteGuard_;

    mutable Mutex_ lock_;
  private:

    ChannelServerCorePtr custom_impl_;
  };

  typedef ReferenceCounting::SmartPtr<ChannelServerControlImpl>
    ChannelServerControlImpl_var;

} /* ChannelSvcs */
} /* AdServer */
