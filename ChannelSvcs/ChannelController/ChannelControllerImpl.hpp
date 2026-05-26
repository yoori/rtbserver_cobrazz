#pragma once

#include <string>
#include <vector>

#include <Commons/ExecutorPool.hpp>
#include <eh/Exception.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Logger/Logger.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

#include <xsd/ChannelSvcs/ChannelControllerConfig.hpp>

#include "ChannelControllerGrpc.pb.h"

namespace AdServer::ChannelSvcs
{
  class ChannelControllerImpl:
    public Generics::CompositeActiveObject,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(NotReady, Exception);

    using ChannelControllerConfig =
      xsd::AdServer::Configuration::ChannelControllerConfigType;
    using SessionDescription =
      adserver::channel_svcs::channel_controller::GetSessionDescriptionResponse;

    ChannelControllerImpl(
      Generics::ActiveObjectCallback* callback,
      Logging::Logger* logger,
      const ChannelControllerConfig& config);

    void fill_session_description(SessionDescription& response) const;

  protected:
    void activate_object_() override;

    ~ChannelControllerImpl() noexcept override;

  private:
    using ChunkArray = std::vector<unsigned long>;

    struct ChannelServerRef
    {
      std::string endpoint;
      std::string update_ref;
      ChunkArray chunks;
      unsigned long check_sum = 0;
    };

    using ChannelServerGroup = std::vector<ChannelServerRef>;
    using ChannelServerGroups = std::vector<ChannelServerGroup>;

    void fill_refs_();
    void set_sources_() const;
    void schedule_set_sources_(const Generics::Time& delay) noexcept;
    void update_sources_() noexcept;

  private:
    Generics::ActiveObjectCallback_var callback_;
    Logging::Logger_var logger_;
    const ChannelControllerConfig config_;
    bool control_sources_;
    ChannelServerGroups channel_server_groups_;
    std::shared_ptr<AdServer::Commons::ExecutorPool> source_update_runner_;
  };

  using ChannelControllerImpl_var =
    ReferenceCounting::SmartPtr<ChannelControllerImpl>;
}
