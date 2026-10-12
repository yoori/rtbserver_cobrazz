#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include <ChannelSvcs/ChannelCommons/ChannelUpdateTypes.hpp>
#include <ChannelSvcs/ChannelServer/ChannelServerGrpc.grpc.pb.h>

namespace AdServer::ChannelSvcs
{
  class ChannelUpdateSource
  {
  public:
    virtual ~ChannelUpdateSource();
    virtual void check(const ChannelUpdate::CheckQuery& query, ChannelUpdate::CheckDataPtr& data)
      = 0;
    virtual void update_triggers(const ChannelIdList& ids, ChannelUpdate::UpdateDataPtr& data) = 0;
    virtual void update_all_ccg(
      const ChannelUpdate::CCGQuery& query, ChannelUpdate::PosCCGResultPtr& data) = 0;
    virtual std::uint32_t get_count_chunks() = 0;
  };

  class ChannelUpdateGrpcClient final : public ChannelUpdateSource
  {
  public:
    explicit ChannelUpdateGrpcClient(
      const std::string& endpoint, std::chrono::milliseconds timeout = std::chrono::seconds(10));
    void check(const ChannelUpdate::CheckQuery& query, ChannelUpdate::CheckDataPtr& data)
      override;
    void update_triggers(const ChannelIdList& ids, ChannelUpdate::UpdateDataPtr& data) override;
    void update_all_ccg(
      const ChannelUpdate::CCGQuery& query, ChannelUpdate::PosCCGResultPtr& data) override;
    std::uint32_t get_count_chunks() override;

  private:
    std::unique_ptr<adserver::channel_svcs::channel_server::ChannelServerGrpc::Stub> stub_;
    std::chrono::milliseconds timeout_;
  };

  struct ChannelLoadDescription
  {
    std::shared_ptr<ChannelUpdateSource> load_server;
    std::vector<unsigned int> chunks;
    unsigned long count_chunks = 0;
  };

  using GroupLoadDescription = std::vector<std::vector<ChannelLoadDescription>>;
}
