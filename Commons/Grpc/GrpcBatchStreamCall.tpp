#pragma once

#include <Commons/ExecutorPool.hpp>

namespace AdServer::Grpc
{
  template<typename ServiceImplType, typename AsyncServiceType>
  struct GrpcBatchStreamCall<ServiceImplType, AsyncServiceType>::BatchContext
    : GrpcServiceBase::BatchResponsePublisher,
      std::enable_shared_from_this<BatchContext>
  {
    google::protobuf::Arena& response_arena() noexcept override
    {
      return response_arena_storage;
    }

    std::shared_ptr<GrpcServiceBase::BatchResponsePublisher> retain()
      noexcept override
    {
      return this->shared_from_this();
    }

    Response* create_response() override
    {
      return google::protobuf::Arena::CreateMessage<Response>(
        &response_arena_storage);
    }

    void publish_response(Response* response) noexcept override
    {
      if (auto owner_ptr = owner.lock())
      {
        owner_ptr->publish_batch_response_(this->shared_from_this(), response);
      }
    }

    std::weak_ptr<GrpcBatchStreamCall> owner;
    google::protobuf::Arena request_arena;
    Request* request = nullptr;
    google::protobuf::Arena response_arena_storage;
    GrpcServiceBase::BatchProcessingHandle operation_handle;
    std::optional<AdServer::Commons::ActivityGate::Guard> process_guard;
    std::optional<std::uint64_t> inprogress_stats_receiver_id;
    std::size_t items_count = 0;
    std::size_t pending_writes = 0;
    bool processing_done = false;
    bool released = false;
  };

  template<typename ServiceImplType, typename AsyncServiceType>
  struct GrpcBatchStreamCall<ServiceImplType, AsyncServiceType>::StartTag final:
    GrpcAsyncCall
  {
    explicit StartTag(std::shared_ptr<GrpcBatchStreamCall> owner)
      : owner_(std::move(owner))
    {}

    void proceed(bool ok) override
    {
      owner_->handle_start_completion_(ok);
      delete this;
    }

  private:
    std::shared_ptr<GrpcBatchStreamCall> owner_;
  };

  template<typename ServiceImplType, typename AsyncServiceType>
  struct GrpcBatchStreamCall<ServiceImplType, AsyncServiceType>::ReadTag final:
    GrpcAsyncCall
  {
    ReadTag(std::shared_ptr<GrpcBatchStreamCall> owner, BatchContextPtr context)
      : owner_(std::move(owner)),
        context_(std::move(context))
    {}

    void proceed(bool ok) override
    {
      owner_->handle_read_completion_(ok, std::move(context_));
      delete this;
    }

  private:
    std::shared_ptr<GrpcBatchStreamCall> owner_;
    BatchContextPtr context_;
  };

  template<typename ServiceImplType, typename AsyncServiceType>
  struct GrpcBatchStreamCall<ServiceImplType, AsyncServiceType>::WriteTag final: GrpcAsyncCall
  {
    WriteTag(std::shared_ptr<GrpcBatchStreamCall> owner, BatchContextPtr context)
      : owner_(std::move(owner)),
        context_(std::move(context))
    {}

    void proceed(bool ok) override
    {
      owner_->handle_write_completion_(ok, std::move(context_));
      delete this;
    }

  private:
    std::shared_ptr<GrpcBatchStreamCall> owner_;
    BatchContextPtr context_;
  };

  template<typename ServiceImplType, typename AsyncServiceType>
  struct GrpcBatchStreamCall<ServiceImplType, AsyncServiceType>::FinishTag final: GrpcAsyncCall
  {
    explicit FinishTag(std::shared_ptr<GrpcBatchStreamCall> owner)
      : owner_(std::move(owner))
    {}

    void proceed(bool ok) override
    {
      owner_->handle_finish_completion_(ok);
      delete this;
    }

  private:
    std::shared_ptr<GrpcBatchStreamCall> owner_;
  };

