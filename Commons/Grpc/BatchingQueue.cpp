#include <Commons/Grpc/BatchingQueue.hpp>

#include <algorithm>
#include <chrono>
#include <utility>

namespace AdServer::Grpc
{
  namespace
  {
    std::chrono::microseconds to_wait_duration(const Generics::Time& time)
    {
      return std::chrono::microseconds(std::max<long long>(0, time.microseconds()));
    }
  }

  BatchingQueue::BatchingQueue(BatchingOptions options)
    : options_(std::move(options))
  {
    const auto hot_buckets_count = std::max<std::size_t>(
      1,
      options_.hot_buckets_count);
    hot_buckets_.reserve(hot_buckets_count);
    for (std::size_t i = 0; i < hot_buckets_count; ++i)
    {
      hot_buckets_.emplace_back(std::make_unique<HotBucket>());
    }
  }

  BatchingQueue::~BatchingQueue() = default;

  bool
  BatchingQueue::enqueue_request(
    const char* full_method,
    std::string payload,
    std::function<void(const adserver::grpc::BatchResponseItem&)> callback)
  {
    if (!active())
    {
      adserver::grpc::BatchResponseItem item;
      item.set_status_code(grpc::StatusCode::UNAVAILABLE);
      item.set_status_message("inactive");
      if (callback)
      {
        callback(item);
      }
      return false;
    }

    auto request = std::make_shared<PendingRequest>();
    request->request_id = next_request_id_.fetch_add(
      1,
      std::memory_order_relaxed);
    request->enqueue_time = Generics::Time::get_time_of_day();
    request->full_method = full_method;
    request->payload = std::move(payload);
    request->callback = std::move(callback);

    std::function<void(const adserver::grpc::BatchResponseItem&)> inactive_callback;
    Batch ready_batch;
    bool notify_hot_waiter = false;
    bool try_flush_full_batch = false;
    {
      auto& bucket = *hot_buckets_[hot_bucket_index_(request->request_id)];
      std::lock_guard<std::mutex> lock(bucket.lock);
      if (!active())
      {
        inactive_callback = std::move(request->callback);
      }
      else
      {
        notify_hot_waiter = bucket.queue.empty();
        bucket.queue.emplace_back(std::move(request));
        try_flush_full_batch =
          hot_size_.fetch_add(1, std::memory_order_acq_rel) + 1 >=
            options_.max_batch_size;
      }
    }

    if (inactive_callback)
    {
      adserver::grpc::BatchResponseItem item;
      item.set_status_code(grpc::StatusCode::UNAVAILABLE);
      item.set_status_message("inactive");
      inactive_callback(item);
      return false;
    }

    if (try_flush_full_batch)
    {
      flush_hot_batch_if_full_(ready_batch);
    }

    if (!ready_batch.empty())
    {
      if (!active())
      {
        finish_batch_with_error_(
          ready_batch,
          grpc::StatusCode::UNAVAILABLE,
          "inactive");
        return false;
      }
      {
        std::lock_guard<std::mutex> lock(ready_lock_);
        ready_batches_.emplace_back(std::move(ready_batch));
      }
      hot_cv_.notify_one();
    }
    else if (notify_hot_waiter)
    {
      hot_cv_.notify_one();
    }

    return true;
  }

  bool
  BatchingQueue::pop_batch(Batch& batch)
  {
    while (active())
    {
      {
        std::lock_guard<std::mutex> lock(ready_lock_);
        if (!ready_batches_.empty())
        {
          batch = std::move(ready_batches_.front());
          ready_batches_.pop_front();
          return true;
        }
      }

      if (flush_hot_batch_if_due_(batch))
      {
        return true;
      }

      auto deadline = hot_deadline_();
      std::unique_lock<std::mutex> lock(hot_cv_lock_);
      if (deadline)
      {
        const auto now = Generics::Time::get_time_of_day();
        hot_cv_.wait_for(lock, to_wait_duration(*deadline - now), [this]() {
          return !active() ||
            has_ready_batch_() ||
            has_due_hot_batch_();
        });
      }
      else
      {
        hot_cv_.wait(lock, [this]() {
          return !active() ||
            has_ready_batch_() ||
            hot_size_.load(std::memory_order_acquire) != 0;
        });
      }
    }

    return false;
  }

