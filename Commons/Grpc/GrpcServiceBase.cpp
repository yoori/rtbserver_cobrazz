#include "GrpcServiceBase.hpp"

#include <Commons/Coro/Utils.hpp>
#include <Commons/ExecutorPool.hpp>
#include <Commons/FastScheduler.hpp>

#include <algorithm>
#include <array>
#include <memory>

namespace AdServer::Grpc
{
  namespace
  {
    std::size_t
    resolve_batch_lane_count(const std::size_t batch_size, const std::size_t max_sequential_ops)
      noexcept
    {
      const std::size_t sequential_ops = std::max<std::size_t>(1, max_sequential_ops);
      return std::max<std::size_t>(1, (batch_size + sequential_ops - 1) / sequential_ops);
    }
  }

  struct GrpcServiceBase::PublishedBatchState final
  {
    using Scheduler = AdServer::Commons::FastScheduler;

    struct LaneProgress
    {
      std::atomic<std::size_t> completed{0};
      std::atomic<std::size_t> published{0};
    };

    PublishedBatchState(
      Scheduler& scheduler_val,
      const adserver::grpc::BatchRequest& request_val,
      BatchResponsePublisher& response_publisher_val,
      BatchCompletion completion_val)
      : scheduler(scheduler_val),
        batch_id(request_val.batch_id()),
        response_publisher(response_publisher_val.retain()),
        response_arena(response_publisher_val.response_arena()),
        item_responses(&batch_arena),
        lanes(&batch_arena),
        response_time_steps_us(&batch_arena),
        completion(std::move(completion_val))
    {
      response_time_steps_us.reserve(request_val.response_time_steps_us().size());
      for (const auto step : request_val.response_time_steps_us())
      {
        if (step > 0 && (response_time_steps_us.empty() || step > response_time_steps_us.back()))
        {
          response_time_steps_us.emplace_back(step);
        }
      }
    }

    ~PublishedBatchState()
    {
      if (lane_progress)
      {
        std::destroy_n(lane_progress, lanes.size());
      }
    }

    void initialize_lane_progress()
    {
      if (lanes.empty())
      {
        return;
      }

      lane_progress = Generics::MonoAllocator<LaneProgress>(batch_arena).allocate(lanes.size());
      for (std::size_t i = 0; i < lanes.size(); ++i)
      {
        std::construct_at(lane_progress + i);
      }
    }

    void schedule_first_response(std::shared_ptr<PublishedBatchState> self)
    {
      if (response_time_steps_us.empty())
      {
        return;
      }

      scheduler.schedule(
        timer_task,
        started_at + Generics::Time(
          response_time_steps_us.front() / Generics::Time::USEC_MAX,
          response_time_steps_us.front() % Generics::Time::USEC_MAX),
        std::move(self),
        &PublishedBatchState::on_timer);
    }

    void publish_ready() noexcept
    {
      const auto publisher = response_publisher.lock();
      if (!publisher)
      {
        return;
      }

      adserver::grpc::BatchResponse* response = nullptr;

      for (std::size_t lane_index = 0; lane_index < lanes.size(); ++lane_index)
      {
        auto& progress = lane_progress[lane_index];
        const auto end = progress.completed.load(std::memory_order_acquire);
        auto begin = progress.published.load(std::memory_order_relaxed);
        while (begin < end &&
          !progress.published.compare_exchange_weak(
            begin,
            end,
            std::memory_order_acq_rel,
            std::memory_order_relaxed))
        {}

        if (begin >= end)
        {
          continue;
        }

        if (!response)
        {
          response = publisher->create_response();
          response->set_batch_id(batch_id);
        }

        auto& lane = lanes[lane_index];
        for (std::size_t item_index = begin; item_index < end; ++item_index)
        {
          response->mutable_items()->UnsafeArenaAddAllocated(lane[item_index].response_item);
        }
      }

      if (response)
      {
        publisher->publish_response(response);
      }
    }

