#pragma once

namespace AdServer::Grpc
{
  template<
    typename ServiceImplType,
    typename AsyncServiceType,
    typename Request,
    typename Response>
  GrpcCall<ServiceImplType, AsyncServiceType, Request, Response>
  make_grpc_call(
    typename GrpcCall<
      ServiceImplType,
      AsyncServiceType,
      Request,
      Response>::RequestMethod request_method,
    typename GrpcCall<
      ServiceImplType,
      AsyncServiceType,
      Request,
      Response>::Handler handler,
    std::string batch_full_method)
  {
    return {request_method, handler, std::move(batch_full_method)};
  }

  template<
    typename ServiceImplType,
    typename AsyncServiceType,
    typename Request,
    typename Response>
  GrpcCoroCall<ServiceImplType, AsyncServiceType, Request, Response>
  make_grpc_coro_call(
    typename GrpcCoroCall<
      ServiceImplType,
      AsyncServiceType,
      Request,
      Response>::RequestMethod request_method,
    typename GrpcCoroCall<
      ServiceImplType,
      AsyncServiceType,
      Request,
      Response>::Handler handler,
    std::string batch_full_method)
  {
    return {
      request_method,
      handler,
      nullptr,
      std::move(batch_full_method),
      {},
      false,
      true
    };
  }

  template<
    typename ServiceImplType,
    typename AsyncServiceType,
    typename Request,
    typename Response,
    typename HashFn>
  GrpcCoroCall<ServiceImplType, AsyncServiceType, Request, Response>
  make_distributed_grpc_coro_call(
    typename GrpcCoroCall<
      ServiceImplType,
      AsyncServiceType,
      Request,
      Response>::RequestMethod request_method,
    typename GrpcCoroCall<
      ServiceImplType,
      AsyncServiceType,
      Request,
      Response>::Handler handler,
    std::string batch_full_method,
    bool use_arena_for_response,
    HashFn hash_fn)
  {
    using Call = GrpcCoroCall<
      ServiceImplType,
      AsyncServiceType,
      Request,
      Response>;
    typename Call::BatchHashFn batch_hash =
      [hash_fn = std::move(hash_fn)](const Request& request)
      {
        return hash_fn(request);
      };

    return {
      request_method,
      handler,
      nullptr,
      std::move(batch_full_method),
      std::move(batch_hash),
      true,
      use_arena_for_response
    };
  }

  template<
    typename ServiceImplType,
    typename AsyncServiceType,
    typename Request,
    typename Response>
  GrpcCoroCall<ServiceImplType, AsyncServiceType, Request, Response>
  make_distributed_grpc_coro_call(
    typename GrpcCoroCall<
      ServiceImplType,
      AsyncServiceType,
      Request,
      Response>::RequestMethod request_method,
    typename GrpcCoroCall<
      ServiceImplType,
      AsyncServiceType,
      Request,
      Response>::Handler handler,
    std::string batch_full_method,
    bool use_arena_for_response)
  {
    return {
      request_method,
      handler,
      nullptr,
      std::move(batch_full_method),
      {},
      true,
      use_arena_for_response
    };
  }

  template<typename ServiceImplType, typename Calls>
  void
  GrpcServiceBase::register_batch_methods(ServiceImplType* service_impl, const Calls& calls)
  {
    std::apply(
      [this, service_impl](const auto&... call)
      {
        (register_batch_method(service_impl, call), ...);
      },
      calls);
  }

  template<typename Request, typename Response, typename Handler>
  void
  GrpcServiceBase::register_batch_method(
    std::string full_method,
    Handler&& handler)
  {
    batch_methods_.emplace(
      std::move(full_method),
      [handler = std::forward<Handler>(handler)](
        const adserver::grpc::BatchRequestItem& batch_request,
        adserver::grpc::BatchResponseItem& batch_response,
        google::protobuf::Arena& response_arena)
      {
        google::protobuf::Arena request_arena;
        auto* request = google::protobuf::Arena::CreateMessage<Request>(&request_arena);
        if (!request->ParseFromString(batch_request.payload()))
        {
          batch_response.set_status_code(::grpc::StatusCode::INVALID_ARGUMENT);
          batch_response.set_status_message("Unable to parse payload");
          return;
        }

        auto* response = google::protobuf::Arena::CreateMessage<Response>(&response_arena);
        ::grpc::Status status;
        handler(*request, *response, status);

        batch_response.set_status_code(status.error_code());
        batch_response.set_status_message(status.error_message());
        if (status.ok())
        {
          response->SerializeToString(batch_response.mutable_payload());
        }
      });
  }


