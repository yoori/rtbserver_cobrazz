#include "UserBindSyncClient.hpp"

#include <utility>

namespace AdServer::UserInfoSvcs
{
  UserBindSyncClient::UserBindSyncClient(
    const std::vector<std::shared_ptr<grpc::Channel>>& channels,
    bool enable_grpc_compression)
    : client_(
        channels,
        enable_grpc_compression,
        [](const std::shared_ptr<grpc::Channel>& channel)
        {
          return GrpcClient::NewStub(channel);
        })
  {}

  void
  UserBindSyncClient::get_bind_request(
    const adserver::user_info_svcs::user_bind::GetBindRequestRequest& request,
    GetBindRequestCallback callback)
  {
    client_.call<
      adserver::user_info_svcs::user_bind::GetBindRequestRequest,
      adserver::user_info_svcs::user_bind::GetBindRequestResponse>(
        request,
        std::move(callback),
        [](auto& stub, auto* context, const auto& req, auto* response)
        {
          return stub.get_bind_request(context, req, response);
        });
  }

  void
  UserBindSyncClient::add_bind_request(
    const adserver::user_info_svcs::user_bind::AddBindRequestRequest& request,
    AddBindRequestCallback callback)
  {
    client_.call<
      adserver::user_info_svcs::user_bind::AddBindRequestRequest,
      adserver::user_info_svcs::user_bind::AddBindRequestResponse>(
        request,
        std::move(callback),
        [](auto& stub, auto* context, const auto& req, auto* response)
        {
          return stub.add_bind_request(context, req, response);
        });
  }

  void
  UserBindSyncClient::get_user_id(
    const adserver::user_info_svcs::user_bind::GetUserIdRequest& request,
    GetUserIdCallback callback)
  {
    client_.call<
      adserver::user_info_svcs::user_bind::GetUserIdRequest,
      adserver::user_info_svcs::user_bind::GetUserIdResponse>(
        request,
        std::move(callback),
        [](auto& stub, auto* context, const auto& req, auto* response)
        {
          return stub.get_user_id(context, req, response);
        });
  }

  void
  UserBindSyncClient::add_user_id(
    const adserver::user_info_svcs::user_bind::AddUserIdRequest& request,
    AddUserIdCallback callback)
  {
    client_.call<
      adserver::user_info_svcs::user_bind::AddUserIdRequest,
      adserver::user_info_svcs::user_bind::AddUserIdResponse>(
        request,
        std::move(callback),
        [](auto& stub, auto* context, const auto& req, auto* response)
        {
          return stub.add_user_id(context, req, response);
        });
  }

  void
  UserBindSyncClient::get_source(
    const adserver::user_info_svcs::user_bind::GetSourceRequest& request,
    GetSourceCallback callback)
  {
    client_.call<
      adserver::user_info_svcs::user_bind::GetSourceRequest,
      adserver::user_info_svcs::user_bind::GetSourceResponse>(
        request,
        std::move(callback),
        [](auto& stub, auto* context, const auto& req, auto* response)
        {
          return stub.get_source(context, req, response);
        });
  }

  UserBindSyncClient::Stats
  UserBindSyncClient::stats() const noexcept
  {
    return client_.stats();
  }
}