    void finish_lane(const std::size_t lane_index, std::optional<std::exception_ptr> exception)
      noexcept
    {
      if (exception)
      {
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

        auto& lane = lanes[lane_index];
        auto& progress = lane_progress[lane_index];
        const auto completed_items = progress.completed.load(std::memory_order_acquire);
        for (std::size_t item_index = completed_items; item_index < lane.size(); ++item_index)
        {
          auto* response_item = lane[item_index].response_item;
          response_item->set_status_code(::grpc::StatusCode::INTERNAL);
          response_item->set_status_message(status_message);
        }
        progress.completed.store(lane.size(), std::memory_order_release);
      }

      if (finished_lanes.fetch_add(1, std::memory_order_acq_rel) + 1 != lane_operations_count)
      {
        return;
      }

      completed.store(true, std::memory_order_release);
      publish_ready();

      auto callback = std::move(completion);
      if (callback)
      {
        callback(std::nullopt);
      }
    }

    static std::optional<Generics::Time> on_timer(void* owner) noexcept
    {
      auto& state = *static_cast<PublishedBatchState*>(owner);
      if (state.completed.load(std::memory_order_acquire))
      {
        return std::nullopt;
      }

      state.publish_ready();

      if (state.completed.load(std::memory_order_acquire) ||
        ++state.response_time_step_index >= state.response_time_steps_us.size())
      {
        return std::nullopt;
      }

      return state.started_at + Generics::Time(
        state.response_time_steps_us[state.response_time_step_index] / Generics::Time::USEC_MAX,
        state.response_time_steps_us[state.response_time_step_index] % Generics::Time::USEC_MAX);
    }

    Scheduler& scheduler;
    const std::uint64_t batch_id;
    std::weak_ptr<BatchResponsePublisher> response_publisher;
    google::protobuf::Arena& response_arena;
    Generics::MonoAllocatorArena batch_arena;
    google::protobuf::Arena request_arena;
    Generics::MonoVector<adserver::grpc::BatchResponseItem*> item_responses;
    Generics::MonoVector<Generics::MonoVector<BatchItemContext>> lanes;
    LaneProgress* lane_progress = nullptr;
    Generics::MonoVector<std::uint32_t> response_time_steps_us;
    const Generics::Time started_at = Generics::Time::get_time_of_day();
    Scheduler::Task timer_task;
    std::size_t response_time_step_index = 0;
    std::atomic<std::size_t> finished_lanes{0};
    std::size_t lane_operations_count = 0;
    std::atomic<bool> completed{false};
    BatchCompletion completion;
  };

  class GrpcServiceBase::BatchRequestAwaiter final
  {
  public:
    BatchRequestAwaiter(
      const GrpcServiceBase& service,
      const adserver::grpc::BatchRequest& batch_request,
      adserver::grpc::BatchResponse& batch_response) noexcept
      : service_(service),
        batch_request_(batch_request),
        batch_response_(batch_response)
    {}

    bool await_ready() const noexcept
    {
      return false;
    }

    bool await_suspend(std::coroutine_handle<> continuation)
    {
      continuation_ = continuation;
      service_.dispatch_batch_request_(
        handle_,
        batch_request_,
        batch_response_,
        [this](std::optional<std::exception_ptr> exception) mutable
        {
          exception_ = std::move(exception);
          if (state_.exchange(2, std::memory_order_release) == 1)
          {
            AdServer::Commons::resume_coroutine(continuation_);
          }
        });

      return state_.exchange(1, std::memory_order_acquire) != 2;
    }

    void await_resume()
    {
      if (exception_)
      {
        std::rethrow_exception(*exception_);
      }
    }

  private:
    const GrpcServiceBase& service_;
    const adserver::grpc::BatchRequest& batch_request_;
    adserver::grpc::BatchResponse& batch_response_;
    BatchProcessingHandle handle_;
    std::optional<std::exception_ptr> exception_;
    std::coroutine_handle<> continuation_;
    std::atomic<unsigned char> state_{0};
  };

