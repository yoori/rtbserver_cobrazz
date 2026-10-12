#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include "LogGeneralizerTypes.hpp"
#include <LogProcessing/LogGeneralizer/LogGeneralizer.grpc.pb.h>

namespace AdServer::LogProcessing
{
  namespace lg_rpc = adserver::log_processing::log_generalizer;

  class LogGeneralizerGrpcService final : public lg_rpc::LogGeneralizer::Service
  {
  public:
    using Stop = std::function<void(std::uint32_t)>;
    using Start = std::function<void(std::uint32_t, bool)>;
    using Get = std::function<StatInfo(std::uint32_t, bool)>;

    LogGeneralizerGrpcService(Stop stop, Start start, Get get);

    grpc::Status stop_stat_upload(
      grpc::ServerContext*, const lg_rpc::StatRequest*, lg_rpc::Empty*) override;
    grpc::Status start_stat_upload(
      grpc::ServerContext*, const lg_rpc::StatRequest*, lg_rpc::Empty*) override;
    grpc::Status get_stat_info(
      grpc::ServerContext*, const lg_rpc::StatRequest*, lg_rpc::StatInfo*) override;

  private:
    Stop stop_;
    Start start_;
    Get get_;
  };

  class LogGeneralizerClient
  {
  public:
    explicit LogGeneralizerClient(
      const std::string& endpoint,
      std::chrono::milliseconds timeout = std::chrono::seconds(30));
    void stop_stat_upload(std::uint32_t client_id) const;
    void start_stat_upload(std::uint32_t client_id, bool clear) const;
    StatInfo get_stat_info(std::uint32_t client_id, bool clear) const;

  private:
    std::unique_ptr<lg_rpc::LogGeneralizer::Stub> stub_;
    const std::chrono::milliseconds timeout_;
  };
}
