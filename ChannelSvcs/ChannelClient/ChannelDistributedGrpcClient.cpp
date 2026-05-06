#include "ChannelDistributedGrpcClient.hpp"

#include <algorithm>
#include <utility>

#include <grpcpp/support/status.h>

namespace AdServer::ChannelSvcs
{
  ChannelDistributedGrpcClient::ChannelDistributedGrpcClient(
    const ChannelServerRefs& channel_server_refs,
    AdServer::Grpc::BatchingOptions batching_options,
    AdServer::Grpc::GrpcExecutor* grpc_executor)
  {
    if (channel_server_refs.empty())
    {
      throw Exception("ChannelDistributedGrpcClient: empty ChannelServer refs");
    }

    clients_.reserve(channel_server_refs.size());
    for (const auto& endpoint : channel_server_refs)
    {
      Client_var client = new Client(
        endpoint,
        grpc_executor,
        batching_options);
      clients_.push_back(client);
      add_child_object(client);
    }
  }

  AdServer::Grpc::Stats
  ChannelDistributedGrpcClient::stats() const noexcept
  {
    AdServer::Grpc::Stats result;
    for (const auto& client : clients_)
    {
      merge_stats_(
        result,
        static_cast<ChannelServerGrpcAsyncClient*>(client.in())->stats());
    }
    return result;
  }

  void
  ChannelDistributedGrpcClient::match(
    const adserver::channel_svcs::channel_server::MatchRequest& request,
    MatchCallback callback)
  {
    Client* client = next_client_();
    if (!client)
    {
      callback(
        grpc::Status(
          grpc::StatusCode::UNAVAILABLE,
          "no available ChannelServer grpc client"),
        adserver::channel_svcs::channel_server::MatchResponse());
      return;
    }
    client->match(request, std::move(callback));
  }

  void
  ChannelDistributedGrpcClient::get_ccg_traits(
    const adserver::channel_svcs::channel_server::GetCcgTraitsRequest& request,
    GetCcgTraitsCallback callback)
  {
    Client* client = next_client_();
    if (!client)
    {
      callback(
        grpc::Status(
          grpc::StatusCode::UNAVAILABLE,
          "no available ChannelServer grpc client"),
        adserver::channel_svcs::channel_server::GetCcgTraitsResponse());
      return;
    }
    client->get_ccg_traits(request, std::move(callback));
  }

  ChannelDistributedGrpcClient::Client*
  ChannelDistributedGrpcClient::next_client_() noexcept
  {
    if (clients_.empty())
    {
      return nullptr;
    }

    const auto index =
      next_client_index_.fetch_add(1, std::memory_order_relaxed) %
      clients_.size();
    return clients_[index].in();
  }

  void
  ChannelDistributedGrpcClient::merge_stats_(
    AdServer::Grpc::Stats& result,
    const AdServer::Grpc::Stats& source) noexcept
  {
    result.write_batches += source.write_batches;
    result.write_items += source.write_items;
    result.queue_wait_count += source.queue_wait_count;
    result.queue_wait_sum_us += source.queue_wait_sum_us;
    result.queue_wait_max_us =
      std::max(result.queue_wait_max_us, source.queue_wait_max_us);
    result.response_wait_count += source.response_wait_count;
    result.response_wait_sum_us += source.response_wait_sum_us;
    result.response_wait_max_us =
      std::max(result.response_wait_max_us, source.response_wait_max_us);
    result.max_streams = std::max(result.max_streams, source.max_streams);
    if (source.consumer_stream_write.has_value())
    {
      if (!result.consumer_stream_write.has_value())
      {
        result.consumer_stream_write = AdServer::Grpc::Stats::ConsumerStreamWrite();
      }
      result.consumer_stream_write->count += source.consumer_stream_write->count;
      result.consumer_stream_write->sum_us += source.consumer_stream_write->sum_us;
      result.consumer_stream_write->max_us = std::max(
        result.consumer_stream_write->max_us,
        source.consumer_stream_write->max_us);
    }
  }
}
