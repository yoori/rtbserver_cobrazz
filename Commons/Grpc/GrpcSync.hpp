#pragma once

#include <future>
#include <sstream>
#include <stdexcept>
#include <utility>

#include <grpcpp/support/status.h>

namespace AdServer::Grpc
{
  inline void throw_grpc_error(const grpc::Status& status)
  {
    std::ostringstream ostr;
    ostr << "gRPC call failed: code=" <<
      static_cast<int>(status.error_code()) <<
      ", message=" << status.error_message();
    throw std::runtime_error(ostr.str());
  }

  template<typename Response, typename Start, typename ErrorHandler>
  Response sync_call(Start&& start, ErrorHandler&& error_handler)
  {
    std::promise<std::pair<grpc::Status, Response>> promise;
    auto future = promise.get_future();

    start([&](
      const grpc::Status& status,
      const Response& response)
    {
      promise.set_value(std::make_pair(status, response));
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
