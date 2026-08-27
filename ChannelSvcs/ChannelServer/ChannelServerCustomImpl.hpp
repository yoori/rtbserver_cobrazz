#pragma once

#include <memory>

#include <CORBACommons/ServantImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

#include <ChannelSvcs/ChannelCommons/ChannelServer_s.hpp>

#include "ChannelServerCore.hpp"

namespace AdServer::ChannelSvcs
{
  class ChannelServerCustomImpl:
    public virtual CORBACommons::ReferenceCounting::ServantImpl
      <POA_AdServer::ChannelSvcs::ChannelServer>
  {
  public:
    explicit ChannelServerCustomImpl(ChannelServerCorePtr core) noexcept;

    void match(
      const ::AdServer::ChannelSvcs::ChannelServerBase::MatchQuery& query,
      ::AdServer::ChannelSvcs::ChannelServer::MatchResult_out result) override;

    void get_ccg_traits(
      const ::AdServer::ChannelSvcs::ChannelIdSeq& query,
      ::AdServer::ChannelSvcs::ChannelServer::TraitsResult_out result) override;

  protected:
    ~ChannelServerCustomImpl() noexcept override;

  private:
    ChannelServerCorePtr core_;
  };

  typedef ReferenceCounting::SmartPtr<ChannelServerCustomImpl>
    ChannelServerCustomImpl_var;
}
