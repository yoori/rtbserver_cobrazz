#pragma once

#include <memory>

#include <eh/Exception.hpp>
#include <Generics/Time.hpp>

#include <Commons/HttpServer/HttpServer.hpp>

#include <xsd/UserInfoSvcs/UserBindServerConfig.hpp>

#include "UserBindServerCore.hpp"
#include "UserBindServerGrpc.hpp"

class UserBindServerApp_
  : public virtual Generics::CompositeActiveObject
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  DECLARE_EXCEPTION(InvalidArgument, Exception);

public:
  UserBindServerApp_() = default;
  ~UserBindServerApp_() noexcept override = default;

  void main(int& argc, char** argv) noexcept;

private:
  const AdServer::UserInfoSvcs::UserBindServerCore::UserBindServerConfig&
  config() const noexcept;

  typedef std::unique_ptr<
    AdServer::UserInfoSvcs::UserBindServerCore::UserBindServerConfig>
    ConfigPtr;

  Logging::Logger* logger() const noexcept;

private:
  ConfigPtr configuration_;
  Logging::Logger_var logger_;

  AdServer::UserInfoSvcs::UserBindServerGrpc_var grpc_adapter_;
  AdServer::Commons::HttpServer::HttpServer_var http_server_;
};

// Inlines
inline
const AdServer::UserInfoSvcs::UserBindServerCore::UserBindServerConfig&
UserBindServerApp_::config() const noexcept
{
  return *configuration_.get();
}

inline
Logging::Logger*
UserBindServerApp_::logger() const noexcept
{
  return logger_.in();
}
