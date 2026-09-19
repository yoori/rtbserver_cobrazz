#include "RequestInfoManagerHttp.hpp"

#include <string>
#include <string_view>
#include <utility>

#include <Commons/JsonFormatter.hpp>

namespace AdServer::RequestInfoSvcs
{
  namespace
  {
    using HttpServer = AdServer::Commons::HttpServer::HttpServer;

    HttpServer::Response error_response(unsigned int status, std::string_view message)
    {
      std::string body;
      {
        AdServer::Commons::JsonFormatter json(body);
        json.add_escaped_string("error", message);
      }
      body += '\n';
      return {status, "application/json", std::move(body)};
    }
  }

  AdServer::Commons::HttpServer::HttpServer::Handler
  make_request_info_manager_stats_http_handler(RequestInfoManagerImpl* request_info_manager)
  {
    RequestInfoManagerImpl_var request_info_manager_holder(
      ReferenceCounting::add_ref(request_info_manager));
    return [request_info_manager_holder = std::move(request_info_manager_holder)](
      HttpServer::Request request)
    {
      if (request.method != "GET")
      {
        return AdServer::Commons::HttpServer::make_ready_response(
          error_response(405, "only GET is supported"));
      }

      const auto stats = request_info_manager_holder->rocksdb_stats();
      std::string body;
      {
        AdServer::Commons::JsonFormatter json(body);
        json.add_number("rocksdb_check_operations", stats.check_total);
        json.add_number("rocksdb_get_operations", stats.get_total);
        json.add_number("rocksdb_touch_operations", stats.touch_total);
        json.add_number("rocksdb_save_operations", stats.save_total);
        json.add_number("rocksdb_remove_operations", stats.remove_total);
        json.add_number("rocksdb_read_batches", stats.read_batch_total);
        json.add_number("rocksdb_read_batch_time_us", stats.read_batch_total_time);
        json.add_number("rocksdb_write_batches", stats.write_batch_total);
        json.add_number("rocksdb_write_batch_time_us", stats.write_batch_total_time);
        json.add_number("rocksdb_pending_operations", stats.pending_operations);
        json.add_number("rocksdb_active_workers", stats.active_workers);
        json.add_number("rocksdb_failed_batches", stats.failed_batch_total);
        json.add_number("rocksdb_failed_operations", stats.failed_operation_total);
        json.add_number(
          "rocksdb_failed_callbacks_expected",
          stats.failed_callback_expected);
        json.add_number(
          "rocksdb_failed_callbacks_completed",
          stats.failed_callback_completed);
        json.add_number("rocksdb_cache_limit", stats.cache_limit);
        json.add_number("rocksdb_cache_size", stats.cache_size);
        json.add_number("rocksdb_cache_entries", stats.cache_entries);
        json.add_number("rocksdb_cache_hits", stats.cache_hits);
        json.add_number("rocksdb_cache_misses", stats.cache_misses);
        json.add_number("rocksdb_cache_evictions", stats.cache_evictions);
      }
      body += '\n';

      return AdServer::Commons::HttpServer::make_ready_response(
        HttpServer::Response{200, "application/json", std::move(body)});
    };
  }
}