  template<typename ServiceImplType, typename AsyncServiceType>
  GrpcBatchStreamCall<ServiceImplType, AsyncServiceType>::GrpcBatchStreamCall(
    ServiceImplType* service_impl,
    AsyncServiceType* async_service,
    RequestMethod request_method,
    ::grpc::ServerCompletionQueue* completion_queue)
    : service_impl_(service_impl),
      async_service_(async_service),
      request_stream_(request_method),
      completion_queue_(completion_queue),
      responder_(&context_)
  {
    service_impl_->add_batch_stream_call_created_();
  }

  template<typename ServiceImplType, typename AsyncServiceType>
  GrpcBatchStreamCall<ServiceImplType, AsyncServiceType>::~GrpcBatchStreamCall() noexcept
  {
    service_impl_->add_batch_stream_call_deleted_();
  }

  template<typename ServiceImplType, typename AsyncServiceType>
  void
  GrpcBatchStreamCall<ServiceImplType, AsyncServiceType>::create(
    ServiceImplType* service_impl,
    AsyncServiceType* async_service,
    RequestMethod request_method,
    ::grpc::ServerCompletionQueue* completion_queue)
  {
    auto call = std::make_shared<GrpcBatchStreamCall>(
      service_impl,
      async_service,
      request_method,
      completion_queue);
    call->start_request_();
  }

  template<typename ServiceImplType, typename AsyncServiceType>
  bool
  GrpcBatchStreamCall<ServiceImplType, AsyncServiceType>::start_request_()
  {
    auto grpc_operation_guard = service_impl_->enter_grpc_operation();
    if (!grpc_operation_guard)
    {
      return false;
    }

    (async_service_->*request_stream_)(
      &context_,
      &responder_,
      completion_queue_,
      completion_queue_,
      new StartTag(this->shared_from_this()));

    return true;
  }

  template<typename ServiceImplType, typename AsyncServiceType>
  void
  GrpcBatchStreamCall<ServiceImplType, AsyncServiceType>::handle_start_completion_(bool ok)
    noexcept
  {
    if (!ok)
    {
      return;
    }

    create(service_impl_, async_service_, request_stream_, completion_queue_);
    try_start_read_();
  }

  template<typename ServiceImplType, typename AsyncServiceType>
  void
  GrpcBatchStreamCall<ServiceImplType, AsyncServiceType>::try_start_read_()
    noexcept
  {
    {
      std::lock_guard<std::mutex> lock(state_lock_);
      if (read_in_flight_ ||
        finish_in_flight_ ||
        client_closed_ ||
        closing_ ||
        waiting_for_read_grant_)
      {
        return;
      }

      waiting_for_read_grant_ = true;
    }

    if (service_impl_->batch_stream_read_limiter().reserve_read_or_enqueue(
      this->shared_from_this()))
    {
      {
        std::lock_guard<std::mutex> lock(state_lock_);
        waiting_for_read_grant_ = false;
      }
      start_read_reserved_();
    }
  }

  template<typename ServiceImplType, typename AsyncServiceType>
  bool
  GrpcBatchStreamCall<ServiceImplType, AsyncServiceType>::start_read_reserved_()
    noexcept
  {
    bool finish = false;

    {
      std::lock_guard<std::mutex> lock(state_lock_);
      if (read_in_flight_ || finish_in_flight_ || client_closed_ || closing_)
      {
        service_impl_->batch_stream_read_limiter().cancel_read_reservation();
        finish = true;
      }
      else
      {
        read_in_flight_ = true;
      }
    }

    if (finish)
    {
      maybe_finish_or_delete_();
      return false;
    }

    auto grpc_operation_guard = service_impl_->enter_grpc_operation();
    if (!grpc_operation_guard)
    {
      {
        std::lock_guard<std::mutex> lock(state_lock_);
        read_in_flight_ = false;
        closing_ = true;
      }
      service_impl_->batch_stream_read_limiter().cancel_read_reservation();
      maybe_finish_or_delete_();
      return false;
    }

    auto context = std::make_shared<BatchContext>();
    context->owner = this->shared_from_this();
    context->request_arena.Reset();
    context->request = google::protobuf::Arena::CreateMessage<Request>(&context->request_arena);
    auto* tag = new ReadTag(this->shared_from_this(), context);
    responder_.Read(context->request, tag);
    return true;
  }

