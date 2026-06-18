#pragma once

#include <memory>
#include <stdexcept>
#include <utility>

#include <google/protobuf/arena.h>

namespace AdServer::Grpc
{
  template<typename Response>
  class ResponseHolder
  {
  public:
    ResponseHolder() = default;

    static ResponseHolder make_value(Response&& response)
    {
      auto owner = std::make_shared<Response>(std::move(response));
      auto* response_ptr = owner.get();
      return ResponseHolder(response_ptr, std::move(owner));
    }

    static ResponseHolder make_arena(
      Response& response,
      std::shared_ptr<google::protobuf::Arena> arena)
    {
      return ResponseHolder(&response, std::move(arena));
    }

    Response& get()
    {
      if (!response_)
      {
        throw std::logic_error("gRPC response is not initialized");
      }
      return *response_;
    }

    const Response& get() const
    {
      if (!response_)
      {
        throw std::logic_error("gRPC response is not initialized");
      }
      return *response_;
    }

    Response& operator*()
    {
      return get();
    }

    const Response& operator*() const
    {
      return get();
    }

    Response* operator->()
    {
      return &get();
    }

    const Response* operator->() const
    {
      return &get();
    }

    explicit operator bool() const noexcept
    {
      return response_ != nullptr;
    }

  private:
    ResponseHolder(Response* response, std::shared_ptr<void> owner)
      : owner_(std::move(owner)),
        response_(response)
    {}

  private:
    std::shared_ptr<void> owner_;
    Response* response_ = nullptr;
  };
}
