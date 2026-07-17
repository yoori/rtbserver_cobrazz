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
      std::string dns_bind_request_id);

    ProcessRequestTask
    co_process_() noexcept;

  private:
    enum ResultUserIdType
    {
      RUIT_COOKIE,
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
    user_match_stage_() noexcept;

    BindResult
    make_bind_result_() const;

  private:
    UserBindFrontend* frontend;
    UserBind::RequestInfo_var request_info;
    std::string dns_bind_request_id;

    int http_status;
    AdServer::Commons::UserId result_user_id;
    AdServer::Commons::UserId result_ssp_user_id;
    ResultUserIdType result_user_id_type;
    AdServer::Commons::UserId merge_user_id;
    bool app_request;
    bool opted_out;
    bool create_user_profile;
    std::vector<ExternalId> external_ids;
    std::set<std::string> resolve_failed_external_ids;
    std::size_t resolved_ext_user_i;
  };
}
