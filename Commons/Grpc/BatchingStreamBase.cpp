#include <Commons/Grpc/BatchingStreamBase.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstring>
#include <cstdint>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

#include <grpcpp/client_context.h>
#include <grpcpp/support/async_stream.h>
#include <google/protobuf/arena.h>

#include <Generics/MonoAllocator.hpp>

#include <Commons/ActivityGate.hpp>
#include <Commons/Grpc/BatchingQueue.hpp>
#include <Commons/Grpc/GrpcExecutor.hpp>

namespace AdServer::Grpc
{
  namespace
  {
    constexpr const char* default_batch_stream_full_method =
      "/adserver.grpc.BatchTransport/stream_batches";

    Generics::Time
    duration_time(const Generics::Time& from, const Generics::Time& to)
    {
      const auto result = to - from;
      return result < Generics::Time::ZERO ? Generics::Time::ZERO : result;
    }

  } // namespace

  struct BatchingStreamBase::Impl
    : std::enable_shared_from_this<BatchingStreamBase::Impl>
  {
    using BatchResponseItem = BatchingStreamBase::BatchResponseItem;
    using BatchRequest = BatchingStreamBase::BatchRequest;
    using BatchResponse = BatchingStreamBase::BatchResponse;
    using BatchTransport = BatchingStreamBase::BatchTransport;
    using PendingRequest = BatchingStreamBase::PendingRequest;
    using PendingBatch = BatchingStreamBase::PendingBatch;
    using ReadyCallback = BatchingStreamBase::ReadyCallback;
    using ClosedCallback = BatchingStreamBase::ClosedCallback;
    using DrainedCallback = BatchingStreamBase::DrainedCallback;

    Impl(
      BatchingStreamBase& owner,
      std::shared_ptr<grpc::Channel> channel,
      std::string endpoint,
      std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor,
      std::shared_ptr<AdServer::Grpc::BatchingQueue> batching_queue,
      unsigned int queue_index,
      ReadyCallback ready_callback,
      ClosedCallback closed_callback,
      DrainedCallback drained_callback,
      AdServer::Grpc::BatchingOptions options);

    ~Impl();

    bool available() noexcept;
    std::uint64_t inflight_items() noexcept;
    bool try_start_write(
      PendingBatch&& pending_batch,
      bool measure_consumer_stream_write,
      PendingBatch* failed_batch);
    void activate_object_();
    void deactivate_object_();
    void wait_object_();

  private:
    enum class StreamState
    {
      Starting,
      Open,
      Closing,
      Broken,
      Finishing,
      Finished
    };

    enum class ReadState
    {
      Idle,
      Reading
    };

    enum class WriteState
    {
      Idle,
      Writing,
      WritesDoneInFlight,
      WritesDoneCompleted
    };

    enum class StreamAction
    {
      None,
      StartRead,
      StartWritesDone,
      StartFinish
    };

    struct StreamTransition
    {
      StreamAction action = StreamAction::None;
      bool ready = false;
    };

    struct CompletionTag;
    struct StartTag;
    struct ReadTag;
    struct WriteTag;
    struct WritesDoneTag;
    struct FinishTag;

    class DetachedBatchStorage final
    {
    public:
      struct BatchGuard
      {
        BatchGuard(std::uint64_t id_val, PendingBatch batch_val)
          : id(id_val),
            batch(std::move(batch_val))
        {}

        PendingBatch claim_payload()
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
        PendingBatch batch;
      };

      using BatchGuardPtr = std::shared_ptr<BatchGuard>;

      struct BatchRecord
      {
        std::uint64_t batch_id = 0;
        Generics::Time write_time;
        BatchGuardPtr guard;
        std::vector<std::uint64_t> request_ids;
      };

      BatchGuardPtr register_batch(
        PendingBatch& batch,
        std::uint64_t batch_id,
        Generics::Time write_time,
        std::vector<std::uint64_t> request_ids);
      std::optional<BatchRecord> take_batch(std::uint64_t batch_id);
      void close();
      std::vector<PendingBatch> drain_all();
      std::uint64_t items_count() const;

    private:
      mutable std::mutex lock_;
      bool closed_ = false;
      std::unordered_map<std::uint64_t, BatchGuardPtr> batches_;
      std::unordered_map<std::uint64_t, BatchRecord> batch_records_;
      std::atomic<std::uint64_t> next_id_{1};
    };

    bool active() const noexcept;
    bool start_write_(
      PendingBatch&& pending_batch,
      bool measure_consumer_stream_write,
      std::vector<PendingRequest>* failed_batch);
    bool start_stream_();
    StreamTransition plan_start_completion_i_(bool ok);
    StreamAction plan_start_read_i_();
    StreamAction plan_shutdown_i_();
    void execute_stream_action_(StreamAction action) noexcept;
    void execute_stream_action_cq_(StreamAction action) noexcept;
    void start_read_cq_() noexcept;
    void start_writes_done_() noexcept;
    void start_finish_() noexcept;
    void process_read_completion_cq_(
      bool ok,
      std::unique_ptr<google::protobuf::Arena> response_arena,
      BatchResponse* response);
    void process_write_completion_cq_(
      bool ok,
      std::uint64_t batch_id);
    void process_start_completion_cq_(bool ok);
    void process_writes_done_completion_cq_(bool ok);
    void process_finish_completion_cq_(bool ok);
    void finish_with_error_(
      grpc::StatusCode status_code,
      const char* status_message);
    void finish_requests_with_error_(
      std::vector<PendingRequest>& requests,
      grpc::StatusCode status_code,
      const char* status_message);
    void fail_inflight_with_error_(
      grpc::StatusCode status_code,
      const char* status_message);
    void record_last_error_(
      grpc::StatusCode status_code,
      const char* status_message,
      const char* source) noexcept;
    bool mark_grpc_queue_shutdown_i_() noexcept;
    bool mark_shutdown_complete_i_() noexcept;
    void deactivate_shutdown_gates_() noexcept;
    void notify_closed_if_needed_() noexcept;
    bool complete_drain_if_ready_() noexcept;
    void release_grpc_resources_i_() noexcept;
    bool accepts_requests_i_() const noexcept;

    void add_write_stats(std::uint64_t batches, std::uint64_t items) noexcept;
    void add_read_stats(std::uint64_t batches, std::uint64_t items) noexcept;
    void add_queue_wait_stats(std::uint64_t wait_us) noexcept;
    void add_queue_timeout_stats() noexcept;
    void add_response_wait_stats(
      std::uint64_t wait_us,
      std::uint64_t count = 1) noexcept;
    void add_consumer_stream_write_stats(std::uint64_t wait_us) noexcept;

  private:
    BatchingStreamBase& owner_;
    const std::string endpoint_;
    const AdServer::Grpc::BatchingOptions options_;
    const std::string batch_stream_full_method_;
    std::shared_ptr<AdServer::Grpc::BatchingQueue> batching_queue_;
    ReadyCallback ready_callback_;
    ClosedCallback closed_callback_;
    DrainedCallback drained_callback_;
    bool closed_notified_ = false;
    bool drained_notified_ = false;

    AdServer::Commons::ActivityGate completion_tags_gate_;
    DetachedBatchStorage detached_batch_storage_;
    std::atomic<std::uint64_t> next_request_id_{1};

    std::mutex state_lock_;

    ReadState read_state_ = ReadState::Idle;
    WriteState write_state_ = WriteState::Idle;
    std::atomic<StreamState> stream_state_{StreamState::Starting};
    std::unique_ptr<grpc::ClientContext> stream_context_;
    std::unique_ptr<BatchTransport> batch_stub_;
    grpc::ClientAsyncReaderWriter<BatchRequest, BatchResponse>* stream_ = nullptr;
    grpc::Status finish_status_;
    google::protobuf::Arena write_arena_;

    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor_;
    const unsigned int queue_index_;
    std::shared_ptr<AdServer::Grpc::GrpcExecutor::CQ> grpc_queue_;
  };

