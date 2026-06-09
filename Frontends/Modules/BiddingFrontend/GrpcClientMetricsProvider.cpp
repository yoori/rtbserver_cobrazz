#include "GrpcClientMetricsProvider.hpp"

#include <utility>

namespace AdServer::Bidding
{
  namespace
  {
    std::string
    sanitize_endpoint_key_(const std::string& endpoint)
    {
      std::string result;
      result.reserve(endpoint.size());
      for (const char ch : endpoint)
      {
        if ((ch >= 'a' && ch <= 'z') ||
          (ch >= 'A' && ch <= 'Z') ||
          (ch >= '0' && ch <= '9'))
        {
          result += ch;
        }
        else
        {
          result += '_';
        }
      }
      return result.empty() ? std::string("unknown") : result;
    }
  }

  GrpcClientMetricsProvider::GrpcClientMetricsProvider(
    std::vector<ClientSource> clients)
    : clients_(std::move(clients))
  {}

  GrpcClientMetricsProvider::MetricArray
  GrpcClientMetricsProvider::get_values()
  {
    MetricArray result;
    for(const auto& source : clients_)
    {
      if(auto client = source.client.lock())
      {
        add_client_stats_(result, source.prefix, client->stats());
        add_client_endpoint_stats_(
          result,
          source.prefix,
          client->endpoint_stats());
      }
    }
    return result;
  }

  void
  GrpcClientMetricsProvider::add_value(std::string_view, double)
  {}

  void
  GrpcClientMetricsProvider::add_value(std::string_view, long)
  {}

  void
  GrpcClientMetricsProvider::add_value(
    std::string_view,
    const std::string&)
  {}

  void
  GrpcClientMetricsProvider::add_value_prometheus(
    const std::string&,
    const std::map<std::string, std::string>&,
    double)
  {}

  void
  GrpcClientMetricsProvider::set_value_prometheus(
    const std::string&,
    const std::map<std::string, std::string>&,
    double)
  {}

  void
  GrpcClientMetricsProvider::add_counter_(
    MetricArray& result,
    const std::string& prefix,
    const char* name,
    const std::uint64_t value)
  {
    result.emplace_back(
      prefix + "_" + name,
      static_cast<long>(value));
  }

  void
  GrpcClientMetricsProvider::add_client_stats_(
    MetricArray& result,
    const std::string& prefix,
    const AdServer::Grpc::Stats& stats)
  {
    add_counter_(result, prefix, "input_items", stats.input_items);
    add_counter_(result, prefix, "call_total", stats.input_items);
    add_counter_(result, prefix, "completed_items", stats.completed_items);
    add_counter_(
      result,
      prefix,
      "completed_error_items",
      stats.completed_error_items);
    add_counter_(
      result,
      prefix,
      "call_error_total",
      stats.completed_error_items);
    add_counter_(
      result,
      prefix,
      "outstanding_items",
      stats.input_items > stats.completed_items ?
        stats.input_items - stats.completed_items :
        0);
    add_counter_(result, prefix, "write_batches", stats.write_batches);
    add_counter_(result, prefix, "batch_total", stats.write_batches);
    add_counter_(result, prefix, "write_batch_total", stats.write_batches);
    add_counter_(result, prefix, "write_items", stats.write_items);
    add_counter_(result, prefix, "read_batches", stats.read_batches);
    add_counter_(result, prefix, "read_batch_total", stats.read_batches);
    add_counter_(result, prefix, "read_items", stats.read_items);
    add_counter_(result, prefix, "queue_wait_total", stats.queue_wait_count);
    add_counter_(result, prefix, "queue_wait_time", stats.queue_wait_sum_us);
    add_counter_(result, prefix, "queue_wait_max_time", stats.queue_wait_max_us);
    add_counter_(
      result,
      prefix,
      "queue_timeout_total",
      stats.queue_timeout_count);
    add_counter_(
      result,
      prefix,
      "response_wait_total",
      stats.response_wait_count);
    add_counter_(
      result,
      prefix,
      "response_wait_time",
      stats.response_wait_sum_us);
    add_counter_(
      result,
      prefix,
      "response_wait_max_time",
      stats.response_wait_max_us);
    add_counter_(result, prefix, "queue_items", stats.queue_items);
    add_counter_(result, prefix, "pending_batches", stats.pending_batches);
    add_counter_(
      result,
      prefix,
      "pending_batch_items",
      stats.pending_batch_items);
    add_counter_(result, prefix, "inflight_items", stats.inflight_items);
    add_counter_(
      result,
      prefix,
      "stream_inflight_items",
      stats.stream_inflight_items);
    add_counter_(result, prefix, "active_streams", stats.active_streams);
    add_counter_(result, prefix, "available_streams", stats.available_streams);
    add_counter_(
      result,
      prefix,
      "connecting_streams",
      stats.connecting_streams);
    add_counter_(result, prefix, "draining_streams", stats.draining_streams);
    add_counter_(result, prefix, "deferred_streams", stats.deferred_streams);

    if (stats.consumer_stream_write.has_value())
    {
      add_counter_(
        result,
        prefix,
        "consumer_stream_write_total",
        stats.consumer_stream_write->count);
      add_counter_(
        result,
        prefix,
        "consumer_stream_write_time",
        stats.consumer_stream_write->sum_us);
      add_counter_(
        result,
        prefix,
        "consumer_stream_write_max_time",
        stats.consumer_stream_write->max_us);
    }

    if(stats.last_error.has_value())
    {
      result.emplace_back(
        prefix + "_last_error_time",
        stats.last_error->time.get_gm_time().format("%F %T"));
      result.emplace_back(
        prefix + "_last_error_endpoint",
        stats.last_error->endpoint);
      result.emplace_back(
        prefix + "_last_error_code",
        static_cast<long>(stats.last_error->code));
      result.emplace_back(
        prefix + "_last_error_message",
        stats.last_error->message);
      result.emplace_back(
        prefix + "_last_error_source",
        stats.last_error->source);
    }
  }

  void
  GrpcClientMetricsProvider::add_client_endpoint_stats_(
    MetricArray& result,
    const std::string& prefix,
    const AdServer::Grpc::Client::EndpointStats& endpoint_stats)
  {
    for (const auto& [endpoint, stats] : endpoint_stats)
    {
      const auto endpoint_prefix =
        prefix + "_endpoints_" + sanitize_endpoint_key_(endpoint);
      result.emplace_back(endpoint_prefix + "_endpoint", endpoint);
      add_client_stats_(result, endpoint_prefix, stats);
    }
  }
}
