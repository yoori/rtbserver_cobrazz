#include "GrpcServiceBase.hpp"

#include <Commons/Coro/Utils.hpp>
#include <Commons/Coro/CoroSet.hpp>
#include <Commons/ExecutorPool.hpp>

#include <algorithm>
#include <array>
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
  GrpcCoroutine::start(Completion completion) const
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
    start([continuation](std::optional<std::exception_ptr>) mutable {
      AdServer::Commons::resume_coroutine(continuation);
    });
  }

  void
  GrpcCoroutine::await_resume()
  {
    if (handle_.promise().exception)
    {
      std::rethrow_exception(*handle_.promise().exception);
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
    auto completion = std::move(promise.completion);
    if (completion)
    {
      completion(promise.exception);
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
  GrpcServiceBase::InprogressStats::remove(const std::uint64_t receiver_id) noexcept
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

  // GrpcServiceBase::BatchStreamReadLimiter impl
  GrpcServiceBase::BatchStreamReadLimiter::Options::Options(
    bool read_ahead_enabled_val,
    std::size_t max_requests_in_progress_val) noexcept
    : read_ahead_enabled(read_ahead_enabled_val),
      max_requests_in_progress(max_requests_in_progress_val)
  {}

  GrpcServiceBase::BatchStreamReadLimiter::BatchStreamReadLimiter(
    Options options) noexcept
    : read_ahead_enabled_(options.read_ahead_enabled),
      max_requests_in_progress_(options.max_requests_in_progress)
  {}

  bool
  GrpcServiceBase::BatchStreamReadLimiter::read_ahead_enabled() const noexcept
  {
    return read_ahead_enabled_;
  }

  bool
  GrpcServiceBase::BatchStreamReadLimiter::reserve_read_or_enqueue(WaiterPtr waiter)
  {
    std::lock_guard<std::mutex> lock(lock_);
    if (!read_ahead_enabled_ ||
      !max_requests_in_progress_ ||
      (waiters_.empty() && has_capacity_i_()))
    {
      ++read_reservations_;
      return true;
    }

    waiters_.emplace_back(std::move(waiter));
    return false;
  }

  void
  GrpcServiceBase::BatchStreamReadLimiter::complete_read_reservation(std::size_t requests) noexcept
  {
    std::array<unsigned char, 1024> waiters_buffer;
    Generics::MonoAllocatorArena waiters_arena(
      waiters_buffer.data(),
      waiters_buffer.size());
    Generics::MonoVector<WaiterPtr> waiters(&waiters_arena);

    {
      std::lock_guard<std::mutex> lock(lock_);
      if (read_reservations_ > 0)
      {
        --read_reservations_;
      }
      requests_in_progress_ += requests;
      take_waiters_i_(waiters);
    }

    grant_waiters_(waiters);
  }

  void
  GrpcServiceBase::BatchStreamReadLimiter::cancel_read_reservation() noexcept
  {
    std::array<unsigned char, 1024> waiters_buffer;
    Generics::MonoAllocatorArena waiters_arena(
      waiters_buffer.data(),
      waiters_buffer.size());
    Generics::MonoVector<WaiterPtr> waiters(&waiters_arena);

    {
      std::lock_guard<std::mutex> lock(lock_);
      if (read_reservations_ > 0)
      {
        --read_reservations_;
      }
      take_waiters_i_(waiters);
    }

    grant_waiters_(waiters);
  }

  void
  GrpcServiceBase::BatchStreamReadLimiter::complete_requests(std::size_t requests) noexcept
  {
    std::array<unsigned char, 1024> waiters_buffer;
    Generics::MonoAllocatorArena waiters_arena(
      waiters_buffer.data(),
      waiters_buffer.size());
    Generics::MonoVector<WaiterPtr> waiters(&waiters_arena);

    {
      std::lock_guard<std::mutex> lock(lock_);
      requests_in_progress_ =
        requests_in_progress_ > requests ? requests_in_progress_ - requests : 0;
      take_waiters_i_(waiters);
    }

    grant_waiters_(waiters);
  }

  void
  GrpcServiceBase::BatchStreamReadLimiter::clear_waiters() noexcept
  {
    decltype(waiters_) waiters;
    std::lock_guard<std::mutex> lock(lock_);
    waiters_.swap(waiters);
  }

  bool
  GrpcServiceBase::BatchStreamReadLimiter::has_capacity_i_() const noexcept
  {
    return !max_requests_in_progress_ ||
      requests_in_progress_ + read_reservations_ < max_requests_in_progress_;
  }

  void
  GrpcServiceBase::BatchStreamReadLimiter::take_waiters_i_(
    Generics::MonoVector<WaiterPtr>& result)
  {
    if (!waiters_.empty() && has_capacity_i_())
    {
      result.reserve(waiters_.size());
    }

    while (!waiters_.empty() && has_capacity_i_())
    {
      auto waiter = std::move(waiters_.front());
      waiters_.pop_front();
      ++read_reservations_;
      result.emplace_back(std::move(waiter));
    }
  }

  void
  GrpcServiceBase::BatchStreamReadLimiter::grant_waiters_(
    Generics::MonoVector<WaiterPtr>& waiters) noexcept
  {
    for (auto& waiter : waiters)
    {
      if (waiter)
      {
        waiter->start_read_from_limiter();
      }
    }
  }

  // GrpcServiceBase impl
  GrpcServiceBase::GrpcServiceBase(
    BatchStreamReadOptions batch_stream_read_options) noexcept
    : batch_stream_read_limiter_(batch_stream_read_options)
  {}

  GrpcServiceBase::~GrpcServiceBase() noexcept
  {
    batch_stream_read_limiter_.clear_waiters();
  }

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
    batch_stream_read_limiter_.clear_waiters();
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
    google::protobuf::Arena fallback_response_arena;
    auto* const batch_response_arena = batch_response.GetArena();
    auto& response_arena = batch_response_arena ?
      *batch_response_arena :
      fallback_response_arena;

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
        it->second(request_item, *response_item, response_arena);
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
    if (batch_request.items_size() <= 1)
    {
      co_await co_handle_batch_request_sequential_(
        batch_request,
        batch_response);
      co_return;
    }

    if (max_split <= 1)
    {
      if (batch_processing_executor_pool())
      {
        co_await co_handle_batch_request_distributed_(
          batch_request,
          batch_response,
          1);
      }
      else
      {
        co_await co_handle_batch_request_sequential_(
          batch_request,
          batch_response);
      }

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

  std::shared_ptr<AdServer::Commons::ExecutorPool>
  GrpcServiceBase::batch_processing_executor_pool() const noexcept
  {
    return {};
  }

  GrpcCoroutine
  GrpcServiceBase::co_handle_batch_request_sequential_(
    const adserver::grpc::BatchRequest& batch_request,
    adserver::grpc::BatchResponse& batch_response) const
  {
    batch_response.mutable_items()->Reserve(batch_request.items_size());
    google::protobuf::Arena fallback_response_arena;
    auto* const batch_response_arena = batch_response.GetArena();
    auto& response_arena = batch_response_arena ?
      *batch_response_arena :
      fallback_response_arena;

    for (int i = 0; i < batch_request.items_size(); ++i)
    {
      const auto& request_item = batch_request.items(i);
      auto* response_item = batch_response.add_items();
      co_await co_handle_batch_item_(
        request_item,
        *response_item,
        response_arena);
    }

    co_return;
  }

  GrpcCoroutine
  GrpcServiceBase::co_handle_batch_request_distributed_(
    const adserver::grpc::BatchRequest& batch_request,
    adserver::grpc::BatchResponse& batch_response,
    const std::size_t max_split) const
  {
    const std::size_t batch_size = static_cast<std::size_t>(batch_request.items_size());
    const std::size_t split = std::min(max_split, batch_size);
    Generics::MonoAllocatorArena batch_arena;

    google::protobuf::Arena fallback_response_arena;
    auto* const batch_response_arena = batch_response.GetArena();
    auto& response_arena = batch_response_arena ?
      *batch_response_arena :
      fallback_response_arena;
    Generics::MonoVector<adserver::grpc::BatchResponseItem*> item_responses(
      &batch_arena);
    item_responses.reserve(batch_size);
    Generics::MonoVector<
      std::unique_ptr<adserver::grpc::BatchResponseItem>>
      heap_item_responses(&batch_arena);
    if (!batch_response_arena)
    {
      heap_item_responses.reserve(batch_size);
    }

    for (std::size_t i = 0; i < batch_size; ++i)
    {
      if (batch_response_arena)
      {
        item_responses.emplace_back(
          google::protobuf::Arena::CreateMessage<
            adserver::grpc::BatchResponseItem>(batch_response_arena));
      }
      else
      {
        auto item_response = std::make_unique<adserver::grpc::BatchResponseItem>();
        item_responses.emplace_back(item_response.get());
        heap_item_responses.emplace_back(std::move(item_response));
      }
    }

    google::protobuf::Arena request_arena;
    Generics::MonoVector<Generics::MonoVector<BatchItemContext>> lanes(
      &batch_arena);
    lanes.reserve(split);
    const std::size_t lane_reserve = (batch_size + split - 1) / split;
    for (std::size_t i = 0; i < split; ++i)
    {
      lanes.emplace_back(
        Generics::MonoAllocator<BatchItemContext>(&batch_arena));
      lanes.back().reserve(lane_reserve);
    }

    for (int i = 0; i < batch_request.items_size(); ++i)
    {
      const auto& request_item = batch_request.items(i);
      auto* response_item = item_responses[static_cast<std::size_t>(i)];
      response_item->set_request_id(request_item.request_id());

      BatchItemContext batch_item{
        &request_item,
        response_item,
        {}
      };

      std::size_t hash = static_cast<std::size_t>(request_item.request_id());
      bool enqueue = true;
      const auto coro_it = batch_coro_methods_.find(request_item.full_method());
      if (coro_it != batch_coro_methods_.end())
      {
        enqueue = prepare_batch_coro_item_(
          coro_it->second,
          request_item,
          *response_item,
          request_arena,
          batch_item.coro_item);
        if (enqueue && coro_it->second.distributed)
        {
          hash = batch_item.coro_item.hash;
        }
      }

      if (enqueue)
      {
        lanes[hash % split].emplace_back(std::move(batch_item));
      }
    }

    const auto executor_pool = batch_processing_executor_pool();
    Generics::MonoVector<GrpcCoroutine> operations(&batch_arena);
    operations.reserve(split);
    for (std::size_t i = 1; i < lanes.size(); ++i)
    {
      auto& lane = lanes[i];
      if (lane.empty())
      {
        continue;
      }

      operations.emplace_back(co_handle_batch_lane_(
        std::move(lane),
        executor_pool,
        true,
        response_arena));
    }

    if (!lanes.empty() && !lanes[0].empty())
    {
      operations.emplace_back(co_handle_batch_lane_(
        std::move(lanes[0]),
        executor_pool,
        false,
        response_arena));
    }

    co_await AdServer::Commons::CoroSet<
      GrpcCoroutine,
      Generics::MonoAllocator<GrpcCoroutine>>(std::move(operations));

    batch_response.mutable_items()->Reserve(batch_request.items_size());
    if (batch_response_arena)
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
    Generics::MonoVector<BatchItemContext> batch_items,
    std::shared_ptr<AdServer::Commons::ExecutorPool> executor_pool,
    bool reschedule,
    google::protobuf::Arena& response_arena) const
  {
    if (executor_pool)
    {
      if (reschedule)
      {
        co_await AdServer::Commons::ExecutorPool::reschedule(std::move(executor_pool));
      }
      else
      {
        co_await AdServer::Commons::ExecutorPool::yield(std::move(executor_pool));
      }
    }

    for (auto& batch_item : batch_items)
    {
      if (batch_item.coro_item.method)
      {
        co_await co_handle_prepared_batch_coro_item_(
          batch_item.coro_item,
          *batch_item.response_item,
          response_arena);
      }
      else
      {
        co_await co_handle_batch_item_(
          *batch_item.request_item,
          *batch_item.response_item,
          response_arena);
      }
    }

    co_return;
  }

  GrpcCoroutine
  GrpcServiceBase::co_handle_batch_item_(
    const adserver::grpc::BatchRequestItem& request_item,
    adserver::grpc::BatchResponseItem& response_item,
    google::protobuf::Arena& response_arena) const
  {
    response_item.set_request_id(request_item.request_id());

    const auto coro_it = batch_coro_methods_.find(request_item.full_method());
    if (coro_it != batch_coro_methods_.end())
    {
      google::protobuf::Arena request_arena;
      PreparedBatchCoroItem coro_item;
      if (prepare_batch_coro_item_(
        coro_it->second,
        request_item,
        response_item,
        request_arena,
        coro_item))
      {
        co_await co_handle_prepared_batch_coro_item_(
          coro_item,
          response_item,
          response_arena);
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
      sync_it->second(request_item, response_item, response_arena);
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

  bool
  GrpcServiceBase::prepare_batch_coro_item_(
    const BatchCoroMethod& method,
    const adserver::grpc::BatchRequestItem& request_item,
    adserver::grpc::BatchResponseItem& response_item,
    google::protobuf::Arena& request_arena,
    PreparedBatchCoroItem& coro_item) const
  {
    try
    {
      if (!method.prepare(
        request_item,
        response_item,
        request_arena,
        coro_item))
      {
        return false;
      }

      coro_item.method = &method;
      return true;
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

    return false;
  }

  GrpcCoroutine
  GrpcServiceBase::co_handle_prepared_batch_coro_item_(
    PreparedBatchCoroItem& coro_item,
    adserver::grpc::BatchResponseItem& response_item,
    google::protobuf::Arena& response_arena) const
  {
    try
    {
      co_await coro_item.method->dispatch(
        coro_item.request,
        response_item,
        response_arena);
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

  GrpcServiceBase::BatchStreamReadLimiter&
  GrpcServiceBase::batch_stream_read_limiter() noexcept
  {
    return batch_stream_read_limiter_;
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
