// THIS
#include "Commons/CorbaAlgs.hpp"
#include "GrpcService.hpp"

namespace AdServer::UserInfoSvcs
{

namespace
{

const char ASPECT[] = "GrpcService";

} // namespace

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
    logger->error(stream.str(), ASPECT);
  }
}

GetBindRequestService::GetBindRequestService(
  Logger* logger,
  UserBindServerImpl* user_bind_server_impl)
  : logger_(ReferenceCounting::add_ref(logger)),
    user_bind_server_impl_(ReferenceCounting::add_ref(user_bind_server_impl))
{
}

void GetBindRequestService::handle(const Reader& reader)
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
        auto response = user_bind_server_impl_->get_bind_request(
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
        logger_->error(stream.str(), ASPECT);
      }
    }
    else if (status == ReadStatus::Finish)
    {
      break;
    }
  }
}

AddBindRequestService::AddBindRequestService(
  Logger* logger,
  UserBindServerImpl* user_bind_server_impl)
  : logger_(ReferenceCounting::add_ref(logger)),
    user_bind_server_impl_(ReferenceCounting::add_ref(user_bind_server_impl))
{
}

void AddBindRequestService::handle(const Reader& reader)
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
        auto response = user_bind_server_impl_->add_bind_request(
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
        logger_->error(stream.str(), ASPECT);
      }
    }
    else if (status == ReadStatus::Finish)
    {
      break;
    }
  }
}

GetUserIdService::GetUserIdService(
  Logger* logger,
  UserBindServerImpl* user_bind_server_impl)
  : logger_(ReferenceCounting::add_ref(logger)),
    user_bind_server_impl_(ReferenceCounting::add_ref(user_bind_server_impl))
{
}

void GetUserIdService::handle(const Reader& reader)
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
        auto response = user_bind_server_impl_->get_user_id(
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
        logger_->error(stream.str(), ASPECT);
      }
    }
    else if (status == ReadStatus::Finish)
    {
      break;
    }
  }
}

AddUserIdService::AddUserIdService(
  Logger* logger,
  UserBindServerImpl* user_bind_server_impl)
  : logger_(ReferenceCounting::add_ref(logger)),
    user_bind_server_impl_(ReferenceCounting::add_ref(user_bind_server_impl))
{
}

void AddUserIdService::handle(const Reader& reader)
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
        auto response = user_bind_server_impl_->add_user_id(
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
        logger_->error(stream.str(), ASPECT);
      }
    }
    else if (status == ReadStatus::Finish)
    {
      break;
    }
  }
}

GetSourceService::GetSourceService(
  Logger* logger,
  UserBindServerImpl* user_bind_server_impl)
  : logger_(ReferenceCounting::add_ref(logger)),
    user_bind_server_impl_(ReferenceCounting::add_ref(user_bind_server_impl))
{
}

void GetSourceService::handle(const Reader& reader)
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
        auto response = user_bind_server_impl_->get_source(
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
        logger_->error(stream.str(), ASPECT);
      }
    }
    else if (status == ReadStatus::Finish)
    {
      break;
    }
  }
}

} // namespace AdServer::UserInfoSvcs