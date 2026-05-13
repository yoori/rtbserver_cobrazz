#pragma once

#include <memory>

#include "ActionFrontend.hpp"

namespace AdServer::Action
{
  class Frontend::ActionRequestState:
    public std::enable_shared_from_this<ActionRequestState>
  {
  public:
    ActionRequestState(
      Frontend* frontend,
      FCGI::HttpRequestHolder_var request_holder,
      FCGI::BaseHttpResponseWriter_var response_writer,
      FCGI::HttpResponse_var response,
      RequestInfo request_info,
      bool return_html);

    void
    start() noexcept;

    void
    resolve_utm_user_id_stage() noexcept;

    void
    finish_advertiser_request_stage() noexcept;

  public:
    FCGI::HttpRequestHolder_var request_holder;
    FCGI::BaseHttpResponseWriter_var response_writer;
    FCGI::HttpResponse_var response;
    RequestInfo request_info;
    bool return_html = false;
    Commons::UserId cookie_resolved_user_id;
    Commons::UserId utm_cookie_resolved_user_id;

  private:
    Frontend* frontend_;
  };
}
