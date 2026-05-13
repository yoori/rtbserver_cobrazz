#include "ActionRequestState.hpp"

#include <utility>

namespace AdServer::Action
{
  Frontend::ActionRequestState::ActionRequestState(
    Frontend* frontend,
    FCGI::HttpRequestHolder_var request_holder_val,
    FCGI::BaseHttpResponseWriter_var response_writer_val,
    FCGI::HttpResponse_var response_val,
    RequestInfo request_info_val,
    bool return_html_val)
    : request_holder(std::move(request_holder_val)),
      response_writer(std::move(response_writer_val)),
      response(std::move(response_val)),
      request_info(std::move(request_info_val)),
      return_html(return_html_val),
      frontend_(frontend)
  {}

  void
  Frontend::ActionRequestState::start() noexcept
  {
    frontend_->process_advertiser_request_(shared_from_this());
  }

  void
  Frontend::ActionRequestState::resolve_utm_user_id_stage() noexcept
  {
    frontend_->resolve_utm_user_id_(shared_from_this());
  }

  void
  Frontend::ActionRequestState::finish_advertiser_request_stage() noexcept
  {
    frontend_->finish_advertiser_request_(shared_from_this());
  }
}
