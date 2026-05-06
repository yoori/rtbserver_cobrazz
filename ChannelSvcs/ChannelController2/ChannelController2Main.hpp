#pragma once

#include <memory>

#include <eh/Exception.hpp>

#include "ChannelController2Grpc.hpp"
#include "ChannelController2Impl.hpp"

class ChannelController2App_
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  DECLARE_EXCEPTION(InvalidArgument, Exception);

  void main(int& argc, char** argv) noexcept;

private:
  using ConfigType =
    AdServer::ChannelSvcs::ChannelController2Impl::ChannelControllerConfig;
  using ConfigPtr = std::unique_ptr<ConfigType>;

  void stop_() noexcept;

  Logging::Logger* logger() const noexcept;

  const ConfigType& config() const noexcept;

private:
  AdServer::ChannelSvcs::ChannelController2Impl_var controller_;
  AdServer::ChannelSvcs::ChannelController2Grpc_var grpc_adapter_;
  ConfigPtr configuration_;
  Logging::Logger_var logger_;
};

inline Logging::Logger*
ChannelController2App_::logger() const noexcept
{
  return logger_;
}

inline const ChannelController2App_::ConfigType&
ChannelController2App_::config() const noexcept
{
  return *configuration_;
}
