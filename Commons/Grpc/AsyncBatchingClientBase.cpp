#include <Commons/Grpc/AsyncBatchingClientBase.hpp>

#include <algorithm>
#include <cassert>
#include <utility>
#include <vector>

#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include <grpcpp/support/channel_arguments.h>

namespace AdServer::Grpc
{
  struct AsyncBatchingClientBase::StreamHolder
  {
    BatchingStreamPtr stream;
  };

  namespace
  {
    void finish_batch_with_error(
      BatchingQueue::Batch& batch,
      grpc::StatusCode status_code,
      const char* status_message)
    {
      for (auto& request : batch)
      {
        adserver::grpc::BatchResponseItem item;
        item.set_status_code(status_code);
        item.set_status_message(status_message);
        if (request->request && request->request->callback)
        {
          request->request->callback(item);
        }
      }
      batch.clear();
    }

    void finish_batches_with_error(
      std::vector<BatchingQueue::Batch>& batches,
      grpc::StatusCode status_code,
      const char* status_message)
    {
      for (auto& batch : batches)
      {
        finish_batch_with_error(batch, status_code, status_message);
      }
      batches.clear();
    }
  }

  // This helper guarantees callback delivery when a batch owner is lost inside
  // the asio executor processing stack before the callback path is reached.
  class AsyncBatchingClientBase::DetachedBatchStorage final:
    public std::enable_shared_from_this<DetachedBatchStorage>
  {
  public:
    struct BatchGuard
    {
      explicit BatchGuard(
        std::uint64_t id_val,
        BatchingStreamBase::PendingBatch batch_val)
        : id(id_val),
          batch(std::move(batch_val))
      {}

      BatchingStreamBase::PendingBatch claim_payload()
      {
        if (claimed.exchange(true, std::memory_order_acq_rel))
        {
          return {};
        }

        return std::move(batch);
      }

      const std::uint64_t id;

    private:
      std::atomic<bool> claimed{false};
      BatchingStreamBase::PendingBatch batch;
    };

    DetachedBatchOwnerPtr register_batch(
      BatchingStreamBase::PendingBatch&& batch);

    void unregister_batch(std::uint64_t id);

    void close();

    std::vector<BatchingStreamBase::PendingBatch> drain_all();

  private:
    using BatchGuardPtr = std::shared_ptr<BatchGuard>;

  private:
    std::mutex lock_;
    bool closed_ = false;
    std::unordered_map<std::uint64_t, BatchGuardPtr> batches_;
    std::atomic<std::uint64_t> next_id_{1};
  };

  struct AsyncBatchingClientBase::DetachedBatchOwner
  {
    DetachedBatchOwner(
      std::weak_ptr<DetachedBatchStorage> holder_val,
      std::shared_ptr<DetachedBatchStorage::BatchGuard> guard_val)
      : holder(std::move(holder_val)),
        guard(std::move(guard_val))
    {}

    ~DetachedBatchOwner() = default;

    DetachedBatchOwner(const DetachedBatchOwner&) = delete;
    DetachedBatchOwner& operator=(const DetachedBatchOwner&) = delete;

    BatchingStreamBase::PendingBatch claim_payload()
    {
      assert(guard);
      return guard->claim_payload();
    }

    std::weak_ptr<DetachedBatchStorage> holder;
    std::shared_ptr<DetachedBatchStorage::BatchGuard> guard;
  };

  AsyncBatchingClientBase::DetachedBatchOwnerPtr
  AsyncBatchingClientBase::DetachedBatchStorage::register_batch(
    BatchingStreamBase::PendingBatch&& batch)
  {
    assert(!batch.empty());

    std::lock_guard<std::mutex> lock(lock_);
    assert(!closed_);

    const auto id = next_id_.fetch_add(1, std::memory_order_relaxed);
    auto batch_guard = std::make_shared<BatchGuard>(id, std::move(batch));
    batches_.emplace(id, batch_guard);
    return std::make_shared<DetachedBatchOwner>(
      shared_from_this(),
      std::move(batch_guard));
  }

