#include <Commons/Grpc/AsyncBatchingClientBase.hpp>

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <utility>

namespace AdServer::Grpc
{
  AsyncBatchingClientBase::AsyncBatchingClientBase(
    const std::string& endpoint,
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor,
    AdServer::Grpc::BatchingOptions options)
    : Generics::CompositeActiveObject(false, false),
      endpoint_(endpoint),
      options_(std::move(options)),
      max_streams_(std::max<std::size_t>(1, options_.channels_number)),
      grpc_executor_(std::move(grpc_executor)),
      batching_queue_(std::make_shared<AdServer::Grpc::BatchingQueue>(options_)),
      inflight_limiter_(options_.max_inflight)
  {
    if (!grpc_executor_)
    {
      throw std::runtime_error("AsyncBatchingClientBase requires GrpcExecutor");
    }

    add_child_object(batching_queue_);
    streams_.reserve(max_streams_);
  }

  AsyncBatchingClientBase::~AsyncBatchingClientBase()
  {
    try
    {
      clear_children();
    }
    catch (...)
    {
    }

    clear_streams_();
    batching_queue_.reset();
  }

  void
  AsyncBatchingClientBase::activate_object_()
  {
    Generics::CompositeActiveObject::activate_object_();

    const auto workers_number = std::max<std::size_t>(
      1,
      options_.workers_number);
    coalesce_threads_.reserve(workers_number);
    for (std::size_t i = 0; i < workers_number; ++i)
    {
      coalesce_threads_.emplace_back([this]() {
        coalesce_loop_();
      });
    }
  }

  void
  AsyncBatchingClientBase::deactivate_object_()
  {
    streams_cv_.notify_all();
    Generics::CompositeActiveObject::deactivate_object_();
  }

  void
  AsyncBatchingClientBase::wait_object_()
  {
    for (auto& thread : coalesce_threads_)
    {
      if (thread.joinable())
      {
        thread.join();
      }
    }
    coalesce_threads_.clear();

    Generics::CompositeActiveObject::wait_object_();
    clear_streams_();
  }

  void
  AsyncBatchingClientBase::release_stream_() noexcept
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

  AsyncBatchingClientBase::BatchingStreamPtr
  AsyncBatchingClientBase::make_stream_()
  {
    return std::make_shared<AdServer::Grpc::BatchingStreamBase>(
        endpoint_,
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
        options_);
  }

  BatchingStreamBase*
  AsyncBatchingClientBase::acquire_stream_()
  {
    std::unique_lock<std::mutex> lock(streams_lock_);
    while (active())
    {
      while (!available_streams_.empty())
      {
        auto* stream = available_streams_.front();
        available_streams_.pop_front();
        return stream;
      }

      lock.unlock();

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
        BatchingStreamPtr stream;
        bool stream_registered = false;
        try
        {
          stream = make_stream_();
          if (!active())
          {
            up_streams_.fetch_sub(1, std::memory_order_acq_rel);
            lock.lock();
            break;
          }
          {
            std::lock_guard<std::mutex> streams_lock(streams_registry_lock_);
            streams_.emplace_back(stream);
          }
          stream_registered = true;
          update_max_streams_(streams_count);
          add_child_object(stream);
        }
        catch (...)
        {
          bool release_stream_slot = !stream_registered;
          if (stream_registered)
          {
            std::lock_guard<std::mutex> streams_lock(streams_registry_lock_);
            auto it = std::find(streams_.begin(), streams_.end(), stream);
            if (it != streams_.end())
            {
              streams_.erase(it);
              release_stream_slot = true;
            }
          }
          if (release_stream_slot)
          {
            up_streams_.fetch_sub(1, std::memory_order_acq_rel);
            streams_cv_.notify_one();
          }
          throw;
        }
      }

      lock.lock();
      if (!active())
      {
        break;
      }

      while (!available_streams_.empty())
      {
        auto* stream = available_streams_.front();
        available_streams_.pop_front();
        return stream;
      }

      lock.unlock();

      const bool pending_failed = fail_pending_if_no_streams_();
      if (pending_failed)
      {
        break;
      }

      lock.lock();
      streams_cv_.wait(lock, [this]() {
        return !active() ||
          !available_streams_.empty() ||
          up_streams_.load(std::memory_order_acquire) < max_streams_;
      });
    }

    return nullptr;
  }

  void
  AsyncBatchingClientBase::release_stream_(BatchingStreamBase* stream) noexcept
  {
    assert(stream);

    bool notify = false;
    {
      std::lock_guard<std::mutex> lock(streams_lock_);
      if (active() && stream->available())
      {
        available_streams_.emplace_back(stream);
        notify = true;
      }
    }
    if (notify)
    {
      streams_cv_.notify_one();
    }
  }

  void
  AsyncBatchingClientBase::handle_stream_ready_(BatchingStreamBase* stream)
  {
    stream_start_attempts_.store(0, std::memory_order_release);
    release_stream_(stream);
  }

  void
  AsyncBatchingClientBase::handle_stream_closed_(
    BatchingStreamBase* stream) noexcept
  {
    assert(stream);

    bool stream_removed = false;
    {
      std::lock_guard<std::mutex> registry_lock(streams_registry_lock_);
      auto it = std::find_if(
        streams_.begin(),
        streams_.end(),
        [stream](const auto& registered_stream) {
          return registered_stream.get() == stream;
        });
      if (it != streams_.end())
      {
        streams_.erase(it);
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
    if (!fail_pending_if_no_streams_())
    {
      streams_cv_.notify_one();
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

    batching_queue_->fail_all_with_error(
      grpc::StatusCode::UNAVAILABLE,
      "no active batching streams");
    batching_queue_->notify_all();
    streams_cv_.notify_all();
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
  AsyncBatchingClientBase::clear_streams_() noexcept
  {
    {
      std::lock_guard<std::mutex> lock(streams_lock_);
      available_streams_.clear();
    }
    {
      std::lock_guard<std::mutex> lock(streams_registry_lock_);
      streams_.clear();
    }
  }

  void
  AsyncBatchingClientBase::coalesce_loop_()
  {
    while (active())
    {
      BatchingStreamBase::PendingBatch pending_batch;
      if (!batching_queue_->pop_batch(pending_batch))
      {
        break;
      }

      bool batch_sent = false;
      while (active() && !pending_batch.empty())
      {
        auto* stream = acquire_stream_();
        if (!stream)
        {
          break;
        }

        BatchingStreamBase::PendingBatch failed_batch;
        if (stream->try_start_write(std::move(pending_batch), false, &failed_batch))
        {
          batch_sent = true;
          break;
        }

        release_stream_(stream);
        pending_batch = std::move(failed_batch);
        if (pending_batch.empty())
        {
          break;
        }
      }

      if (!batch_sent && !pending_batch.empty())
      {
        for (auto& request : pending_batch)
        {
          adserver::grpc::BatchResponseItem item;
          item.set_request_id(request->request_id);
          item.set_status_code(grpc::StatusCode::UNAVAILABLE);
          item.set_status_message("no active batching streams");
          if (request->callback)
          {
            request->callback(item);
          }
        }
      }
    }
  }
}
