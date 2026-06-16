#include <Commons/Grpc/AsyncBatchingClientBase.hpp>

#include <algorithm>
#include <cassert>
#include <shared_mutex>
#include <stdexcept>
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
    Generics::Time last_use = Generics::Time::get_time_of_day();
  };

  namespace
  {
    const char NO_ACTIVE_BATCHING_STREAMS_FORCED_SEND[] =
      "no active batching streams (forced send)";
    const char NO_ACTIVE_BATCHING_STREAMS_TIMED_SEND[] =
      "no active batching streams (timed send)";
    const char NO_ACTIVE_BATCHING_STREAMS_AFTER_CONNECT_TRY[] =
      "no active batching streams (after connect trying)";
    const char NO_ACTIVE_BATCHING_STREAMS_CONNECT_EXCEPTION[] =
      "no active batching streams (connect exception)";
    const char NO_ACTIVE_BATCHING_STREAMS_RELEASE_OR_DISPATCH[] =
      "no active batching streams (release or dispatch)";
    const char NO_ACTIVE_BATCHING_STREAMS_SUBMISSION_CLOSED[] =
      "no active batching streams (submission closed)";

    std::uint64_t batch_items_count(
      const std::deque<BatchingStreamBase::PendingBatch>& batches) noexcept
    {
      std::uint64_t result = 0;
      for (const auto& batch : batches)
      {
        result += batch.size();
      }
      return result;
    }

    void finish_batch_with_error(
      BatchingQueue::Batch& batch,
      grpc::StatusCode status_code,
      const char* status_message,
      const std::string& endpoint)
    {
      const auto message = message_with_endpoint(
        status_message ? status_message : "",
        endpoint);
      for (auto& request : batch)
      {
        adserver::grpc::BatchResponseItem item;
        item.set_status_code(status_code);
        item.set_status_message(message);
        if (request.request)
        {
          request.request->complete(item);
        }
      }
      batch.clear();
    }

    void finish_batches_with_error(
      std::vector<BatchingQueue::Batch>& batches,
      grpc::StatusCode status_code,
      const char* status_message,
      const std::string& endpoint)
    {
      for (auto& batch : batches)
      {
        finish_batch_with_error(batch, status_code, status_message, endpoint);
      }
      batches.clear();
    }
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

    channel_ = create_custom_channel(
      endpoint_,
      grpc::InsecureChannelCredentials(),
      channel_args);

    streams_.reserve(max_streams_);
  }

  AsyncBatchingClientBase::~AsyncBatchingClientBase() = default;

  void
  AsyncBatchingClientBase::activate_object_()
  {
    timing_coalesce_gate_ =
      std::make_shared<AdServer::Commons::ActivityGate>();
    stream_shrink_gate_ =
      std::make_shared<AdServer::Commons::ActivityGate>();
    submission_gate_->activate_object();
    timing_coalesce_gate_->activate_object();
    stream_shrink_gate_->activate_object();
    {
      std::unique_lock<std::shared_mutex> lock(coalesce_timer_lock_);
      coalesce_timer_deadline_.reset();
    }
    Generics::CompositeActiveObject::activate_object_();
    schedule_stream_shrink_();
  }

  void
  AsyncBatchingClientBase::deactivate_object_()
  {
    submission_gate_->deactivate_object();
    assert(timing_coalesce_gate_);
    timing_coalesce_gate_->deactivate_object();
    assert(stream_shrink_gate_);
    stream_shrink_gate_->deactivate_object();
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
    assert(stream_shrink_gate_);
    stream_shrink_gate_->wait_object();
    std::vector<BatchingStreamBase::PendingBatch> pending_batches;
    {
      std::lock_guard<std::mutex> lock(streams_lock_);
      pending_batches.reserve(pending_batches_.size());
      while (!pending_batches_.empty())
      {
        pending_batches.emplace_back(std::move(pending_batches_.front()));
        pending_batches_.pop_front();
      }
      connecting_ = false;
      connecting_stream_.reset();
      last_connect_failure_time_.reset();
    }
    finish_batches_with_error(
      pending_batches,
      grpc::StatusCode::UNAVAILABLE,
      "inactive",
      endpoint_);
    auto batches = batching_queue_->drain_all();
    finish_batches_with_error(
      batches,
      grpc::StatusCode::UNAVAILABLE,
      "inactive",
      endpoint_);
    clear_streams_();
    timing_coalesce_gate_.reset();
    stream_shrink_gate_.reset();
  }

  AdServer::Grpc::Stats
  AsyncBatchingClientBase::stats() const noexcept
  {
    auto total = AdServer::Grpc::Client::stats();
    total.max_streams = max_streams_seen_.load(std::memory_order_relaxed);
    total.inflight_items = inflight_limiter_.count();
    total.queue_items = batching_queue_ ? batching_queue_->size() : 0;
    {
      std::lock_guard<std::mutex> lock(streams_lock_);
      total.pending_batches = pending_batches_.size();
      total.pending_batch_items = batch_items_count(pending_batches_);
      total.available_streams = available_streams_.size();
      total.connecting_streams = connecting_ ? 1 : 0;
    }
    {
      std::lock_guard<std::mutex> lock(streams_registry_lock_);
      total.active_streams = streams_.size();
      total.draining_streams = draining_streams_.size();
      total.deferred_streams = deferred_streams_.size();
      for (const auto& [_, stream_holder] : streams_)
      {
        if (stream_holder && stream_holder->stream)
        {
          total.stream_inflight_items +=
            stream_holder->stream->inflight_items();
        }
      }
    }
    return total;
  }

  void
  AsyncBatchingClientBase::enqueue_serialized_request_(
    const char* full_method,
    std::string payload,
    BatchResponseCallback callback)
  {
    add_input_stats(1);

    auto submission_guard = submission_gate_->enter();
    if (!submission_guard)
    {
      if (callback)
      {
        adserver::grpc::BatchResponseItem item;
        item.set_status_code(NO_ACTIVE_BATCHING_STREAMS_STATUS.error_code());
        item.set_status_message(message_with_endpoint(
          NO_ACTIVE_BATCHING_STREAMS_SUBMISSION_CLOSED,
          endpoint_));
        callback(item);
      }
      return;
    }

    auto pending_request = std::make_shared<BatchingQueue::PendingRequest>();
    pending_request->full_method = full_method;
    pending_request->payload = std::move(payload);
    pending_request->callback = std::move(callback);
    pending_request->stats_owner = this;

    const auto now = Generics::Time::get_time_of_day();
    auto enqueue_result = batching_queue_->enqueue(
      std::move(pending_request),
      now);

    if (enqueue_result.was_empty_before_push &&
      !enqueue_result.queue_empty_after_enqueue &&
      options_.max_batch_delay.has_value())
    {
      assert(enqueue_result.oldest_enqueue_time.has_value());
      schedule_timing_coalesce_(*enqueue_result.oldest_enqueue_time);
    }

    if (!enqueue_result.ready_batch.empty())
    {
      if (acquire_batch_inflight_(
            enqueue_result.ready_batch,
            options_.error_on_inflight_reaching))
      {
        process_batch_(
          std::move(enqueue_result.ready_batch),
          now,
          NO_ACTIVE_BATCHING_STREAMS_FORCED_SEND);
      }
    }
  }

  AsyncBatchingClientBase::StreamHolderPtr
  AsyncBatchingClientBase::make_stream_()
  {
    auto stream_holder = std::make_shared<StreamHolder>();
    std::weak_ptr<StreamHolder> weak_stream_holder = stream_holder;
    stream_holder->stream = std::make_shared<AdServer::Grpc::BatchingStreamBase>(
      channel_,
      endpoint_,
      grpc_executor_,
      batching_queue_,
      next_queue_index_.fetch_add(1, std::memory_order_relaxed),
      this,
      [this](auto* stream) { //< ready callback.
        handle_stream_ready_(stream);
      },
      [this](auto* stream) noexcept { //< close callback.
        handle_stream_closed_(stream);
      },
      [this, weak_stream_holder](auto* stream) noexcept { //< drain callback.
        if (auto stream_holder = weak_stream_holder.lock())
        {
          handle_stream_drained_(stream, stream_holder);
        }
      },
      options_);
    return stream_holder;
  }

  void
  AsyncBatchingClientBase::process_batch_(
    BatchingStreamBase::PendingBatch&& batch,
    const Generics::Time& now,
    const char* no_active_streams_context) noexcept
  {
    assert(!batch.empty());

    StreamHolderPtr stream_holder;
    bool start_connect = false;
    bool fail_batch = false;
    std::vector<BatchingStreamBase::PendingBatch> failed_batches;
    {
      std::lock_guard<std::mutex> lock(streams_lock_);
      if (!active())
      {
        fail_batch = true;
      }
      else if (!available_streams_.empty())
      {
        stream_holder = std::move(available_streams_.front());
        available_streams_.pop_front();
        stream_holder->last_use = now;
      }
      else
      {
        if (last_connect_failure_time_ &&
          now - *last_connect_failure_time_ < options_.reconnect_period)
        {
          fail_batch = true;
        }
        else
        {
          pending_batches_.emplace_back(std::move(batch));
          start_connect =
            maybe_start_connect_for_pending_(failed_batches, now);
        }
      }
    }

    if (fail_batch)
    {
      finish_batch_with_error_(
        batch,
        grpc::StatusCode::UNAVAILABLE,
        no_active_streams_context,
        "process_batch");
      return;
    }

    if (!failed_batches.empty())
    {
      finish_batches_with_error_(
        failed_batches,
        grpc::StatusCode::UNAVAILABLE,
        NO_ACTIVE_BATCHING_STREAMS_AFTER_CONNECT_TRY,
        "after_connect_try");
    }

    if (stream_holder)
    {
      dispatch_batch_(std::move(batch), stream_holder);
    }

    if (start_connect)
    {
      start_connect_();
    }
  }

  bool
  AsyncBatchingClientBase::acquire_batch_inflight_(
    BatchingStreamBase::PendingBatch& batch,
    bool allow_limit_error) noexcept
  {
    assert(!batch.empty());

    if (!options_.max_inflight)
    {
      return true;
    }

    if (allow_limit_error)
    {
      if (!inflight_limiter_.try_acquire(batch.size()))
      {
        finish_batch_with_error_(
          batch,
          grpc::StatusCode::RESOURCE_EXHAUSTED,
          "inflight limit reached",
          "inflight_limit");
        return false;
      }
    }
    else
    {
      inflight_limiter_.acquire(batch.size());
    }

    for (auto& request : batch)
    {
      if (request.request)
      {
        request.request->inflight_limiter = &inflight_limiter_;
      }
    }

    return true;
  }

  bool
  AsyncBatchingClientBase::maybe_start_connect_for_pending_(
    std::vector<BatchingStreamBase::PendingBatch>& failed_batches,
    const Generics::Time& now) noexcept
  {
    if (!active() ||
      pending_batches_.empty() ||
      !available_streams_.empty() ||
      connecting_)
    {
      return false;
    }

    if (last_connect_failure_time_ &&
      now - *last_connect_failure_time_ < options_.reconnect_period)
    {
      failed_batches.reserve(
        failed_batches.size() + pending_batches_.size());
      while (!pending_batches_.empty())
      {
        failed_batches.emplace_back(std::move(pending_batches_.front()));
        pending_batches_.pop_front();
      }
      return false;
    }

    const auto streams_count =
      up_streams_.load(std::memory_order_acquire);
    if (streams_count >= max_streams_)
    {
      return false;
    }

    connecting_ = true;
    const auto new_streams_count =
      up_streams_.fetch_add(1, std::memory_order_acq_rel) + 1;
    update_max_streams_(new_streams_count);

    return true;
  }

  void
  AsyncBatchingClientBase::finish_batch_with_error_(
    BatchingStreamBase::PendingBatch& batch,
    grpc::StatusCode status_code,
    const char* status_message,
    const char* source) noexcept
  {
    set_last_error(
      endpoint_,
      status_code,
      status_message ? status_message : "",
      source);
    finish_batch_with_error(batch, status_code, status_message, endpoint_);
  }

  void
  AsyncBatchingClientBase::finish_batches_with_error_(
    std::vector<BatchingStreamBase::PendingBatch>& batches,
    grpc::StatusCode status_code,
    const char* status_message,
    const char* source) noexcept
  {
    if (batches.empty())
    {
      return;
    }

    set_last_error(
      endpoint_,
      status_code,
      status_message ? status_message : "",
      source);
    finish_batches_with_error(batches, status_code, status_message, endpoint_);
  }

  void
  AsyncBatchingClientBase::start_connect_() noexcept
  {
    StreamHolderPtr stream_holder;
    BatchingStreamBase* stream = nullptr;
    bool stream_registered = false;
    try
    {
      stream_holder = make_stream_();
      stream = stream_holder->stream.get();
      if (!active())
      {
        throw std::runtime_error("inactive");
      }

      {
        std::lock_guard<std::mutex> streams_lock(streams_registry_lock_);
        streams_.emplace(stream, stream_holder);
      }

      stream_registered = true;

      {
        std::lock_guard<std::mutex> lock(streams_lock_);
        connecting_stream_ = stream_holder;
      }

      stream_holder->stream->activate_object();
    }
    catch (const std::exception& ex)
    {
      set_last_error(
        endpoint_,
        grpc::StatusCode::UNAVAILABLE,
        ex.what(),
        "connect_exception");

      if (stream_registered)
      {
        std::lock_guard<std::mutex> streams_lock(streams_registry_lock_);
        streams_.erase(stream);
      }

      std::vector<BatchingStreamBase::PendingBatch> failed_batches;
      const auto failure_time = Generics::Time::get_time_of_day();

      {
        std::lock_guard<std::mutex> lock(streams_lock_);
        connecting_ = false;
        connecting_stream_.reset();
        last_connect_failure_time_ = failure_time;
        up_streams_.fetch_sub(1, std::memory_order_acq_rel);
        while (!pending_batches_.empty())
        {
          failed_batches.emplace_back(std::move(pending_batches_.front()));
          pending_batches_.pop_front();
        }
      }

      finish_batches_with_error_(
        failed_batches,
        grpc::StatusCode::UNAVAILABLE,
        NO_ACTIVE_BATCHING_STREAMS_CONNECT_EXCEPTION,
        "connect_exception");
    }
    catch (...)
    {
      set_last_error(
        endpoint_,
        grpc::StatusCode::UNAVAILABLE,
        "unknown connect exception",
        "connect_exception");

      if (stream_registered)
      {
        std::lock_guard<std::mutex> streams_lock(streams_registry_lock_);
        streams_.erase(stream);
      }

      std::vector<BatchingStreamBase::PendingBatch> failed_batches;
      const auto failure_time = Generics::Time::get_time_of_day();

      {
        std::lock_guard<std::mutex> lock(streams_lock_);
        connecting_ = false;
        connecting_stream_.reset();
        last_connect_failure_time_ = failure_time;
        up_streams_.fetch_sub(1, std::memory_order_acq_rel);
        while (!pending_batches_.empty())
        {
          failed_batches.emplace_back(std::move(pending_batches_.front()));
          pending_batches_.pop_front();
        }
      }

      finish_batches_with_error_(
        failed_batches,
        grpc::StatusCode::UNAVAILABLE,
        NO_ACTIVE_BATCHING_STREAMS_CONNECT_EXCEPTION,
        "connect_exception");
    }
  }

  void
  AsyncBatchingClientBase::release_or_dispatch_(
    const StreamHolderPtr& stream_holder) noexcept
  {
    assert(stream_holder);
    assert(stream_holder->stream);

    BatchingStreamBase::PendingBatch batch;
    bool start_connect = false;
    std::vector<BatchingStreamBase::PendingBatch> failed_batches;
    const auto now = Generics::Time::get_time_of_day();

    {
      std::lock_guard<std::mutex> lock(streams_lock_);
      if (active() && stream_holder->stream->available())
      {
        stream_holder->last_use = now;

        if (!pending_batches_.empty())
        {
          batch = std::move(pending_batches_.front());
          pending_batches_.pop_front();
          start_connect =
            maybe_start_connect_for_pending_(failed_batches, now);
        }
        else
        {
          available_streams_.emplace_front(stream_holder);
        }
      }
    }

    finish_batches_with_error_(
      failed_batches,
      grpc::StatusCode::UNAVAILABLE,
      NO_ACTIVE_BATCHING_STREAMS_RELEASE_OR_DISPATCH,
      "release_or_dispatch");

    if (!batch.empty())
    {
      dispatch_batch_(std::move(batch), stream_holder);
    }

    if (start_connect)
    {
      start_connect_();
    }
  }

  bool
  AsyncBatchingClientBase::dispatch_batch_(
    BatchingStreamBase::PendingBatch&& batch,
    const StreamHolderPtr& stream_holder) noexcept
  {
    assert(stream_holder);
    assert(stream_holder->stream);

    BatchingStreamBase::PendingBatch failed_batch;
    bool write_started = false;
    try
    {
      write_started = stream_holder->stream->try_start_write(
        std::move(batch),
        false,
        &failed_batch);
    }
    catch (...)
    {
      failed_batch = std::move(batch);
    }

    if (write_started)
    {
      return true;
    }

    if (!failed_batch.empty())
    {
      finish_batch_with_error_(
        failed_batch,
        grpc::StatusCode::UNAVAILABLE,
        "stream write failed",
        "stream_write");
    }

    try
    {
      stream_holder->stream->deactivate_object();
    }
    catch (...)
    {}

    return false;
  }

  void
  AsyncBatchingClientBase::schedule_timing_coalesce_() noexcept
  {
    if (!active() || !options_.max_batch_delay.has_value())
    {
      return;
    }

    const auto oldest_enqueue_time = batching_queue_->oldest_enqueue_time();
    if (!oldest_enqueue_time)
    {
      return;
    }
    schedule_timing_coalesce_(*oldest_enqueue_time);
  }

  void
  AsyncBatchingClientBase::schedule_timing_coalesce_(
    const Generics::Time& oldest_enqueue_time) noexcept
  {
    if (!active() || !options_.max_batch_delay.has_value())
    {
      return;
    }

    const auto deadline = oldest_enqueue_time + *options_.max_batch_delay;

    {
      std::shared_lock<std::shared_mutex> lock(coalesce_timer_lock_);
      if (coalesce_timer_deadline_.has_value() &&
        *coalesce_timer_deadline_ <= deadline)
      {
        return;
      }
    }

    {
      std::unique_lock<std::shared_mutex> lock(coalesce_timer_lock_);
      if (coalesce_timer_deadline_.has_value() &&
        *coalesce_timer_deadline_ <= deadline)
      {
        return;
      }
      coalesce_timer_deadline_ = deadline;
    }

    const auto gate = timing_coalesce_gate_;
    assert(gate);

    try
    {
      coalesce_runner_->schedule(
        deadline - Generics::Time::get_time_of_day(),
        [this, gate, deadline]()
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
      std::unique_lock<std::shared_mutex> lock(coalesce_timer_lock_);
      if (coalesce_timer_deadline_.has_value() &&
        *coalesce_timer_deadline_ == deadline)
      {
        coalesce_timer_deadline_.reset();
      }
    }
  }

  void
  AsyncBatchingClientBase::run_timing_coalesce_(Generics::Time deadline)
    noexcept
  {
    try
    {
      {
        std::unique_lock<std::shared_mutex> lock(coalesce_timer_lock_);
        if (!coalesce_timer_deadline_.has_value() ||
          *coalesce_timer_deadline_ != deadline)
        {
          return;
        }
        coalesce_timer_deadline_.reset();
      }
      coalesce_timed_batch_(Generics::Time::get_time_of_day());
      schedule_timing_coalesce_();
    }
    catch (...)
    {
    }
  }

  void
  AsyncBatchingClientBase::schedule_stream_shrink_() noexcept
  {
    if (!active() || options_.stream_shrink_period == Generics::Time::ZERO)
    {
      return;
    }

    const auto gate = stream_shrink_gate_;
    assert(gate);
    try
    {
      coalesce_runner_->schedule(
        options_.stream_shrink_period,
        [this, gate]()
        {
          auto guard = gate->enter();
          if (!guard)
          {
            return;
          }
          shrink_idle_streams_();
          schedule_stream_shrink_();
        });
    }
    catch (...)
    {}
  }

  void
  AsyncBatchingClientBase::shrink_idle_streams_() noexcept
  {
    clear_deferred_streams_();

    if (options_.stream_idle_timeout == Generics::Time::ZERO)
    {
      return;
    }

    const auto now = Generics::Time::get_time_of_day();
    std::vector<StreamHolderPtr> streams_to_close;
    {
      std::lock_guard<std::mutex> lock(streams_lock_);
      while (available_streams_.size() > 1 &&
        up_streams_.load(std::memory_order_acquire) >
          streams_to_close.size() + 1)
      {
        auto stream_holder = available_streams_.back();
        if (!stream_holder ||
          now - stream_holder->last_use <= options_.stream_idle_timeout)
        {
          break;
        }

        available_streams_.pop_back();
        streams_to_close.emplace_back(std::move(stream_holder));
      }
    }

    for (auto& stream_holder : streams_to_close)
    {
      try
      {
        if (stream_holder && stream_holder->stream)
        {
          stream_holder->stream->deactivate_object();
        }
      }
      catch (...)
      {}
    }
  }

  bool
  AsyncBatchingClientBase::can_process_timed_batch_() const noexcept
  {
    std::lock_guard<std::mutex> lock(streams_lock_);
    if (!active())
    {
      return false;
    }

    if (!available_streams_.empty())
    {
      return true;
    }

    if (!pending_batches_.empty() || connecting_)
    {
      return false;
    }

    return up_streams_.load(std::memory_order_acquire) < max_streams_;
  }

  bool
  AsyncBatchingClientBase::coalesce_timed_batch_(const Generics::Time& now)
    noexcept
  {
    BatchingStreamBase::PendingBatch pending_batch;
    if (!options_.max_batch_delay.has_value())
    {
      return false;
    }

    if (!can_process_timed_batch_())
    {
      return false;
    }

    const bool has_batch = batching_queue_->try_pop_due_batch(
      pending_batch,
      now,
      *options_.max_batch_delay);
    if (!has_batch)
    {
      return false;
    }

    add_timing_coalesce_stats(pending_batch.size());
    if (options_.error_on_inflight_reaching &&
      !acquire_batch_inflight_(pending_batch, true))
    {
      return true;
    }

    process_batch_(
      std::move(pending_batch),
      now,
      NO_ACTIVE_BATCHING_STREAMS_TIMED_SEND);
    return true;
  }

  void
  AsyncBatchingClientBase::handle_stream_ready_(BatchingStreamBase* stream)
  {
    StreamHolderPtr stream_holder;
    bool connect_success = false;
    {
      std::lock_guard<std::mutex> registry_lock(streams_registry_lock_);
      auto it = streams_.find(stream);
      if (it != streams_.end())
      {
        stream_holder = it->second;
      }
    }

    if (!stream_holder)
    {
      return;
    }

    {
      std::lock_guard<std::mutex> lock(streams_lock_);
      if (connecting_stream_ == stream_holder)
      {
        connecting_ = false;
        connecting_stream_.reset();
        connect_success = true;
      }
    }

    (void)connect_success;
    release_or_dispatch_(stream_holder);
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
        stream_holder = it->second;
        streams_.erase(it);
        draining_streams_.emplace_back(stream_holder);
        stream_removed = true;
      }
    }

    if (!stream_removed)
    {
      return;
    }

    std::vector<BatchingStreamBase::PendingBatch> failed_batches;
    bool start_connect = false;
    const auto failure_time = Generics::Time::get_time_of_day();
    {
      std::lock_guard<std::mutex> lock(streams_lock_);
      available_streams_.erase(
        std::remove(
          available_streams_.begin(),
          available_streams_.end(),
          stream_holder),
        available_streams_.end());
      if (connecting_stream_ == stream_holder)
      {
        connecting_ = false;
        connecting_stream_.reset();
        last_connect_failure_time_ = failure_time;
        if (available_streams_.empty())
        {
          while (!pending_batches_.empty())
          {
            failed_batches.emplace_back(std::move(pending_batches_.front()));
            pending_batches_.pop_front();
          }
        }
      }
      up_streams_.fetch_sub(1, std::memory_order_acq_rel);
      start_connect =
        maybe_start_connect_for_pending_(failed_batches, failure_time);
    }

    finish_batches_with_error_(
      failed_batches,
      grpc::StatusCode::UNAVAILABLE,
      NO_ACTIVE_BATCHING_STREAMS_AFTER_CONNECT_TRY,
      "after_connect_try");
    if (start_connect)
    {
      start_connect_();
    }
  }

  void
  AsyncBatchingClientBase::handle_stream_drained_(
    BatchingStreamBase* stream,
    const StreamHolderPtr& stream_holder) noexcept
  {
    assert(stream);

    StreamHolderPtr holder;
    {
      std::lock_guard<std::mutex> registry_lock(streams_registry_lock_);
      auto it = std::find(
        draining_streams_.begin(),
        draining_streams_.end(),
        stream_holder);
      if (it == draining_streams_.end())
      {
        return;
      }

      holder = std::move(*it);
      draining_streams_.erase(it);
    }

    if (holder && holder->stream)
    {
      try
      {
        holder->stream->deactivate_object();
        holder->stream->wait_object();
      }
      catch (...)
      {}

      std::lock_guard<std::mutex> registry_lock(streams_registry_lock_);
      deferred_streams_.emplace_back(holder->stream);
    }
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
      connecting_ = false;
      connecting_stream_.reset();
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
      pending_batches_.clear();
      connecting_ = false;
      connecting_stream_.reset();
      last_connect_failure_time_.reset();
    }
    std::unordered_map<BatchingStreamBase*, StreamHolderPtr> streams;
    {
      std::lock_guard<std::mutex> lock(streams_registry_lock_);
      streams.swap(streams_);
      draining_streams.swap(draining_streams_);
      deferred_streams.swap(deferred_streams_);
    }
    up_streams_.store(0, std::memory_order_release);
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
