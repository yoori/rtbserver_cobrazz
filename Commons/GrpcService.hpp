#ifndef CHANNELSVCS_CHANNELSERVER_GRPCSERVICE
#define CHANNELSVCS_CHANNELSERVER_GRPCSERVICE

// UNIXCOMMONS
#include <Generics/TaskPool.hpp>
#include <Logger/Logger.hpp>
#include <UServerUtils/Grpc/Server/DefaultErrorCreator.hpp>
#include <UServerUtils/Grpc/Server/ServiceCoro.hpp>

// USERVER
#include <engine/task/task_processor.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/task/task_with_result.hpp>

// THIS
#include <Commons/DelegateTaskGoal.hpp>

namespace AdServer::Commons
{

namespace Aspect
{

constexpr char GRPC_SERVICE[] = "GrpcService";

} // namespace Aspect

template<class Request, class Response>
class DefaultErrorCreator final
  : public UServerUtils::Grpc::Server::DefaultErrorCreator<Response>
{
public:
  DefaultErrorCreator(const Request& request)
  {
    id_request_grpc_ = request.id_request_grpc();
  }

  ~DefaultErrorCreator() override = default;

  std::unique_ptr<Response> create() noexcept override
  {
    static const std::string descritpion =
      "write was not called due to the coroutine being closed "
      "or an internal error";
    auto response = std::make_unique<Response>();
    response->set_id_request_grpc(id_request_grpc_);
    auto* error = response->mutable_error();
    error->set_type(std::decay_t<decltype(error->type())>::Error_Type_Implementation);
    error->set_description(descritpion);

    return response;
  }

private:
  std::uint32_t id_request_grpc_;
};

template<
  class BaseService,
  class Impl,
  std::unique_ptr<typename BaseService::Response>
  (Impl::*ptr_to_mem)(std::unique_ptr<typename BaseService::Request>&&)>
class GrpcService final
  : public BaseService,
    public ReferenceCounting::AtomicImpl
{
public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using ReadStatus = UServerUtils::Grpc::Server::ReadStatus;
  using Reader = typename BaseService::Reader;
  using Impl_var = ReferenceCounting::SmartPtr<Impl>;
  using TaskPool = Generics::TaskPool;
  using TaskPool_var = Generics::TaskPool_var;

public:
  explicit GrpcService(
    Logger* logger,
    Impl* impl,
    TaskPool* task_pool)
    : logger_(ReferenceCounting::add_ref(logger)),
      impl_(ReferenceCounting::add_ref(impl)),
      task_pool_(ReferenceCounting::add_ref(task_pool))
  {
  }

  void handle(const Reader& reader) override
  {
     auto data = reader.read();
     // only for UServerUtils::Grpc::Server::ServiceMode::EventToCoroutine
     assert(reader.is_finish());

     const auto status = data.status;
     if (status == ReadStatus::Read)
     {
       auto& request = data.request;
       auto& writer = data.writer;

       try
       {
         if (task_pool_)
         {
           auto task = Commons::make_delegate_task(
             [writer = std::move(writer),
              request = std::move(request),
              this] () mutable {
               try
               {
                 auto response = (impl_->*ptr_to_mem)(std::move(request));
                 send_reponse(
                   std::move(writer),
                   std::move(response),
                   logger_.in());
               }
               catch (const eh::Exception& exc)
               {
                 Stream::Error stream;
                 stream << FNS
                        << exc.what();
                 logger_->error(stream.str(), Aspect::GRPC_SERVICE);
               }
               catch (...)
               {
                 Stream::Error stream;
                 stream << FNS
                        << "Unknown error";
                 logger_->error(stream.str(), Aspect::GRPC_SERVICE);
               }
           });
           task_pool_->enqueue_task(task.in());
         }
         else
         {
           auto response = (impl_->*ptr_to_mem)(std::move(request));
           send_reponse(
             std::move(writer),
             std::move(response),
             logger_.in());
         }
       }
       catch (const eh::Exception& exc)
       {
         Stream::Error stream;
         stream << FNS
                << exc.what();
         logger_->error(stream.str(), Aspect::GRPC_SERVICE);
       }
       catch (...)
       {
         Stream::Error stream;
         stream << FNS
                << "Unknown error";
         logger_->error(stream.str(), Aspect::GRPC_SERVICE);
       }
     }
  }

  typename BaseService::DefaultErrorCreatorPtr default_error_creator(
    const typename BaseService::Request& request) noexcept override
  {
    using Request = typename BaseService::Request;
    using Response = typename BaseService::Response;

    return std::make_unique<DefaultErrorCreator<Request, Response>>(request);
  }

protected:
  ~GrpcService() override = default;

private:
  template<class Writer, class Response>
  static void send_reponse(
    std::unique_ptr<Writer>&& writer,
    std::unique_ptr<Response>&& response,
    Logging::Logger* logger)
  {
    using WriterStatus = UServerUtils::Grpc::Server::WriterStatus;

    const auto write_status = writer->write(std::move(response));
    if (write_status != WriterStatus::Ok)
    {
      Stream::Error stream;
      stream << FNS
             << "Write is failed. Reason: "
             << (write_status == WriterStatus::RpcClosed ?
                 "rpc already closed" : "internal error");
      logger->error(stream.str(), Aspect::GRPC_SERVICE);
    }
  }

private:
  const Logger_var logger_;

  const Impl_var impl_;

  TaskPool_var task_pool_;
};

template<
  class BaseService,
  class Impl,
  std::unique_ptr<typename BaseService::Response>
  (Impl::*ptr_to_mem)(std::unique_ptr<typename BaseService::Request>&&)>
using GrpcService_var = ReferenceCounting::SmartPtr<GrpcService<BaseService, Impl, ptr_to_mem>>;

template<
  class BaseService,
  class Impl,
  std::unique_ptr<typename BaseService::Response>
  (Impl::*ptr_to_mem)(std::unique_ptr<typename BaseService::Request>&&)>
auto create_grpc_service(
  Logging::Logger* logger,
  Impl* impl,
  Generics::TaskPool* task_pool)
{
  using Service = GrpcService<BaseService, Impl, ptr_to_mem>;
  return ReferenceCounting::SmartPtr<Service>(
    new Service(
      logger,
      impl,
      task_pool));
}

} // namespace AdServer::ChannelSvcs

#endif // CHANNELSVCS_CHANNELSERVER_GRPCSERVICE