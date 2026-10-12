#pragma once

#include <Commons/HttpServer/HttpServer.hpp>
#include "UrlInfoStorage.hpp"

namespace AdServer::RequestInfoSvcs
{
  // POST /get_url_profile: {"url":"..."} -> {"text":"..."}.
  // POST /set_url_profile: {"url":"...","text":"..."} -> {}.
  // A JSON array invokes a batch, returning an array in request order (HTTP 200).
  // Failed items contain {"status":400|500,"error":"..."}; other items still execute.
  // Batches are not atomic. Repeated writes to one URL execute in request order.
  void add_url_info_storage_http_handlers(
    Commons::HttpServer::HttpServer& server, UrlInfoStorage* storage);
}
