#include "UserBindAsyncClient.hpp"

#include <utility>

namespace AdServer::UserInfoSvcs
{
  UserBindAsyncClient::UserBindAsyncClient(
    const std::vector<std::shared_ptr<grpc::Channel>>& channels,
    std::size_t completion_queues_count,
    std::optional<std::size_t> max_inflight,
    bool enable_grpc_compression)
    : client_(
        channels,
        completion_queues_count,
        max_inflight,
        enable_grpc_compression,
        [](const std::shared_ptr<grpc::Channel>& channel)
        {
          return GrpcClient::NewStub(channel);
        })
  {}

  UserBindAsyncClient::~UserBindAsyncClient() = default;

  void
  UserBindAsyncClient::get_bind_request(
    const adserver::user_info_svcs::user_bind::GetBindRequestRequest& request,
    GetBindRequestCallback callback)
  {
    client_.call<
      adserver::user_info_svcs::user_bind::GetBindRequestRequest,
      adserver::user_info_svcs::user_bind::GetBindRequestResponse>(
        request,
        std::move(callback),
        [](auto& stub, auto* context, const auto& req, auto* cq)
        {
          return stub.PrepareAsyncget_bind_request(context, req, cq);
        });
  }

  void
  UserBindAsyncClient::add_bind_request(
    const adserver::user_info_svcs::user_bind::AddBindRequestRequest& request,
    AddBindRequestCallback callback)
  {
    client_.call<
      adserver::user_info_svcs::user_bind::AddBindRequestRequest,
      adserver::user_info_svcs::user_bind::AddBindRequestResponse>(
        request,
        std::move(callback),
        [](auto& stub, auto* context, const auto& req, auto* cq)
        {
          return stub.PrepareAsyncadd_bind_request(context, req, cq);
        });
  }

  void
  UserBindAsyncClient::get_user_id(
    const adserver::user_info_svcs::user_bind::GetUserIdRequest& request,
    GetUserIdCallback callback)
  {
    client_.call<
      adserver::user_info_svcs::user_bind::GetUserIdRequest,
      adserver::user_info_svcs::user_bind::GetUserIdResponse>(
        request,
        std::move(callback),
        [](auto& stub, auto* context, const auto& req, auto* cq)
        {
          return stub.PrepareAsyncget_user_id(context, req, cq);
        });
  }

  void
  UserBindAsyncClient::add_user_id(
    const adserver::user_info_svcs::user_bind::AddUserIdRequest& request,
    AddUserIdCallback callback)
  {
    client_.call<
      adserver::user_info_svcs::user_bind::AddUserIdRequest,
      adserver::user_info_svcs::user_bind::AddUserIdResponse>(
        request,
        std::move(callback),
        [](auto& stub, auto* context, const auto& req, auto* cq)
        {
          return stub.PrepareAsyncadd_user_id(context, req, cq);
        });
  }

  void
  UserBindAsyncClient::get_source(
    const adserver::user_info_svcs::user_bind::GetSourceRequest& request,
    GetSourceCallback callback)
  {
    client_.call<
      adserver::user_info_svcs::user_bind::GetSourceRequest,
      adserver::user_info_svcs::user_bind::GetSourceResponse>(
        request,
        std::move(callback),
        [](auto& stub, auto* context, const auto& req, auto* cq)
        {
          return stub.PrepareAsyncget_source(context, req, cq);
        });
  }

  UserBindAsyncClient::Stats
  UserBindAsyncClient::stats() const noexcept
  {
    return client_.stats();
  }
}
