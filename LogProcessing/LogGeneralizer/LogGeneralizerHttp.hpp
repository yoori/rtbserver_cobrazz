#pragma once

#include <Generics/Values.hpp>
#include <Commons/HttpServer/HttpServer.hpp>

namespace AdServer::LogProcessing
{
  Commons::HttpServer::HttpServer::Handler
  make_log_generalizer_stats_http_handler(Generics::Values* stats);
}