  template<typename ServiceImplType, typename AsyncServiceType>
  void
  GrpcBatchStreamCall<ServiceImplType, AsyncServiceType>::handle_read_completion_(
    bool ok,
    BatchContextPtr context) noexcept
  {
    {
      std::lock_guard<std::mutex> lock(state_lock_);
      read_in_flight_ = false;
    }

    if (!ok)
    {
      service_impl_->batch_stream_read_limiter().cancel_read_reservation();

      {
        std::lock_guard<std::mutex> lock(state_lock_);
        client_closed_ = true;
      }

      maybe_finish_or_delete_();
      return;
    }

    context->items_count = static_cast<std::size_t>(context->request->items_size());

    service_impl_->batch_stream_read_limiter().complete_read_reservation(context->items_count);

    start_batch_processing_(context);

    if (service_impl_->batch_stream_read_limiter().read_ahead_enabled())
    {
      try_start_read_();
    }
  }

  template<typename ServiceImplType, typename AsyncServiceType>
  void
  GrpcBatchStreamCall<ServiceImplType, AsyncServiceType>::start_batch_processing_(
    BatchContextPtr context) noexcept
  {
    start_inprogress_stats_(*context);

    auto process_guard = service_impl_->enter_grpc_operation();
    if (!process_guard)
    {
      drop_context_(std::move(context));
      {
        std::lock_guard<std::mutex> lock(state_lock_);
        closing_ = true;
      }
      maybe_finish_or_delete_();
      return;
    }

    context->process_guard.emplace(std::move(process_guard));
    bool drop = false;

    {
      std::lock_guard<std::mutex> lock(state_lock_);
      if (closing_)
      {
        drop = true;
      }
      else
      {
        ++processing_count_;
      }
    }

    if (drop)
    {
      drop_context_(std::move(context));
      maybe_finish_or_delete_();
      return;
    }

    const auto executor_pool =
      static_cast<GrpcServiceBase*>(service_impl_)->batch_processing_executor_pool();
    if (executor_pool)
    {
      const auto owner = this->shared_from_this();
      try
      {
        executor_pool->post(
          [owner, context]() mutable
          {
            owner->process_batch_(std::move(context));
          });
      }
      catch (...)
      {
        handle_batch_processed_(
          std::move(context),
          std::optional<std::exception_ptr>(std::current_exception()));
      }

      return;
    }

    process_batch_(std::move(context));
  }

  template<typename ServiceImplType, typename AsyncServiceType>
  void
  GrpcBatchStreamCall<ServiceImplType, AsyncServiceType>::process_batch_(BatchContextPtr context)
    noexcept
  {
    try
    {
      static_cast<GrpcServiceBase*>(service_impl_)->start_handle_batch_request(
        context->operation_handle,
        *context->request,
        *context,
        [owner = this->shared_from_this(), context](std::optional<std::exception_ptr> exception)
          mutable
        {
          owner->handle_batch_processed_(std::move(context), std::move(exception));
        });
    }
    catch (...)
    {
      handle_batch_processed_(
        std::move(context),
        std::optional<std::exception_ptr>(std::current_exception()));
      return;
    }
  }

  template<typename ServiceImplType, typename AsyncServiceType>
  void
  GrpcBatchStreamCall<ServiceImplType, AsyncServiceType>::handle_batch_processed_(
    BatchContextPtr context,
    std::optional<std::exception_ptr> exception) noexcept
  {
    if (exception)
    {
      auto* response = context->create_response();
      response->set_batch_id(context->request->batch_id());
      std::string status_message;
      try
      {
        std::rethrow_exception(*exception);
      }
      catch (const std::exception& ex)
      {
        status_message = ex.what();
      }
      catch (...)
      {
        status_message = "Unknown batch handler exception";
      }

      for (const auto& request_item : context->request->items())
      {
        auto* response_item = response->add_items();
        response_item->set_request_id(request_item.request_id());
        response_item->set_status_code(::grpc::StatusCode::INTERNAL);
        response_item->set_status_message(status_message);
      }

      context->publish_response(response);
    }

    bool drop = false;

    {
      std::lock_guard<std::mutex> lock(state_lock_);
      if (processing_count_ > 0)
      {
        --processing_count_;
      }

      context->processing_done = true;
      if (context->pending_writes == 0 && !context->released)
      {
        context->released = true;
        drop = true;
      }
    }

    if (drop)
    {
      drop_context_(std::move(context));
    }

    maybe_finish_or_delete_();
  }

