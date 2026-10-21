#pragma once

#include <Commons/HttpServer/HttpServer.hpp>
#include "UrlInfoStorage.hpp"

namespace AdServer::RequestInfoSvcs
{
  // GET /get_url_profile_count and /get_url_description_count -> {"count":N}.
  // Counts scan a RocksDB snapshot, including empty values, and take O(number of keys).
  // POST /get_url_profile: {"url":"..."} -> {"text":"..."}.
  // POST /set_url_profile: {"url":"...","text":"..."} -> {}.
  // POST /get_url_description_versioned: {url} -> {text,version} (SHA-256 of stored text).
  // POST /order_url_description_full_indexing: {url,version,job_id} -> {updated:true,version}.
  // Ordering preserves description content; stale versions return VERSION_CONFLICT (409).
  // A JSON array invokes a batch, returning an array in request order (HTTP 200).
  // Failed items contain {"status":400|409|500,"error":"..."}; other items still execute.
  // Batches are not atomic. Repeated writes to one URL execute in request order.
  void add_url_info_storage_http_handlers(
    Commons::HttpServer::HttpServer& server, UrlInfoStorage* storage);
}