  template<typename Request, typename Response, typename Handler>
  void
  GrpcServiceBase::register_batch_coro_method(
    std::string full_method,
    Handler&& handler,
    typename GrpcCoroCall<
      GrpcServiceBase,
      GrpcServiceBase,
      Request,
      Response>::BatchHashFn hash,
    bool distributed,
    bool use_arena_for_response)
  {
    batch_coro_methods_.emplace(
      std::move(full_method),
      BatchCoroMethod{
        [hash = std::move(hash)](
          const adserver::grpc::BatchRequestItem& batch_request,
          adserver::grpc::BatchResponseItem& batch_response,
          google::protobuf::Arena& request_arena,
          PreparedBatchCoroItem& coro_item)
        {
          auto* request = google::protobuf::Arena::CreateMessage<Request>(&request_arena);
          if (!request->ParseFromString(batch_request.payload()))
          {
            batch_response.set_status_code(
              ::grpc::StatusCode::INVALID_ARGUMENT);
            batch_response.set_status_message("Unable to parse payload");
            return false;
          }

          coro_item.request = request;
          if (hash)
          {
            coro_item.hash = hash(*request);
            coro_item.hash_present = true;
          }
          return true;
        },
        [
          handler = std::forward<Handler>(handler),
          use_arena_for_response
        ](
          void* request_ptr,
          adserver::grpc::BatchResponseItem& batch_response,
          google::protobuf::Arena& response_arena)
          -> AdServer::Commons::StartableAwaitable<void>
        {
          auto& request = *static_cast<Request*>(request_ptr);
          ::grpc::Status status;
          if (use_arena_for_response)
          {
            auto* response =
              google::protobuf::Arena::CreateMessage<Response>(&response_arena);
            co_await handler(std::move(request), *response, status);

            batch_response.set_status_code(status.error_code());
            batch_response.set_status_message(status.error_message());
            if (status.ok())
            {
              response->SerializeToString(batch_response.mutable_payload());
            }
          }
          else
          {
            Response response;
            co_await handler(std::move(request), response, status);

            batch_response.set_status_code(status.error_code());
            batch_response.set_status_message(status.error_message());
            if (status.ok())
            {
              response.SerializeToString(batch_response.mutable_payload());
            }
          }
        },
        distributed
      });
  }

  template<
    typename ServiceImplType,
    typename AsyncServiceType,
    typename Request,
    typename Response>
  void
  GrpcServiceBase::register_batch_method(
    ServiceImplType* service_impl,
    const GrpcCall<ServiceImplType, AsyncServiceType, Request, Response>& call)
  {
    if (call.batch_full_method.empty())
    {
      return;
    }

    register_batch_method<Request, Response>(
      call.batch_full_method,
      [service_impl, handler = call.handler](
        const Request& request,
        Response& response,
        ::grpc::Status& status)
      {
        (service_impl->*handler)(request, response, status);
      });
  }


  template<
    typename ServiceImplType,
    typename AsyncServiceType,
    typename Request,
    typename Response>
  void
  GrpcServiceBase::register_batch_method(
    ServiceImplType* service_impl,
    const GrpcCoroCall<ServiceImplType, AsyncServiceType, Request, Response>& call)
  {
    if (call.batch_full_method.empty())
    {
      return;
    }

    register_batch_coro_method<Request, Response>(
      call.batch_full_method,
      [service_impl, handler = call.handler](
        Request&& request,
        Response& response,
        ::grpc::Status& status) -> AdServer::Commons::StartableAwaitable<void>
      {
        co_await (service_impl->*handler)(std::move(request), response, status);
      },
      call.batch_hash,
      call.distributed_batch,
      call.use_arena_for_response);

    if (!call.batch_handler)
    {
      return;
    }

    register_batch_method<Request, Response>(
      call.batch_full_method,
      [service_impl, handler = call.batch_handler](
        const Request& request,
        Response& response,
        ::grpc::Status& status)
      {
        (service_impl->*handler)(request, response, status);
      });
  }

  template<
    typename ServiceImplType,
    typename ServiceType,
    typename AsyncServiceType>
  GrpcAsyncServiceBase<
    ServiceImplType,
    ServiceType,
    AsyncServiceType>::GrpcAsyncServiceBase(
      BatchStreamReadOptions batch_stream_read_options)
    : GrpcServiceBase(batch_stream_read_options)
  {
    add_grpc_service(&async_service_);
    add_grpc_service(&batch_transport_service_);
    register_batch_methods(ServiceImplType::grpc_calls());
  }

  template<
    typename ServiceImplType,
    typename ServiceType,
    typename AsyncServiceType>
  template<typename Request, typename Response>
  GrpcCall<ServiceImplType, AsyncServiceType, Request, Response>
  GrpcAsyncServiceBase<ServiceImplType, ServiceType, AsyncServiceType>::
    make_grpc_call(
      typename GrpcCall<
        ServiceImplType,
        AsyncServiceType,
        Request,
        Response>::RequestMethod request_method,
      typename GrpcCall<
        ServiceImplType,
        AsyncServiceType,
        Request,
        Response>::Handler handler,
      const char* batch_method_name)
  {
    return {
      request_method,
      handler,
      batch_method_name ?
        std::string("/") + ServiceType::service_full_name() + "/" + batch_method_name :
        std::string()
    };
  }