  template<typename ServiceImplType, typename AsyncServiceType>
  void
  GrpcBatchStreamCall<ServiceImplType, AsyncServiceType>::publish_batch_response_(
    BatchContextPtr context,
    Response* response) noexcept
  {
    if (!context || !response || response->items_size() == 0)
    {
      return;
    }

    bool drop = false;

    {
      std::lock_guard<std::mutex> lock(state_lock_);
      if (closing_ || context->released)
      {
        if (context->processing_done && context->pending_writes == 0 && !context->released)
        {
          context->released = true;
          drop = true;
        }
      }
      else
      {
        ++context->pending_writes;
        ready_responses_.push_back(ReadyResponse{context, response});
      }
    }

    if (drop)
    {
      drop_context_(std::move(context));
    }
    else
    {
      try_start_write_();
    }
  }

  template<typename ServiceImplType, typename AsyncServiceType>
  void
  GrpcBatchStreamCall<ServiceImplType, AsyncServiceType>::try_start_write_() noexcept
  {
    ReadyResponse ready_response;
    {
      std::lock_guard<std::mutex> lock(state_lock_);
      if (write_in_flight_ ||
        finish_in_flight_ ||
        closing_ ||
        ready_responses_.empty())
      {
        return;
      }

      ready_response = std::move(ready_responses_.front());
      ready_responses_.pop_front();
      write_in_flight_ = true;
    }

    if (!start_write_i_(std::move(ready_response)))
    {
      maybe_finish_or_delete_();
    }
  }

  template<typename ServiceImplType, typename AsyncServiceType>
  bool
  GrpcBatchStreamCall<ServiceImplType, AsyncServiceType>::start_write_i_(
    ReadyResponse ready_response)
    noexcept
  {
    auto grpc_operation_guard = service_impl_->enter_grpc_operation();
    if (!grpc_operation_guard)
    {
      bool drop = false;

      {
        std::lock_guard<std::mutex> lock(state_lock_);
        write_in_flight_ = false;
        closing_ = true;

        if (ready_response.context && ready_response.context->pending_writes > 0)
        {
          --ready_response.context->pending_writes;
        }

        if (ready_response.context &&
          ready_response.context->processing_done &&
          ready_response.context->pending_writes == 0 &&
          !ready_response.context->released)
        {
          ready_response.context->released = true;
          drop = true;
        }
      }

      if (drop)
      {
        drop_context_(std::move(ready_response.context));
      }

      drop_ready_responses_();
      return false;
    }

    auto* tag = new WriteTag(this->shared_from_this(), ready_response.context);
    responder_.Write(*ready_response.response, tag);
    return true;
  }

  template<typename ServiceImplType, typename AsyncServiceType>
  void
  GrpcBatchStreamCall<ServiceImplType, AsyncServiceType>::handle_write_completion_(
    bool ok,
    BatchContextPtr context) noexcept
  {
    bool drop = false;

    {
      std::lock_guard<std::mutex> lock(state_lock_);
      write_in_flight_ = false;
      if (!ok)
      {
        closing_ = true;
      }

      if (context->pending_writes > 0)
      {
        --context->pending_writes;
      }

      if (context->processing_done && context->pending_writes == 0 && !context->released)
      {
        context->released = true;
        drop = true;
      }
    }

    if (drop)
    {
      drop_context_(std::move(context));
    }

    if (ok)
    {
      try_start_write_();
      try_start_read_();
    }
    else
    {
      drop_ready_responses_();
    }

    maybe_finish_or_delete_();
  }

  template<typename ServiceImplType, typename AsyncServiceType>
  void
  GrpcBatchStreamCall<ServiceImplType, AsyncServiceType>::handle_finish_completion_(bool) noexcept
  {
    std::lock_guard<std::mutex> lock(state_lock_);
    finish_in_flight_ = false;
  }

