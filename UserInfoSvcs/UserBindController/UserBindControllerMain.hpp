#pragma once

#include <memory>

#include <eh/Exception.hpp>

#include "UserBindControllerGrpc.hpp"
#include "UserBindControllerImpl.hpp"

class UserBindControllerApp_
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  DECLARE_EXCEPTION(InvalidArgument, Exception);

  void main(int& argc, char** argv) noexcept;

private:
  using ConfigType = AdServer::UserInfoSvcs::UserBindControllerImpl::UserBindControllerConfig;
  using ConfigPtr = std::unique_ptr<ConfigType>;

  void stop_() noexcept;

  Logging::Logger* logger() const noexcept;

  const ConfigType& config() const noexcept;

private:
  AdServer::UserInfoSvcs::UserBindControllerImpl_var controller_;
  AdServer::UserInfoSvcs::UserBindControllerGrpc_var grpc_adapter_;
  ConfigPtr configuration_;
  Logging::Logger_var logger_;
};

inline Logging::Logger*
UserBindControllerApp_::logger() const noexcept
{
  return logger_;
}

inline const UserBindControllerApp_::ConfigType&
UserBindControllerApp_::config() const noexcept
{
  return *configuration_;
}
