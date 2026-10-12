#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include <Generics/ActiveObject.hpp>
#include <Commons/Grpc/ProcessControl.grpc.pb.h>

namespace AdServer::Grpc
{
  class DbAccessService final : public adserver::grpc::process_control::ProcessControl::Service
  {
  public:
    using GetState = std::function<std::optional<bool>()>;
    using SetState = std::function<void(bool)>;
    using GetStatus = std::function<void(adserver::grpc::process_control::GetStatusResponse&)>;

    DbAccessService(GetState get_state, SetState set_state, GetStatus get_status);
    void stop();

    ::grpc::Status get_status(
      ::grpc::ServerContext* context,
      const adserver::grpc::process_control::GetStatusRequest* request,
      adserver::grpc::process_control::GetStatusResponse* response) override;
    ::grpc::Status get_db_state(
      ::grpc::ServerContext* context,
      const adserver::grpc::process_control::GetDbStateRequest* request,
      adserver::grpc::process_control::GetDbStateResponse* response) override;
    ::grpc::Status set_db_state(
      ::grpc::ServerContext* context,
      const adserver::grpc::process_control::SetDbStateRequest* request,
      adserver::grpc::process_control::SetDbStateResponse* response) override;

  private:
    ::grpc::Status invoke_(::grpc::ServerContext* context, const std::function<void()>& action);

    GetState get_state_;
    SetState set_state_;
    GetStatus get_status_;
    std::mutex mutex_;
    std::atomic<bool> stopping_{false};
  };

  class DbAccessServer final : public Generics::SimpleActiveObject
  {
  public:
    // Additional services are borrowed and must outlive the server.
    DbAccessServer(
      std::string address, unsigned int threads, DbAccessService::GetState get_state,
      DbAccessService::SetState set_state, DbAccessService::GetStatus get_status,
      std::vector<::grpc::Service*> additional_services = {});
    ~DbAccessServer() noexcept override;

  private:
    void activate_object_() override;
    void deactivate_object_() override;
    void wait_object_() override;

    const std::string address_;
    const unsigned int threads_;
    DbAccessService service_;
    const std::vector<::grpc::Service*> additional_services_;
    std::unique_ptr<::grpc::Server> server_;
  };
}
