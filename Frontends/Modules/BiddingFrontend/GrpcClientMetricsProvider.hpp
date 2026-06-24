#pragma once

#include <memory>
#include <string>
#include <vector>

#include <Commons/Grpc/GrpcClient.hpp>
#include <Generics/MetricsProvider.hpp>

namespace AdServer::Bidding
{
  class GrpcClientMetricsProvider final : public Generics::MetricsProvider
  {
  public:
    struct ClientSource
    {
      std::string prefix;
      std::weak_ptr<AdServer::Grpc::Client> client;
    };

    explicit GrpcClientMetricsProvider(std::vector<ClientSource> clients);

    MetricArray
    get_values() override;

  private:
    static void
    add_counter_(
      MetricArray& result,
      const std::string& prefix,
      const char* name,
      std::uint64_t value);

    static void
    add_client_stats_(
      MetricArray& result,
      const std::string& prefix,
      const AdServer::Grpc::Stats& stats);

    static void
    add_client_endpoint_stats_(
      MetricArray& result,
      const std::string& prefix,
      const AdServer::Grpc::Client::EndpointStats& endpoint_stats);

  private:
    std::vector<ClientSource> clients_;
  };
}