  std::uint64_t
  GrpcServiceBase::InprogressStats::add(
    const std::uint64_t call_inflight,
    const Generics::Time& read_time)
  {
    const auto receiver_id = next_receiver_id_.fetch_add(1, std::memory_order_relaxed);

    std::lock_guard<std::mutex> lock(lock_);
    requests_.emplace(receiver_id, Request{read_time, call_inflight});
    call_inflight_ += call_inflight;
    if (!min_time_of_request_in_progress_ || read_time < *min_time_of_request_in_progress_)
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
    return InprogressStatsSnapshot{call_inflight_, min_time_of_request_in_progress_};
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

  GrpcServiceBase::BatchStreamReadLimiter::BatchStreamReadLimiter(Options options) noexcept
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
    if (!read_ahead_enabled_ || !max_requests_in_progress_ || (
      waiters_.empty() && has_capacity_i_()))
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
    Generics::MonoAllocatorArena waiters_arena(waiters_buffer.data(), waiters_buffer.size());
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
    Generics::MonoAllocatorArena waiters_arena(waiters_buffer.data(), waiters_buffer.size());
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
    Generics::MonoAllocatorArena waiters_arena(waiters_buffer.data(), waiters_buffer.size());
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
  GrpcServiceBase::BatchStreamReadLimiter::take_waiters_i_(Generics::MonoVector<WaiterPtr>& result)
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
  GrpcServiceBase::BatchStreamReadLimiter::grant_waiters_(Generics::MonoVector<WaiterPtr>& waiters)
    noexcept
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
  GrpcServiceBase::GrpcServiceBase(BatchStreamReadOptions batch_stream_read_options) noexcept
    : batch_stream_read_limiter_(batch_stream_read_options),
      response_time_scheduler_(std::make_unique<AdServer::Commons::FastScheduler>(1))
  {}

  GrpcServiceBase::~GrpcServiceBase() noexcept
  {
    batch_stream_read_limiter_.clear_waiters();
    response_time_scheduler_.reset();
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
      batch_stream_call_live_.load(std::memory_order_relaxed)
    };
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
    batch_response.set_batch_id(batch_request.batch_id());
    google::protobuf::Arena fallback_response_arena;
    auto* const batch_response_arena = batch_response.GetArena();
    auto& response_arena = batch_response_arena ? *batch_response_arena : fallback_response_arena;

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

  AdServer::Commons::StartableAwaitable<void>
  GrpcServiceBase::co_handle_batch_request(
    const adserver::grpc::BatchRequest& batch_request,
    adserver::grpc::BatchResponse& batch_response) const
  {
    co_await BatchRequestAwaiter(*this, batch_request, batch_response);
    co_return;
  }

  void
  GrpcServiceBase::start_handle_batch_request(
    BatchProcessingHandle& handle,
    const adserver::grpc::BatchRequest& batch_request,
    adserver::grpc::BatchResponse& batch_response,
    BatchCompletion completion) const
  {
    batch_response.set_batch_id(batch_request.batch_id());
    dispatch_batch_request_(
      handle,
      batch_request,
      batch_response,
      std::move(completion));
  }

  void
  GrpcServiceBase::start_handle_batch_request(
    BatchProcessingHandle& handle,
    const adserver::grpc::BatchRequest& batch_request,
    BatchResponsePublisher& response_publisher,
    BatchCompletion completion) const
  {
    if (batch_request.response_time_steps_us().empty())
    {
      auto publisher = response_publisher.retain();
      auto* response = response_publisher.create_response();
      response->set_batch_id(batch_request.batch_id());
      dispatch_batch_request_(
        handle,
        batch_request,
        *response,
        [response, publisher = std::move(publisher), completion = std::move(completion)](
          std::optional<std::exception_ptr> exception) mutable
        {
          if (!exception)
          {
            publisher->publish_response(response);
          }

          if (completion)
          {
            completion(std::move(exception));
          }
        });
      return;
    }

    const std::size_t max_sequential_ops =
      std::max<std::size_t>(1, distributed_batch_max_sequential_ops());
    start_publish_batch_request_distributed_(
      handle,
      batch_request,
      response_publisher,
      std::move(completion),
      max_sequential_ops,
      batch_processing_executor_pool());
  }

  void
  GrpcServiceBase::dispatch_batch_request_(
    BatchProcessingHandle& handle,
    const adserver::grpc::BatchRequest& batch_request,
    adserver::grpc::BatchResponse& batch_response,
    BatchCompletion completion) const
  {
    const std::size_t max_sequential_ops =
      std::max<std::size_t>(1, distributed_batch_max_sequential_ops());
    const std::size_t batch_size = static_cast<std::size_t>(batch_request.items_size());
    const auto executor_pool = batch_processing_executor_pool();
    const std::size_t lane_count = resolve_batch_lane_count(batch_size, max_sequential_ops);

    if (batch_size <= 1 || (lane_count <= 1 && !executor_pool))
    {
      start_handle_batch_request_sequential_(
        handle,
        batch_request,
        batch_response,
        std::move(completion));
      return;
    }

    start_handle_batch_request_distributed_(
      handle,
      batch_request,
      batch_response,
      std::move(completion),
      max_sequential_ops,
      std::move(executor_pool));
  }

  void
  GrpcServiceBase::start_handle_batch_request_sequential_(
    BatchProcessingHandle& handle,
    const adserver::grpc::BatchRequest& batch_request,
    adserver::grpc::BatchResponse& batch_response,
    BatchCompletion completion) const
  {
    struct SequentialBatchHandle final
    {
      explicit SequentialBatchHandle(
        AdServer::Commons::StartableAwaitable<void> operation_val) noexcept
        : operation(std::move(operation_val))
      {}

      AdServer::Commons::StartableAwaitable<void> operation;
    };

    auto sequential_handle = std::make_shared<SequentialBatchHandle>(
      co_handle_batch_request_sequential_(batch_request, batch_response));
    handle = sequential_handle;
    sequential_handle->operation.start(
      [
        sequential_handle = std::move(sequential_handle),
        completion = std::move(completion)
      ](std::optional<std::exception_ptr> exception) mutable
      {
        completion(std::move(exception));
      });
  }

  std::size_t
  GrpcServiceBase::distributed_batch_max_sequential_ops() const noexcept
  {
    return 1;
  }

  std::shared_ptr<AdServer::Commons::ExecutorPool>
  GrpcServiceBase::batch_processing_executor_pool() const noexcept
  {
    return {};
  }

  AdServer::Commons::StartableAwaitable<void>
  GrpcServiceBase::co_handle_batch_request_sequential_(
    const adserver::grpc::BatchRequest& batch_request,
    adserver::grpc::BatchResponse& batch_response) const
  {
    batch_response.mutable_items()->Reserve(batch_request.items_size());
    google::protobuf::Arena fallback_response_arena;
    auto* const batch_response_arena = batch_response.GetArena();
    auto& response_arena = batch_response_arena ? *batch_response_arena : fallback_response_arena;

    for (int i = 0; i < batch_request.items_size(); ++i)
    {
      const auto& request_item = batch_request.items(i);
      auto* response_item = batch_response.add_items();
      co_await co_handle_batch_item_(request_item, *response_item, response_arena);
    }

    co_return;
  }

  void
  GrpcServiceBase::start_handle_batch_request_distributed_(
    BatchProcessingHandle& handle,
    const adserver::grpc::BatchRequest& batch_request,
    adserver::grpc::BatchResponse& batch_response,
    BatchCompletion completion,
    const std::size_t max_sequential_ops,
    std::shared_ptr<AdServer::Commons::ExecutorPool> executor_pool) const
  {
    struct DistributedBatchHandle final
    {
      DistributedBatchHandle(
        adserver::grpc::BatchResponse& batch_response_val,
        BatchCompletion completion_val)
        : batch_response(batch_response_val),
          batch_response_arena(batch_response.GetArena()),
          item_responses(&batch_arena),
          heap_item_responses(&batch_arena),
          lanes(&batch_arena),
          operations(&batch_arena),
          completion(std::move(completion_val))
      {}

      google::protobuf::Arena& response_arena() noexcept
      {
        return batch_response_arena ? *batch_response_arena : fallback_response_arena;
      }

      Generics::MonoAllocatorArena batch_arena;
      google::protobuf::Arena request_arena;
      google::protobuf::Arena fallback_response_arena;
      adserver::grpc::BatchResponse& batch_response;
      google::protobuf::Arena* const batch_response_arena;
      Generics::MonoVector<adserver::grpc::BatchResponseItem*> item_responses;
      Generics::MonoVector<std::unique_ptr<adserver::grpc::BatchResponseItem>> heap_item_responses;
      Generics::MonoVector<Generics::MonoVector<BatchItemContext>> lanes;
      Generics::MonoVector<AdServer::Commons::StartableAwaitable<void>> operations;
      std::mutex lock;
      std::optional<std::exception_ptr> exception;
      std::size_t remaining = 0;
      BatchCompletion completion;
    };

    const std::size_t batch_size = static_cast<std::size_t>(batch_request.items_size());
    const std::size_t sequential_ops = std::max<std::size_t>(1, max_sequential_ops);
    const std::size_t lane_count = resolve_batch_lane_count(batch_size, sequential_ops);
    const std::size_t split = std::min(lane_count, batch_size);

    auto state = std::make_shared<DistributedBatchHandle>(batch_response, std::move(completion));
    handle = state;

    state->item_responses.reserve(batch_size);
    if (!state->batch_response_arena)
    {
      state->heap_item_responses.reserve(batch_size);
    }

    for (std::size_t i = 0; i < batch_size; ++i)
    {
      if (state->batch_response_arena)
      {
        state->item_responses.emplace_back(
          google::protobuf::Arena::CreateMessage<
            adserver::grpc::BatchResponseItem>(state->batch_response_arena));
      }
      else
      {
        auto item_response = std::make_unique<adserver::grpc::BatchResponseItem>();
        state->item_responses.emplace_back(item_response.get());
        state->heap_item_responses.emplace_back(std::move(item_response));
      }
    }

    state->lanes.reserve(split);
    const std::size_t lane_reserve = std::min(sequential_ops, batch_size);
    for (std::size_t i = 0; i < split; ++i)
    {
      state->lanes.emplace_back(Generics::MonoAllocator<BatchItemContext>(&state->batch_arena));
      state->lanes.back().reserve(lane_reserve);
    }

    std::size_t sequential_lane = 0;
    std::size_t sequential_lane_size = 0;
    auto next_sequential_lane = [&]() noexcept
    {
      const std::size_t result = sequential_lane;
      if (++sequential_lane_size >= sequential_ops && sequential_lane + 1 < split)
      {
        ++sequential_lane;
        sequential_lane_size = 0;
      }

      return result;
    };

    for (int i = 0; i < batch_request.items_size(); ++i)
    {
      const auto& request_item = batch_request.items(i);
      auto* response_item = state->item_responses[static_cast<std::size_t>(i)];
      response_item->set_request_id(request_item.request_id());

      BatchItemContext batch_item{&request_item, response_item, {}};

      std::size_t lane_index = 0;
      bool hash_present = false;
      bool enqueue = true;
      const auto coro_it = batch_coro_methods_.find(request_item.full_method());
      if (coro_it != batch_coro_methods_.end())
      {
        enqueue = prepare_batch_coro_item_(
          coro_it->second,
          request_item,
          *response_item,
          state->request_arena,
          batch_item.coro_item);
        if (enqueue && coro_it->second.distributed && batch_item.coro_item.hash_present)
        {
          lane_index = batch_item.coro_item.hash % split;
          hash_present = true;
        }
      }

      if (enqueue)
      {
        if (!hash_present)
        {
          lane_index = next_sequential_lane();
        }
        state->lanes[lane_index].emplace_back(std::move(batch_item));
      }
    }

    state->operations.reserve(split);
    for (std::size_t i = 1; i < state->lanes.size(); ++i)
    {
      auto& lane = state->lanes[i];
      if (lane.empty())
      {
        continue;
      }

      state->operations.emplace_back(co_handle_batch_lane_(
        std::move(lane),
        executor_pool,
        true,
        state->response_arena()));
    }

    if (!state->lanes.empty() && !state->lanes[0].empty())
    {
      state->operations.emplace_back(co_handle_batch_lane_(
        std::move(state->lanes[0]),
        executor_pool,
        false,
        state->response_arena()));
    }

    auto finish_batch =
      [state](std::optional<std::exception_ptr> exception) mutable noexcept
      {
        if (!exception)
        {
          try
          {
            state->batch_response.mutable_items()->Reserve(
              static_cast<int>(state->item_responses.size()));
            if (state->batch_response_arena)
            {
              for (auto* item_response : state->item_responses)
              {
                state->batch_response.mutable_items()->UnsafeArenaAddAllocated(item_response);
              }
            }
            else
            {
              for (auto& item_response : state->heap_item_responses)
              {
                state->batch_response.mutable_items()->AddAllocated(item_response.release());
              }
            }
          }
          catch (...)
          {
            exception = std::current_exception();
          }
        }

        auto completion = std::move(state->completion);
        if (completion)
        {
          completion(std::move(exception));
        }
      };

    auto finish_lane =
      [state, finish_batch](std::optional<std::exception_ptr> exception) mutable noexcept
      {
        bool last = false;
        std::optional<std::exception_ptr> result_exception;

        {
          std::lock_guard<std::mutex> lock(state->lock);
          if (exception && !state->exception)
          {
            state->exception = std::move(exception);
          }

          if (state->remaining > 0 && --state->remaining == 0)
          {
            last = true;
            result_exception = std::move(state->exception);
          }
        }

        if (last)
        {
          finish_batch(std::move(result_exception));
        }
      };

    state->remaining = state->operations.size();
    if (state->operations.empty())
    {
      finish_batch(std::nullopt);
      return;
    }

    for (auto& operation : state->operations)
    {
      operation.start(finish_lane);
    }
  }

  void
  GrpcServiceBase::start_publish_batch_request_distributed_(
    BatchProcessingHandle& handle,
    const adserver::grpc::BatchRequest& batch_request,
    BatchResponsePublisher& response_publisher,
    BatchCompletion completion,
    const std::size_t max_sequential_ops,
    std::shared_ptr<AdServer::Commons::ExecutorPool> executor_pool) const
  {
    const std::size_t batch_size = static_cast<std::size_t>(batch_request.items_size());
    const std::size_t sequential_ops = std::max<std::size_t>(1, max_sequential_ops);
    const std::size_t lane_count = resolve_batch_lane_count(batch_size, sequential_ops);
    const std::size_t split = std::min(lane_count, batch_size);

    auto state = std::make_shared<PublishedBatchState>(
      *response_time_scheduler_,
      batch_request,
      response_publisher,
      std::move(completion));
    handle = state;

    state->item_responses.reserve(batch_size);
    for (std::size_t i = 0; i < batch_size; ++i)
    {
      state->item_responses.emplace_back(
        google::protobuf::Arena::CreateMessage<
          adserver::grpc::BatchResponseItem>(&state->response_arena));
    }

    state->lanes.reserve(split);
    const std::size_t lane_reserve = std::min(sequential_ops, batch_size);
    for (std::size_t i = 0; i < split; ++i)
    {
      state->lanes.emplace_back(Generics::MonoAllocator<BatchItemContext>(&state->batch_arena));
      state->lanes.back().reserve(lane_reserve);
    }

    std::size_t sequential_lane = 0;
    std::size_t sequential_lane_size = 0;
    auto next_sequential_lane = [&]() noexcept
    {
      const std::size_t result = sequential_lane;
      if (++sequential_lane_size >= sequential_ops && sequential_lane + 1 < split)
      {
        ++sequential_lane;
        sequential_lane_size = 0;
      }
      return result;
    };

    for (int i = 0; i < batch_request.items_size(); ++i)
    {
      const auto& request_item = batch_request.items(i);
      auto* response_item = state->item_responses[static_cast<std::size_t>(i)];
      response_item->set_request_id(request_item.request_id());

      BatchItemContext batch_item{&request_item, response_item, {}};
      std::size_t lane_index = 0;
      bool hash_present = false;
      const auto coro_it = batch_coro_methods_.find(request_item.full_method());
      if (coro_it != batch_coro_methods_.end())
      {
        if (!prepare_batch_coro_item_(
          coro_it->second,
          request_item,
          *response_item,
          state->request_arena,
          batch_item.coro_item))
        {
          batch_item.request_item = nullptr;
        }
        else if (coro_it->second.distributed && batch_item.coro_item.hash_present)
        {
          lane_index = batch_item.coro_item.hash % split;
          hash_present = true;
        }
      }

      if (!hash_present)
      {
        lane_index = next_sequential_lane();
      }

      state->lanes[lane_index].emplace_back(std::move(batch_item));
    }

    state->initialize_lane_progress();
    for (std::size_t lane_index = 0; lane_index < state->lanes.size(); ++lane_index)
    {
      if (state->lanes[lane_index].empty())
      {
        continue;
      }

      ++state->lane_operations_count;
    }

    if (state->lane_operations_count == 0)
    {
      state->completed.store(true, std::memory_order_release);
      state->publish_ready();
      auto callback = std::move(state->completion);
      if (callback)
      {
        callback(std::nullopt);
      }
      return;
    }

    state->schedule_first_response(state);

    std::size_t operation_index = 0;
    for (std::size_t lane_index = 0; lane_index < state->lanes.size(); ++lane_index)
    {
      if (state->lanes[lane_index].empty())
      {
        continue;
      }

      auto operation = co_handle_publish_batch_lane_(
        state, lane_index, executor_pool, operation_index != 0);
      ++operation_index;
      operation.start_detached(
        [state, lane_index](std::optional<std::exception_ptr> exception) mutable noexcept
        {
          state->finish_lane(lane_index, std::move(exception));
        });
    }
  }

  AdServer::Commons::StartableAwaitable<void>
  GrpcServiceBase::co_handle_publish_batch_lane_(
    std::shared_ptr<PublishedBatchState> state,
    const std::size_t lane_index,
    std::shared_ptr<AdServer::Commons::ExecutorPool> executor_pool,
    const bool reschedule) const
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

    auto& lane = state->lanes[lane_index];
    auto& progress = state->lane_progress[lane_index];
    for (std::size_t item_index = 0; item_index < lane.size(); ++item_index)
    {
      auto& batch_item = lane[item_index];
      if (batch_item.request_item)
      {
        if (batch_item.coro_item.method)
        {
          co_await co_handle_prepared_batch_coro_item_(
            batch_item.coro_item, *batch_item.response_item, state->response_arena);
        }
        else
        {
          co_await co_handle_batch_item_(
            *batch_item.request_item, *batch_item.response_item, state->response_arena);
        }
      }

      progress.completed.store(item_index + 1, std::memory_order_release);
    }

    co_return;
  }

  AdServer::Commons::StartableAwaitable<void>
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
          batch_item.coro_item, *batch_item.response_item, response_arena);
      }
      else
      {
        co_await co_handle_batch_item_(
          *batch_item.request_item, *batch_item.response_item, response_arena);
      }
    }

    co_return;
  }

  AdServer::Commons::StartableAwaitable<void>
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
        co_await co_handle_prepared_batch_coro_item_(coro_item, response_item, response_arena);
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
      if (!method.prepare(request_item, response_item, request_arena, coro_item))
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

  AdServer::Commons::StartableAwaitable<void>
  GrpcServiceBase::co_handle_prepared_batch_coro_item_(
    PreparedBatchCoroItem& coro_item,
    adserver::grpc::BatchResponseItem& response_item,
    google::protobuf::Arena& response_arena) const
  {
    try
    {
      co_await coro_item.method->dispatch(coro_item.request, response_item, response_arena);
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
}
