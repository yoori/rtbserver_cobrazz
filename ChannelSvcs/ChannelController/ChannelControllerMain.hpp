#pragma once

#include <memory>

#include <eh/Exception.hpp>

#include "ChannelControllerGrpc.hpp"
#include "ChannelControllerImpl.hpp"

class ChannelControllerApp_
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  DECLARE_EXCEPTION(InvalidArgument, Exception);

  void main(int& argc, char** argv) noexcept;

private:
  using ConfigType =
    AdServer::ChannelSvcs::ChannelControllerImpl::ChannelControllerConfig;
  using ConfigPtr = std::unique_ptr<ConfigType>;

  void stop_() noexcept;

  Logging::Logger* logger() const noexcept;

  const ConfigType& config() const noexcept;

private:
  AdServer::ChannelSvcs::ChannelControllerImpl_var controller_;
  AdServer::ChannelSvcs::ChannelControllerGrpc_var grpc_adapter_;
  ConfigPtr configuration_;
  Logging::Logger_var logger_;
};

inline Logging::Logger*
ChannelControllerApp_::logger() const noexcept
{
  return logger_;
}

inline const ChannelControllerApp_::ConfigType&
ChannelControllerApp_::config() const noexcept
{
  return *configuration_;
}
