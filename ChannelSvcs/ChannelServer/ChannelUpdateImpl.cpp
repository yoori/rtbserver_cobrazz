#include <eh/Exception.hpp>
#include <ReferenceCounting/DefaultImpl.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <Logger/Logger.hpp>

#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/ServantImpl.hpp>

#include <ChannelSvcs/ChannelCommons/ChannelServer_s.hpp>
#include "ChannelServerImpl.hpp"
#include "ChannelUpdateImpl.hpp"

namespace AdServer
{
namespace ChannelSvcs
{
  ChannelUpdateImpl::ChannelUpdateImpl(ChannelServerCustomImpl* server)
    /*throw(eh::Exception)*/
    : server_(ReferenceCounting::add_ref(server))
  {
  }

  ChannelUpdateImpl::~ChannelUpdateImpl() noexcept
  {
  }

  //
  // IDL:AdServer/ChannelSvcs/ChannelCurrent/check:1.0
  //
  void ChannelUpdateImpl::check(
    const ::AdServer::ChannelSvcs::ChannelCurrent::CheckQuery& query,
    ::AdServer::ChannelSvcs::ChannelCurrent::CheckData_out data)
    /*throw(AdServer::ChannelSvcs::ImplementationException,
      AdServer::ChannelSvcs::NotConfigured)*/
  {
    server_->check(query, data);
  }

  //
  // IDL:AdServer/ChannelSvcs/ChannelServerControl/update_channels:1.0
  //
  void ChannelUpdateImpl::update_triggers(
    const ::AdServer::ChannelSvcs::ChannelIdSeq& ids,
    ::AdServer::ChannelSvcs::ChannelCurrent::UpdateData_out result)
    /*throw(AdServer::ChannelSvcs::ImplementationException,
      AdServer::ChannelSvcs::NotConfigured)*/
  {
    server_->update_triggers(ids, result);
  }

  //
  // IDL:AdServer/ChannelSvcs/ChannelServerControl/update_all_ccg:1.0
  //
  void ChannelUpdateImpl::update_all_ccg(
    const AdServer::ChannelSvcs::ChannelCurrent::CCGQuery& query,
    AdServer::ChannelSvcs::ChannelCurrent::PosCCGResult_out result)
    /*throw(AdServer::ChannelSvcs::ImplementationException,
      AdServer::ChannelSvcs::NotConfigured)*/
  {
    server_->update_all_ccg(query, result);
  }

  //
  // IDL:AdServer/ChannelSvcs/ChannelProxy/get_count_chunks:1.0
  //
  ::CORBA::ULong ChannelUpdateImpl::get_count_chunks()
    /*throw(AdServer::ChannelSvcs::ImplementationException)*/
  {
    return server_->get_count_chunks();
  }
}
}

