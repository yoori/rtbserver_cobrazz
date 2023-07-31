#ifndef USERINFOSVCS_USERINFOMANAGER_GRPCSERVICE_HPP_
#define USERINFOSVCS_USERINFOMANAGER_GRPCSERVICE_HPP_

#include "UserInfoManager_service.cobrazz.pb.hpp"

// THIS
#include <Logger/Logger.hpp>
#include "UserInfoManagerImpl.hpp"

namespace AdServer::UserInfoSvcs
{

namespace Internal
{

constexpr char ASPECT[] = "GrpcService";

} // namespace

template<
  class BaseService,
  std::unique_ptr<typename BaseService::Response>
  (UserInfoManagerImpl::*ptr_to_mem)(
    std::unique_ptr<typename BaseService::Request>&&)>
class GrpcService final
  : public BaseService,
    public ReferenceCounting::AtomicImpl
{
public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using ReadStatus = UServerUtils::Grpc::Core::Server::ReadStatus;
  using Reader = typename BaseService::Reader;
  using WriterPtr = typename Reader::WriterPtr;

public:
  explicit GrpcService(
    Logger* logger,
    UserInfoManagerImpl* user_info_manager_impl)
    : logger_(ReferenceCounting::add_ref(logger)),
      user_info_manager_impl_(ReferenceCounting::add_ref(user_info_manager_impl))
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
          auto response = (user_info_manager_impl_->*ptr_to_mem)(
            std::move(request));
          send_reponse(
            std::move(writer),
            std::move(response),
            logger_.in());
        }
        catch (const eh::Exception& exc)
        {
          Stream::Error stream;
          stream << FNS
                 << ": "
                 << exc.what();
          logger_->error(stream.str(), Internal::ASPECT);
        }
        catch (...)
        {
          Stream::Error stream;
          stream << FNS
                 << ": Unknown error";
          logger_->error(stream.str(), Internal::ASPECT);
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
  void send_reponse(
    std::unique_ptr<Writer>&& writer,
    std::unique_ptr<Response>&& response,
    Logging::Logger* logger)
  {
    using WriterStatus =
      UServerUtils::Grpc::Core::Server::WriterStatus;

    const auto write_status = writer->write(
      std::move(response));
    if (write_status != WriterStatus::Ok)
    {
      Stream::Error stream;
      stream << FNS
             << ": Write is failed. Reason: "
             << (write_status == WriterStatus::RpcClosed ?
                 "rpc already closed" : "internal error");
      logger->error(stream.str(), Internal::ASPECT);
    }
  }

private:
  Logger_var logger_;

  UserInfoManagerImpl_var user_info_manager_impl_;
};

template<
  class BaseService,
  std::unique_ptr<typename BaseService::Response>
  (UserInfoManagerImpl::*ptr_to_mem)(
    std::unique_ptr<typename BaseService::Request>&&)>
using GrpcService_var =
  ReferenceCounting::SmartPtr<GrpcService<BaseService, ptr_to_mem>>;

template<
  class BaseService,
  std::unique_ptr<typename BaseService::Response>
  (UserInfoManagerImpl::*ptr_to_mem)(
    std::unique_ptr<typename BaseService::Request>&&)>
auto create_grpc_service(
  Logging::Logger* logger,
  UserInfoManagerImpl* user_info_manager_impl)
{
  return ReferenceCounting::SmartPtr<GrpcService<BaseService, ptr_to_mem>>(
    new GrpcService<BaseService, ptr_to_mem>(
      logger,
      user_info_manager_impl));
}

} // namespace AdServer::UserInfoSvcs

#endif /* USERINFOSVCS_USERINFOMANAGER_GRPCSERVICE_HPP_ */