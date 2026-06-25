#include "GrpcServiceBase.hpp"

#include <Commons/Coro.hpp>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <memory>

namespace AdServer::Grpc
{

  GrpcCoroutine::GrpcCoroutine(Handle handle) noexcept
    : handle_(handle)
  {}

  GrpcCoroutine::GrpcCoroutine(GrpcCoroutine&& other) noexcept
    : handle_(std::exchange(other.handle_, {}))
  {}

  GrpcCoroutine&
  GrpcCoroutine::operator=(GrpcCoroutine&& other) noexcept
  {
    if (this != &other)
    {
      if (handle_)
      {
        handle_.destroy();
      }
      handle_ = std::exchange(other.handle_, {});
    }

    return *this;
  }

  GrpcCoroutine::~GrpcCoroutine()
  {
    if (handle_)
    {
      handle_.destroy();
    }
  }

  void
  GrpcCoroutine::start(Completion completion)
  {
    handle_.promise().completion = std::move(completion);
    AdServer::Commons::resume_coroutine(handle_);
  }

  bool
  GrpcCoroutine::await_ready() const noexcept
  {
    return false;
  }

  void
  GrpcCoroutine::await_suspend(std::coroutine_handle<> continuation)
  {
    start([continuation](std::exception_ptr) mutable {
      AdServer::Commons::resume_coroutine(continuation);
    });
  }

  void
  GrpcCoroutine::await_resume()
  {
    if (handle_.promise().exception)
    {
      std::rethrow_exception(handle_.promise().exception);
    }
  }

  GrpcCoroutine
  GrpcCoroutine::promise_type::get_return_object() noexcept
  {
    return GrpcCoroutine(Handle::from_promise(*this));
  }

  std::suspend_always
  GrpcCoroutine::promise_type::initial_suspend() const noexcept
  {
    return {};
  }

  bool
  GrpcCoroutine::promise_type::FinalAwaiter::await_ready() const noexcept
  {
    return false;
  }

  void
  GrpcCoroutine::promise_type::FinalAwaiter::await_suspend(
    Handle handle) const noexcept
  {
    auto& promise = handle.promise();
    if (promise.completion)
    {
      promise.completion(promise.exception);
    }
  }

  void
  GrpcCoroutine::promise_type::FinalAwaiter::await_resume() const noexcept
  {}

  GrpcCoroutine::promise_type::FinalAwaiter
  GrpcCoroutine::promise_type::final_suspend() const noexcept
  {
    return {};
  }

  void
  GrpcCoroutine::promise_type::return_void() const noexcept
  {}

  void
  GrpcCoroutine::promise_type::unhandled_exception() noexcept
  {
    exception = std::current_exception();
  }

  GrpcCoroutineAll::GrpcCoroutineAll(std::vector<GrpcCoroutine> operations)
    : state_(std::make_shared<State>())
  {
    state_->operations = std::move(operations);
  }

  bool
  GrpcCoroutineAll::await_ready() const noexcept
  {
    return state_->operations.empty();
  }

  bool
  GrpcCoroutineAll::await_suspend(std::coroutine_handle<> continuation)
  {
    state_->continuation = continuation;
    state_->remaining = state_->operations.size();

    for (auto& operation : state_->operations)
    {
      operation.start([state = state_](std::exception_ptr exception) mutable {
        bool resume = false;
        {
          std::lock_guard<std::mutex> guard(state->lock);
          if (exception && !state->exception)
          {
            state->exception = exception;
          }

          if (--state->remaining == 0)
          {
            resume = state->suspended;
          }
        }

        if (resume)
        {
          AdServer::Commons::resume_coroutine(state->continuation);
        }
      });
    }

    std::lock_guard<std::mutex> guard(state_->lock);
    if (state_->remaining == 0)
    {
      return false;
    }

    state_->suspended = true;
    return true;
  }

  void
  GrpcCoroutineAll::await_resume()
  {
    if (state_->exception)
    {
      std::rethrow_exception(state_->exception);
    }
  }

  GrpcCoroutineAll
  when_all(std::vector<GrpcCoroutine> operations)
  {
    return GrpcCoroutineAll(std::move(operations));
  }

#ifdef ADS_GRPC_BATCH_STREAM_DEBUG_TIMEOUT
  GrpcBatchStreamDebugTimerService&
  GrpcBatchStreamDebugTimerService::instance()
  {
    static auto* service = new GrpcBatchStreamDebugTimerService();
    return *service;
  }

