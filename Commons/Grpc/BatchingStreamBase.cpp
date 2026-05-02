#include <Commons/Grpc/BatchingStreamBase.hpp>

#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <utility>

namespace AdServer::Grpc
{
  namespace
  {
    constexpr const char* default_batch_stream_full_method =
      "/adserver.grpc.BatchTransport/stream_batches";

    std::uint64_t
    duration_us(
      const std::chrono::steady_clock::time_point& from,
      const std::chrono::steady_clock::time_point& to)
    {
      return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(to - from).count());
    }
  } // namespace

  struct BatchingStreamBase::CompletionTag
    : AdServer::Grpc::GrpcCompletionEvent
  {
    explicit CompletionTag(BatchingStreamBase* owner)
      : owner_(owner)
    {
      this->owner_->add_pending_completion_tag_();
    }

    ~CompletionTag() override
    {
      this->owner_->remove_pending_completion_tag_();
    }

  protected:
    BatchingStreamBase* owner_;
  };

  struct BatchingStreamBase::StartTag final : CompletionTag
  {
    explicit StartTag(BatchingStreamBase* owner)
      : CompletionTag(owner)
    {}

    void
    proceed(bool ok) override
    {
      this->owner_->process_start_completion_(ok);
    }
  };

  struct BatchingStreamBase::ReadTag final : CompletionTag
  {
    ReadTag(
      BatchingStreamBase* owner,
      std::unique_ptr<BatchResponse> response)
      : CompletionTag(owner),
        response_(std::move(response))
    {}

    void
    proceed(bool ok) override
    {
      this->owner_->process_read_completion_(ok, std::move(response_));
    }

  private:
    std::unique_ptr<BatchResponse> response_;
  };

  struct BatchingStreamBase::WriteTag final : CompletionTag
  {
    WriteTag(
      BatchingStreamBase* owner,
      std::vector<std::shared_ptr<PendingRequest>> write_requests)
      : CompletionTag(owner),
        write_requests_(std::move(write_requests))
    {}

    void
    proceed(bool ok) override
    {
      this->owner_->process_write_completion_(ok, std::move(write_requests_));
    }

  private:
    std::vector<std::shared_ptr<PendingRequest>> write_requests_;
  };

  struct BatchingStreamBase::WritesDoneTag final : CompletionTag
  {
    explicit WritesDoneTag(BatchingStreamBase* owner)
      : CompletionTag(owner)
    {}

    void
    proceed(bool ok) override
    {
      this->owner_->process_writes_done_completion_(ok);
    }
  };

  struct BatchingStreamBase::FinishTag final : CompletionTag
  {
    explicit FinishTag(BatchingStreamBase* owner)
      : CompletionTag(owner)
    {}

    void
    proceed(bool ok) override
    {
      this->owner_->process_finish_completion_(ok);
    }
  };

  BatchingStreamBase::BatchingStreamBase(
    const std::string& endpoint,
    AdServer::Grpc::GrpcExecutor* grpc_executor,
    AdServer::Grpc::BatchingQueue* batching_queue,
    unsigned int queue_index,
    AdServer::Grpc::Client* stats_owner,
    ReadyCallback ready_callback,
    ClosedCallback closed_callback,
    AdServer::Grpc::BatchingOptions options)
    : AdServer::Grpc::Client(stats_owner),
      options_(options),
      batch_stream_full_method_(
        options_.batch_stream_full_method.empty() ?
          default_batch_stream_full_method :
          options_.batch_stream_full_method),
      batching_queue_(ReferenceCounting::add_ref(batching_queue)),
      ready_callback_(std::move(ready_callback)),
      closed_callback_(std::move(closed_callback)),
      grpc_executor_(ReferenceCounting::add_ref(grpc_executor)),
      queue_index_(queue_index)
  {
    if (!grpc_executor_)
    {
      throw std::runtime_error("BatchingStreamBase requires GrpcExecutor");
    }
    if (!batching_queue_)
    {
      throw std::runtime_error("BatchingStreamBase requires BatchingQueue");
    }

    grpc::ChannelArguments channel_args;
    channel_args.SetInt(
      GRPC_ARG_USE_LOCAL_SUBCHANNEL_POOL,
      options_.use_local_subchannel_pool ? 1 : 0);
    if (!options_.enable_grpc_compression)
    {
      channel_args.SetCompressionAlgorithm(GRPC_COMPRESS_NONE);
    }

    batch_stub_ = std::make_unique<BatchTransport>(
      grpc::CreateCustomChannel(
        endpoint,
        grpc::InsecureChannelCredentials(),
        channel_args));
  }

  BatchingStreamBase::~BatchingStreamBase() = default;

  bool
  BatchingStreamBase::available() noexcept
  {
    std::lock_guard<std::mutex> lock(state_lock_);
    return active() && accepts_requests_i_();
  }

  void
  BatchingStreamBase::activate_object_()
  {
    start_stream_();
  }

  void
  BatchingStreamBase::deactivate_object_()
  {
    callback_cv_.notify_all();
    batching_queue_->notify_all();

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
        maybe_start_shutdown_i_();
      }
    }

    if (stream_context_)
    {
      stream_context_->TryCancel();
    }
  }

  bool
  BatchingStreamBase::wait_more_()
  {
    return pending_completion_tags_.load(std::memory_order_acquire) != 0;
  }

  void
  BatchingStreamBase::wait_object_()
  {
    std::unique_lock<std::mutex> lock(state_lock_);
    shutdown_cv_.wait(lock, [this]() {
      return stream_state_.load() == StreamState::Finished &&
        pending_completion_tags_.load(std::memory_order_acquire) == 0;
    });
    lock.unlock();

    finish_with_error_(grpc::StatusCode::UNAVAILABLE, "inactive");
  }

  bool
  BatchingStreamBase::start_stream_()
  {
    {
      std::lock_guard<std::mutex> lock(state_lock_);
      if (!active())
      {
        return false;
      }

      stream_context_ = std::make_unique<grpc::ClientContext>();
      grpc_queue_ = grpc_executor_->queue(queue_index_);
      finish_status_ = grpc::Status();
      read_in_flight_ = false;
      writes_done_started_ = false;
      writes_done_in_flight_ = false;
      finish_in_flight_ = false;
      write_in_flight_.store(false);
      stream_state_.store(StreamState::Starting);
    }

    if (!grpc_queue_->execute([this]() {
      auto start_tag = std::make_unique<StartTag>(this);
      stream_ = batch_stub_->Call(
        stream_context_.get(),
        batch_stream_full_method_,
        &grpc_queue_->completion_queue(),
        start_tag.get());
      start_tag.release();
    }))
    {
      std::lock_guard<std::mutex> lock(state_lock_);
      stream_state_.store(StreamState::Broken);
      callback_cv_.notify_one();
      batching_queue_->notify_all();
      maybe_start_shutdown_i_();
      return false;
    }

    callback_cv_.notify_one();
    return true;
  }

  bool
  BatchingStreamBase::try_start_write(
    PendingBatch&& pending_batch,
    bool measure_consumer_stream_write,
    PendingBatch* failed_batch)
  {
    {
      std::lock_guard<std::mutex> lock(state_lock_);
      if (!active() || stream_state_.load() != StreamState::Open ||
        write_in_flight_.load())
      {
        if (failed_batch)
        {
          *failed_batch = std::move(pending_batch);
        }
        return false;
      }

      write_in_flight_.store(true);
    }

    return start_write_(
      std::move(pending_batch),
      measure_consumer_stream_write,
      failed_batch);
  }

  bool
  BatchingStreamBase::start_write_(
    std::vector<std::shared_ptr<PendingRequest>>&& pending_batch,
    bool measure_consumer_stream_write,
    std::vector<std::shared_ptr<PendingRequest>>* failed_batch)
  {
    write_arena_.Reset();
    auto* write_batch =
      google::protobuf::Arena::CreateMessage<BatchRequest>(&write_arena_);
    auto write_requests =
      std::make_shared<std::vector<std::shared_ptr<PendingRequest>>>();
    write_requests->reserve(pending_batch.size());
    const auto write_time = std::chrono::steady_clock::now();

    {
      std::lock_guard<std::mutex> inflight_lock(inflight_lock_);
      for (auto& pending : pending_batch)
      {
        auto* item = write_batch->add_items();
        item->set_request_id(pending->request_id);
        item->set_full_method(pending->full_method);
        item->set_payload(pending->payload);
        pending->write_time = write_time;

        const auto wait_us = duration_us(pending->enqueue_time, write_time);
        add_queue_wait_stats(wait_us);

        inflight_.emplace(pending->request_id, pending);
        write_requests->emplace_back(std::move(pending));
      }
    }

    const auto batch_size = write_batch->items_size();
    const bool write_scheduled = grpc_queue_->execute(
      [
        this,
        write_batch,
        write_requests,
        measure_consumer_stream_write
      ]() {
        auto write_tag = std::make_unique<WriteTag>(
          this,
          std::move(*write_requests));
        auto* write_tag_ptr = write_tag.get();
        const auto start = std::chrono::steady_clock::now();
        stream_->Write(*write_batch, write_tag_ptr);
        write_tag.release();
        if (measure_consumer_stream_write)
        {
          const auto wait_us = duration_us(start, std::chrono::steady_clock::now());
          add_consumer_stream_write_stats(wait_us);
        }
      });

    if (!write_scheduled)
    {
      {
        std::lock_guard<std::mutex> lock(state_lock_);
        write_in_flight_.store(false);
        handle_executor_shutdown_i_();
      }

      {
        std::lock_guard<std::mutex> inflight_lock(inflight_lock_);
        for (const auto& pending : *write_requests)
        {
          inflight_.erase(pending->request_id);
        }
      }

      if (failed_batch)
      {
        *failed_batch = std::move(*write_requests);
      }

      write_arena_.Reset();

      return false;
    }
    else
    {
      add_write_stats(1, batch_size);
      return true;
    }
  }

  void
  BatchingStreamBase::maybe_start_read_i_()
  {
    if (stream_state_.load() != StreamState::Open || read_in_flight_)
    {
      return;
    }

    read_in_flight_ = true;
    auto response = std::make_unique<BatchResponse>();
    auto* response_ptr = response.get();
    auto read_tag = std::make_unique<ReadTag>(
      this,
      std::move(response));
    auto* read_tag_ptr = read_tag.get();
    if (!grpc_queue_->execute([this, response_ptr, read_tag_ptr]() {
      stream_->Read(response_ptr, read_tag_ptr);
    }))
    {
      read_in_flight_ = false;
      handle_executor_shutdown_i_();
    }
    else
    {
      read_tag.release();
    }
  }

  void
  BatchingStreamBase::maybe_start_shutdown_i_()
  {
    if (stream_state_.load() == StreamState::Finished)
    {
      return;
    }

    if (!stream_)
    {
      complete_shutdown_i_();
      return;
    }

    if (stream_state_.load() == StreamState::Starting)
    {
      return;
    }

    if (stream_state_.load() == StreamState::Closing &&
      !writes_done_started_ &&
      !writes_done_in_flight_ &&
      !write_in_flight_.load())
    {
      writes_done_started_ = true;
      writes_done_in_flight_ = true;
      auto writes_done_tag = std::make_unique<WritesDoneTag>(this);
      auto* writes_done_tag_ptr = writes_done_tag.get();
      if (!grpc_queue_->execute([this, writes_done_tag_ptr]() {
        stream_->WritesDone(writes_done_tag_ptr);
      }))
      {
        writes_done_started_ = false;
        writes_done_in_flight_ = false;
        handle_executor_shutdown_i_();
      }
      else
      {
        writes_done_tag.release();
      }
      return;
    }

    if (!finish_in_flight_ &&
      !read_in_flight_ &&
      !write_in_flight_.load() &&
      !writes_done_in_flight_)
    {
      finish_in_flight_ = true;
      auto finish_tag = std::make_unique<FinishTag>(this);
      auto* finish_tag_ptr = finish_tag.get();
      if (!grpc_queue_->execute([this, finish_tag_ptr]() {
        stream_->Finish(&finish_status_, finish_tag_ptr);
      }))
      {
        finish_in_flight_ = false;
        handle_executor_shutdown_i_();
      }
      else
      {
        finish_tag.release();
      }
    }
  }

  void
  BatchingStreamBase::process_start_completion_(bool ok)
  {
    bool ready = false;
    {
      std::lock_guard<std::mutex> lock(state_lock_);
      if (!ok)
      {
        stream_state_.store(StreamState::Broken);
      }
      else if (stream_state_.load() == StreamState::Starting)
      {
        stream_state_.store(StreamState::Open);
      }

      if (stream_state_.load() == StreamState::Open)
      {
        maybe_start_read_i_();
        ready = true;
        callback_cv_.notify_one();
      }
      else
      {
        callback_cv_.notify_one();
        batching_queue_->notify_all();
        maybe_start_shutdown_i_();
      }
    }

    if (!ok)
    {
      finish_with_error_(grpc::StatusCode::UNAVAILABLE, "stream start failed");
    }
    else if (ready && ready_callback_)
    {
      ready_callback_(this);
    }
  }

  void
  BatchingStreamBase::process_read_completion_(
    bool ok,
    std::unique_ptr<BatchResponse> response)
  {
    std::vector<std::shared_ptr<PendingRequest>> completed_requests;
    std::vector<BatchResponseItem> completed_items;

    if (ok)
    {
      completed_requests.reserve(response->items_size());
      completed_items.reserve(response->items_size());

      std::lock_guard<std::mutex> inflight_lock(inflight_lock_);
      const auto response_time = std::chrono::steady_clock::now();
      for (int i = 0; i < response->items_size(); ++i)
      {
        const auto& item = response->items(i);
        const auto it = inflight_.find(item.request_id());
        if (it == inflight_.end())
        {
          continue;
        }

        completed_requests.emplace_back(std::move(it->second));
        completed_items.emplace_back(item);
        const auto wait_us = duration_us(
          completed_requests.back()->write_time,
          response_time);
        add_response_wait_stats(wait_us);
        inflight_.erase(it);
      }
    }

    {
      std::lock_guard<std::mutex> lock(state_lock_);
      read_in_flight_ = false;
      if (ok)
      {
        maybe_start_read_i_();
      }
      else
      {
        stream_state_.store(StreamState::Broken);
        callback_cv_.notify_one();
        batching_queue_->notify_all();
      }

      if (stream_state_.load() != StreamState::Open)
      {
        maybe_start_shutdown_i_();
      }
    }

    for (std::size_t i = 0; i < completed_requests.size(); ++i)
    {
      auto& request = completed_requests[i];
      if (request && request->callback)
      {
        request->callback(completed_items[i]);
      }
    }

    if (!ok)
    {
      fail_inflight_with_error_(grpc::StatusCode::UNAVAILABLE, "stream read failed");
    }
  }

  void
  BatchingStreamBase::process_write_completion_(
    bool ok,
    std::vector<std::shared_ptr<PendingRequest>>&& write_requests)
  {
    write_arena_.Reset();

    bool ready = false;
    {
      std::lock_guard<std::mutex> lock(state_lock_);
      write_in_flight_.store(false);

      if (ok)
      {
        ready = stream_state_.load() == StreamState::Open;
      }
      else
      {
        stream_state_.store(StreamState::Broken);
        callback_cv_.notify_one();
        batching_queue_->notify_all();
      }

      if (stream_state_.load() != StreamState::Open)
      {
        maybe_start_shutdown_i_();
      }
    }

    if (ready)
    {
      callback_cv_.notify_one();
      if (ready_callback_)
      {
        ready_callback_(this);
      }
    }

    if (!ok)
    {
      std::vector<std::shared_ptr<PendingRequest>> failed_write_requests;
      {
        std::lock_guard<std::mutex> inflight_lock(inflight_lock_);
        for (const auto& pending : write_requests)
        {
          if (inflight_.erase(pending->request_id) > 0)
          {
            failed_write_requests.emplace_back(pending);
          }
        }
      }
      finish_requests_with_error_(
        failed_write_requests,
        grpc::StatusCode::UNAVAILABLE,
        "stream write failed");
      fail_inflight_with_error_(grpc::StatusCode::UNAVAILABLE, "stream write failed");
    }
  }

  void
  BatchingStreamBase::process_writes_done_completion_(bool /*ok*/)
  {
    std::lock_guard<std::mutex> lock(state_lock_);
    writes_done_in_flight_ = false;
    maybe_start_shutdown_i_();
  }

  void
  BatchingStreamBase::process_finish_completion_(bool /*ok*/)
  {
    std::lock_guard<std::mutex> lock(state_lock_);
    finish_in_flight_ = false;
    complete_shutdown_i_();
  }

  void
  BatchingStreamBase::finish_requests_with_error_(
    std::vector<std::shared_ptr<PendingRequest>>& requests,
    grpc::StatusCode status_code,
    const char* status_message)
  {
    for (auto& request : requests)
    {
      BatchResponseItem item;
      item.set_request_id(request->request_id);
      item.set_status_code(status_code);
      item.set_status_message(status_message);
      if (request->callback)
      {
        request->callback(item);
      }
    }
    requests.clear();
  }

  void
  BatchingStreamBase::fail_inflight_with_error_(
    grpc::StatusCode status_code,
    const char* status_message)
  {
    std::vector<std::shared_ptr<PendingRequest>> inflight_requests;

    {
      std::lock_guard<std::mutex> lock(inflight_lock_);
      inflight_requests.reserve(inflight_.size());
      for (auto& [_, request] : inflight_)
      {
        inflight_requests.emplace_back(std::move(request));
      }
      inflight_.clear();
    }

    finish_requests_with_error_(inflight_requests, status_code, status_message);
  }

  void
  BatchingStreamBase::finish_with_error_(
    grpc::StatusCode status_code,
    const char* status_message)
  {
    std::vector<std::shared_ptr<PendingRequest>> requests;

    {
      std::lock_guard<std::mutex> lock(inflight_lock_);
      requests.reserve(requests.size() + inflight_.size());
      for (auto& [_, request] : inflight_)
      {
        requests.emplace_back(std::move(request));
      }
      inflight_.clear();
    }

    finish_requests_with_error_(requests, status_code, status_message);
  }

  void
  BatchingStreamBase::handle_executor_shutdown_i_() noexcept
  {
    stream_state_.store(StreamState::Broken);
    callback_cv_.notify_one();
    batching_queue_->notify_all();

    if (!read_in_flight_ && !write_in_flight_.load() &&
      !writes_done_in_flight_ && !finish_in_flight_)
    {
      complete_shutdown_i_();
    }
  }

  void
  BatchingStreamBase::complete_shutdown_i_() noexcept
  {
    const bool already_finished =
      stream_state_.load() == StreamState::Finished;
    stream_.reset();
    stream_context_.reset();
    grpc_queue_.reset();
    stream_state_.store(StreamState::Finished);
    shutdown_cv_.notify_all();
    callback_cv_.notify_all();
    batching_queue_->notify_all();
    if (!already_finished && closed_callback_)
    {
      closed_callback_(this);
    }
  }

  bool
  BatchingStreamBase::accepts_requests_i_() const noexcept
  {
    const auto stream_state = stream_state_.load();
    return stream_state == StreamState::Starting ||
      stream_state == StreamState::Open;
  }

  void
  BatchingStreamBase::add_pending_completion_tag_() noexcept
  {
    pending_completion_tags_.fetch_add(1, std::memory_order_acq_rel);
  }

  void
  BatchingStreamBase::remove_pending_completion_tag_() noexcept
  {
    if (pending_completion_tags_.fetch_sub(1, std::memory_order_acq_rel) == 1)
    {
      shutdown_cv_.notify_all();
      Sync::PosixGuard guard(cond_);
      cond_.broadcast();
    }
  }

} // namespace AdServer::Grpc
