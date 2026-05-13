#pragma once

#include <functional>
#include <memory>

#include <grpcpp/grpcpp.h>

#include <Commons/UserInfoManip.hpp>
#include <UserBindServerGrpc.pb.h>

#include "RequestInfoFiller.hpp"

namespace AdServer::ImprTrack
{
  class Frontend;

  class ImprTrackRequestState:
    public std::enable_shared_from_this<ImprTrackRequestState>
  {
  public:
    ImprTrackRequestState(
      Frontend* frontend,
      const RequestInfo& request_info,
      const AdServer::Commons::UserId& input_user_id,
      std::function<void(
        const AdServer::Commons::UserId& result_user_id,
        bool invalid_bind_operation)> finish);

    void
    start();

  private:
    void
    complete_stage_();

    void
    rebind_external_stage_();

    void
    add_external_user_stage_();

    void
    add_external_user_done_stage_(
      const grpc::Status& status,
      const adserver::user_info_svcs::user_bind::AddUserIdResponse& response);

    void
    resolve_external_user_stage_();

    void
    resolve_external_user_done_stage_(
      const grpc::Status& status,
      const adserver::user_info_svcs::user_bind::GetUserIdResponse& response);

    void
    resolve_current_user_stage_();

    void
    resolve_current_user_done_stage_(
      const grpc::Status& status,
      const adserver::user_info_svcs::user_bind::GetUserIdResponse& response);

    void
    log_rebind_error_(const grpc::Status& status) const noexcept;

  private:
    Frontend* frontend_;
    RequestInfo request_info_;
    AdServer::Commons::UserId result_user_id_;
    bool invalid_bind_operation_;
    std::function<void(
      const AdServer::Commons::UserId& result_user_id,
      bool invalid_bind_operation)> finish_;
  };
}