  bool
  BatchingQueue::try_pop_batch(Batch& batch)
  {
    if (!active())
    {
      return false;
    }

    {
      std::lock_guard<std::mutex> lock(ready_lock_);
      if (!ready_batches_.empty())
      {
        batch = std::move(ready_batches_.front());
        ready_batches_.pop_front();
        return true;
      }
    }

    return flush_hot_batch_if_full_(batch) ||
      flush_hot_batch_if_due_(batch);
  }

  void
  BatchingQueue::return_batch_to_front(Batch&& batch)
  {
    if (batch.empty())
    {
      return;
    }

    if (!active())
    {
      finish_batch_with_error_(
        batch,
        grpc::StatusCode::UNAVAILABLE,
        "inactive");
      return;
    }

    {
      std::lock_guard<std::mutex> lock(ready_lock_);
      ready_batches_.emplace_front(std::move(batch));
    }
    hot_cv_.notify_one();
  }

  void
  BatchingQueue::fail_all_with_error(
    grpc::StatusCode status_code,
    const char* status_message)
  {
    finish_all_with_error_(status_code, status_message);
  }

  void
  BatchingQueue::notify_all()
  {
    std::lock_guard<std::mutex> lock(hot_cv_lock_);
    hot_cv_.notify_all();
  }

  std::optional<Generics::Time>
  BatchingQueue::next_deadline()
  {
    return hot_deadline_();
  }

  void
  BatchingQueue::activate_object_()
  {
  }

  void
  BatchingQueue::deactivate_object_()
  {
    // Hold the wait mutex while notifying to avoid losing the wakeup between
    // a pop_batch() predicate check and the thread actually entering wait().
    std::lock_guard<std::mutex> lock(hot_cv_lock_);
    hot_cv_.notify_all();
  }

  bool
  BatchingQueue::wait_more_()
  {
    return false;
  }

  void
  BatchingQueue::wait_object_()
  {
    finish_all_with_error_(grpc::StatusCode::UNAVAILABLE, "inactive");
  }

  inline std::size_t
  BatchingQueue::hot_bucket_index_(std::uint64_t request_id) const noexcept
  {
    return request_id % hot_buckets_.size();
  }

  bool
  BatchingQueue::flush_hot_batch_if_full_(Batch& batch)
  {
    if (hot_size_.load(std::memory_order_acquire) < options_.max_batch_size)
    {
      return false;
    }

    std::lock_guard<std::mutex> pop_lock(hot_pop_lock_);
    if (hot_size_.load(std::memory_order_acquire) < options_.max_batch_size)
    {
      return false;
    }

    batch.reserve(options_.max_batch_size);
    for (auto& bucket_ptr : hot_buckets_)
    {
      auto& bucket = *bucket_ptr;
      std::lock_guard<std::mutex> lock(bucket.lock);
      while (!bucket.queue.empty() && batch.size() < options_.max_batch_size)
      {
        batch.emplace_back(std::move(bucket.queue.front()));
        bucket.queue.pop_front();
      }

      if (batch.size() == options_.max_batch_size)
      {
        break;
      }
    }

    if (batch.size() != options_.max_batch_size)
    {
      hot_size_.fetch_sub(batch.size(), std::memory_order_acq_rel);
      return !batch.empty();
    }

    hot_size_.fetch_sub(batch.size(), std::memory_order_acq_rel);
    return true;
  }

