#include <eh/Exception.hpp>
#include <ReferenceCounting/DefaultImpl.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>

#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/ServantImpl.hpp>
#include <ChannelSvcs/ChannelCommons/ChannelServer_s.hpp>

#include "ChannelServerControlImpl.hpp"
#include "ChannelServerImpl.hpp"

namespace AdServer
{
namespace ChannelSvcs
{
  /**
   * Implementation of control part ChannelServer
   */
  ChannelServerControlImpl::ChannelServerControlImpl(
      ChannelServerCustomImpl* custom) noexcept
      : custom_impl_(ReferenceCounting::add_ref(custom))
  {
  }

  //
  // IDL:AdServer/ChannelSvcs/ChannelServerControl/set_sources:1.0
  //
  void ChannelServerControlImpl::set_sources(
      const ::AdServer::ChannelSvcs::ChannelServerControl::DBSourceInfo& db_info,
      const ::AdServer::ChannelSvcs::ChunkKeySeq& sources)
      /*throw(AdServer::ChannelSvcs::ImplementationException)*/
  {
    WriteGuard_ guard(lock_);
    custom_impl_->set_sources(db_info, sources);
  }


  //
  // IDL:AdServer/ChannelSvcs/ChannelServerControl/set_proxy_sources:1.0
  //
  void ChannelServerControlImpl::set_proxy_sources(
      const ::AdServer::ChannelSvcs::
      ChannelServerControl::ProxySourceInfo& proxy_info,
      const ::AdServer::ChannelSvcs::ChunkKeySeq& sources)
      /*throw(AdServer::ChannelSvcs::ImplementationException)*/
  {
    custom_impl_->set_proxy_sources(proxy_info, sources);
  }

  //
  // IDL:AdServer/ChannelSvcs/ChannelServerControl/check_configuration:1.0
  //
  ::CORBA::ULong ChannelServerControlImpl::check_configuration()
    noexcept
  {
    return custom_impl_->check_configuration();
  }

  ChannelServerControlImpl::~ChannelServerControlImpl() noexcept
  {
  }

} /* ChannelSvcs */
} /* AdServer */


