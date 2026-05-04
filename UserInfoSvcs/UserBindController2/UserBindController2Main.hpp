#pragma once

#include <memory>

#include <eh/Exception.hpp>

#include "UserBindController2Grpc.hpp"
#include "UserBindController2Impl.hpp"

class UserBindController2App_
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  DECLARE_EXCEPTION(InvalidArgument, Exception);

  void main(int& argc, char** argv) noexcept;

private:
  using ConfigType =
    AdServer::UserInfoSvcs::UserBindController2Impl::UserBindControllerConfig;
  using ConfigPtr = std::unique_ptr<ConfigType>;

  void stop_() noexcept;

  Logging::Logger* logger() const noexcept;

  const ConfigType& config() const noexcept;

private:
  AdServer::UserInfoSvcs::UserBindController2Impl_var controller_;
  AdServer::UserInfoSvcs::UserBindController2Grpc_var grpc_adapter_;
  ConfigPtr configuration_;
  Logging::Logger_var logger_;
};

inline Logging::Logger*
UserBindController2App_::logger() const noexcept
{
  return logger_;
}

inline const UserBindController2App_::ConfigType&
UserBindController2App_::config() const noexcept
{
  return *configuration_;
}