  template<
    typename ServiceImplType,
    typename ServiceType,
    typename AsyncServiceType>
  template<typename Request, typename Response>
  GrpcCoroCall<ServiceImplType, AsyncServiceType, Request, Response>
  GrpcAsyncServiceBase<ServiceImplType, ServiceType, AsyncServiceType>::
    make_grpc_coro_call(
      typename GrpcCoroCall<
        ServiceImplType,
        AsyncServiceType,
        Request,
        Response>::RequestMethod request_method,
      typename GrpcCoroCall<
        ServiceImplType,
        AsyncServiceType,
        Request,
        Response>::Handler handler,
      const char* batch_method_name)
  {
    return {
      request_method,
      handler,
      nullptr,
      batch_method_name ?
        std::string("/") + ServiceType::service_full_name() + "/" + batch_method_name :
        std::string(),
      {},
      false,
      true
    };
  }

  template<
    typename ServiceImplType,
    typename ServiceType,
    typename AsyncServiceType>
  template<typename Request, typename Response, typename HashFn>
  GrpcCoroCall<ServiceImplType, AsyncServiceType, Request, Response>
  GrpcAsyncServiceBase<ServiceImplType, ServiceType, AsyncServiceType>::
    make_distributed_grpc_coro_call(
      typename GrpcCoroCall<
        ServiceImplType,
        AsyncServiceType,
        Request,
        Response>::RequestMethod request_method,
      typename GrpcCoroCall<
        ServiceImplType,
        AsyncServiceType,
        Request,
        Response>::Handler handler,
      const char* batch_method_name,
      bool use_arena_for_response,
      HashFn hash_fn)
  {
    using Call = GrpcCoroCall<
      ServiceImplType,
      AsyncServiceType,
      Request,
      Response>;
    typename Call::BatchHashFn batch_hash =
      [hash_fn = std::move(hash_fn)](const Request& request)
      {
        return hash_fn(request);
      };

    return {
      request_method,
      handler,
      nullptr,
      batch_method_name ?
        std::string("/") + ServiceType::service_full_name() + "/" + batch_method_name :
        std::string(),
      std::move(batch_hash),
      true,
      use_arena_for_response
    };
  }

  template<
    typename ServiceImplType,
    typename ServiceType,
    typename AsyncServiceType>
  template<typename Request, typename Response>
  GrpcCoroCall<ServiceImplType, AsyncServiceType, Request, Response>
  GrpcAsyncServiceBase<ServiceImplType, ServiceType, AsyncServiceType>::
    make_distributed_grpc_coro_call(
      typename GrpcCoroCall<
        ServiceImplType,
        AsyncServiceType,
        Request,
        Response>::RequestMethod request_method,
      typename GrpcCoroCall<
        ServiceImplType,
        AsyncServiceType,
        Request,
        Response>::Handler handler,
      const char* batch_method_name,
      bool use_arena_for_response)
  {
    return {
      request_method,
      handler,
      nullptr,
      batch_method_name ?
        std::string("/") + ServiceType::service_full_name() + "/" + batch_method_name :
        std::string(),
      {},
      true,
      use_arena_for_response
    };
  }

  template<
    typename ServiceImplType,
    typename ServiceType,
    typename AsyncServiceType>
  template<typename Calls>
  void
  GrpcAsyncServiceBase<ServiceImplType, ServiceType, AsyncServiceType>::
    register_batch_methods(const Calls& calls)
  {
    GrpcServiceBase::register_batch_methods(
      static_cast<ServiceImplType*>(this),
      calls);
  }

  template<
    typename ServiceImplType,
    typename ServiceType,
    typename AsyncServiceType>
  void
  GrpcAsyncServiceBase<ServiceImplType, ServiceType, AsyncServiceType>::
    register_in_queue(::grpc::ServerCompletionQueue* completion_queue)
  {
    auto* service_impl = static_cast<ServiceImplType*>(this);
    std::apply(
      [this, service_impl, completion_queue](const auto&... call)
      {
        (register_grpc_unary_call(
          service_impl,
          &async_service_,
          call,
          completion_queue), ...);
      },
      ServiceImplType::grpc_calls());

    GrpcBatchStreamCall<
      ServiceImplType,
      adserver::grpc::BatchTransport::AsyncService>::create(
      service_impl,
      &batch_transport_service_,
      &adserver::grpc::BatchTransport::AsyncService::Requeststream_batches,
      completion_queue);
  }

}