  void
  AsyncBatchingClientBase::DetachedBatchStorage::unregister_batch(
    std::uint64_t id)
  {
    std::lock_guard<std::mutex> lock(lock_);
    batches_.erase(id);
  }

  void
  AsyncBatchingClientBase::DetachedBatchStorage::close()
  {
    std::lock_guard<std::mutex> lock(lock_);
    closed_ = true;
  }

  std::vector<BatchingStreamBase::PendingBatch>
  AsyncBatchingClientBase::DetachedBatchStorage::drain_all()
  {
    decltype(batches_) batches;
    {
      std::lock_guard<std::mutex> lock(lock_);
      batches.swap(batches_);
    }

    std::vector<BatchingStreamBase::PendingBatch> result;
    result.reserve(batches.size());
    for (auto& [_, batch] : batches)
    {
      auto pending_batch = batch->claim_payload();
      if (!pending_batch.empty())
      {
        result.emplace_back(std::move(pending_batch));
      }
    }

    return result;
  }

  AsyncBatchingClientBase::AsyncBatchingClientBase(
    const std::string& endpoint,
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor,
    std::shared_ptr<AdServer::Commons::BoostAsioContextRunActiveObject>
      coalesce_runner,
    AdServer::Grpc::BatchingOptions options)
    : Generics::CompositeActiveObject(false, false),
      endpoint_(endpoint),
      options_(std::move(options)),
      max_streams_(std::max<std::size_t>(1, options_.channels_number)),
      grpc_executor_(std::move(grpc_executor)),
      coalesce_runner_(std::move(coalesce_runner)),
      batching_queue_(std::make_shared<AdServer::Grpc::BatchingQueue>(options_)),
      detached_batch_storage_(std::make_shared<DetachedBatchStorage>()),
      inflight_limiter_(options_.max_inflight),
      submission_gate_(std::make_shared<AdServer::Commons::ActivityGate>())
  {
    if (!grpc_executor_)
    {
      throw InvalidParam("AsyncBatchingClientBase requires GrpcExecutor");
    }

    if (!coalesce_runner_)
    {
      throw InvalidParam(
        "AsyncBatchingClientBase requires BoostAsioContextRunActiveObject");
    }

    grpc::ChannelArguments channel_args;
    channel_args.SetMaxReceiveMessageSize(-1);
    channel_args.SetMaxSendMessageSize(-1);
    channel_args.SetInt(GRPC_ARG_ENABLE_HTTP_PROXY, 0);
    channel_args.SetInt(
      GRPC_ARG_USE_LOCAL_SUBCHANNEL_POOL,
      options_.use_local_subchannel_pool ? 1 : 0);
    if (!options_.enable_grpc_compression)
    {
      channel_args.SetCompressionAlgorithm(GRPC_COMPRESS_NONE);
    }

    channel_ = grpc::CreateCustomChannel(
      endpoint_,
      grpc::InsecureChannelCredentials(),
      channel_args);

    streams_.reserve(max_streams_);
  }

  AsyncBatchingClientBase::~AsyncBatchingClientBase() = default;

  void
  AsyncBatchingClientBase::activate_object_()
  {
    detached_batch_storage_ = std::make_shared<DetachedBatchStorage>();
    timing_coalesce_gate_ =
      std::make_shared<AdServer::Commons::ActivityGate>();
    submission_gate_->activate_object();
    timing_coalesce_gate_->activate_object();
    {
      std::lock_guard<std::mutex> lock(coalesce_timer_lock_);
      coalesce_timer_deadline_.reset();
    }
    Generics::CompositeActiveObject::activate_object_();
  }

  void
  AsyncBatchingClientBase::deactivate_object_()
  {
    submission_gate_->deactivate_object();
    assert(timing_coalesce_gate_);
    timing_coalesce_gate_->deactivate_object();
    detached_batch_storage_->close();
    Generics::CompositeActiveObject::deactivate_object_();
    deactivate_streams_();
  }

