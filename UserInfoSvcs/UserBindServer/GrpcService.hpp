#ifndef RTBSERVER_COBRAZZ_USERBINDSERVER_GRPCSERVICE_HPP
#define RTBSERVER_COBRAZZ_USERBINDSERVER_GRPCSERVICE_HPP

// PROTOBUF
#include "UserBindServer_service.cobrazz.pb.hpp"

// THIS
#include <Logger/Logger.hpp>
#include "UserBindServerImpl.hpp"

namespace AdServer::UserInfoSvcs
{

class GetBindRequestService final
  : public UserBindService_get_bind_request_Service,
    public ReferenceCounting::AtomicImpl
{
public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using WriterPtr = typename Reader::WriterPtr;

public:
  explicit GetBindRequestService(
    Logger* logger,
    UserBindServerImpl* user_bind_server_impl);

  ~GetBindRequestService() override = default;

  void handle(const Reader& reader) override;

private:
  Logger_var logger_;

  UserBindServerImpl_var user_bind_server_impl_;
};

using GetBindRequestService_var =
  ReferenceCounting::SmartPtr<GetBindRequestService>;

class AddBindRequestService final
  : public UserBindService_add_bind_request_Service,
    public ReferenceCounting::AtomicImpl
{
public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using WriterPtr = typename Reader::WriterPtr;

public:
  explicit AddBindRequestService(
    Logger* logger,
    UserBindServerImpl* user_bind_server_impl);

  ~AddBindRequestService() override = default;

  void handle(const Reader& reader) override;

private:
  Logger_var logger_;

  UserBindServerImpl_var user_bind_server_impl_;
};

using AddBindRequestService_var =
  ReferenceCounting::SmartPtr<AddBindRequestService>;

class GetUserIdService final
  : public UserBindService_get_user_id_Service,
    public ReferenceCounting::AtomicImpl
{
public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using WriterPtr = typename Reader::WriterPtr;

public:
  explicit GetUserIdService(
    Logger* logger,
    UserBindServerImpl* user_bind_server_impl);

  ~GetUserIdService() override = default;

  void handle(const Reader& reader) override;

private:
  Logger_var logger_;

  UserBindServerImpl_var user_bind_server_impl_;
};

using GetUserIdService_var =
  ReferenceCounting::SmartPtr<GetUserIdService>;

class AddUserIdService final
  : public UserBindService_add_user_id_Service,
    public ReferenceCounting::AtomicImpl
{
public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using WriterPtr = typename Reader::WriterPtr;

public:
  explicit AddUserIdService(
    Logger* logger,
    UserBindServerImpl* user_bind_server_impl);

  ~AddUserIdService() override = default;

  void handle(const Reader& reader) override;

private:
  Logger_var logger_;

  UserBindServerImpl_var user_bind_server_impl_;
};

using AddUserIdService_var =
  ReferenceCounting::SmartPtr<AddUserIdService>;

class GetSourceService final
  : public UserBindService_get_source_Service,
    public ReferenceCounting::AtomicImpl
{
public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using WriterPtr = typename Reader::WriterPtr;

public:
  explicit GetSourceService(
    Logger* logger,
    UserBindServerImpl* user_bind_server_impl);

  ~GetSourceService() override = default;

  void handle(const Reader& reader) override;

private:
  Logger_var logger_;

  UserBindServerImpl_var user_bind_server_impl_;
};

using GetSourceService_var =
  ReferenceCounting::SmartPtr<GetSourceService>;

} // namespace AdServer::UserInfoSvcs

#endif // RTBSERVER_COBRAZZ_USERBINDSERVER_GRPCSERVICE_HPP
