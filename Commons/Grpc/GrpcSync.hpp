#pragma once

#include <future>
#include <sstream>
#include <stdexcept>
#include <utility>

#include <grpcpp/support/status.h>

#include <Commons/Grpc/ResponseHolder.hpp>

namespace AdServer::Grpc
{
  inline const char*
  status_code_name(grpc::StatusCode code) noexcept
  {
    switch (code)
    {
    case grpc::StatusCode::OK:
      return "OK";
    case grpc::StatusCode::CANCELLED:
      return "CANCELLED";
    case grpc::StatusCode::UNKNOWN:
      return "UNKNOWN";
    case grpc::StatusCode::INVALID_ARGUMENT:
      return "INVALID_ARGUMENT";
    case grpc::StatusCode::DEADLINE_EXCEEDED:
      return "DEADLINE_EXCEEDED";
    case grpc::StatusCode::NOT_FOUND:
      return "NOT_FOUND";
    case grpc::StatusCode::ALREADY_EXISTS:
      return "ALREADY_EXISTS";
    case grpc::StatusCode::PERMISSION_DENIED:
      return "PERMISSION_DENIED";
    case grpc::StatusCode::RESOURCE_EXHAUSTED:
      return "RESOURCE_EXHAUSTED";
    case grpc::StatusCode::FAILED_PRECONDITION:
      return "FAILED_PRECONDITION";
    case grpc::StatusCode::ABORTED:
      return "ABORTED";
    case grpc::StatusCode::OUT_OF_RANGE:
      return "OUT_OF_RANGE";
    case grpc::StatusCode::UNIMPLEMENTED:
      return "UNIMPLEMENTED";
    case grpc::StatusCode::INTERNAL:
      return "INTERNAL";
    case grpc::StatusCode::UNAVAILABLE:
      return "UNAVAILABLE";
    case grpc::StatusCode::DATA_LOSS:
      return "DATA_LOSS";
    case grpc::StatusCode::UNAUTHENTICATED:
      return "UNAUTHENTICATED";
    default:
      return "UNKNOWN";
    }
  }

  inline void throw_grpc_error(const grpc::Status& status)
  {
    if (!status.error_message().empty())
    {
      throw std::runtime_error(status.error_message());
    }

    if (!status.error_details().empty())
    {
      throw std::runtime_error(status.error_details());
    }

    std::ostringstream ostr;
    ostr << "gRPC call failed: " << status_code_name(status.error_code());
    throw std::runtime_error(ostr.str());
  }

  template<typename Response, typename Start, typename ErrorHandler>
  Response sync_call(Start&& start, ErrorHandler&& error_handler)
  {
    std::promise<std::pair<grpc::Status, Response>> promise;
    auto future = promise.get_future();

    start([&](const grpc::Status& status, ResponseHolder<Response>&& response_holder)
    {
      promise.set_value(std::make_pair(status, response_holder.get()));
    });

    auto result = future.get();
    if (!result.first.ok())
    {
      error_handler(result.first);
    }

    return std::move(result.second);
  }

  template<typename Response, typename Start>
  Response sync_call(Start&& start)
  {
    return sync_call<Response>(
      std::forward<Start>(start),
      [](const grpc::Status& status)
      {
        throw_grpc_error(status);
      });
  }
}