  void
  AsyncBatchingClientBase::wait_object_()
  {
    submission_gate_->wait_object();
    wait_streams_();
    clear_deferred_streams_();
    Generics::CompositeActiveObject::wait_object_();
    assert(timing_coalesce_gate_);
    timing_coalesce_gate_->wait_object();
    auto detached_batches = detached_batch_storage_->drain_all();
    finish_batches_with_error(
      detached_batches,
      grpc::StatusCode::UNAVAILABLE,
      "inactive");
    auto deferred_dispatch_batches = drain_deferred_dispatch_batches_();
    finish_batches_with_error(
      deferred_dispatch_batches,
      grpc::StatusCode::UNAVAILABLE,
      "inactive");
    auto batches = batching_queue_->drain_all();
    finish_batches_with_error(
      batches,
      grpc::StatusCode::UNAVAILABLE,
      "inactive");
    clear_streams_();
    timing_coalesce_gate_.reset();
  }

  void
  AsyncBatchingClientBase::release_request_() noexcept
  {
    if (options_.max_outstanding_requests.has_value())
    {
      outstanding_requests_.fetch_sub(1, std::memory_order_acq_rel);
    }
    inflight_limiter_.release();
  }

  AdServer::Grpc::Stats
  AsyncBatchingClientBase::stats() const noexcept
  {
    auto total = AdServer::Grpc::Client::stats();
    total.max_streams = max_streams_seen_.load(std::memory_order_relaxed);
    return total;
  }

  void
  AsyncBatchingClientBase::enqueue_serialized_request_(
    const char* full_method,
    std::string payload,
    BatchResponseCallback callback)
  {
    auto submission_guard = submission_gate_->enter();
    if (!submission_guard)
    {
      if (callback)
      {
        adserver::grpc::BatchResponseItem item;
        item.set_status_code(NO_ACTIVE_BATCHING_STREAMS_STATUS.error_code());
        item.set_status_message(NO_ACTIVE_BATCHING_STREAMS_STATUS.error_message());
        callback(item);
      }
      return;
    }

    // Increase and check inflight limits.
    if (options_.error_on_inflight_reaching)
    {
      if (!inflight_limiter_.try_acquire())
      {
        if (callback)
        {
          adserver::grpc::BatchResponseItem item;
          item.set_status_code(grpc::StatusCode::RESOURCE_EXHAUSTED);
          item.set_status_message("inflight limit reached");
          callback(item);
        }
        return;
      }
    }
    else
    {
      inflight_limiter_.acquire();
    }

    if (options_.max_outstanding_requests.has_value())
    {
      auto outstanding_requests =
        outstanding_requests_.load(std::memory_order_acquire);
      while (outstanding_requests < *options_.max_outstanding_requests)
      {
        if (outstanding_requests_.compare_exchange_strong(
              outstanding_requests,
              outstanding_requests + 1,
              std::memory_order_acq_rel,
              std::memory_order_acquire))
        {
          break;
        }
      }

      if (outstanding_requests >= *options_.max_outstanding_requests)
      {
        inflight_limiter_.release();
        if (callback)
        {
          adserver::grpc::BatchResponseItem item;
          item.set_status_code(grpc::StatusCode::UNAVAILABLE);
          item.set_status_message("max outstanding requests reached");
          callback(item);
        }
        return;
      }
    }

    auto pending_request = std::make_shared<BatchingQueue::PendingRequest>();
    pending_request->full_method = full_method;
    pending_request->payload = std::move(payload);
    pending_request->callback =
      [
        this,
        callback = std::move(callback)
      ](const auto& batch_response) mutable
      {
        if (callback)
        {
          callback(batch_response);
        }
        release_request_();
      };

    auto pending_operation =
      std::make_shared<BatchingQueue::PendingOperation>();
    pending_operation->request = std::move(pending_request);
    BatchingQueue::Batch batch;
    batch.emplace_back(std::move(pending_operation));
    auto enqueue_result = batching_queue_->enqueue(std::move(batch));

    if (enqueue_result.was_empty_before_push &&
      options_.max_batch_delay.has_value())
    {
      schedule_timing_coalesce_();
    }

    if (!enqueue_result.ready_batch.empty())
    {
      auto* stream = try_acquire_stream_();
      if (!stream)
      {
        if (fail_pending_if_no_streams_())
        {
          finish_batch_with_error(
            enqueue_result.ready_batch,
            grpc::StatusCode::UNAVAILABLE,
            "no active batching streams");
          return;
        }

        defer_dispatch_batch_(std::move(enqueue_result.ready_batch));
        schedule_timing_coalesce_();
        return;
      }

      dispatch_batch_(std::move(enqueue_result.ready_batch), stream);
    }
  }

