#pragma once

#include <memory>

#include <eh/Exception.hpp>

#include "UserInfoControllerGrpc.hpp"
#include "UserInfoControllerImpl.hpp"

class UserInfoControllerApp_
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  DECLARE_EXCEPTION(InvalidArgument, Exception);

  void main(int& argc, char** argv) noexcept;

private:
  using ConfigType =
    AdServer::UserInfoSvcs::UserInfoControllerImpl::UserInfoControllerConfig;
  using ConfigPtr = std::unique_ptr<ConfigType>;

  void stop_() noexcept;

  Logging::Logger* logger() const noexcept;

  const ConfigType& config() const noexcept;

private:
  AdServer::UserInfoSvcs::UserInfoControllerImpl_var controller_;
  AdServer::UserInfoSvcs::UserInfoControllerGrpc_var grpc_adapter_;
  ConfigPtr configuration_;
  Logging::Logger_var logger_;
};

inline Logging::Logger*
UserInfoControllerApp_::logger() const noexcept
{
  return logger_;
}

inline const UserInfoControllerApp_::ConfigType&
UserInfoControllerApp_::config() const noexcept
{
  return *configuration_;
}