  void
  GrpcBatchStreamDebugTimerService::schedule(
    std::shared_ptr<WatchdogState> state)
  {
    auto timer = std::make_shared<boost::asio::steady_timer>(io_service_);
    timer->expires_after(std::chrono::seconds(20));
    timer->async_wait(
      [
        timer = std::move(timer),
        state = std::move(state)
      ](const boost::system::error_code& error)
      {
        if (!error && state &&
          !state->done.load(std::memory_order_acquire))
        {
          std::cout
            << "ADS_GRPC_BATCH_STREAM_DEBUG_TIMEOUT: "
            << "response timeout after 20 sec"
            << ", peer=" << state->peer
            << ", batch_items=" << state->items_size;
          if (!state->first_method.empty())
          {
            std::cout << ", first_method=" << state->first_method;
          }
          std::cout << std::endl;
        }
      });
  }

  GrpcBatchStreamDebugTimerService::GrpcBatchStreamDebugTimerService()
    : work_(io_service_),
      thread_([this] { io_service_.run(); })
  {}
#endif

  std::uint64_t
  GrpcServiceBase::InprogressStats::add(
    const std::uint64_t call_inflight,
    const Generics::Time& read_time)
  {
    const auto receiver_id = next_receiver_id_.fetch_add(
      1,
      std::memory_order_relaxed);

    std::lock_guard<std::mutex> lock(lock_);
    requests_.emplace(receiver_id, Request{read_time, call_inflight});
    call_inflight_ += call_inflight;
    if (!min_time_of_request_in_progress_ ||
      read_time < *min_time_of_request_in_progress_)
    {
      min_time_of_request_in_progress_ = read_time;
    }

    return receiver_id;
  }

  void
  GrpcServiceBase::InprogressStats::remove(
    const std::uint64_t receiver_id) noexcept
  {
    std::lock_guard<std::mutex> lock(lock_);
    const auto it = requests_.find(receiver_id);
    if (it == requests_.end())
    {
      return;
    }

    call_inflight_ -= it->second.call_inflight;
    requests_.erase(it);
    recalculate_min_time_();
  }

  GrpcServiceBase::InprogressStatsSnapshot
  GrpcServiceBase::InprogressStats::snapshot() const
  {
    std::lock_guard<std::mutex> lock(lock_);
    return InprogressStatsSnapshot{
      call_inflight_,
      min_time_of_request_in_progress_};
  }

  void
  GrpcServiceBase::InprogressStats::recalculate_min_time_() noexcept
  {
    min_time_of_request_in_progress_.reset();
    for (const auto& [_, request] : requests_)
    {
      if (!min_time_of_request_in_progress_ ||
        request.read_time < *min_time_of_request_in_progress_)
      {
        min_time_of_request_in_progress_ = request.read_time;
      }
    }
  }

  GrpcServiceBase::~GrpcServiceBase() noexcept = default;

  void
  GrpcServiceBase::register_services(::grpc::ServerBuilder& builder)
  {
    for (auto* service : grpc_services_)
    {
      builder.RegisterService(service);
    }
  }

  void
  GrpcServiceBase::start(const CompletionQueues& completion_queues)
  {
    grpc_operation_gate_.activate_object();

    const auto registrations = registrations_per_queue();
    for (auto* completion_queue : completion_queues)
    {
      for (std::size_t i = 0; i < registrations; ++i)
      {
        register_in_queue(completion_queue);
      }
    }
  }

  void
  GrpcServiceBase::stop_accepting_requests() noexcept
  {
    grpc_operation_gate_.deactivate_object();
    grpc_operation_gate_.wait_object();
  }

  void
  GrpcServiceBase::stop_finishing_requests() noexcept
  {}

  GrpcServiceBase::InprogressStatsSnapshot
  GrpcServiceBase::inprogress_stats() const
  {
    return inprogress_stats_->snapshot();
  }

  GrpcServiceBase::LifecycleStatsSnapshot
  GrpcServiceBase::lifecycle_stats() const noexcept
  {
    return LifecycleStatsSnapshot{
      unary_call_created_total_.load(std::memory_order_relaxed),
      unary_call_deleted_total_.load(std::memory_order_relaxed),
      unary_call_live_.load(std::memory_order_relaxed),
      coro_unary_call_created_total_.load(std::memory_order_relaxed),
      coro_unary_call_deleted_total_.load(std::memory_order_relaxed),
      coro_unary_call_live_.load(std::memory_order_relaxed),
      batch_stream_call_created_total_.load(std::memory_order_relaxed),
      batch_stream_call_deleted_total_.load(std::memory_order_relaxed),
      batch_stream_call_live_.load(std::memory_order_relaxed),
      debug_watchdog_scheduled_total_.load(std::memory_order_relaxed),
      debug_watchdog_finished_total_.load(std::memory_order_relaxed),
      debug_watchdog_live_.load(std::memory_order_relaxed)};
  }