  AsyncBatchingClientBase::StreamHolderPtr
  AsyncBatchingClientBase::make_stream_()
  {
    auto stream_holder = std::make_shared<StreamHolder>();
    std::weak_ptr<StreamHolder> weak_stream_holder = stream_holder;
    stream_holder->stream = std::make_shared<AdServer::Grpc::BatchingStreamBase>(
        channel_,
        grpc_executor_,
        batching_queue_,
        next_queue_index_.fetch_add(1, std::memory_order_relaxed),
        this,
        [this](auto* stream) {
          handle_stream_ready_(stream);
        },
        [this](auto* stream) noexcept {
          handle_stream_closed_(stream);
        },
        [this, weak_stream_holder](auto* stream) noexcept {
          if (auto stream_holder = weak_stream_holder.lock())
          {
            handle_stream_drained_(stream, stream_holder);
          }
        },
        options_);
    return stream_holder;
  }

  BatchingStreamBase*
  AsyncBatchingClientBase::try_acquire_stream_()
  {
    clear_deferred_streams_();

    if (!active())
    {
      return nullptr;
    }

    {
      std::lock_guard<std::mutex> lock(streams_lock_);
      while (!available_streams_.empty())
      {
        auto* stream = available_streams_.front();
        available_streams_.pop_front();
        return stream;
      }
    }

    auto streams_count = up_streams_.load(std::memory_order_acquire);
    bool stream_slot_reserved = false;
    while (streams_count < max_streams_)
    {
      if (up_streams_.compare_exchange_strong(
            streams_count,
            streams_count + 1,
            std::memory_order_acq_rel,
            std::memory_order_acquire))
      {
        ++streams_count;
        stream_slot_reserved = true;
        break;
      }
    }

    if (stream_slot_reserved)
    {
      stream_start_attempts_.fetch_add(1, std::memory_order_acq_rel);
      StreamHolderPtr stream_holder;
      BatchingStreamBase* stream = nullptr;
      bool stream_registered = false;
      try
      {
        stream_holder = make_stream_();
        stream = stream_holder->stream.get();
        if (!active())
        {
          up_streams_.fetch_sub(1, std::memory_order_acq_rel);
          return nullptr;
        }
        {
          std::lock_guard<std::mutex> streams_lock(streams_registry_lock_);
          streams_.emplace(stream, stream_holder);
        }
        stream_registered = true;
        update_max_streams_(streams_count);
        stream_holder->stream->activate_object();
      }
      catch (...)
      {
        bool release_stream_slot = !stream_registered;
        if (stream_registered)
        {
          std::lock_guard<std::mutex> streams_lock(streams_registry_lock_);
          auto it = streams_.find(stream);
          if (it != streams_.end())
          {
            streams_.erase(it);
            release_stream_slot = true;
          }
        }
        if (release_stream_slot)
        {
          up_streams_.fetch_sub(1, std::memory_order_acq_rel);
        }
        throw;
      }
    }

    {
      std::lock_guard<std::mutex> lock(streams_lock_);
      while (!available_streams_.empty())
      {
        auto* stream = available_streams_.front();
        available_streams_.pop_front();
        return stream;
      }
    }

    return nullptr;
  }