  template<typename ServiceImplType, typename AsyncServiceType>
  void
  GrpcBatchStreamCall<ServiceImplType, AsyncServiceType>::finish_or_delete_() noexcept
  {
    auto grpc_operation_guard = service_impl_->enter_grpc_operation();
    if (!grpc_operation_guard)
    {
      {
        std::lock_guard<std::mutex> lock(state_lock_);
        finish_in_flight_ = false;
        closing_ = true;
      }
      maybe_finish_or_delete_();
      return;
    }

    responder_.Finish(::grpc::Status::OK, new FinishTag(this->shared_from_this()));
  }

  template<typename ServiceImplType, typename AsyncServiceType>
  void
  GrpcBatchStreamCall<ServiceImplType, AsyncServiceType>::start_inprogress_stats_(
    BatchContext& context)
  {
    finish_inprogress_stats_(context);

    context.inprogress_stats_receiver_id = service_impl_->inprogress_stats_->add(
      context.items_count,
      Generics::Time::get_time_of_day());
  }

  template<typename ServiceImplType, typename AsyncServiceType>
  void
  GrpcBatchStreamCall<ServiceImplType, AsyncServiceType>::finish_inprogress_stats_(
    BatchContext& context) noexcept
  {
    if (context.inprogress_stats_receiver_id)
    {
      service_impl_->inprogress_stats_->remove(*context.inprogress_stats_receiver_id);
      context.inprogress_stats_receiver_id.reset();
    }
  }

  template<typename ServiceImplType, typename AsyncServiceType>
  void
  GrpcBatchStreamCall<ServiceImplType, AsyncServiceType>::drop_ready_responses_() noexcept
  {
    std::deque<ReadyResponse> responses;
    std::vector<BatchContextPtr> contexts;

    {
      std::lock_guard<std::mutex> lock(state_lock_);
      responses.swap(ready_responses_);
      for (auto& response : responses)
      {
        auto& context = response.context;

        if (!context)
        {
          continue;
        }

        if (context->pending_writes > 0)
        {
          --context->pending_writes;
        }

        if (context->processing_done && context->pending_writes == 0 && !context->released)
        {
          context->released = true;
          contexts.emplace_back(context);
        }
      }
    }

    for (auto& context : contexts)
    {
      drop_context_(std::move(context));
    }
  }

  template<typename ServiceImplType, typename AsyncServiceType>
  void
  GrpcBatchStreamCall<ServiceImplType, AsyncServiceType>::drop_context_(BatchContextPtr context)
    noexcept
  {
    if (!context)
    {
      return;
    }

    finish_inprogress_stats_(*context);
    context->process_guard.reset();
    context->operation_handle.reset();
    service_impl_->batch_stream_read_limiter().complete_requests(context->items_count);
  }

  template<typename ServiceImplType, typename AsyncServiceType>
  void
  GrpcBatchStreamCall<ServiceImplType, AsyncServiceType>::maybe_finish_or_delete_() noexcept
  {
    bool start_finish = false;

    {
      std::lock_guard<std::mutex> lock(state_lock_);
      if (read_in_flight_ ||
        write_in_flight_ ||
        finish_in_flight_ ||
        waiting_for_read_grant_ ||
        processing_count_ != 0 ||
        !ready_responses_.empty())
      {
        return;
      }

      if (closing_)
      {
        return;
      }
      else if (client_closed_)
      {
        finish_in_flight_ = true;
        start_finish = true;
      }
    }

    if (start_finish)
    {
      finish_or_delete_();
    }
  }

  template<typename ServiceImplType, typename AsyncServiceType>
  void
  GrpcBatchStreamCall<ServiceImplType, AsyncServiceType>::start_read_from_limiter() noexcept
  {
    bool was_waiting = false;

    {
      std::lock_guard<std::mutex> lock(state_lock_);
      was_waiting = waiting_for_read_grant_;
      waiting_for_read_grant_ = false;
    }

    if (was_waiting)
    {
      start_read_reserved_();
    }
    else
    {
      service_impl_->batch_stream_read_limiter().cancel_read_reservation();
    }
  }
}