  BatchingStreamBase::Impl::DetachedBatchStorage::BatchGuardPtr
  BatchingStreamBase::Impl::DetachedBatchStorage::register_batch(
    PendingBatch& batch,
    std::uint64_t batch_id,
    Generics::Time write_time,
    std::vector<std::uint64_t> request_ids)
  {
    if (batch.empty())
    {
      return {};
    }

    std::lock_guard<std::mutex> lock(lock_);
    if (closed_)
    {
      return {};
    }

    const auto id = next_id_.fetch_add(1, std::memory_order_relaxed);
    auto batch_guard = std::make_shared<BatchGuard>(id, std::move(batch));
    batches_.emplace(id, batch_guard);
    batch_records_.emplace(batch_id, BatchRecord{
      batch_id,
      write_time,
      batch_guard,
      std::move(request_ids)});

    return batch_guard;
  }

  std::optional<BatchingStreamBase::Impl::DetachedBatchStorage::BatchRecord>
  BatchingStreamBase::Impl::DetachedBatchStorage::take_batch(
    std::uint64_t batch_id)
  {
    std::lock_guard<std::mutex> lock(lock_);
    auto it = batch_records_.find(batch_id);
    if (it == batch_records_.end())
    {
      return std::nullopt;
    }

    auto batch_record = std::move(it->second);
    batch_records_.erase(it);
    if (batch_record.guard)
    {
      batches_.erase(batch_record.guard->id);
    }

    return batch_record;
  }

  void
  BatchingStreamBase::Impl::DetachedBatchStorage::close()
  {
    std::lock_guard<std::mutex> lock(lock_);
    closed_ = true;
  }

