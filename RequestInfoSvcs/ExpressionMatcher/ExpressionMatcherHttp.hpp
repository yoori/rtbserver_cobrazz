#pragma once

#include <Commons/HttpServer/HttpServer.hpp>

#include "ExpressionMatcherImpl.hpp"

namespace AdServer::RequestInfoSvcs
{
  AdServer::Commons::HttpServer::HttpServer::Handler
  make_expression_matcher_stats_http_handler(ExpressionMatcherImpl* expression_matcher);

  AdServer::Commons::HttpServer::HttpServer::Handler
  make_user_navigation_profile_http_handler(ExpressionMatcherImpl* expression_matcher);
}