  std::size_t
  GrpcServiceBase::registrations_per_queue() const noexcept
  {
    return 64;
  }

  void
  GrpcServiceBase::add_grpc_service(::grpc::Service* service)
  {
    grpc_services_.push_back(service);
  }

  void
  GrpcServiceBase::handle_batch_request(
    const adserver::grpc::BatchRequest& batch_request,
    adserver::grpc::BatchResponse& batch_response) const
  {
    for (int i = 0; i < batch_request.items_size(); ++i)
    {
      const auto& request_item = batch_request.items(i);
      auto* response_item = batch_response.add_items();
      response_item->set_request_id(request_item.request_id());

      const auto it = batch_methods_.find(request_item.full_method());
      if (it == batch_methods_.end())
      {
        response_item->set_status_code(::grpc::StatusCode::UNIMPLEMENTED);
        response_item->set_status_message("Unknown method");
        continue;
      }

      try
      {
        it->second(request_item, *response_item);
      }
      catch (const std::exception& ex)
      {
        response_item->set_status_code(::grpc::StatusCode::INTERNAL);
        response_item->set_status_message(ex.what());
      }
      catch (...)
      {
        response_item->set_status_code(::grpc::StatusCode::INTERNAL);
        response_item->set_status_message("Unknown batch handler exception");
      }
    }
  }


  GrpcCoroutine
  GrpcServiceBase::co_handle_batch_request(
    const adserver::grpc::BatchRequest& batch_request,
    adserver::grpc::BatchResponse& batch_response) const
  {
    const std::size_t max_split = distributed_batch_max_split();
    if (max_split <= 1 || batch_request.items_size() <= 1)
    {
      co_await co_handle_batch_request_sequential_(
        batch_request,
        batch_response);
      co_return;
    }

    co_await co_handle_batch_request_distributed_(
      batch_request,
      batch_response,
      max_split);
    co_return;
  }

  std::size_t
  GrpcServiceBase::distributed_batch_max_split() const noexcept
  {
    return 1;
  }

  GrpcCoroutine
  GrpcServiceBase::co_handle_batch_request_sequential_(
    const adserver::grpc::BatchRequest& batch_request,
    adserver::grpc::BatchResponse& batch_response) const
  {
    for (int i = 0; i < batch_request.items_size(); ++i)
    {
      const auto& request_item = batch_request.items(i);
      auto* response_item = batch_response.add_items();
      co_await co_handle_batch_item_(request_item, *response_item);
    }

    co_return;
  }

  GrpcCoroutine
  GrpcServiceBase::co_handle_batch_request_distributed_(
    const adserver::grpc::BatchRequest& batch_request,
    adserver::grpc::BatchResponse& batch_response,
    const std::size_t max_split) const
  {
    const std::size_t batch_size =
      static_cast<std::size_t>(batch_request.items_size());
    const std::size_t split = std::min(max_split, batch_size);

    std::vector<std::vector<int>> lanes(split);
    for (int i = 0; i < batch_request.items_size(); ++i)
    {
      const auto& item = batch_request.items(i);
      lanes[batch_item_hash_(item) % split].emplace_back(i);
    }

    auto* const response_arena = batch_response.GetArena();
    std::vector<adserver::grpc::BatchResponseItem*> item_responses;
    item_responses.reserve(batch_size);
    std::vector<std::unique_ptr<adserver::grpc::BatchResponseItem>> heap_item_responses;
    if (!response_arena)
    {
      heap_item_responses.reserve(batch_size);
    }

    for (std::size_t i = 0; i < batch_size; ++i)
    {
      if (response_arena)
      {
        item_responses.emplace_back(
          google::protobuf::Arena::CreateMessage<
            adserver::grpc::BatchResponseItem>(response_arena));
      }
      else
      {
        auto item_response =
          std::make_unique<adserver::grpc::BatchResponseItem>();
        item_responses.emplace_back(item_response.get());
        heap_item_responses.emplace_back(std::move(item_response));
      }
    }

    std::vector<GrpcCoroutine> operations;
    operations.reserve(split);
    for (auto& lane : lanes)
    {
      if (lane.empty())
      {
        continue;
      }

      operations.emplace_back(co_handle_batch_lane_(
        batch_request,
        item_responses,
        std::move(lane)));
    }

    co_await when_all(std::move(operations));

    if (response_arena)
    {
      for (auto* item_response : item_responses)
      {
        batch_response.mutable_items()->UnsafeArenaAddAllocated(item_response);
      }
    }
    else
    {
      for (auto& item_response : heap_item_responses)
      {
        batch_response.mutable_items()->AddAllocated(item_response.release());
      }
    }

    co_return;
  }

