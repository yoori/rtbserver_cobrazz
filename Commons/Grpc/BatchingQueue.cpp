#include <Commons/Grpc/BatchingQueue.hpp>

#include <algorithm>
#include <cassert>
#include <utility>

namespace AdServer::Grpc
{
  BatchingQueue::BatchingQueue(BatchingOptions options)
    : options_(std::move(options))
  {
    const auto hot_buckets_count = std::max<std::size_t>(1, options_.hot_buckets_count);
    hot_buckets_.reserve(hot_buckets_count);
    for (std::size_t i = 0; i < hot_buckets_count; ++i)
    {
      hot_buckets_.emplace_back(std::make_unique<HotBucket>());
    }
  }

  BatchingQueue::~BatchingQueue() = default;

  BatchingQueue::EnqueueResult
  BatchingQueue::enqueue(PendingRequestPtr request, const Generics::Time& enqueue_time)
  {
    EnqueueResult result;
    if (!request)
    {
      return result;
    }

    const auto bucket_id = next_bucket_id_.fetch_add(1, std::memory_order_relaxed);
    auto& bucket = *hot_buckets_[hot_bucket_index_(bucket_id)];
    {
      std::lock_guard<std::mutex> lock(bucket.lock);
      bucket.queue.emplace_back(PendingOperation{ enqueue_time, std::move(request)});
    }

    const auto hot_size = hot_size_.fetch_add(1, std::memory_order_acq_rel) + 1;
    result.was_empty_before_push = hot_size == 1;
    if (result.was_empty_before_push)
    {
      result.oldest_enqueue_time = enqueue_time;
    }

    if (hot_size >= options_.max_batch_size)
    {
      flush_hot_batch_if_full_(result.ready_batch);
    }
    result.queue_empty_after_enqueue = hot_size_.load(std::memory_order_acquire) == 0;

    return result;
  }

  bool
  BatchingQueue::try_pop_due_batch(
    Batch& batch,
    const Generics::Time& now,
    const Generics::Time& max_batch_delay)
  {
    if (hot_size_.load(std::memory_order_acquire) == 0)
    {
      return false;
    }

    std::lock_guard<std::mutex> pop_lock(hot_pop_lock_);

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

      const auto enqueue_time = bucket.queue.front().enqueue_time;
      if (!oldest_enqueue_time.has_value() || enqueue_time < *oldest_enqueue_time)
      {
        oldest_enqueue_time = enqueue_time;
        oldest_bucket_index = i;
      }
    }

    if (!oldest_enqueue_time.has_value())
    {
      return false;
    }

    if (now < *oldest_enqueue_time + max_batch_delay)
    {
      return false;
    }

    const auto reserved_items = std::min(
      hot_size_.load(std::memory_order_acquire),
      options_.max_batch_size);
    pop_reserved_items_(batch, reserved_items, oldest_bucket_index);
    return !batch.empty();
  }

  std::optional<Generics::Time>
  BatchingQueue::oldest_enqueue_time()
  {
    return oldest_enqueue_time_();
  }

  std::size_t
  BatchingQueue::size() const noexcept
  {
    return hot_size_.load(std::memory_order_acquire);
  }

  std::vector<BatchingQueue::Batch>
  BatchingQueue::drain_all()
  {
    std::vector<Batch> batches;

    for (auto& bucket_ptr : hot_buckets_)
    {
      auto& bucket = *bucket_ptr;
      std::lock_guard<std::mutex> lock(bucket.lock);
      while (!bucket.queue.empty())
      {
        Batch hot_batch;
        hot_batch.reserve(std::min(bucket.queue.size(), options_.max_batch_size));
        while (!bucket.queue.empty() && hot_batch.size() < options_.max_batch_size)
        {
          hot_batch.emplace_back(std::move(bucket.queue.front()));
          bucket.queue.pop_front();
        }
        batches.emplace_back(std::move(hot_batch));
      }
    }
    hot_size_.store(0, std::memory_order_release);

    return batches;
  }

  inline std::size_t
  BatchingQueue::hot_bucket_index_(std::uint64_t bucket_id) const noexcept
  {
    return bucket_id % hot_buckets_.size();
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

    pop_reserved_items_(batch, options_.max_batch_size, 0);
    return !batch.empty();
  }

  std::size_t
  BatchingQueue::pop_reserved_items_(
    Batch& batch,
    std::size_t count,
    std::size_t start_bucket_index)
  {
    const auto old_batch_size = batch.size();
    batch.reserve(count);
    for (std::size_t offset = 0; offset < hot_buckets_.size() && batch.size() < count; ++offset)
    {
      const auto bucket_index = (start_bucket_index + offset) % hot_buckets_.size();
      auto& bucket = *hot_buckets_[bucket_index];
      std::lock_guard<std::mutex> lock(bucket.lock);
      while (!bucket.queue.empty() && batch.size() < count)
      {
        batch.emplace_back(std::move(bucket.queue.front()));
        bucket.queue.pop_front();
      }
    }

    const auto popped_items = batch.size() - old_batch_size;
    hot_size_.fetch_sub(popped_items, std::memory_order_acq_rel);
    return popped_items;
  }

  std::optional<Generics::Time>
  BatchingQueue::oldest_enqueue_time_()
  {
    std::optional<Generics::Time> oldest_enqueue_time;

    for (auto& bucket_ptr : hot_buckets_)
    {
      auto& bucket = *bucket_ptr;
      std::lock_guard<std::mutex> lock(bucket.lock);
      if (bucket.queue.empty())
      {
        continue;
      }

      const auto enqueue_time = bucket.queue.front().enqueue_time;
      if (!oldest_enqueue_time.has_value() || enqueue_time < *oldest_enqueue_time)
      {
        oldest_enqueue_time = enqueue_time;
      }
    }

    return oldest_enqueue_time;
  }
}
