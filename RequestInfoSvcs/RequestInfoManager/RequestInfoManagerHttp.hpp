#pragma once

#include <Commons/HttpServer/HttpServer.hpp>

#include "RequestInfoManagerImpl.hpp"

namespace AdServer::RequestInfoSvcs
{
  AdServer::Commons::HttpServer::HttpServer::Handler
  make_request_info_manager_stats_http_handler(RequestInfoManagerImpl* request_info_manager);
}
