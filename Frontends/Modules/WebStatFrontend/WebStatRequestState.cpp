#include "WebStatRequestState.hpp"

#include <utility>

namespace AdServer::WebStat
{
  Frontend::WebStatRequestState::WebStatRequestState(
    Frontend* frontend,
    FCGI::BaseHttpResponseWriter_var response_writer_val,
    FCGI::HttpResponse_var response_val,
    FCGI::HttpRequest::Method request_method_val,
    std::string origin_val)
    : response_writer(std::move(response_writer_val)),
      response(std::move(response_val)),
      request_method(request_method_val),
      origin(std::move(origin_val)),
      frontend_(frontend)
  {}

  void
  Frontend::WebStatRequestState::start() noexcept
  {
    consider_web_operation_stage(0);
  }

  void
  Frontend::WebStatRequestState::consider_web_operation_stage(
    std::size_t index)
    noexcept
  {
    frontend_->consider_web_operation_(shared_from_this(), index);
  }

  void
  Frontend::WebStatRequestState::finish_request_stage(int http_result) noexcept
  {
    frontend_->finish_request_(shared_from_this(), http_result);
  }
}
