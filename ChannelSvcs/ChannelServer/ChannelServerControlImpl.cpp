#include <eh/Exception.hpp>
#include <utility>
#include <vector>

#include <CORBACommons/CorbaAdapters.hpp>
#include <ChannelSvcs/ChannelCommons/ChannelServer_s.hpp>

#include "ChannelServerControlImpl.hpp"
#include "ChannelServerCore.hpp"

namespace AdServer
{
namespace ChannelSvcs
{
  namespace
  {
    std::vector<unsigned long>
    unpack_sources(
      const ::AdServer::ChannelSvcs::ChunkKeySeq& sources)
    {
      std::vector<unsigned long> result;
      result.reserve(sources.length());
      for(CORBA::ULong i = 0; i < sources.length(); ++i)
      {
        result.push_back(sources[i]);
      }
      return result;
    }

    std::vector<ChannelServerCore::DBSourceInfo::ObjectRef>
    unpack_refs(
      const ChannelServerControl::CorbaObjectRefDefSeq& refs)
    {
      std::vector<ChannelServerCore::DBSourceInfo::ObjectRef> result;
      result.reserve(refs.length());
      for(CORBA::ULong i = 0; i < refs.length(); ++i)
      {
        CORBACommons::CorbaObjectRef ref;
        ref.load(refs[i]);
        result.push_back(ref.object_ref);
      }
      return result;
    }

  }

  /**
   * Implementation of control part ChannelServer
   */
  ChannelServerControlImpl::ChannelServerControlImpl(
      ChannelServerCorePtr custom) noexcept
      : custom_impl_(std::move(custom))
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
    try
    {
      ChannelServerCore::DBSourceInfo core_db_info;
      core_db_info.pg_connection = db_info.pg_connection.in();
      core_db_info.colo = db_info.colo;
      core_db_info.version = db_info.version.in();
      core_db_info.count_chunks = db_info.count_chunks;
      core_db_info.campaign_refs = unpack_refs(db_info.campaign_refs);
      core_db_info.check_sum = db_info.check_sum;

      custom_impl_->set_sources(core_db_info, unpack_sources(sources));
    }
    catch(const eh::Exception& ex)
    {
      CORBACommons::throw_desc<ImplementationException>(
        String::SubString(ex.what()));
    }
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
    WriteGuard_ guard(lock_);
    try
    {
      ChannelServerCore::ProxySourceInfo core_proxy_info;
      core_proxy_info.local_descriptor = proxy_info.local_descriptor;
      core_proxy_info.count_chunks = proxy_info.count_chunks;
      core_proxy_info.colo = proxy_info.colo;
      core_proxy_info.version = proxy_info.version.in();
      core_proxy_info.check_sum = proxy_info.check_sum;
      core_proxy_info.proxy_refs = unpack_refs(proxy_info.proxy_refs);
      core_proxy_info.campaign_refs = unpack_refs(proxy_info.campaign_refs);

      custom_impl_->set_proxy_sources(
        core_proxy_info,
        unpack_sources(sources));
    }
    catch(const eh::Exception& ex)
    {
      CORBACommons::throw_desc<ImplementationException>(
        String::SubString(ex.what()));
    }
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
