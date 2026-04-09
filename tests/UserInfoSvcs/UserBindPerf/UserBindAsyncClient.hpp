#pragma once

#include <memory>
#include <optional>
#include <vector>

#include <grpcpp/channel.h>

#include <Commons/Grpc/GrpcClient.hpp>
#include "UserBindServerGrpc.grpc-client.hpp"

namespace AdServer::UserInfoSvcs
{
  class UserBindAsyncClient final : public UserBindServerGrpcAsyncClient
  {
  public:
    explicit UserBindAsyncClient(
      const std::vector<std::shared_ptr<grpc::Channel>>& channels,
      std::size_t completion_queues_count,
      std::optional<std::size_t> max_inflight = std::nullopt,
      bool enable_grpc_compression = true);

    ~UserBindAsyncClient() override;

    void get_bind_request(
      const adserver::user_info_svcs::user_bind::GetBindRequestRequest& request,
      GetBindRequestCallback callback) override;

    void add_bind_request(
      const adserver::user_info_svcs::user_bind::AddBindRequestRequest& request,
      AddBindRequestCallback callback) override;

    void get_user_id(
      const adserver::user_info_svcs::user_bind::GetUserIdRequest& request,
      GetUserIdCallback callback) override;

    void add_user_id(
      const adserver::user_info_svcs::user_bind::AddUserIdRequest& request,
      AddUserIdCallback callback) override;

    void get_source(
      const adserver::user_info_svcs::user_bind::GetSourceRequest& request,
      GetSourceCallback callback) override;

    Stats stats() const noexcept override;

  private:
    using GrpcClient = adserver::user_info_svcs::user_bind::UserBindServerGrpc;

  private:
    AdServer::Grpc::AsyncUnaryClient<GrpcClient::Stub> client_;
  };
}
