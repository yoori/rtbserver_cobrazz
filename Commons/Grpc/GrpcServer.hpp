#pragma once

#include <chrono>
#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <grpcpp/grpcpp.h>

#include <Generics/ActiveObject.hpp>
#include <Logger/Logger.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Stream/MemoryStream.hpp>

#include <Commons/Grpc/GrpcServiceBase.hpp>

namespace AdServer::Grpc
{
  inline ::grpc::Status
  error_status(::grpc::StatusCode status_code, const char* message)
  {
    return ::grpc::Status(status_code, message ? message : "");
  }

  template<typename ServiceImplType>
  class GrpcServer final:
    public Generics::SimpleActiveObject
  {
  public:
    GrpcServer(
      Logging::Logger* logger,
      std::string_view aspect,
      std::string_view bind_address,
      std::size_t threads,
      std::unique_ptr<ServiceImplType> service);

    ServiceImplType& service() noexcept
    {
      return *service_;
    }

    const ServiceImplType& service() const noexcept
    {
      return *service_;
    }

  private:
    void activate_object_() override;

    void deactivate_object_() override;

    void wait_object_() override;

    void process_queue_loop_(::grpc::ServerCompletionQueue* completion_queue);

  private:
    Logging::Logger_var logger_;
    const std::string aspect_;
    const std::string bind_address_;
    const std::size_t threads_;
    std::unique_ptr<ServiceImplType> service_;
    std::unique_ptr<::grpc::Server> server_;
    std::vector<std::unique_ptr<::grpc::ServerCompletionQueue>> completion_queues_;
    std::vector<std::thread> workers_;
    std::optional<std::thread> server_wait_thread_;
  };

  template<typename ServiceImplType>
  GrpcServer<ServiceImplType>::GrpcServer(
    Logging::Logger* logger,
    std::string_view aspect,
    std::string_view bind_address,
    std::size_t threads,
    std::unique_ptr<ServiceImplType> service)
    : logger_(ReferenceCounting::add_ref(logger)),
      aspect_(aspect),
      bind_address_(bind_address),
      threads_(std::max<std::size_t>(1, threads)),
      service_(std::move(service))
  {}

  template<typename ServiceImplType>
  void GrpcServer<ServiceImplType>::activate_object_()
  {
    ::grpc::ServerBuilder builder;
    builder.SetMaxReceiveMessageSize(-1);
    builder.SetMaxSendMessageSize(-1);
    builder.AddListeningPort(bind_address_, ::grpc::InsecureServerCredentials());
    service_->register_services(builder);

    const auto completion_queues_count = threads_;
    completion_queues_.reserve(completion_queues_count);
    for (std::size_t i = 0; i < completion_queues_count; ++i)
    {
      completion_queues_.emplace_back(builder.AddCompletionQueue());
    }

    server_ = builder.BuildAndStart();
    if (!server_)
    {
      Stream::Error ostr;
      ostr << "Can't start gRPC server at " << bind_address_;
      throw Exception(ostr);
    }

    GrpcServiceBase::CompletionQueues raw_completion_queues;
    raw_completion_queues.reserve(completion_queues_.size());
    for (auto& completion_queue : completion_queues_)
    {
      raw_completion_queues.push_back(completion_queue.get());
    }
    service_->start(raw_completion_queues);

    workers_.reserve(completion_queues_.size());
    for (auto& completion_queue : completion_queues_)
    {
      auto* completion_queue_ptr = completion_queue.get();
      workers_.emplace_back([this, completion_queue_ptr]() {
        process_queue_loop_(completion_queue_ptr);
      });
    }

    if (completion_queues_.empty())
    {
      server_wait_thread_.emplace([this]() {
        server_->Wait();
      });
    }

    if (logger_)
    {
      logger_->sstream(Logging::Logger::NOTICE, aspect_.c_str()) <<
        "gRPC endpoint started at " << bind_address_ <<
        ", threads = " << threads_;
    }
  }

  template<typename ServiceImplType>
  void GrpcServer<ServiceImplType>::deactivate_object_()
  {
    service_->stop_accepting_requests();
    service_->stop_finishing_requests();

    if (server_)
    {
      server_->Shutdown(std::chrono::system_clock::now());
    }

    for (auto& completion_queue : completion_queues_)
    {
      if (completion_queue)
      {
        completion_queue->Shutdown();
      }
    }
  }

  template<typename ServiceImplType>
  void GrpcServer<ServiceImplType>::wait_object_()
  {
    for (auto& worker : workers_)
    {
      worker.join();
    }
    workers_.clear();

    if (server_wait_thread_)
    {
      server_wait_thread_->join();
      server_wait_thread_.reset();
    }

    if (server_)
    {
      if (!completion_queues_.empty())
      {
        server_->Wait();
      }
      server_.reset();
    }

    completion_queues_.clear();
  }

  template<typename ServiceImplType>
  void GrpcServer<ServiceImplType>::process_queue_loop_(
    ::grpc::ServerCompletionQueue* completion_queue)
  {
    void* tag = nullptr;
    bool ok = false;
    while (completion_queue && completion_queue->Next(&tag, &ok))
    {
      auto* call = static_cast<GrpcAsyncCall*>(tag);
      call->proceed(ok);
    }
  }
}
