#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/completion_queue.h>
#include <grpcpp/support/async_unary_call.h>
#include <grpcpp/support/status.h>

#include <Generics/Time.hpp>

namespace AdServer::Grpc
{
  struct BatchingOptions
  {
    BatchingOptions();

    std::size_t channels_number = 16;
    std::size_t max_batch_size = 1024;
    // Limits requests that have left BatchingQueue for stream processing.
    // Queued requests are not counted. Accounting is reserved and released per
    // batch, not per request. Concurrent flushes can overshoot the limit. With
    // error_on_inflight_reaching enabled, flushes complete their callbacks with
    // RESOURCE_EXHAUSTED instead of blocking on the limiter.
    std::optional<std::size_t> max_inflight{10000};
    bool error_on_inflight_reaching = true;
    std::size_t workers_number = 4;
    std::size_t hot_buckets_count = 1;
    std::optional<Generics::Time> max_batch_delay{
      Generics::Time(0, 3000)};
    std::optional<Generics::Time> max_queue_wait;
    std::optional<Generics::Time> stream_start_timeout;
    bool enable_grpc_compression = true;
    bool use_local_subchannel_pool = true;
    std::string batch_stream_full_method;
    Generics::Time reconnect_period;
    Generics::Time stream_idle_timeout;
    Generics::Time stream_shrink_period;
  };

  struct Stats
  {
    struct ConsumerStreamWrite
    {
      std::uint64_t count = 0;
      std::uint64_t sum_us = 0;
      std::uint64_t max_us = 0;
    };

    struct LastError
    {
      Generics::Time time;
      std::string endpoint;
      int code = 0;
      std::string message;
      std::string source;
    };

    std::uint64_t write_batches = 0;
    std::uint64_t write_items = 0;
    std::uint64_t read_batches = 0;
    std::uint64_t read_items = 0;
    std::uint64_t input_items = 0;
    std::uint64_t completed_items = 0;
    std::uint64_t completed_error_items = 0;
    std::uint64_t queue_wait_count = 0;
    std::uint64_t queue_wait_sum_us = 0;
    std::uint64_t queue_wait_max_us = 0;
    std::uint64_t queue_timeout_count = 0;
    std::uint64_t response_wait_count = 0;
    std::uint64_t response_wait_sum_us = 0;
    std::uint64_t response_wait_max_us = 0;
    std::uint64_t timing_coalesce_items = 0;
    std::uint64_t max_streams = 0;
    std::uint64_t inflight_items = 0;
    std::uint64_t stream_inflight_items = 0;
    std::uint64_t queue_items = 0;
    std::uint64_t pending_batches = 0;
    std::uint64_t pending_batch_items = 0;
    std::uint64_t active_streams = 0;
    std::uint64_t available_streams = 0;
    std::uint64_t connecting_streams = 0;
    std::uint64_t draining_streams = 0;
    std::uint64_t deferred_streams = 0;
    std::optional<ConsumerStreamWrite> consumer_stream_write;
    std::optional<LastError> last_error;
  };

  class Client
  {
  public:
    virtual ~Client() = default;

    virtual Stats stats() const noexcept;

    void add_completed_stats(bool error) noexcept;

  protected:
    explicit Client(Client* stats_owner = nullptr) noexcept;

    void add_input_stats(std::uint64_t items) noexcept;

    void add_write_stats(std::uint64_t batches, std::uint64_t items) noexcept;

    void add_read_stats(std::uint64_t batches, std::uint64_t items) noexcept;

    void add_queue_wait_stats(std::uint64_t wait_us) noexcept;

    void add_queue_timeout_stats() noexcept;

    void add_response_wait_stats(std::uint64_t wait_us) noexcept;

    void add_consumer_stream_write_stats(std::uint64_t wait_us) noexcept;

    void add_timing_coalesce_stats(std::uint64_t items) noexcept;

    void set_last_error(
      const std::string& endpoint,
      grpc::StatusCode code,
      const std::string& message,
      const char* source) noexcept;

  private:
    Stats own_stats_() const noexcept;

    static void update_max_(
      std::atomic<std::uint64_t>& value,
      std::uint64_t candidate) noexcept;