  GrpcCoroutine
  GrpcServiceBase::co_handle_batch_lane_(
    const adserver::grpc::BatchRequest& batch_request,
    std::vector<adserver::grpc::BatchResponseItem*>& item_responses,
    std::vector<int> indexes) const
  {
    for (const int index : indexes)
    {
      co_await co_handle_batch_item_(
        batch_request.items(index),
        *item_responses[static_cast<std::size_t>(index)]);
    }

    co_return;
  }

  GrpcCoroutine
  GrpcServiceBase::co_handle_batch_item_(
    const adserver::grpc::BatchRequestItem& request_item,
    adserver::grpc::BatchResponseItem& response_item) const
  {
    response_item.set_request_id(request_item.request_id());

    const auto coro_it = batch_coro_methods_.find(request_item.full_method());
    if (coro_it != batch_coro_methods_.end())
    {
      try
      {
        co_await coro_it->second.dispatch(request_item, response_item);
      }
      catch (const std::exception& ex)
      {
        response_item.set_status_code(::grpc::StatusCode::INTERNAL);
        response_item.set_status_message(ex.what());
      }
      catch (...)
      {
        response_item.set_status_code(::grpc::StatusCode::INTERNAL);
        response_item.set_status_message("Unknown batch handler exception");
      }

      co_return;
    }

    const auto sync_it = batch_methods_.find(request_item.full_method());
    if (sync_it == batch_methods_.end())
    {
      response_item.set_status_code(::grpc::StatusCode::UNIMPLEMENTED);
      response_item.set_status_message("Unknown method");
      co_return;
    }

    try
    {
      sync_it->second(request_item, response_item);
    }
    catch (const std::exception& ex)
    {
      response_item.set_status_code(::grpc::StatusCode::INTERNAL);
      response_item.set_status_message(ex.what());
    }
    catch (...)
    {
      response_item.set_status_code(::grpc::StatusCode::INTERNAL);
      response_item.set_status_message("Unknown batch handler exception");
    }

    co_return;
  }

  std::size_t
  GrpcServiceBase::batch_item_hash_(
    const adserver::grpc::BatchRequestItem& request_item) const
  {
    const auto coro_it = batch_coro_methods_.find(request_item.full_method());
    if (coro_it != batch_coro_methods_.end() &&
      coro_it->second.distributed &&
      coro_it->second.hash)
    {
      return coro_it->second.hash(request_item);
    }

    return static_cast<std::size_t>(request_item.request_id());
  }

  AdServer::Commons::ActivityGate::Guard
  GrpcServiceBase::enter_grpc_operation() noexcept
  {
    return grpc_operation_gate_.enter();
  }

  void
  GrpcServiceBase::add_unary_call_created_() noexcept
  {
    unary_call_created_total_.fetch_add(1, std::memory_order_relaxed);
    unary_call_live_.fetch_add(1, std::memory_order_relaxed);
  }

  void
  GrpcServiceBase::add_unary_call_deleted_() noexcept
  {
    unary_call_deleted_total_.fetch_add(1, std::memory_order_relaxed);
    unary_call_live_.fetch_sub(1, std::memory_order_relaxed);
  }

  void
  GrpcServiceBase::add_coro_unary_call_created_() noexcept
  {
    coro_unary_call_created_total_.fetch_add(1, std::memory_order_relaxed);
    coro_unary_call_live_.fetch_add(1, std::memory_order_relaxed);
  }

  void
  GrpcServiceBase::add_coro_unary_call_deleted_() noexcept
  {
    coro_unary_call_deleted_total_.fetch_add(1, std::memory_order_relaxed);
    coro_unary_call_live_.fetch_sub(1, std::memory_order_relaxed);
  }

  void
  GrpcServiceBase::add_batch_stream_call_created_() noexcept
  {
    batch_stream_call_created_total_.fetch_add(1, std::memory_order_relaxed);
    batch_stream_call_live_.fetch_add(1, std::memory_order_relaxed);
  }

  void
  GrpcServiceBase::add_batch_stream_call_deleted_() noexcept
  {
    batch_stream_call_deleted_total_.fetch_add(1, std::memory_order_relaxed);
    batch_stream_call_live_.fetch_sub(1, std::memory_order_relaxed);
  }

  void
  GrpcServiceBase::add_debug_watchdog_scheduled_() noexcept
  {
    debug_watchdog_scheduled_total_.fetch_add(1, std::memory_order_relaxed);
    debug_watchdog_live_.fetch_add(1, std::memory_order_relaxed);
  }

  void
  GrpcServiceBase::add_debug_watchdog_finished_() noexcept
  {
    debug_watchdog_finished_total_.fetch_add(1, std::memory_order_relaxed);
    debug_watchdog_live_.fetch_sub(1, std::memory_order_relaxed);
  }
}