  void
  AsyncBatchingClientBase::release_stream_(BatchingStreamBase* stream) noexcept
  {
    assert(stream);

    {
      std::lock_guard<std::mutex> lock(streams_lock_);
      if (active() && stream->available())
      {
        available_streams_.emplace_back(stream);
      }
    }
  }

  bool
  AsyncBatchingClientBase::dispatch_batch_(
    BatchingStreamBase::PendingBatch&& batch,
    BatchingStreamBase* stream) noexcept
  {
    assert(stream);

    auto owner = detached_batch_storage_->register_batch(std::move(batch));
    assert(owner);
    auto owned_batch = owner->claim_payload();
    assert(!owned_batch.empty());

    BatchingStreamBase::PendingBatch failed_batch;
    bool write_started = false;
    try
    {
      write_started = stream->try_start_write(
        std::move(owned_batch),
        false,
        &failed_batch);
    }
    catch (...)
    {
      failed_batch = std::move(owned_batch);
    }

    detached_batch_storage_->unregister_batch(owner->guard->id);
    if (write_started)
    {
      return true;
    }

    release_stream_(stream);
    if (!failed_batch.empty())
    {
      finish_batch_with_error(
        failed_batch,
        grpc::StatusCode::UNAVAILABLE,
        "stream write failed");
    }
    return false;
  }

  void
  AsyncBatchingClientBase::schedule_timing_coalesce_() noexcept
  {
    const auto deadline = batching_queue_->next_deadline();
    if (!active() || !deadline)
    {
      return;
    }

    {
      std::lock_guard<std::mutex> lock(coalesce_timer_lock_);
      if (coalesce_timer_deadline_ && *coalesce_timer_deadline_ <= *deadline)
      {
        return;
      }
      coalesce_timer_deadline_ = *deadline;
    }

    const auto gate = timing_coalesce_gate_;
    assert(gate);

    const auto now = Generics::Time::get_time_of_day();
    const auto timeout =
      now < *deadline ? *deadline - now : Generics::Time::ZERO;

    try
    {
      coalesce_runner_->schedule(
        timeout,
        [this, gate, deadline = *deadline]()
        {
          auto guard = gate->enter();
          if (!guard)
          {
            return;
          }
          run_timing_coalesce_(deadline);
        });
    }
    catch (...)
    {
      std::lock_guard<std::mutex> lock(coalesce_timer_lock_);
      if (coalesce_timer_deadline_ &&
        *coalesce_timer_deadline_ == *deadline)
      {
        coalesce_timer_deadline_.reset();
      }
    }
  }

  void
  AsyncBatchingClientBase::run_timing_coalesce_(
    Generics::Time deadline) noexcept
  {
    try
    {
      {
        std::lock_guard<std::mutex> lock(coalesce_timer_lock_);
        if (!coalesce_timer_deadline_ ||
          *coalesce_timer_deadline_ != deadline)
        {
          return;
        }
        coalesce_timer_deadline_.reset();
      }
      coalesce_timed_batch_();
      schedule_timing_coalesce_();
    }
    catch (...)
    {
    }
  }

  bool
  AsyncBatchingClientBase::coalesce_timed_batch_() noexcept
  {
    auto* stream = try_acquire_stream_();
    if (!stream)
    {
      fail_pending_if_no_streams_();
      return false;
    }

    BatchingStreamBase::PendingBatch pending_batch;
    const bool has_batch = batching_queue_->try_pop_due_batch(pending_batch);
    if (!has_batch)
    {
      release_stream_(stream);
      return false;
    }

    return dispatch_batch_(std::move(pending_batch), stream);
  }

  bool
  AsyncBatchingClientBase::dispatch_ready_batch_() noexcept
  {
    auto* stream = try_acquire_stream_();
    if (!stream)
    {
      fail_pending_if_no_streams_();
      return false;
    }

    BatchingStreamBase::PendingBatch pending_batch;
    if (!batching_queue_->try_pop_ready_batch(pending_batch))
    {
      release_stream_(stream);
      return false;
    }

    return dispatch_batch_(std::move(pending_batch), stream);
  }

