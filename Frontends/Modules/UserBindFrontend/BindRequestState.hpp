#pragma once

#include <memory>
#include <set>
#include <string>
#include <vector>

#include <Commons/Algs.hpp>

#include "UserBindFrontend.hpp"

namespace AdServer
{
  class UserBindFrontend::BindRequestState:
    public std::enable_shared_from_this<BindRequestState>
  {
  public:
    BindRequestState(
      UserBindFrontend* frontend,
      UserBind::RequestInfo_var request_info,
      std::string dns_bind_request_id,
      ProcessRequestCallback callback);

    void
    start() noexcept;

  private:
    enum ResultUserIdType
    {
      RUIT_COOKIE,
      RUIT_CRESOLVE,
      RUIT_EXTIDRESOLVE,
      RUIT_EXTIDRESOLVE_NOCOOKIE
    };

    struct ExternalId
    {
      std::string id;
      bool set_uid;
      bool can_be_in_cookie;
    };

    void
    country_filter_stage_() noexcept;

    void
    cookie_resolve_stage_() noexcept;

    void
    cookie_resolve_done_stage_(
      const grpc::Status& status,
      const adserver::user_info_svcs::user_bind::GetUserIdResponse& response)
      noexcept;

    void
    delete_bind_stage_() noexcept;

    void
    delete_bind_done_stage_(const grpc::Status& status) noexcept;

    void
    external_ids_prepare_stage_() noexcept;

    void
    external_id_resolve_stage_(std::size_t index) noexcept;

    void
    external_id_resolve_done_stage_(
      std::size_t index,
      const grpc::Status& status,
      const adserver::user_info_svcs::user_bind::GetUserIdResponse& response)
      noexcept;

    void
    external_id_add_stage_(std::size_t index) noexcept;

    void
    external_id_add_done_stage_(
      std::size_t index,
      const grpc::Status& status,
      const adserver::user_info_svcs::user_bind::AddUserIdResponse& response)
      noexcept;

    void
    generate_user_id_stage_() noexcept;

    void
    finish_result_stage_() noexcept;

    void
    user_match_stage_() noexcept;

    void
    bind_request_stage_(const BindResult& bind_result) noexcept;

    void
    bind_request_done_stage_(const BindResult& bind_result) noexcept;

    void
    finish_(const BindResult& bind_result, int status = -1) noexcept;

  private:
    UserBindFrontend* frontend;
    UserBind::RequestInfo_var request_info;
    std::string dns_bind_request_id;
    ProcessRequestCallback callback;

    int http_status;
    AdServer::Commons::UserId result_user_id;
    AdServer::Commons::UserId result_ssp_user_id;
    ResultUserIdType result_user_id_type;
    AdServer::Commons::UserId merge_user_id;
    bool app_request;
    bool opted_out;
    bool cresolve_failed;
    bool create_user_profile;
    std::vector<ExternalId> external_ids;
    std::set<std::string> resolve_failed_external_ids;
    std::size_t resolved_ext_user_i;
  };
}
