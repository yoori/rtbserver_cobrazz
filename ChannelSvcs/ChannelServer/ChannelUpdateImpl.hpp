#pragma once

#include <memory>

#include <eh/Exception.hpp>
#include <ReferenceCounting/DefaultImpl.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <Logger/Logger.hpp>
#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/ServantImpl.hpp>
#include <ChannelSvcs/ChannelCommons/ChannelUpdateBase_s.hpp>
#include <ChannelSvcs/ChannelCommons/ChannelServer.hpp>

#include "ChannelServerCore.hpp"

namespace AdServer
{
namespace ChannelSvcs
{
  typedef ChannelUpdateBase ChannelCurrent;

  class ChannelUpdateImpl:
    public virtual CORBACommons::ReferenceCounting::ServantImpl
      <POA_AdServer::ChannelSvcs::ChannelUpdate>
  {
  public:

    ChannelUpdateImpl(ChannelServerCorePtr server)
      /*throw(eh::Exception)*/;

    virtual ~ChannelUpdateImpl() noexcept;

  public:

    //
    // IDL:AdServer/ChannelSvcs/ChannelCurrent/check:1.0
    //
    virtual void check(
      const ::AdServer::ChannelSvcs::ChannelCurrent::CheckQuery& query,
      ::AdServer::ChannelSvcs::ChannelCurrent::CheckData_out data)
      /*throw(AdServer::ChannelSvcs::ImplementationException,
        AdServer::ChannelSvcs::NotConfigured)*/;

    //
    // IDL:AdServer/ChannelSvcs/ChannelServerControl/update_triggers:1.0
    //
    virtual void update_triggers(
      const ::AdServer::ChannelSvcs::ChannelIdSeq& ids,
      ::AdServer::ChannelSvcs::ChannelCurrent::UpdateData_out result)
      /*throw(AdServer::ChannelSvcs::ImplementationException,
        AdServer::ChannelSvcs::NotConfigured)*/;

    virtual ::CORBA::ULong get_count_chunks()
      /*throw(AdServer::ChannelSvcs::ImplementationException)*/;

    //
    // IDL:AdServer/ChannelSvcs/ChannelServerControl/update_all_ccg:1.0
    //
    virtual void update_all_ccg(
      const AdServer::ChannelSvcs::ChannelCurrent::CCGQuery& query,
      AdServer::ChannelSvcs::ChannelCurrent::PosCCGResult_out result)
      /*throw(AdServer::ChannelSvcs::ImplementationException,
        AdServer::ChannelSvcs::NotConfigured)*/;

  private:
    ChannelServerCorePtr server_;
  };

  typedef ReferenceCounting::SmartPtr<ChannelUpdateImpl> ChannelUpdateImpl_var;
}
}
