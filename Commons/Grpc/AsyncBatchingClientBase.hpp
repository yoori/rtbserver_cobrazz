#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <grpcpp/support/status.h>

#include <Generics/CompositeActiveObject.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

#include <Commons/Grpc/BatchingStreamBase.hpp>
#include <Commons/Grpc/GrpcClient.hpp>
#include <Commons/Grpc/GrpcExecutor.hpp>

namespace AdServer::Grpc
{
  inline const grpc::Status NO_ACTIVE_BATCHING_STREAMS_STATUS(
    grpc::StatusCode::UNAVAILABLE,
    "no active batching streams");

  class AsyncBatchingClientBase
    : public Generics::CompositeActiveObject,
      public virtual AdServer::Grpc::Client,
      public virtual ReferenceCounting::AtomicImpl
  {
  protected:
    AsyncBatchingClientBase(
      const std::string& endpoint,
      std::size_t channels_number,
      AdServer::Grpc::GrpcExecutor* grpc_executor,
      AdServer::Grpc::BatchingOptions options = {});

    ~AsyncBatchingClientBase() override;

    AdServer::Grpc::Stats stats() const noexcept override;

    template<typename Request, typename Response, typename Callback>
    void enqueue_request_(
      const char* full_method,
      const Request& request,
      Callback callback,
      const char* parse_error_message);

  private:
    using BatchingStream_var =
      ReferenceCounting::SmartPtr<AdServer::Grpc::BatchingStreamBase>;
    using BatchingQueue_var =
      ReferenceCounting::SmartPtr<AdServer::Grpc::BatchingQueue>;

    void activate_object_() override;
    void deactivate_object_() override;
    void wait_object_() override;

    BatchingStream_var make_stream_();
    BatchingStreamBase* acquire_stream_();
    void release_stream_(BatchingStreamBase* stream) noexcept;
    void coalesce_loop_();
    void handle_stream_ready_(BatchingStreamBase* stream);
    void handle_stream_closed_(BatchingStreamBase* stream) noexcept;
    bool fail_pending_if_no_streams_() noexcept;
    void update_max_streams_(std::size_t streams_count) noexcept;
    void release_stream_() noexcept;

  private:
    const std::string endpoint_;
    const AdServer::Grpc::BatchingOptions options_;
    const std::size_t max_streams_;
    AdServer::Grpc::GrpcExecutor_var grpc_executor_;
    BatchingQueue_var batching_queue_;
    std::vector<BatchingStream_var> streams_;
    mutable std::mutex streams_registry_lock_;
    std::deque<BatchingStreamBase*> available_streams_;
    mutable std::mutex streams_lock_;
    std::condition_variable streams_cv_;
    std::vector<std::thread> coalesce_threads_;
    AdServer::Grpc::InflightLimiter inflight_limiter_;
    std::atomic<std::size_t> outstanding_requests_{0};
    std::atomic<unsigned int> next_queue_index_{0};
    std::atomic<std::size_t> up_streams_{0};
    std::atomic<std::size_t> stream_start_attempts_{0};
    std::atomic<std::uint64_t> max_streams_seen_{0};
  };

  template<typename Request, typename Response, typename Callback>
  void
  AsyncBatchingClientBase::enqueue_request_(
    const char* full_method,
    const Request& request,
    Callback callback,
    const char* parse_error_message)
  {
    if (!active())
    {
      if (callback)
      {
        callback(
          NO_ACTIVE_BATCHING_STREAMS_STATUS,
          {});
      }
      return;
    }

    inflight_limiter_.acquire();
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
          callback(
            grpc::Status(
              grpc::StatusCode::UNAVAILABLE,
              "max outstanding requests reached"),
            {});
        }
        return;
      }
    }

    const bool enqueued = batching_queue_->enqueue_request(
      full_method,
      request.SerializeAsString(),
      [
        this,
        callback = std::move(callback),
        parse_error_message
      ](const auto& batch_response) mutable
      {
        const auto status = grpc::Status(
          static_cast<grpc::StatusCode>(batch_response.status_code()),
          batch_response.status_message());

        Response response;
        if (status.ok() && !response.ParseFromString(batch_response.payload()))
        {
          if (callback)
          {
            callback(
              grpc::Status(grpc::StatusCode::INTERNAL, parse_error_message),
              {});
          }
          release_stream_();
          return;
        }

        if (callback)
        {
          callback(status, response);
        }
        release_stream_();
      });
    if (enqueued)
    {
      streams_cv_.notify_one();
    }
  }
}