    std::atomic<std::uint64_t> write_batches_{0};
    std::atomic<std::uint64_t> write_items_{0};
    std::atomic<std::uint64_t> read_batches_{0};
    std::atomic<std::uint64_t> read_items_{0};
    std::atomic<std::uint64_t> input_items_{0};
    std::atomic<std::uint64_t> completed_items_{0};
    std::atomic<std::uint64_t> completed_error_items_{0};
    std::atomic<std::uint64_t> queue_wait_count_{0};
    std::atomic<std::uint64_t> queue_wait_sum_us_{0};
    std::atomic<std::uint64_t> queue_wait_max_us_{0};
    std::atomic<std::uint64_t> queue_timeout_count_{0};
    std::atomic<std::uint64_t> response_wait_count_{0};
    std::atomic<std::uint64_t> response_wait_sum_us_{0};
    std::atomic<std::uint64_t> response_wait_max_us_{0};
    std::atomic<std::uint64_t> timing_coalesce_items_{0};
    std::atomic<bool> consumer_stream_write_enabled_{false};
    std::atomic<std::uint64_t> consumer_stream_write_count_{0};
    std::atomic<std::uint64_t> consumer_stream_write_sum_us_{0};
    std::atomic<std::uint64_t> consumer_stream_write_max_us_{0};
    mutable std::mutex last_error_lock_;
    std::optional<Stats::LastError> last_error_;
    Client* const stats_owner_;
  };

  class InflightLimiter final
  {
  public:
    explicit InflightLimiter(std::optional<std::size_t> max_inflight = std::nullopt);

    bool try_acquire() noexcept;
    bool try_acquire(std::size_t count) noexcept;

    void acquire();
    void acquire(std::size_t count);

    void release() noexcept;
    void release(std::size_t count) noexcept;
    std::size_t count() const noexcept;

  private:
    std::mutex lock_;
    std::condition_variable cv_;
    std::atomic<std::size_t> inflight_count_{0};
    std::atomic<std::size_t> waiters_count_{0};
    const std::optional<std::size_t> max_inflight_;
  };

  inline
  BatchingOptions::BatchingOptions()
    : reconnect_period(Generics::Time::ONE_SECOND),
      stream_idle_timeout(Generics::Time(20)),
      stream_shrink_period(Generics::Time::ONE_SECOND)
  {}

  inline constexpr const char QUEUE_WAIT_TIMEOUT_STATUS[] =
    "queue wait timeout";

  inline constexpr const char NO_ACTIVE_BATCHING_STREAMS_MESSAGE[] =
    "no active batching streams";

  inline bool
  is_queue_wait_timeout(const grpc::Status& status)
  {
    return status.error_code() == grpc::StatusCode::RESOURCE_EXHAUSTED &&
      status.error_message().compare(
        0,
        sizeof(QUEUE_WAIT_TIMEOUT_STATUS) - 1,
        QUEUE_WAIT_TIMEOUT_STATUS) == 0;
  }

  inline bool
  is_transport_timeout(const grpc::Status& status)
  {
    return status.error_code() == grpc::StatusCode::DEADLINE_EXCEEDED;
  }

  inline bool
  is_no_active_batching_streams(const grpc::Status& status)
  {
    const auto message = status.error_message();
    return status.error_code() == grpc::StatusCode::UNAVAILABLE &&
      message.rfind(NO_ACTIVE_BATCHING_STREAMS_MESSAGE, 0) == 0;
  }

  inline void
  merge_last_error(Stats& result, const Stats& source) noexcept
  {
    if (source.last_error.has_value() &&
      (!result.last_error.has_value() ||
        result.last_error->time < source.last_error->time))
    {
      result.last_error = source.last_error;
    }
  }

  inline grpc::Status
  status_with_endpoint(
    const grpc::Status& status,
    const std::string& endpoint)
  {
    if (status.ok() || endpoint.empty())
    {
      return status;
    }

    std::string message = status.error_message();
    if (!message.empty())
    {
      message += ' ';
    }
    message += "[grpc_endpoint=";
    message += endpoint;
    message += "]";
    return grpc::Status(
      status.error_code(),
      std::move(message),
      status.error_details());
  }

  inline
  Client::Client(Client* stats_owner) noexcept
    : stats_owner_(stats_owner ? stats_owner : this)
  {}

  inline Stats
  Client::stats() const noexcept
  {
    return stats_owner_->own_stats_();
  }

  inline void
  Client::add_completed_stats(bool error) noexcept
  {
    stats_owner_->completed_items_.fetch_add(1, std::memory_order_relaxed);
    if (error)
    {
      stats_owner_->completed_error_items_.fetch_add(
        1,
        std::memory_order_relaxed);
    }
  }