  void
  AsyncBatchingClientBase::dispatch_deferred_batches_() noexcept
  {
    while (true)
    {
      auto* stream = try_acquire_stream_();
      if (!stream)
      {
        fail_pending_if_no_streams_();
        return;
      }

      BatchingStreamBase::PendingBatch pending_batch;
      if (!try_pop_deferred_dispatch_batch_(pending_batch))
      {
        release_stream_(stream);
        return;
      }

      dispatch_batch_(std::move(pending_batch), stream);
    }
  }

  void
  AsyncBatchingClientBase::defer_dispatch_batch_(
    BatchingStreamBase::PendingBatch&& batch) noexcept
  {
    if (batch.empty())
    {
      return;
    }

    std::lock_guard<std::mutex> lock(deferred_dispatch_lock_);
    deferred_dispatch_batches_.emplace_back(std::move(batch));
  }

  bool
  AsyncBatchingClientBase::try_pop_deferred_dispatch_batch_(
    BatchingStreamBase::PendingBatch& batch) noexcept
  {
    std::lock_guard<std::mutex> lock(deferred_dispatch_lock_);
    if (deferred_dispatch_batches_.empty())
    {
      return false;
    }

    batch = std::move(deferred_dispatch_batches_.front());
    deferred_dispatch_batches_.pop_front();
    return true;
  }

  std::vector<BatchingStreamBase::PendingBatch>
  AsyncBatchingClientBase::drain_deferred_dispatch_batches_() noexcept
  {
    std::deque<BatchingStreamBase::PendingBatch> batches;
    {
      std::lock_guard<std::mutex> lock(deferred_dispatch_lock_);
      batches.swap(deferred_dispatch_batches_);
    }

    return std::vector<BatchingStreamBase::PendingBatch>(
      std::make_move_iterator(batches.begin()),
      std::make_move_iterator(batches.end()));
  }

  void
  AsyncBatchingClientBase::handle_stream_ready_(BatchingStreamBase* stream)
  {
    stream_start_attempts_.store(0, std::memory_order_release);
    release_stream_(stream);
    dispatch_deferred_batches_();
    dispatch_ready_batch_();
  }

  void
  AsyncBatchingClientBase::handle_stream_closed_(
    BatchingStreamBase* stream) noexcept
  {
    assert(stream);

    bool stream_removed = false;
    StreamHolderPtr stream_holder;
    {
      std::lock_guard<std::mutex> registry_lock(streams_registry_lock_);
      auto it = streams_.find(stream);
      if (it != streams_.end())
      {
        stream_holder = std::move(it->second);
        streams_.erase(it);
        draining_streams_.emplace_back(std::move(stream_holder));
        stream_removed = true;
      }
    }

    if (!stream_removed)
    {
      return;
    }

    {
      std::lock_guard<std::mutex> lock(streams_lock_);
      available_streams_.erase(
        std::remove(
          available_streams_.begin(),
          available_streams_.end(),
          stream),
        available_streams_.end());
    }

    up_streams_.fetch_sub(1, std::memory_order_acq_rel);
    fail_pending_if_no_streams_();
  }

  void
  AsyncBatchingClientBase::handle_stream_drained_(
    BatchingStreamBase* stream,
    const StreamHolderPtr& stream_holder) noexcept
  {
    assert(stream);

    std::lock_guard<std::mutex> registry_lock(streams_registry_lock_);
    auto it = std::find(
      draining_streams_.begin(),
      draining_streams_.end(),
      stream_holder);
    if (it == draining_streams_.end())
    {
      return;
    }

    auto holder = std::move(*it);
    draining_streams_.erase(it);
    if (holder && holder->stream)
    {
      deferred_streams_.emplace_back(std::move(holder->stream));
    }
  }

