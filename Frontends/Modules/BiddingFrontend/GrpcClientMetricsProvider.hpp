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

    void
    add_value(std::string_view, double) override;

    void
    add_value(std::string_view, long) override;

    void
    add_value(std::string_view, const std::string&) override;

    void
    add_value_prometheus(
      const std::string&,
      const std::map<std::string, std::string>&,
      double) override;

    void
    set_value_prometheus(
      const std::string&,
      const std::map<std::string, std::string>&,
      double) override;

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

  private:
    std::vector<ClientSource> clients_;
  };
}
