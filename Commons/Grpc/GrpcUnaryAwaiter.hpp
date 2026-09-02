#pragma once

#include <atomic>
#include <coroutine>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <utility>

#include <grpcpp/support/status.h>

#include <Commons/Coro/Utils.hpp>
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

    bool
    await_suspend(std::coroutine_handle<> handle);

    ResultType
    await_resume();

  private:
    struct State
    {
      enum class Status
      {
        Running,
        Suspended,
        Completed
      };

      std::mutex lock;
      std::atomic<Status> status{Status::Running};
      grpc::Status grpc_status;
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
  bool
  GrpcUnaryAwaiter<
    AsyncClientType,
    RequestType,
    ResponseType,
    ResultType>::await_suspend(std::coroutine_handle<> handle)
  {
    auto state = state_;
    auto client = client_;
    auto executor_pool = executor_pool_;
    auto complete =
      [state, executor_pool, handle](
        const grpc::Status* status,
        ResponseHolder<ResponseType>* response_holder,
        std::exception_ptr exception) mutable
      {
        typename State::Status previous_status;
        {
          std::lock_guard<std::mutex> guard(state->lock);
          if (state->status.load(std::memory_order_acquire) == State::Status::Completed)
          {
            return;
          }

          if (exception)
          {
            state->exception.emplace(std::move(exception));
          }
          else
          {
            try
            {
              state->grpc_status = *status;
              state->response_holder = std::move(*response_holder);
            }
            catch (...)
            {
              state->exception.emplace(std::current_exception());
            }
          }

          previous_status = state->status.exchange(
            State::Status::Completed,
            std::memory_order_acq_rel);
        }

        if (previous_status != State::Status::Suspended)
        {
          return;
        }

        try
        {
          executor_pool->post(
            [handle]() mutable
            {
              AdServer::Commons::resume_coroutine(handle);
            });
        }
        catch (...)
        {
          AdServer::Commons::resume_coroutine(handle);
        }
      };

    try
    {
      (client.get()->*method_)(
        request_,
        [complete](
          const grpc::Status& status,
          ResponseHolder<ResponseType>&& response_holder) mutable
        {
          complete(&status, &response_holder, {});
        });
    }
    catch (...)
    {
      complete(nullptr, nullptr, std::current_exception());
    }

    auto expected_status = State::Status::Running;
    return state->status.compare_exchange_strong(
      expected_status,
      State::Status::Suspended,
      std::memory_order_acq_rel,
      std::memory_order_acquire);
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
      if (state_->grpc_status.ok())
      {
        throw std::logic_error(default_response_error_);
      }

      state_->response_holder = ResponseHolder<ResponseType>::make_value(ResponseType());
    }

    const auto& response = state_->response_holder.get();
    return {
      std::move(state_->grpc_status),
      std::move(state_->response_holder),
      response};
  }
}
