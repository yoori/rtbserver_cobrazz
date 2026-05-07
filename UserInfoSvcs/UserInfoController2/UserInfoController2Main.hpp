#pragma once

#include <memory>

#include <eh/Exception.hpp>

#include "UserInfoController2Grpc.hpp"
#include "UserInfoController2Impl.hpp"

class UserInfoController2App_
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  DECLARE_EXCEPTION(InvalidArgument, Exception);

  void main(int& argc, char** argv) noexcept;

private:
  using ConfigType =
    AdServer::UserInfoSvcs::UserInfoController2Impl::UserInfoControllerConfig;
  using ConfigPtr = std::unique_ptr<ConfigType>;

  void stop_() noexcept;

  Logging::Logger* logger() const noexcept;

  const ConfigType& config() const noexcept;

private:
  AdServer::UserInfoSvcs::UserInfoController2Impl_var controller_;
  AdServer::UserInfoSvcs::UserInfoController2Grpc_var grpc_adapter_;
  ConfigPtr configuration_;
  Logging::Logger_var logger_;
};

inline Logging::Logger*
UserInfoController2App_::logger() const noexcept
{
  return logger_;
}

inline const UserInfoController2App_::ConfigType&
UserInfoController2App_::config() const noexcept
{
  return *configuration_;
}
