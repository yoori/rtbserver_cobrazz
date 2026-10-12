#include "DbAccess.hpp"

#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <utility>
#include <grpcpp/grpcpp.h>

namespace AdServer::Grpc
{
  namespace pc = adserver::grpc::process_control;

  DbAccessService::DbAccessService(GetState get_state, SetState set_state, GetStatus get_status)
    : get_state_(std::move(get_state)),
      set_state_(std::move(set_state)),
      get_status_(std::move(get_status))
  {}

  void DbAccessService::stop()
  {
    stopping_.store(true);
    // Drain an in-flight switch before the owning service deactivates its DB objects.
    std::lock_guard lock(mutex_);
  }

  ::grpc::Status DbAccessService::invoke_(
    ::grpc::ServerContext* context, const std::function<void()>& action)
  {
    std::lock_guard lock(mutex_);
    if (stopping_.load())
    {
      return {::grpc::StatusCode::UNAVAILABLE, "Service is stopping"};
    }

    if (context->IsCancelled())
    {
      return {::grpc::StatusCode::CANCELLED, "DBAccess request was cancelled"};
    }

    try
    {
      action();
      return ::grpc::Status::OK;
    }
    catch (const std::exception& error)
    {
      return {::grpc::StatusCode::FAILED_PRECONDITION, error.what()};
    }
    catch (...)
    {
      return {::grpc::StatusCode::INTERNAL, "Unknown DBAccess failure"};
    }
  }

  ::grpc::Status DbAccessService::get_status(
    ::grpc::ServerContext* context, const pc::GetStatusRequest*, pc::GetStatusResponse* response)
  {
    return invoke_(context, [&] { get_status_(*response); });
  }

  ::grpc::Status DbAccessService::get_db_state(
    ::grpc::ServerContext* context, const pc::GetDbStateRequest*, pc::GetDbStateResponse* response)
  {
    return invoke_(context, [&]
    {
      const auto state = get_state_();
      response->set_supported(state.has_value());
      response->set_enabled(state.value_or(false));
    });
  }

  ::grpc::Status DbAccessService::set_db_state(
    ::grpc::ServerContext* context, const pc::SetDbStateRequest* request, pc::SetDbStateResponse*)
  {
    return invoke_(context, [&]
    {
      const auto state = get_state_();
      if (!state)
      {
        throw std::runtime_error("DB access is not applicable to this service configuration");
      }

      if (*state != request->enabled())
      {
        set_state_(request->enabled());
      }

      if (get_state_() != std::optional<bool>(request->enabled()))
      {
        throw std::runtime_error("DB state did not reach the requested value");
      }
    });
  }

  DbAccessServer::DbAccessServer(
    std::string address, unsigned int threads, DbAccessService::GetState get_state,
    DbAccessService::SetState set_state, DbAccessService::GetStatus get_status,
    std::vector<::grpc::Service*> additional_services)
    : address_(std::move(address)),
      threads_(std::max(1U, threads)),
      service_(std::move(get_state), std::move(set_state), std::move(get_status)),
      additional_services_(std::move(additional_services))
  {}

  DbAccessServer::~DbAccessServer() noexcept
  {
    deactivate_object();
    wait_object();
  }

  void DbAccessServer::activate_object_()
  {
    ::grpc::ServerBuilder builder;
    builder.AddListeningPort(address_, ::grpc::InsecureServerCredentials());
    builder.SetSyncServerOption(::grpc::ServerBuilder::MAX_POLLERS, threads_);
    builder.RegisterService(&service_);
    for (auto* service : additional_services_)
    {
      builder.RegisterService(service);
    }

    if (!additional_services_.empty())
    {
      builder.SetMaxSendMessageSize(-1);
      builder.SetMaxReceiveMessageSize(-1);
    }

    server_ = builder.BuildAndStart();
    if (!server_)
    {
      throw Exception("Cannot start DBAccess gRPC server at " + address_);
    }
  }

  void DbAccessServer::deactivate_object_()
  {
    service_.stop();
    if (server_)
    {
      server_->Shutdown(std::chrono::system_clock::now());
    }
  }

  void DbAccessServer::wait_object_()
  {
    if (server_)
    {
      server_->Wait();
      server_.reset();
    }
  }
}