  std::vector<BatchingStreamBase::PendingBatch>
  BatchingStreamBase::Impl::DetachedBatchStorage::drain_all()
  {
    decltype(batches_) batches;
    {
      std::lock_guard<std::mutex> lock(lock_);
      batches.swap(batches_);
      batch_records_.clear();
    }

    std::vector<PendingBatch> result;
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

  std::uint64_t
  BatchingStreamBase::Impl::DetachedBatchStorage::items_count() const
  {
    std::lock_guard<std::mutex> lock(lock_);
    std::uint64_t result = 0;
    for (const auto& [_, batch_record] : batch_records_)
    {
      result += batch_record.request_ids.size();
    }
    return result;
  }

  struct BatchingStreamBase::Impl::CompletionTag
    : AdServer::Grpc::GrpcCompletionEvent
  {
    explicit CompletionTag(std::shared_ptr<Impl> impl)
      : completion_tag_guard_(impl->completion_tags_gate_.enter()),
        impl_(std::move(impl))
    {}

    bool
    active() const noexcept
    {
      return static_cast<bool>(completion_tag_guard_);
    }

    ~CompletionTag() override
    {
      if (auto impl = impl_.lock())
      {
        completion_tag_guard_.reset();
        impl->complete_drain_if_ready_();
      }
    }

  protected:
    template<typename Functor>
    void with_impl_(Functor&& functor)
    {
      if (auto impl = impl_.lock())
      {
        functor(*impl);
      }
    }

  private:
    AdServer::Commons::ActivityGate::Guard completion_tag_guard_;
    std::weak_ptr<Impl> impl_;
  };

  struct BatchingStreamBase::Impl::StartTag final : CompletionTag
  {
    explicit StartTag(std::shared_ptr<Impl> impl)
      : CompletionTag(std::move(impl))
    {}

    void
    proceed(bool ok) override
    {
      with_impl_([ok](auto& impl) {
        impl.process_start_completion_cq_(ok);
      });
    }
  };

  struct BatchingStreamBase::Impl::ReadTag final : CompletionTag
  {
    ReadTag(
      std::shared_ptr<Impl> impl,
      std::unique_ptr<google::protobuf::Arena> response_arena,
      BatchResponse* response)
      : CompletionTag(std::move(impl)),
        response_arena_(std::move(response_arena)),
        response_(response)
    {}

    void
    proceed(bool ok) override
    {
      with_impl_([this, ok](auto& impl) {
        impl.process_read_completion_cq_(
          ok,
          std::move(response_arena_),
          response_);
      });
    }

  private:
    std::unique_ptr<google::protobuf::Arena> response_arena_;
    BatchResponse* response_;
  };

  struct BatchingStreamBase::Impl::WriteTag final : CompletionTag
  {
    WriteTag(
      std::shared_ptr<Impl> impl,
      std::uint64_t batch_id)
      : CompletionTag(std::move(impl)),
        batch_id_(batch_id)
    {}

    void
    proceed(bool ok) override
    {
      with_impl_([this, ok](auto& impl) {
        impl.process_write_completion_cq_(ok, batch_id_);
      });
    }

  private:
    std::uint64_t batch_id_;
  };

  struct BatchingStreamBase::Impl::WritesDoneTag final : CompletionTag
  {
    explicit WritesDoneTag(std::shared_ptr<Impl> impl)
      : CompletionTag(std::move(impl))
    {}

    void
    proceed(bool ok) override
    {
      with_impl_([ok](auto& impl) {
        impl.process_writes_done_completion_cq_(ok);
      });
    }
  };

  struct BatchingStreamBase::Impl::FinishTag final : CompletionTag
  {
    explicit FinishTag(std::shared_ptr<Impl> impl)
      : CompletionTag(std::move(impl))
    {}

    void
    proceed(bool ok) override
    {
      with_impl_([ok](auto& impl) {
        impl.process_finish_completion_cq_(ok);
      });
    }
  };

  BatchingStreamBase::Impl::Impl(
    BatchingStreamBase& owner,
    std::shared_ptr<grpc::Channel> channel,
    std::string endpoint,
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor,
    std::shared_ptr<AdServer::Grpc::BatchingQueue> batching_queue,
    unsigned int queue_index,
    ReadyCallback ready_callback,
    ClosedCallback closed_callback,
    DrainedCallback drained_callback,
    AdServer::Grpc::BatchingOptions options)
    : owner_(owner),
      endpoint_(std::move(endpoint)),
      options_(options),
      batch_stream_full_method_(
        options_.batch_stream_full_method.empty() ?
          default_batch_stream_full_method :
          options_.batch_stream_full_method),
      batching_queue_(std::move(batching_queue)),
      ready_callback_(std::move(ready_callback)),
      closed_callback_(std::move(closed_callback)),
      drained_callback_(std::move(drained_callback)),
      grpc_executor_(std::move(grpc_executor)),
      queue_index_(queue_index)
  {
    completion_tags_gate_.activate_object();
    if (!grpc_executor_)
    {
      throw std::runtime_error("BatchingStreamBase requires GrpcExecutor");
    }
    if (!batching_queue_)
    {
      throw std::runtime_error("BatchingStreamBase requires BatchingQueue");
    }
    if (!channel)
    {
      throw std::runtime_error("BatchingStreamBase requires grpc Channel");
    }

    batch_stub_ = std::make_unique<BatchTransport>(std::move(channel));
  }

  BatchingStreamBase::Impl::~Impl()
  {
    completion_tags_gate_.deactivate_object();
    completion_tags_gate_.wait_object();
  }

  BatchingStreamBase::BatchingStreamBase(
    std::shared_ptr<grpc::Channel> channel,
    std::string endpoint,
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor,
    std::shared_ptr<AdServer::Grpc::BatchingQueue> batching_queue,
    unsigned int queue_index,
    AdServer::Grpc::Client* stats_owner,
    ReadyCallback ready_callback,
    ClosedCallback closed_callback,
    DrainedCallback drained_callback,
    AdServer::Grpc::BatchingOptions options)
    : AdServer::Grpc::Client(stats_owner)
  {
    impl_ = std::make_shared<Impl>(
      *this,
      std::move(channel),
      std::move(endpoint),
      std::move(grpc_executor),
      std::move(batching_queue),
      queue_index,
      std::move(ready_callback),
      std::move(closed_callback),
      std::move(drained_callback),
      std::move(options));
  }

  BatchingStreamBase::~BatchingStreamBase() = default;

  bool
  BatchingStreamBase::available() noexcept
  {
    return impl_->available();
  }

  std::uint64_t
  BatchingStreamBase::inflight_items() noexcept
  {
    return impl_->inflight_items();
  }

  std::string
  BatchingStreamBase::last_error_message() const noexcept
  {
    const auto current_stats = stats();
    return current_stats.last_error.has_value() ?
      current_stats.last_error->message :
      std::string();
  }

  bool
  BatchingStreamBase::try_start_write(
    PendingBatch&& pending_batch,
    bool measure_consumer_stream_write,
    PendingBatch* failed_batch)
  {
    return impl_->try_start_write(
      std::move(pending_batch),
      measure_consumer_stream_write,
      failed_batch);
  }

  void
  BatchingStreamBase::activate_object_()
  {
    impl_->activate_object_();
  }

  void
  BatchingStreamBase::deactivate_object_()
  {
    impl_->deactivate_object_();
  }

  void
  BatchingStreamBase::wait_object_()
  {
    impl_->wait_object_();
  }

  bool
  BatchingStreamBase::Impl::active() const noexcept
  {
    return owner_.active();
  }

  void
  BatchingStreamBase::Impl::add_write_stats(
    std::uint64_t batches,
    std::uint64_t items) noexcept
  {
    owner_.add_write_stats(batches, items);
  }

  void
  BatchingStreamBase::Impl::add_read_stats(
    std::uint64_t batches,
    std::uint64_t items) noexcept
  {
    owner_.add_read_stats(batches, items);
  }

  void
  BatchingStreamBase::Impl::add_queue_wait_stats(
    std::uint64_t wait_us) noexcept
  {
    owner_.add_queue_wait_stats(wait_us);
  }

  void
  BatchingStreamBase::Impl::add_queue_timeout_stats() noexcept
  {
    owner_.add_queue_timeout_stats();
  }

  void
  BatchingStreamBase::Impl::add_response_wait_stats(
    std::uint64_t wait_us,
    std::uint64_t count) noexcept
  {
    owner_.add_response_wait_stats(wait_us, count);
  }

  void
  BatchingStreamBase::Impl::add_consumer_stream_write_stats(
    std::uint64_t wait_us) noexcept
  {
    owner_.add_consumer_stream_write_stats(wait_us);
  }

  bool
  BatchingStreamBase::Impl::available() noexcept
  {
    return active() && accepts_requests_i_();
  }

  std::uint64_t
  BatchingStreamBase::Impl::inflight_items() noexcept
  {
    return detached_batch_storage_.items_count();
  }

  void
  BatchingStreamBase::Impl::activate_object_()
  {
    start_stream_();
  }

  void
  BatchingStreamBase::Impl::deactivate_object_()
  {
    StreamAction action = StreamAction::None;

    {
      std::lock_guard<std::mutex> lock(state_lock_);
      const auto stream_state = stream_state_.load();
      if (stream_state == StreamState::Starting)
      {
        stream_state_.store(StreamState::Closing);
      }
      else if (stream_state == StreamState::Open)
      {
        stream_state_.store(StreamState::Closing);
        action = plan_shutdown_i_();
      }
    }

    execute_stream_action_(action);

    detached_batch_storage_.close();
    fail_inflight_with_error_(grpc::StatusCode::UNAVAILABLE, "inactive");

    if (stream_context_)
    {
      stream_context_->TryCancel();
    }
  }

  void
  BatchingStreamBase::Impl::wait_object_()
  {
    completion_tags_gate_.deactivate_object();
    completion_tags_gate_.wait_object();

    {
      std::lock_guard<std::mutex> lock(state_lock_);
      if (stream_state_.load() != StreamState::Finished)
      {
        mark_shutdown_complete_i_();
      }
    }
    release_grpc_resources_i_();

    notify_closed_if_needed_();
    complete_drain_if_ready_();
  }

  bool
  BatchingStreamBase::Impl::start_stream_()
  {
    auto stream_context = std::make_unique<grpc::ClientContext>();
    auto grpc_queue = grpc_executor_->queue(queue_index_);

    {
      std::lock_guard<std::mutex> lock(state_lock_);
      if (!active())
      {
        return false;
      }

      stream_context_ = std::move(stream_context);
      grpc_queue_ = std::move(grpc_queue);
      finish_status_ = grpc::Status();
      read_state_ = ReadState::Idle;
      write_state_ = WriteState::Idle;
      stream_state_.store(StreamState::Starting);
    }

    const auto operation_start = grpc_queue_->execute_result([this]() mutable {
      auto start_tag = std::make_unique<StartTag>(shared_from_this());
      if (!start_tag->active())
      {
        return false;
      }

      stream_ = batch_stub_->Call(
        stream_context_.get(),
        batch_stream_full_method_,
        &grpc_queue_->completion_queue(),
        start_tag.get()).release();
      start_tag.release();
      return true;
    });

    if (!operation_start.has_value() || !*operation_start)
    {
      bool shutdown_completed = false;
      {
        std::lock_guard<std::mutex> lock(state_lock_);
        shutdown_completed = mark_shutdown_complete_i_();
      }

      if (shutdown_completed)
      {
        deactivate_shutdown_gates_();
      }

      if (!operation_start.has_value())
      {
        record_last_error_(
          grpc::StatusCode::UNAVAILABLE,
          "grpc executor rejected stream start",
          "stream_start");
      }
      else
      {
        record_last_error_(
          grpc::StatusCode::UNAVAILABLE,
          "grpc operation gate rejected stream start",
          "stream_start");
      }
      notify_closed_if_needed_();
      complete_drain_if_ready_();
      return false;
    }

    return true;
  }

  bool
  BatchingStreamBase::Impl::try_start_write(
    PendingBatch&& pending_batch,
    bool measure_consumer_stream_write,
    PendingBatch* failed_batch)
  {
    if (!active())
    {
      if (failed_batch)
      {
        *failed_batch = std::move(pending_batch);
      }
      return false;
    }

    bool write_started = false;
    {
      std::lock_guard<std::mutex> lock(state_lock_);
      if (stream_state_.load() == StreamState::Open &&
        write_state_ == WriteState::Idle)
      {
        write_state_ = WriteState::Writing;
        write_started = true;
      }
    }

    if (!write_started)
    {
      if (failed_batch)
      {
        *failed_batch = std::move(pending_batch);
      }
      return false;
    }

    return start_write_(
      std::move(pending_batch),
      measure_consumer_stream_write,
      failed_batch);
  }

  bool
  BatchingStreamBase::Impl::start_write_(
    PendingBatch&& pending_batch,
    bool measure_consumer_stream_write,
    std::vector<PendingRequest>* failed_batch)
  {
    struct WriteSlotGuard
    {
      explicit WriteSlotGuard(Impl& impl_val)
        : impl(impl_val)
      {}

      ~WriteSlotGuard()
      {
        if (released)
        {
          return;
        }

        StreamAction action = StreamAction::None;

        {
          std::lock_guard<std::mutex> lock(impl.state_lock_);
          impl.write_state_ = WriteState::Idle;
          if (impl.stream_state_.load() != StreamState::Open)
          {
            action = impl.plan_shutdown_i_();
          }
        }

        if (action != StreamAction::None)
        {
          impl.execute_stream_action_(action);
        }
      }

      void release() noexcept
      {
        released = true;
      }

      Impl& impl;
      bool released = false;
    };

    WriteSlotGuard write_slot_guard(*this);

    if (pending_batch.empty())
    {
      return true;
    }

    write_arena_.Reset();
    auto* write_batch =
      google::protobuf::Arena::CreateMessage<BatchRequest>(&write_arena_);
    std::vector<PendingRequest> accepted_requests;
    std::vector<std::uint64_t> request_ids;
    accepted_requests.reserve(pending_batch.size());
    request_ids.reserve(pending_batch.size());
    std::optional<std::vector<std::pair<PendingRequest, Generics::Time>>>
      timed_out_requests;
    const auto write_time = Generics::Time::get_time_of_day();
    auto request_id = next_request_id_.fetch_add(
      pending_batch.size(),
      std::memory_order_relaxed);
    std::uint64_t batch_id = 0;

    for (auto& pending : pending_batch)
    {
      const auto current_request_id = request_id++;
      const auto wait_time = duration_time(pending.enqueue_time, write_time);
      const auto wait_us = wait_time.microseconds();
      add_queue_wait_stats(wait_us);
      if (options_.max_queue_wait && wait_time > *options_.max_queue_wait)
      {
        add_queue_timeout_stats();
        if (!timed_out_requests.has_value())
        {
          timed_out_requests.emplace();
        }
        timed_out_requests->emplace_back(std::move(pending), wait_time);
        continue;
      }

      if (batch_id == 0)
      {
        batch_id = current_request_id;
      }

      auto* item = write_batch->add_items();
      item->set_request_id(current_request_id);
      item->set_full_method(pending.request->full_method);
      item->set_payload(pending.request->payload);

      request_ids.emplace_back(current_request_id);
      accepted_requests.emplace_back(std::move(pending));
    }

    pending_batch.clear();

    if (timed_out_requests.has_value())
    {
      for (auto& [request, wait_time] : *timed_out_requests)
      {
        if (!request.request)
        {
          continue;
        }

        const auto status_message =
          std::string(QUEUE_WAIT_TIMEOUT_STATUS) +
            ": wait_us=" + std::to_string(wait_time.microseconds()) +
            ", limit_us=" +
              std::to_string(options_.max_queue_wait->microseconds());
        BatchResponseItem item;
        item.set_status_code(grpc::StatusCode::RESOURCE_EXHAUSTED);
        item.set_status_message(message_with_endpoint(
          status_message,
          endpoint_));
        request.request->complete(item);
      }
    }

    const auto batch_size = write_batch->items_size();
    if (batch_size == 0)
    {
      write_arena_.Reset();

      bool stream_open = false;
      StreamAction action = StreamAction::None;

      {
        std::lock_guard<std::mutex> lock(state_lock_);
        write_state_ = WriteState::Idle;
        stream_open = stream_state_.load() == StreamState::Open;
        if (!stream_open)
        {
          action = plan_shutdown_i_();
        }
      }
      write_slot_guard.release();
      execute_stream_action_(action);

      if (stream_open && active() && ready_callback_)
      {
        ready_callback_(&owner_);
      }

      return true;
    }

    auto batch_guard = detached_batch_storage_.register_batch(
      accepted_requests,
      batch_id,
      write_time,
      std::move(request_ids));

    if (!batch_guard)
    {
      StreamAction action = StreamAction::None;

      {
        std::lock_guard<std::mutex> lock(state_lock_);
        write_state_ = WriteState::Idle;
        if (stream_state_.load() != StreamState::Open)
        {
          action = plan_shutdown_i_();
        }
      }
      write_slot_guard.release();

      if (failed_batch)
      {
        failed_batch->reserve(failed_batch->size() + accepted_requests.size());
        for (auto& request : accepted_requests)
        {
          failed_batch->emplace_back(std::move(request));
        }
      }

      write_arena_.Reset();
      execute_stream_action_(action);
      return false;
    }

    const auto operation_start = grpc_queue_->execute_result(
      [
        this,
        write_batch,
        batch_id,
        measure_consumer_stream_write
      ]() mutable {
        auto write_tag = std::make_unique<WriteTag>(
          shared_from_this(),
          batch_id);
        if (!write_tag->active())
        {
          return false;
        }

        const auto start = Generics::Time::get_time_of_day();
        stream_->Write(*write_batch, write_tag.get());
        write_tag.release();
        if (measure_consumer_stream_write)
        {
          const auto wait_us =
            duration_time(start, Generics::Time::get_time_of_day()).microseconds();
          add_consumer_stream_write_stats(wait_us);
        }
        return true;
      });

    if (!operation_start.has_value() || !*operation_start)
    {
      bool shutdown_completed = false;
      const bool grpc_queue_shutdown = !operation_start.has_value();
      {
        std::lock_guard<std::mutex> lock(state_lock_);
        write_state_ = WriteState::Idle;
        if (grpc_queue_shutdown)
        {
          shutdown_completed = mark_grpc_queue_shutdown_i_();
        }
        else
        {
          shutdown_completed = mark_shutdown_complete_i_();
        }
      }
      write_slot_guard.release();

      if (shutdown_completed)
      {
        deactivate_shutdown_gates_();
      }

      if (grpc_queue_shutdown)
      {
        record_last_error_(
          grpc::StatusCode::UNAVAILABLE,
          "grpc executor shutdown",
          "executor_shutdown");
      }

      auto failed_batch_record = detached_batch_storage_.take_batch(batch_id);
      auto failed_requests = failed_batch_record && failed_batch_record->guard ?
        failed_batch_record->guard->claim_payload() : PendingBatch{};
      if (failed_batch && !failed_requests.empty())
      {
        failed_batch->reserve(failed_batch->size() + failed_requests.size());
        for (auto& request : failed_requests)
        {
          failed_batch->emplace_back(std::move(request));
        }
      }

      write_arena_.Reset();

      return false;
    }

    add_write_stats(1, batch_size);
    write_slot_guard.release();
    return true;
  }

  BatchingStreamBase::Impl::StreamTransition
  BatchingStreamBase::Impl::plan_start_completion_i_(bool ok)
  {
    struct StateTransition
    {
      StreamState state;
      bool ready;
    };

    constexpr auto state_count =
      static_cast<std::size_t>(StreamState::Finished) + 1;
    constexpr std::array<StateTransition, state_count> success_transitions = {{
      {StreamState::Open, true},       // Starting
      {StreamState::Open, true},       // Open
      {StreamState::Closing, false},   // Closing
      {StreamState::Broken, false},    // Broken
      {StreamState::Finishing, false}, // Finishing
      {StreamState::Finished, false}   // Finished
    }};
    constexpr std::array<StateTransition, state_count> failure_transitions = {{
      {StreamState::Broken, false}, // Starting
      {StreamState::Broken, false}, // Open
      {StreamState::Broken, false}, // Closing
      {StreamState::Broken, false}, // Broken
      {StreamState::Broken, false}, // Finishing
      {StreamState::Broken, false}  // Finished
    }};

    const auto old_state = stream_state_.load();
    const auto transition =
      (ok ? success_transitions : failure_transitions)[
        static_cast<std::size_t>(old_state)];
    stream_state_.store(transition.state);

    return {
      transition.ready ? plan_start_read_i_() : plan_shutdown_i_(),
      transition.ready
    };
  }

  BatchingStreamBase::Impl::StreamAction
  BatchingStreamBase::Impl::plan_start_read_i_()
  {
    if (stream_state_.load() != StreamState::Open ||
      read_state_ == ReadState::Reading)
    {
      return StreamAction::None;
    }

    read_state_ = ReadState::Reading;
    return StreamAction::StartRead;
  }

  void
  BatchingStreamBase::Impl::start_read_cq_() noexcept
  {
    const auto operation_start = grpc_queue_->execute_result([this]() mutable {
      auto response_arena = std::make_unique<google::protobuf::Arena>();
      auto* response = google::protobuf::Arena::CreateMessage<BatchResponse>(
        response_arena.get());
      auto read_tag = std::make_unique<ReadTag>(
        shared_from_this(),
        std::move(response_arena),
        response);
      if (!read_tag->active())
      {
        return false;
      }

      stream_->Read(response, read_tag.get());
      read_tag.release();
      return true;
    });

    if (!operation_start.has_value() || !*operation_start)
    {
      bool shutdown_completed = false;
      const bool grpc_queue_shutdown = !operation_start.has_value();
      {
        std::lock_guard<std::mutex> lock(state_lock_);
        read_state_ = ReadState::Idle;
        if (grpc_queue_shutdown)
        {
          shutdown_completed = mark_grpc_queue_shutdown_i_();
        }
        else
        {
          shutdown_completed = mark_shutdown_complete_i_();
        }
      }

      if (shutdown_completed)
      {
        deactivate_shutdown_gates_();
      }

      if (grpc_queue_shutdown)
      {
        record_last_error_(
          grpc::StatusCode::UNAVAILABLE,
          "grpc executor shutdown",
          "executor_shutdown");
      }
    }
  }

  BatchingStreamBase::Impl::StreamAction
  BatchingStreamBase::Impl::plan_shutdown_i_()
  {
    const auto stream_state = stream_state_.load();
    if (stream_state == StreamState::Finished ||
      stream_state == StreamState::Finishing ||
      stream_state == StreamState::Starting)
    {
      return StreamAction::None;
    }

    if (stream_state == StreamState::Closing &&
      write_state_ == WriteState::Idle)
    {
      write_state_ = WriteState::WritesDoneInFlight;
      return StreamAction::StartWritesDone;
    }

    if (read_state_ != ReadState::Reading &&
      write_state_ != WriteState::Writing &&
      write_state_ != WriteState::WritesDoneInFlight)
    {
      stream_state_.store(StreamState::Finishing);
      return StreamAction::StartFinish;
    }

    return StreamAction::None;
  }

  void
  BatchingStreamBase::Impl::start_writes_done_() noexcept
  {
    const auto operation_start = grpc_queue_->execute_result([this]() mutable {
      auto writes_done_tag =
        std::make_unique<WritesDoneTag>(shared_from_this());
      if (!writes_done_tag->active())
      {
        return false;
      }

      stream_->WritesDone(writes_done_tag.get());
      writes_done_tag.release();
      return true;
    });

    if (!operation_start.has_value() || !*operation_start)
    {
      bool shutdown_completed = false;
      const bool grpc_queue_shutdown = !operation_start.has_value();
      {
        std::lock_guard<std::mutex> lock(state_lock_);
        if (grpc_queue_shutdown)
        {
          write_state_ = WriteState::WritesDoneCompleted;
          shutdown_completed = mark_grpc_queue_shutdown_i_();
        }
        else
        {
          shutdown_completed = mark_shutdown_complete_i_();
        }
      }

      if (shutdown_completed)
      {
        deactivate_shutdown_gates_();
      }

      if (grpc_queue_shutdown)
      {
        record_last_error_(
          grpc::StatusCode::UNAVAILABLE,
          "grpc executor shutdown",
          "executor_shutdown");
      }
    }
  }

  void
  BatchingStreamBase::Impl::start_finish_() noexcept
  {
    const auto operation_start = grpc_queue_->execute_result([this]() mutable {
      auto finish_tag = std::make_unique<FinishTag>(shared_from_this());
      if (!finish_tag->active())
      {
        return false;
      }

      stream_->Finish(&finish_status_, finish_tag.get());
      finish_tag.release();
      return true;
    });

    if (!operation_start.has_value() || !*operation_start)
    {
      bool shutdown_completed = false;
      const bool grpc_queue_shutdown = !operation_start.has_value();
      {
        std::lock_guard<std::mutex> lock(state_lock_);
        if (grpc_queue_shutdown)
        {
          shutdown_completed = mark_grpc_queue_shutdown_i_();
        }
        else
        {
          shutdown_completed = mark_shutdown_complete_i_();
        }
      }

      if (shutdown_completed)
      {
        deactivate_shutdown_gates_();
      }

      if (grpc_queue_shutdown)
      {
        record_last_error_(
          grpc::StatusCode::UNAVAILABLE,
          "grpc executor shutdown",
          "executor_shutdown");
      }
    }
  }

  void
  BatchingStreamBase::Impl::execute_stream_action_(
    StreamAction action) noexcept
  {
    switch (action)
    {
    case StreamAction::None:
      return;
    case StreamAction::StartRead:
      start_read_cq_();
      return;
    case StreamAction::StartWritesDone:
      start_writes_done_();
      return;
    case StreamAction::StartFinish:
      start_finish_();
      return;
    }
  }

  void
  BatchingStreamBase::Impl::execute_stream_action_cq_(
    StreamAction action) noexcept
  {
    switch (action)
    {
    case StreamAction::None:
      return;
    case StreamAction::StartRead:
      start_read_cq_();
      return;
    case StreamAction::StartWritesDone:
      start_writes_done_();
      return;
    case StreamAction::StartFinish:
      start_finish_();
      return;
    }
  }

  void
  BatchingStreamBase::Impl::process_start_completion_cq_(bool ok)
  {
    StreamTransition transition;
    {
      std::lock_guard<std::mutex> lock(state_lock_);
      transition = plan_start_completion_i_(ok);
    }

    execute_stream_action_cq_(transition.action);

    if (!ok)
    {
      record_last_error_(
        grpc::StatusCode::UNAVAILABLE,
        "stream start failed",
        "stream_start");
      finish_with_error_(grpc::StatusCode::UNAVAILABLE, "stream start failed");
    }
    else if (transition.ready && ready_callback_)
    {
      ready_callback_(&owner_);
    }

    notify_closed_if_needed_();
    complete_drain_if_ready_();
  }

  void
  BatchingStreamBase::Impl::process_read_completion_cq_(
    bool ok,
    std::unique_ptr<google::protobuf::Arena> response_arena,
    BatchResponse* response)
  {
    // response points into response_arena.
    (void)response_arena;

    PendingBatch completed_requests;
    std::vector<const BatchResponseItem*> completed_items;
    bool protocol_error = false;
    bool has_missing_item = false;

    if (ok)
    {
      add_read_stats(1, response->items_size());

      if (response->items_size() == 0)
      {
        protocol_error = true;
      }
      else
      {
        std::uint64_t batch_id = response->items(0).request_id();
        for (int i = 1; i < response->items_size(); ++i)
        {
          batch_id = std::min<std::uint64_t>(batch_id, response->items(i).request_id());
        }

        auto completed_batch = detached_batch_storage_.take_batch(batch_id);

        if (!completed_batch || !completed_batch->guard)
        {
          protocol_error = true;
        }
        else
        {
          completed_requests = completed_batch->guard->claim_payload();

          if (!completed_requests.empty())
          {
            completed_items.resize(completed_requests.size());
            std::size_t response_items_errors_count = 0;
            const auto request_ids_size = completed_batch->request_ids.size();
            Generics::MonoAllocatorArena request_index_arena(
              std::max<std::size_t>(
                Generics::MonoAllocatorArena::DEFAULT_INITIAL_SIZE,
                request_ids_size * 64));
            Generics::MonoUnorderedMap<std::uint64_t, std::size_t> request_index(
              &request_index_arena);
            request_index.reserve(request_ids_size);
            for (std::size_t i = 0; i < request_ids_size; ++i)
            {
              request_index.emplace(completed_batch->request_ids[i], i);
            }

            const auto response_time = Generics::Time::get_time_of_day();
            const auto wait_us = duration_time(
              completed_batch->write_time, response_time).microseconds();
            const auto response_items_size = response->items_size();
            for (int i = 0; i < response_items_size; ++i)
            {
              const auto& item = response->items(i);
              const auto index_it = request_index.find(item.request_id());
              if (index_it == request_index.end())
              {
                protocol_error = true;
                ++response_items_errors_count;
                continue;
              }

              auto*& completed_item = completed_items[index_it->second];
              if (completed_item != nullptr)
              {
                protocol_error = true;
                ++response_items_errors_count;
                continue;
              }

              completed_item = &item;
            }

            const auto completed_items_count = static_cast<std::size_t>(response_items_size) -
              response_items_errors_count;
            add_response_wait_stats(wait_us, completed_items_count);

            if (completed_items_count != completed_items.size())
            {
              protocol_error = true;
              has_missing_item = true;
            }
          }
        }
      }
    }

    StreamAction action = StreamAction::None;

    {
      std::lock_guard<std::mutex> lock(state_lock_);
      read_state_ = ReadState::Idle;
      if (ok && !protocol_error)
      {
        action = plan_start_read_i_();
      }
      else
      {
        stream_state_.store(StreamState::Broken);
      }

      if (stream_state_.load() != StreamState::Open)
      {
        action = plan_shutdown_i_();
      }
    }

    execute_stream_action_cq_(action);

    if (protocol_error)
    {
      record_last_error_(
        grpc::StatusCode::INTERNAL, "batch response protocol error", "stream_read");
    }

    BatchResponseItem missing_item;
    if (has_missing_item)
    {
      missing_item.set_status_code(grpc::StatusCode::INTERNAL);
      missing_item.set_status_message(message_with_endpoint(
        "batch response missing item", endpoint_));
    }

    for (std::size_t i = 0; i < completed_requests.size(); ++i)
    {
      auto& request = completed_requests[i];
      if (request.request)
      {
        request.request->complete(completed_items[i] ? *completed_items[i] : missing_item);
      }
    }

    if (!ok)
    {
      record_last_error_(grpc::StatusCode::UNAVAILABLE, "stream read failed", "stream_read");
      fail_inflight_with_error_(grpc::StatusCode::UNAVAILABLE, "stream read failed");
    }
    else if (protocol_error)
    {
      fail_inflight_with_error_(grpc::StatusCode::INTERNAL, "batch response protocol error");
    }

    notify_closed_if_needed_();
    complete_drain_if_ready_();
  }

  void
  BatchingStreamBase::Impl::process_write_completion_cq_(
    bool ok,
    std::uint64_t batch_id)
  {
    write_arena_.Reset();

    bool stream_open = false;
    StreamAction action = StreamAction::None;

    {
      std::lock_guard<std::mutex> lock(state_lock_);
      write_state_ = WriteState::Idle;

      if (!ok)
      {
        stream_state_.store(StreamState::Broken);
      }
      else
      {
        stream_open = stream_state_.load() == StreamState::Open;
      }

      if (stream_state_.load() != StreamState::Open)
      {
        action = plan_shutdown_i_();
      }
    }

    execute_stream_action_cq_(action);

    if (!ok)
    {
      auto failed_batch = detached_batch_storage_.take_batch(batch_id);

      record_last_error_(
        grpc::StatusCode::UNAVAILABLE,
        "stream write failed",
        "stream_write");

      if (failed_batch && failed_batch->guard)
      {
        auto failed_requests = failed_batch->guard->claim_payload();
        finish_requests_with_error_(
          failed_requests,
          grpc::StatusCode::UNAVAILABLE,
          "stream write failed");
      }

      fail_inflight_with_error_(
        grpc::StatusCode::UNAVAILABLE,
        "stream write failed");
      notify_closed_if_needed_();
      complete_drain_if_ready_();
      return;
    }

    if (stream_open && active())
    {
      if (ready_callback_)
      {
        ready_callback_(&owner_);
      }
    }

    notify_closed_if_needed_();
    complete_drain_if_ready_();
  }

  void
  BatchingStreamBase::Impl::process_writes_done_completion_cq_(bool /*ok*/)
  {
    StreamAction action = StreamAction::None;

    {
      std::lock_guard<std::mutex> lock(state_lock_);
      write_state_ = WriteState::WritesDoneCompleted;
      action = plan_shutdown_i_();
    }

    execute_stream_action_cq_(action);

    notify_closed_if_needed_();
    complete_drain_if_ready_();
  }

  void
  BatchingStreamBase::Impl::process_finish_completion_cq_(bool /*ok*/)
  {
    if (!finish_status_.ok())
    {
      record_last_error_(
        finish_status_.error_code(),
        finish_status_.error_message().c_str(),
        "stream_finish");
    }

    bool shutdown_completed = false;
    {
      std::lock_guard<std::mutex> lock(state_lock_);
      shutdown_completed = mark_shutdown_complete_i_();
    }

    if (shutdown_completed)
    {
      deactivate_shutdown_gates_();
    }

    notify_closed_if_needed_();
    complete_drain_if_ready_();
  }

  void
  BatchingStreamBase::Impl::finish_requests_with_error_(
    std::vector<PendingRequest>& requests,
    grpc::StatusCode status_code,
    const char* status_message)
  {
    const auto message = message_with_endpoint(
      status_message ? status_message : "",
      endpoint_);
    for (auto& request : requests)
    {
      BatchResponseItem item;
      item.set_status_code(status_code);
      item.set_status_message(message);
      if (request.request)
      {
        request.request->complete(item);
      }
    }
    requests.clear();
  }

  void
  BatchingStreamBase::Impl::fail_inflight_with_error_(
    grpc::StatusCode status_code,
    const char* status_message)
  {
    auto batches = detached_batch_storage_.drain_all();
    for (auto& batch : batches)
    {
      finish_requests_with_error_(batch, status_code, status_message);
    }
  }

  void
  BatchingStreamBase::Impl::record_last_error_(
    grpc::StatusCode status_code,
    const char* status_message,
    const char* source) noexcept
  {
    owner_.set_last_error(
      endpoint_,
      status_code,
      status_message ? status_message : "",
      source);
  }

  void
  BatchingStreamBase::Impl::finish_with_error_(
    grpc::StatusCode status_code,
    const char* status_message)
  {
    fail_inflight_with_error_(status_code, status_message);
  }

  bool
  BatchingStreamBase::Impl::mark_grpc_queue_shutdown_i_() noexcept
  {
    stream_state_.store(StreamState::Broken);

    if (read_state_ != ReadState::Reading &&
      write_state_ != WriteState::Writing &&
      write_state_ != WriteState::WritesDoneInFlight)
    {
      return mark_shutdown_complete_i_();
    }

    return false;
  }

  bool
  BatchingStreamBase::Impl::mark_shutdown_complete_i_() noexcept
  {
    if (stream_state_.load() == StreamState::Finished)
    {
      return false;
    }

    stream_state_.store(StreamState::Finished);
    return true;
  }

  void
  BatchingStreamBase::Impl::deactivate_shutdown_gates_() noexcept
  {
    completion_tags_gate_.deactivate_object();
  }

  void
  BatchingStreamBase::Impl::notify_closed_if_needed_() noexcept
  {
    ClosedCallback closed_callback;

    {
      std::lock_guard<std::mutex> lock(state_lock_);
      if (stream_state_.load() != StreamState::Finished || closed_notified_)
      {
        return;
      }

      closed_notified_ = true;
      closed_callback = closed_callback_;
    }

    if (closed_callback)
    {
      closed_callback(&owner_);
    }
  }

  bool
  BatchingStreamBase::Impl::complete_drain_if_ready_() noexcept
  {
    if (completion_tags_gate_.has_activities())
    {
      return false;
    }

    DrainedCallback drained_callback;
    BatchingStreamBase* drained_stream = nullptr;

    {
      std::lock_guard<std::mutex> lock(state_lock_);
      if (stream_state_.load() != StreamState::Finished)
      {
        return false;
      }

      if (!drained_notified_)
      {
        drained_notified_ = true;
        drained_callback = drained_callback_;
        drained_stream = &owner_;
      }
    }

    if (drained_callback)
    {
      drained_callback(drained_stream);
    }

    return true;
  }

  void
  BatchingStreamBase::Impl::release_grpc_resources_i_() noexcept
  {
    stream_ = nullptr;
    stream_context_.reset();
    grpc_queue_.reset();
  }

  bool
  BatchingStreamBase::Impl::accepts_requests_i_() const noexcept
  {
    const auto stream_state = stream_state_.load();
    return stream_state == StreamState::Starting ||
      stream_state == StreamState::Open;
  }

} // namespace AdServer::Grpc
