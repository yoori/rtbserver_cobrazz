#pragma once

#include <memory>
#include <string>
#include <vector>

#include "WebStatFrontend.hpp"

namespace AdServer::WebStat
{
  class Frontend::WebStatRequestState:
    public std::enable_shared_from_this<WebStatRequestState>
  {
  public:
    WebStatRequestState(
      Frontend* frontend,
      FCGI::BaseHttpResponseWriter_var response_writer,
      FCGI::HttpResponse_var response,
      FCGI::HttpRequest::Method request_method,
      std::string origin);

    void
    start() noexcept;

    void
    consider_web_operation_stage(std::size_t index) noexcept;

    void
    finish_request_stage(int http_result) noexcept;

  public:
    FCGI::BaseHttpResponseWriter_var response_writer;
    FCGI::HttpResponse_var response;
    FCGI::HttpRequest::Method request_method;
    std::string origin;
    std::vector<std::shared_ptr<
      adserver::campaign_svcs::campaign_manager::
        ConsiderWebOperationRequest>> requests;

  private:
    Frontend* frontend_;
  };
}