  bool
  AsyncBatchingClientBase::fail_pending_if_no_streams_() noexcept
  {
    const auto stream_start_attempts =
      stream_start_attempts_.load(std::memory_order_acquire);
    const auto up_streams =
      up_streams_.load(std::memory_order_acquire);
    const auto start_attempts_threshold = std::max<std::size_t>(
      1,
      std::min(max_streams_, options_.workers_number));

    if (!active() ||
      up_streams != 0 ||
      stream_start_attempts < start_attempts_threshold ||
      stream_start_attempts <= up_streams)
    {
      return false;
    }

    {
      std::lock_guard<std::mutex> lock(streams_lock_);
      if (!available_streams_.empty())
      {
        return false;
      }
    }

    auto batches = batching_queue_->drain_all();
    finish_batches_with_error(
      batches,
      grpc::StatusCode::UNAVAILABLE,
      "no active batching streams");
    stream_start_attempts_.store(0, std::memory_order_release);
    return true;
  }

  void
  AsyncBatchingClientBase::update_max_streams_(
    std::size_t streams_count) noexcept
  {
    auto current = max_streams_seen_.load(std::memory_order_relaxed);
    while (current < streams_count &&
      !max_streams_seen_.compare_exchange_weak(
        current,
        streams_count,
        std::memory_order_relaxed))
    {}
  }

  void
  AsyncBatchingClientBase::deactivate_streams_() noexcept
  {
    std::vector<StreamHolderPtr> streams;
    {
      std::lock_guard<std::mutex> lock(streams_registry_lock_);
      streams.reserve(streams_.size());
      for (const auto& [_, stream_holder] : streams_)
      {
        streams.emplace_back(stream_holder);
      }
    }

    for (auto& stream_holder : streams)
    {
      try
      {
        if (stream_holder && stream_holder->stream)
        {
          stream_holder->stream->deactivate_object();
        }
      }
      catch (...)
      {
      }
    }
  }

  void
  AsyncBatchingClientBase::wait_streams_() noexcept
  {
    std::unordered_map<BatchingStreamBase*, StreamHolderPtr> streams;
    std::vector<StreamHolderPtr> draining_streams;
    std::vector<BatchingStreamPtr> deferred_streams;
    {
      std::lock_guard<std::mutex> lock(streams_registry_lock_);
      streams.swap(streams_);
      draining_streams.swap(draining_streams_);
      deferred_streams.swap(deferred_streams_);
    }
    {
      std::lock_guard<std::mutex> lock(streams_lock_);
      available_streams_.clear();
    }

    for (auto& [_, stream_holder] : streams)
    {
      try
      {
        if (stream_holder && stream_holder->stream)
        {
          stream_holder->stream->wait_object();
        }
      }
      catch (...)
      {
      }
    }
    for (auto& stream_holder : draining_streams)
    {
      try
      {
        if (stream_holder && stream_holder->stream)
        {
          stream_holder->stream->wait_object();
        }
      }
      catch (...)
      {
      }
    }
  }

  void
  AsyncBatchingClientBase::clear_streams_() noexcept
  {
    std::vector<StreamHolderPtr> draining_streams;
    std::vector<BatchingStreamPtr> deferred_streams;
    {
      std::lock_guard<std::mutex> lock(streams_lock_);
      available_streams_.clear();
    }
    std::unordered_map<BatchingStreamBase*, StreamHolderPtr> streams;
    {
      std::lock_guard<std::mutex> lock(streams_registry_lock_);
      streams.swap(streams_);
      draining_streams.swap(draining_streams_);
      deferred_streams.swap(deferred_streams_);
    }
  }

  void
  AsyncBatchingClientBase::clear_deferred_streams_() noexcept
  {
    std::vector<BatchingStreamPtr> streams_to_destroy;
    {
      std::lock_guard<std::mutex> lock(streams_registry_lock_);
      streams_to_destroy.swap(deferred_streams_);
    }
  }

}
