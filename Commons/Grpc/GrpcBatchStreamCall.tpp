#pragma once

#include <Commons/ExecutorPool.hpp>

namespace AdServer::Grpc
{
  template<typename ServiceImplType, typename AsyncServiceType>
  struct GrpcBatchStreamCall<ServiceImplType, AsyncServiceType>::BatchContext
  {
    google::protobuf::Arena request_arena;
    Request* request = nullptr;
    google::protobuf::Arena response_arena;
    Response* response = nullptr;
    std::optional<GrpcCoroutine> operation;
    std::optional<AdServer::Commons::ActivityGate::Guard> process_guard;
    std::optional<std::uint64_t> inprogress_stats_receiver_id;
    std::size_t items_count = 0;
#ifdef ADS_GRPC_BATCH_STREAM_DEBUG_TIMEOUT
    std::shared_ptr<DebugWatchdogState> debug_response_watchdog_state;
#endif
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
    context->request_arena.Reset();
    context->request = google::protobuf::Arena::CreateMessage<Request>(
      &context->request_arena);
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

    if (service_impl_->batch_stream_read_limiter().enabled())
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
#ifdef ADS_GRPC_BATCH_STREAM_DEBUG_TIMEOUT
    start_debug_response_watchdog_(*context);
#endif

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
        handle_batch_processed_(std::move(context), std::current_exception());
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
      context->response_arena.Reset();
      context->response = google::protobuf::Arena::CreateMessage<Response>(
        &context->response_arena);
      context->operation.emplace(
        service_impl_->co_handle_batch_request(
          *context->request,
          *context->response));
    }
    catch (...)
    {
      handle_batch_processed_(std::move(context), std::current_exception());
      return;
    }

    auto owner = this->shared_from_this();
    context->operation->start(
      [owner = std::move(owner), context = std::move(context)](
        std::exception_ptr exception) mutable
      {
        owner->handle_batch_processed_(std::move(context), exception);
      });
  }

  template<typename ServiceImplType, typename AsyncServiceType>
  void
  GrpcBatchStreamCall<ServiceImplType, AsyncServiceType>::handle_batch_processed_(
    BatchContextPtr context,
    std::exception_ptr exception) noexcept
  {
    if (exception)
    {
      context->response_arena.Reset();
      context->response = google::protobuf::Arena::CreateMessage<Response>(
        &context->response_arena);
      std::string status_message;
      try
      {
        std::rethrow_exception(exception);
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
        auto* response_item = context->response->add_items();
        response_item->set_request_id(request_item.request_id());
        response_item->set_status_code(::grpc::StatusCode::INTERNAL);
        response_item->set_status_message(status_message);
      }
    }

    bool drop = false;

    {
      std::lock_guard<std::mutex> lock(state_lock_);
      if (processing_count_ > 0)
      {
        --processing_count_;
      }

      if (closing_)
      {
        drop = true;
      }
      else
      {
        ready_responses_.emplace_back(context);
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

    maybe_finish_or_delete_();
  }

  template<typename ServiceImplType, typename AsyncServiceType>
  void
  GrpcBatchStreamCall<ServiceImplType, AsyncServiceType>::try_start_write_() noexcept
  {
    BatchContextPtr context;
    {
      std::lock_guard<std::mutex> lock(state_lock_);
      if (write_in_flight_ ||
        finish_in_flight_ ||
        closing_ ||
        ready_responses_.empty())
      {
        return;
      }

      context = std::move(ready_responses_.front());
      ready_responses_.pop_front();
      write_in_flight_ = true;
    }

    if (!start_write_i_(std::move(context)))
    {
      maybe_finish_or_delete_();
    }
  }

  template<typename ServiceImplType, typename AsyncServiceType>
  bool
  GrpcBatchStreamCall<ServiceImplType, AsyncServiceType>::start_write_i_(BatchContextPtr context)
    noexcept
  {
    auto grpc_operation_guard = service_impl_->enter_grpc_operation();
    if (!grpc_operation_guard)
    {
      {
        std::lock_guard<std::mutex> lock(state_lock_);
        write_in_flight_ = false;
        closing_ = true;
      }
      drop_context_(std::move(context));
      drop_ready_responses_();
      return false;
    }

    auto* tag = new WriteTag(this->shared_from_this(), context);
    responder_.Write(*context->response, tag);
    return true;
  }

  template<typename ServiceImplType, typename AsyncServiceType>
  void
  GrpcBatchStreamCall<ServiceImplType, AsyncServiceType>::handle_write_completion_(
    bool ok,
    BatchContextPtr context) noexcept
  {
    {
      std::lock_guard<std::mutex> lock(state_lock_);
      write_in_flight_ = false;
      if (!ok)
      {
        closing_ = true;
      }
    }

    drop_context_(std::move(context));

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
    {
      std::lock_guard<std::mutex> lock(state_lock_);
      finish_in_flight_ = false;
    }
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
    std::deque<BatchContextPtr> contexts;

    {
      std::lock_guard<std::mutex> lock(state_lock_);
      contexts.swap(ready_responses_);
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

#ifdef ADS_GRPC_BATCH_STREAM_DEBUG_TIMEOUT
    finish_debug_response_watchdog_(*context);
#endif
    finish_inprogress_stats_(*context);
    context->process_guard.reset();
    context->operation.reset();
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

#ifdef ADS_GRPC_BATCH_STREAM_DEBUG_TIMEOUT
  template<typename ServiceImplType, typename AsyncServiceType>
  void
  GrpcBatchStreamCall<ServiceImplType, AsyncServiceType>::start_debug_response_watchdog_(
    BatchContext& context)
  {
    finish_debug_response_watchdog_(context);

    auto state = std::make_shared<DebugWatchdogState>();
    state->peer = context_.peer();
    state->items_size = context.request->items_size();
    if (state->items_size > 0)
    {
      state->first_method = context.request->items(0).full_method();
    }

    context.debug_response_watchdog_state = state;

    service_impl_->add_debug_watchdog_scheduled_();
    GrpcBatchStreamDebugTimerService::instance().schedule(std::move(state));
  }

  template<typename ServiceImplType, typename AsyncServiceType>
  void
  GrpcBatchStreamCall<ServiceImplType, AsyncServiceType>::finish_debug_response_watchdog_(
    BatchContext& context) noexcept
  {
    if (context.debug_response_watchdog_state)
    {
      context.debug_response_watchdog_state->done.store(true, std::memory_order_release);
      context.debug_response_watchdog_state.reset();
      service_impl_->add_debug_watchdog_finished_();
    }
  }
#endif
}
