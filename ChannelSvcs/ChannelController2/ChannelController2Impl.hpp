#pragma once

#include <string>
#include <vector>

#include <eh/Exception.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Logger/Logger.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

#include <xsd/ChannelSvcs/ChannelController2Config.hpp>

#include "ChannelControllerGrpc.pb.h"

namespace AdServer::ChannelSvcs
{
  class ChannelController2Impl:
    public Generics::CompositeActiveObject,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(NotReady, Exception);

    using ChannelControllerConfig =
      xsd::AdServer::Configuration::ChannelController2ConfigType;
    using SessionDescription =
      adserver::channel_svcs::channel_controller::GetSessionDescriptionResponse;

    ChannelController2Impl(
      Generics::ActiveObjectCallback* callback,
      Logging::Logger* logger,
      const ChannelControllerConfig& config);

    void fill_session_description(SessionDescription& response) const;

  protected:
    ~ChannelController2Impl() noexcept override;

  private:
    using ChunkArray = std::vector<unsigned long>;

    struct ChannelServerRef
    {
      std::string endpoint;
      ChunkArray chunks;
    };

    using ChannelServerGroup = std::vector<ChannelServerRef>;
    using ChannelServerGroups = std::vector<ChannelServerGroup>;

    void fill_refs_();

  private:
    Generics::ActiveObjectCallback_var callback_;
    Logging::Logger_var logger_;
    const ChannelControllerConfig config_;
    ChannelServerGroups channel_server_groups_;
  };

  using ChannelController2Impl_var =
    ReferenceCounting::SmartPtr<ChannelController2Impl>;
}
