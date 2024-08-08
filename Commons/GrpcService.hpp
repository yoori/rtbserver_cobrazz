#ifndef CHANNELSVCS_CHANNELSERVER_GRPCSERVICE
#define CHANNELSVCS_CHANNELSERVER_GRPCSERVICE

// UNIXCOMMONS
#include <Logger/Logger.hpp>
#include <UServerUtils/Grpc/Server/ServiceCoro.hpp>

// USERVER
#include <userver/engine/task/task.hpp>
#include <userver/engine/task/task_processor.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/task/task_with_result.hpp>

namespace AdServer::Commons
{

constexpr char GRPC_SERVICE_ASPECT[] = "GrpcService";

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
  using WriterPtr = typename Reader::WriterPtr;
  using Impl_var = ReferenceCounting::SmartPtr<Impl>;

public:
  explicit GrpcService(
    Logger* logger,
    Impl* impl,
    const bool need_create_new_coroutine = true)
    : logger_(ReferenceCounting::add_ref(logger)),
      impl_(ReferenceCounting::add_ref(impl)),
      need_create_new_coroutine_(need_create_new_coroutine)
  {
  }

  ~GrpcService() override = default;

  void handle(const Reader& reader) override
  {
    while(true)
    {
      auto data = reader.read();
      const auto status = data.status;

      if (status == ReadStatus::Read)
      {
        auto& request = data.request;
        auto& writer = data.writer;

        try
        {
          if (need_create_new_coroutine_)
          {
            auto& current_task_processor = userver::engine::current_task::GetTaskProcessor();
            userver::engine::AsyncNoSpan(
              current_task_processor,
              [request = std::move(request),
               writer = std::move(writer),
               impl = impl_,
               logger = logger_] () mutable {
                  try
                  {
                    auto response = (impl->*ptr_to_mem)(std::move(request));
                    send_reponse(std::move(writer), std::move(response), logger.in());
                  }
                  catch (const eh::Exception& exc)
                  {
                    Stream::Error stream;
                    stream << FNS
                           << ": "
                           << exc.what();
                    logger->error(stream.str(), GRPC_SERVICE_ASPECT);
                  }
                  catch (...)
                  {
                    Stream::Error stream;
                    stream << FNS
                           << ": Unknown error";
                    logger->error(stream.str(), GRPC_SERVICE_ASPECT);
                  }
              }).Detach();
          }
          else
          {
            auto response = (impl_->*ptr_to_mem)(std::move(request));
            send_reponse(std::move(writer), std::move(response), logger_.in());
          }
        }
        catch (const eh::Exception& exc)
        {
          Stream::Error stream;
          stream << FNS
                 << ": "
                 << exc.what();
          logger_->error(stream.str(), GRPC_SERVICE_ASPECT);
        }
        catch (...)
        {
          Stream::Error stream;
          stream << FNS
                 << ": Unknown error";
          logger_->error(stream.str(), GRPC_SERVICE_ASPECT);
        }
      }
      else if (status == ReadStatus::Finish)
      {
        break;
      }
    }
  }

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
             << ": Write is failed. Reason: "
             << (write_status == WriterStatus::RpcClosed ?
                 "rpc already closed" : "internal error");
      logger->error(stream.str(), GRPC_SERVICE_ASPECT);
    }
  }

private:
  const Logger_var logger_;

  const Impl_var impl_;

  const bool need_create_new_coroutine_ = true;
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
auto create_grpc_service(Logging::Logger* logger, Impl* impl)
{
  using Service = GrpcService<BaseService, Impl, ptr_to_mem>;
  return ReferenceCounting::SmartPtr<Service>(
    new Service(logger, impl));
}

} // namespace AdServer::ChannelSvcs

#endif // CHANNELSVCS_CHANNELSERVER_GRPCSERVICE