  bool
  BatchingQueue::flush_hot_batch_if_due_(Batch& batch)
  {
    if (hot_size_.load(std::memory_order_acquire) == 0)
    {
      return false;
    }

    std::lock_guard<std::mutex> pop_lock(hot_pop_lock_);

    {
      std::lock_guard<std::mutex> lock(ready_lock_);
      if (!ready_batches_.empty())
      {
        return false;
      }
    }

    std::size_t oldest_bucket_index = 0;
    std::optional<Generics::Time> oldest_enqueue_time;
    for (std::size_t i = 0; i < hot_buckets_.size(); ++i)
    {
      auto& bucket = *hot_buckets_[i];
      std::lock_guard<std::mutex> lock(bucket.lock);
      if (bucket.queue.empty())
      {
        continue;
      }

      const auto enqueue_time = bucket.queue.front()->enqueue_time;
      if (!oldest_enqueue_time.has_value() ||
        enqueue_time < *oldest_enqueue_time)
      {
        oldest_enqueue_time = enqueue_time;
        oldest_bucket_index = i;
      }
    }

    if (!oldest_enqueue_time.has_value())
    {
      return false;
    }

    if (options_.max_batch_delay &&
      Generics::Time::get_time_of_day() <
        *oldest_enqueue_time + *options_.max_batch_delay)
    {
      return false;
    }

    batch.reserve(std::min(
      hot_size_.load(std::memory_order_acquire),
      options_.max_batch_size));
    for (std::size_t offset = 0;
      offset < hot_buckets_.size() && batch.size() < options_.max_batch_size;
      ++offset)
    {
      auto& bucket =
        *hot_buckets_[(oldest_bucket_index + offset) % hot_buckets_.size()];
      std::lock_guard<std::mutex> lock(bucket.lock);
      while (!bucket.queue.empty() && batch.size() < options_.max_batch_size)
      {
        batch.emplace_back(std::move(bucket.queue.front()));
        bucket.queue.pop_front();
      }
    }
    hot_size_.fetch_sub(batch.size(), std::memory_order_acq_rel);

    return !batch.empty();
  }

  bool
  BatchingQueue::has_due_hot_batch_()
  {
    if (hot_size_.load(std::memory_order_acquire) == 0)
    {
      return false;
    }

    const auto now = Generics::Time::get_time_of_day();
    for (auto& bucket_ptr : hot_buckets_)
    {
      auto& bucket = *bucket_ptr;
      std::lock_guard<std::mutex> lock(bucket.lock);
      if (!bucket.queue.empty() &&
        (!options_.max_batch_delay ||
          now >= bucket.queue.front()->enqueue_time +
            *options_.max_batch_delay))
      {
        return true;
      }
    }

    return false;
  }

  std::optional<Generics::Time>
  BatchingQueue::hot_deadline_()
  {
    std::optional<Generics::Time> deadline;
    if (!options_.max_batch_delay)
    {
      return deadline;
    }

    for (auto& bucket_ptr : hot_buckets_)
    {
      auto& bucket = *bucket_ptr;
      std::lock_guard<std::mutex> lock(bucket.lock);
      if (bucket.queue.empty())
      {
        continue;
      }

      const auto bucket_deadline =
        bucket.queue.front()->enqueue_time + *options_.max_batch_delay;
      if (!deadline.has_value() || bucket_deadline < *deadline)
      {
        deadline = bucket_deadline;
      }
    }

    return deadline;
  }

  bool
  BatchingQueue::has_ready_batch_()
  {
    std::lock_guard<std::mutex> lock(ready_lock_);
    return !ready_batches_.empty();
  }

  void
  BatchingQueue::finish_batch_with_error_(
    Batch& batch,
    grpc::StatusCode status_code,
    const char* status_message)
  {
    for (auto& request : batch)
    {
      adserver::grpc::BatchResponseItem item;
      item.set_request_id(request->request_id);
      item.set_status_code(status_code);
      item.set_status_message(status_message);
      if (request->callback)
      {
        request->callback(item);
      }
    }
    batch.clear();
  }

  void
  BatchingQueue::finish_all_with_error_(
    grpc::StatusCode status_code,
    const char* status_message)
  {
    std::vector<Batch> batches;

    for (auto& bucket_ptr : hot_buckets_)
    {
      auto& bucket = *bucket_ptr;
      std::lock_guard<std::mutex> lock(bucket.lock);
      while (!bucket.queue.empty())
      {
        Batch hot_batch;
        hot_batch.reserve(std::min(
          bucket.queue.size(),
          options_.max_batch_size));
        while (!bucket.queue.empty() &&
          hot_batch.size() < options_.max_batch_size)
        {
          hot_batch.emplace_back(std::move(bucket.queue.front()));
          bucket.queue.pop_front();
          hot_size_.fetch_sub(1, std::memory_order_acq_rel);
        }
        batches.emplace_back(std::move(hot_batch));
      }
    }

    {
      std::lock_guard<std::mutex> lock(ready_lock_);
      while (!ready_batches_.empty())
      {
        batches.emplace_back(std::move(ready_batches_.front()));
        ready_batches_.pop_front();
      }
    }

    for (auto& batch : batches)
    {
      finish_batch_with_error_(batch, status_code, status_message);
    }
  }
}
