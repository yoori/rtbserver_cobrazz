#pragma once

#include <coroutine>
#include <exception>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>

#include <grpcpp/support/status.h>

#include <Commons/ExecutorPool.hpp>
#include <Commons/Grpc/ResponseHolder.hpp>

namespace AdServer::Grpc
{
  template<
    typename AsyncClientType,
    typename RequestType,
    typename ResponseType,
    typename ResultType>
  class GrpcUnaryAwaiter final
  {
  public:
    using Callback = std::function<void(const grpc::Status&, ResponseHolder<ResponseType>&&)>;
    using Method = void (AsyncClientType::*)(const RequestType&, Callback);

    GrpcUnaryAwaiter(
      std::shared_ptr<AsyncClientType> client,
      std::shared_ptr<AdServer::Commons::ExecutorPool> executor_pool,
      const RequestType& request,
      Method method,
      const char* default_response_error);

    bool
    await_ready() const noexcept;

    void
    await_suspend(std::coroutine_handle<> handle) noexcept;

    ResultType
    await_resume();

  private:
    struct State
    {
      grpc::Status status;
      ResponseHolder<ResponseType> response_holder;
      std::optional<std::exception_ptr> exception;
    };

    std::shared_ptr<AsyncClientType> client_;
    std::shared_ptr<AdServer::Commons::ExecutorPool> executor_pool_;
    const RequestType& request_;
    Method method_;
    const char* default_response_error_;
    std::shared_ptr<State> state_;
  };

  template<
    typename AsyncClientType,
    typename RequestType,
    typename ResponseType,
    typename ResultType>
  GrpcUnaryAwaiter<
    AsyncClientType,
    RequestType,
    ResponseType,
    ResultType>::GrpcUnaryAwaiter(
      std::shared_ptr<AsyncClientType> client,
      std::shared_ptr<AdServer::Commons::ExecutorPool> executor_pool,
      const RequestType& request,
      Method method,
      const char* default_response_error)
    : client_(std::move(client)),
      executor_pool_(std::move(executor_pool)),
      request_(request),
      method_(method),
      default_response_error_(default_response_error),
      state_(std::make_shared<State>())
  {}

  template<
    typename AsyncClientType,
    typename RequestType,
    typename ResponseType,
    typename ResultType>
  bool
  GrpcUnaryAwaiter<
    AsyncClientType,
    RequestType,
    ResponseType,
    ResultType>::await_ready() const noexcept
  {
    return false;
  }

  template<
    typename AsyncClientType,
    typename RequestType,
    typename ResponseType,
    typename ResultType>
  void
  GrpcUnaryAwaiter<
    AsyncClientType,
    RequestType,
    ResponseType,
    ResultType>::await_suspend(std::coroutine_handle<> handle) noexcept
  {
    auto state = state_;
    auto client = client_;
    auto executor_pool = executor_pool_;
    try
    {
      auto callback_state = state;
      auto callback_executor_pool = executor_pool;
      (client.get()->*method_)(
        request_,
        [state = std::move(callback_state),
         executor_pool = std::move(callback_executor_pool),
         handle](const grpc::Status& status, ResponseHolder<ResponseType>&& response_holder) mutable
        {
          state->status = status;
          state->response_holder = std::move(response_holder);
          executor_pool->post([handle]() mutable { handle.resume(); });
        });
    }
    catch(...)
    {
      state->exception.emplace(std::current_exception());
      executor_pool->post([handle]() mutable { handle.resume(); });
    }
  }

  template<
    typename AsyncClientType,
    typename RequestType,
    typename ResponseType,
    typename ResultType>
  ResultType
  GrpcUnaryAwaiter<
    AsyncClientType,
    RequestType,
    ResponseType,
    ResultType>::await_resume()
  {
    if (state_->exception)
    {
      std::rethrow_exception(std::move(*state_->exception));
    }

    if (!state_->response_holder)
    {
      if (state_->status.ok())
      {
        throw std::logic_error(default_response_error_);
      }

      state_->response_holder = ResponseHolder<ResponseType>::make_value(ResponseType());
    }

    const auto& response = state_->response_holder.get();
    return {
      std::move(state_->status),
      std::move(state_->response_holder),
      response};
  }
}