  inline void
  Client::add_input_stats(std::uint64_t items) noexcept
  {
    stats_owner_->input_items_.fetch_add(items, std::memory_order_relaxed);
  }

  inline void
  Client::add_write_stats(
    std::uint64_t batches,
    std::uint64_t items) noexcept
  {
    stats_owner_->write_batches_.fetch_add(batches, std::memory_order_relaxed);
    stats_owner_->write_items_.fetch_add(items, std::memory_order_relaxed);
  }

  inline void
  Client::add_read_stats(
    std::uint64_t batches,
    std::uint64_t items) noexcept
  {
    stats_owner_->read_batches_.fetch_add(batches, std::memory_order_relaxed);
    stats_owner_->read_items_.fetch_add(items, std::memory_order_relaxed);
  }

  inline void
  Client::add_queue_wait_stats(std::uint64_t wait_us) noexcept
  {
    stats_owner_->queue_wait_count_.fetch_add(1, std::memory_order_relaxed);
    stats_owner_->queue_wait_sum_us_.fetch_add(wait_us, std::memory_order_relaxed);
    update_max_(stats_owner_->queue_wait_max_us_, wait_us);
  }

  inline void
  Client::add_queue_timeout_stats() noexcept
  {
    stats_owner_->queue_timeout_count_.fetch_add(1, std::memory_order_relaxed);
  }

  inline void
  Client::add_response_wait_stats(std::uint64_t wait_us) noexcept
  {
    stats_owner_->response_wait_count_.fetch_add(1, std::memory_order_relaxed);
    stats_owner_->response_wait_sum_us_.fetch_add(wait_us, std::memory_order_relaxed);
    update_max_(stats_owner_->response_wait_max_us_, wait_us);
  }

  inline void
  Client::add_consumer_stream_write_stats(std::uint64_t wait_us) noexcept
  {
    stats_owner_->consumer_stream_write_enabled_.store(true, std::memory_order_relaxed);
    stats_owner_->consumer_stream_write_count_.fetch_add(1, std::memory_order_relaxed);
    stats_owner_->consumer_stream_write_sum_us_.fetch_add(wait_us, std::memory_order_relaxed);
    update_max_(stats_owner_->consumer_stream_write_max_us_, wait_us);
  }

  inline void
  Client::add_timing_coalesce_stats(std::uint64_t items) noexcept
  {
    stats_owner_->timing_coalesce_items_.fetch_add(
      items,
      std::memory_order_relaxed);
  }

  inline void
  Client::set_last_error(
    const std::string& endpoint,
    grpc::StatusCode code,
    const std::string& message,
    const char* source) noexcept
  {
    if (message.empty() || message == "inactive")
    {
      return;
    }

    try
    {
      std::lock_guard<std::mutex> lock(stats_owner_->last_error_lock_);
      stats_owner_->last_error_ = Stats::LastError{
        Generics::Time::get_time_of_day(),
        endpoint,
        static_cast<int>(code),
        message,
        source ? source : ""
      };
    }
    catch (...)
    {}
  }

  inline Stats
  Client::own_stats_() const noexcept
  {
    Stats stats;
    stats.write_batches = write_batches_.load(std::memory_order_relaxed);
    stats.write_items = write_items_.load(std::memory_order_relaxed);
    stats.read_batches = read_batches_.load(std::memory_order_relaxed);
    stats.read_items = read_items_.load(std::memory_order_relaxed);
    stats.input_items = input_items_.load(std::memory_order_relaxed);
    stats.completed_items = completed_items_.load(std::memory_order_relaxed);
    stats.completed_error_items =
      completed_error_items_.load(std::memory_order_relaxed);
    stats.queue_wait_count = queue_wait_count_.load(std::memory_order_relaxed);
    stats.queue_wait_sum_us = queue_wait_sum_us_.load(std::memory_order_relaxed);
    stats.queue_wait_max_us = queue_wait_max_us_.load(std::memory_order_relaxed);
    stats.queue_timeout_count = queue_timeout_count_.load(std::memory_order_relaxed);
    stats.response_wait_count = response_wait_count_.load(std::memory_order_relaxed);
    stats.response_wait_sum_us = response_wait_sum_us_.load(std::memory_order_relaxed);
    stats.response_wait_max_us = response_wait_max_us_.load(std::memory_order_relaxed);
    stats.timing_coalesce_items =
      timing_coalesce_items_.load(std::memory_order_relaxed);
    if (consumer_stream_write_enabled_.load(std::memory_order_relaxed))
    {
      stats.consumer_stream_write = Stats::ConsumerStreamWrite{
        consumer_stream_write_count_.load(std::memory_order_relaxed),
        consumer_stream_write_sum_us_.load(std::memory_order_relaxed),
        consumer_stream_write_max_us_.load(std::memory_order_relaxed)
      };
    }
    {
      std::lock_guard<std::mutex> lock(last_error_lock_);
      stats.last_error = last_error_;
    }
    return stats;
  }

