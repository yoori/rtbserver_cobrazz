#include <Commons/Grpc/BatchingQueue.hpp>

#include <algorithm>
#include <cassert>
#include <utility>

namespace AdServer::Grpc
{
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

  BatchingQueue::EnqueueResult
  BatchingQueue::enqueue(Batch&& batch)
  {
    EnqueueResult result;
    if (batch.empty())
    {
      return result;
    }

    bool try_flush_full_batch = false;
    bool was_empty_before_push = false;
    const auto enqueue_time = Generics::Time::get_time_of_day();
    for (auto& operation : batch)
    {
      assert(operation);
      assert(operation->request);
      const auto bucket_id = next_bucket_id_.fetch_add(
        1,
        std::memory_order_relaxed);
      operation->enqueue_time = enqueue_time;
      auto& bucket = *hot_buckets_[hot_bucket_index_(bucket_id)];
      std::lock_guard<std::mutex> lock(bucket.lock);
      bucket.queue.emplace_back(std::move(operation));
      const auto hot_size =
        hot_size_.fetch_add(1, std::memory_order_acq_rel) + 1;
      was_empty_before_push = was_empty_before_push || hot_size == 1;
      try_flush_full_batch =
        try_flush_full_batch || hot_size >= options_.max_batch_size;
    }

    if (try_flush_full_batch)
    {
      flush_hot_batch_if_full_(result.ready_batch);
    }
    if (result.ready_batch.empty())
    {
      flush_hot_batch_if_due_(result.ready_batch);
    }

    result.was_empty_before_push = was_empty_before_push;
    return result;
  }

  bool
  BatchingQueue::try_pop_ready_batch(Batch& batch)
  {
    if (flush_hot_batch_if_full_(batch))
    {
      return true;
    }

    return flush_hot_batch_if_due_(batch);
  }

  bool
  BatchingQueue::try_pop_due_batch(Batch& batch)
  {
    return flush_hot_batch_if_due_(batch);
  }

  std::optional<Generics::Time>
  BatchingQueue::next_deadline()
  {
    return hot_deadline_();
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
}