  inline void
  Client::update_max_(
    std::atomic<std::uint64_t>& value,
    std::uint64_t candidate) noexcept
  {
    auto current = value.load(std::memory_order_relaxed);
    while (current < candidate &&
      !value.compare_exchange_weak(
        current,
        candidate,
        std::memory_order_relaxed))
    {}
  }

  inline
  InflightLimiter::InflightLimiter(std::optional<std::size_t> max_inflight)
    : max_inflight_(max_inflight)
  {}

  inline bool
  InflightLimiter::try_acquire() noexcept
  {
    return try_acquire(1);
  }

  inline bool
  InflightLimiter::try_acquire(std::size_t count) noexcept
  {
    if (!max_inflight_)
    {
      return true;
    }

    auto inflight_count = inflight_count_.load(std::memory_order_acquire);
    while (inflight_count < *max_inflight_)
    {
      if (inflight_count_.compare_exchange_weak(
            inflight_count,
            inflight_count + count,
            std::memory_order_acq_rel,
            std::memory_order_acquire))
      {
        return true;
      }
    }

    return false;
  }

  inline void
  InflightLimiter::acquire()
  {
    acquire(1);
  }

  inline void
  InflightLimiter::acquire(std::size_t count)
  {
    if (!max_inflight_)
    {
      return;
    }

    if (try_acquire(count))
    {
      return;
    }

    std::unique_lock lock(lock_);
    waiters_count_.fetch_add(1, std::memory_order_acq_rel);
    cv_.wait(lock, [this, count] { return try_acquire(count); });
    waiters_count_.fetch_sub(1, std::memory_order_acq_rel);
  }

  inline void
  InflightLimiter::release() noexcept
  {
    release(1);
  }

  inline void
  InflightLimiter::release(std::size_t count) noexcept
  {
    if (!max_inflight_)
    {
      return;
    }

    inflight_count_.fetch_sub(count, std::memory_order_acq_rel);
    if (waiters_count_.load(std::memory_order_acquire) != 0)
    {
      cv_.notify_all();
    }
  }

  inline std::size_t
  InflightLimiter::count() const noexcept
  {
    return inflight_count_.load(std::memory_order_acquire);
  }

  template<typename Stub>
  class SyncUnaryClient final : public virtual Client
  {
  public:
    template<typename StubFactory>
    SyncUnaryClient(
      const std::vector<std::shared_ptr<grpc::Channel>>& channels,
      bool enable_grpc_compression,
      StubFactory&& stub_factory)
      : enable_grpc_compression_(enable_grpc_compression)
    {
      if (channels.empty())
      {
        throw std::runtime_error("SyncUnaryClient requires at least one channel");
      }

      stubs_.reserve(channels.size());
      for (const auto& channel : channels)
      {
        stubs_.emplace_back(stub_factory(channel));
      }
    }

    template<typename Request, typename Response, typename Callback, typename Rpc>
    void call(
      const Request& request,
      Callback callback,
      Rpc&& rpc)
    {
      grpc::ClientContext context;
      Response response;
      if (!enable_grpc_compression_)
      {
        context.set_compression_algorithm(GRPC_COMPRESS_NONE);
      }

      const auto status = rpc(choose_stub_(), &context, request, &response);
      add_write_stats(1, 1);
      if (callback)
      {
        callback(status, response);
      }
    }

  private:
    Stub& choose_stub_()
    {
      const auto size = stubs_.size();
      const auto idx = next_stub_.fetch_add(1, std::memory_order_relaxed) % size;
      return *stubs_[idx];
    }

  private:
    std::vector<std::unique_ptr<Stub>> stubs_;
    const bool enable_grpc_compression_;
    std::atomic<std::size_t> next_stub_{0};
  };

  template<typename Stub>
  class AsyncUnaryClient final : public virtual Client
  {
  public:
    template<typename StubFactory>
    AsyncUnaryClient(
      const std::vector<std::shared_ptr<grpc::Channel>>& channels,
      std::size_t completion_queues_count,
      std::optional<std::size_t> max_inflight,
      bool enable_grpc_compression,
      StubFactory&& stub_factory)
      : limiter_(max_inflight),
        enable_grpc_compression_(enable_grpc_compression)
    {
      if (channels.empty())
      {
        throw std::runtime_error("AsyncUnaryClient requires at least one channel");
      }

      if (completion_queues_count == 0)
      {
        throw std::runtime_error("AsyncUnaryClient requires at least one completion queue");
      }

      stubs_.reserve(channels.size());
      for (const auto& channel : channels)
      {
        stubs_.emplace_back(stub_factory(channel));
      }

      completion_queues_.reserve(completion_queues_count);
      for (std::size_t i = 0; i < completion_queues_count; ++i)
      {
        completion_queues_.emplace_back(std::make_unique<grpc::CompletionQueue>());
      }

      completion_threads_.reserve(completion_queues_.size());
      for (auto& completion_queue : completion_queues_)
      {
        completion_threads_.emplace_back([cq = completion_queue.get()]() {
          void* tag = nullptr;
          bool ok = false;
          while (cq->Next(&tag, &ok))
          {
            std::unique_ptr<CompletionTag> completion_tag(
              static_cast<CompletionTag*>(tag));
            completion_tag->complete(ok);
          }
        });
      }
    }

    ~AsyncUnaryClient()
    {
      for (auto& completion_queue : completion_queues_)
      {
        completion_queue->Shutdown();
      }

      for (auto& thread : completion_threads_)
      {
        thread.join();
      }
    }

    template<typename Request, typename Response, typename Callback, typename PrepareRpc>
    void call(
      const Request& request,
      Callback callback,
      PrepareRpc&& prepare_rpc)
    {
      limiter_.acquire();
      auto call = std::make_unique<Call<Response, Callback>>();
      if (!enable_grpc_compression_)
      {
        call->context.set_compression_algorithm(GRPC_COMPRESS_NONE);
      }

      call->callback = std::move(callback);
      call->on_complete = [this]() { limiter_.release(); };
      call->rpc = prepare_rpc(
        choose_stub_(),
        &call->context,
        request,
        &choose_completion_queue_());
      call->rpc->StartCall();
      call->rpc->Finish(&call->response, &call->status, call.get());
      add_write_stats(1, 1);
      call.release();
    }

  private:
    struct CompletionTag
    {
      virtual ~CompletionTag() = default;
      virtual void complete(bool ok) = 0;
    };

    template<typename Response, typename Callback>
    struct Call final : CompletionTag
    {
      grpc::ClientContext context;
      Response response;
      grpc::Status status;
      std::unique_ptr<grpc::ClientAsyncResponseReader<Response>> rpc;
      Callback callback;
      std::function<void()> on_complete;

      void complete(bool ok) override
      {
        if (callback)
        {
          if (ok)
          {
            callback(status, response);
          }
          else
          {
            callback(
              grpc::Status(
                grpc::StatusCode::UNKNOWN,
                "completion queue event failed"),
              {});
          }
        }

        if (on_complete)
        {
          on_complete();
        }
      }
    };

    Stub& choose_stub_()
    {
      const auto size = stubs_.size();
      const auto idx = next_stub_.fetch_add(1, std::memory_order_relaxed) % size;
      return *stubs_[idx];
    }

    grpc::CompletionQueue& choose_completion_queue_()
    {
      const auto size = completion_queues_.size();
      const auto idx =
        next_completion_queue_.fetch_add(1, std::memory_order_relaxed) % size;
      return *completion_queues_[idx];
    }

  private:
    std::vector<std::unique_ptr<Stub>> stubs_;
    std::vector<std::unique_ptr<grpc::CompletionQueue>> completion_queues_;
    std::vector<std::thread> completion_threads_;
    InflightLimiter limiter_;
    const bool enable_grpc_compression_;
    std::atomic<std::size_t> next_stub_{0};
    std::atomic<std::size_t> next_completion_queue_{0};
  };
}
