#include <algorithm>

#include "SimpleRocksDBBatchingProcessorQueue.hpp"

namespace AdServer::ProfilingCommons::Test
{
  SimpleRocksDBBatchingProcessorQueue::SimpleRocksDBBatchingProcessorQueue(
    unsigned long batch_size,
    const Generics::Time& max_delay,
    unsigned long)
    : batch_size_(std::max(1UL, batch_size)),
      max_delay_(max_delay)
  {}

  void
  SimpleRocksDBBatchingProcessorQueue::activate() noexcept
  {
    accepting_.store(true, std::memory_order_release);
  }

  SimpleRocksDBBatchingProcessorQueue::ReadyState
  SimpleRocksDBBatchingProcessorQueue::deactivate() noexcept
  {
    std::lock_guard guard(lock_);
    accepting_.store(false, std::memory_order_release);
    return ready_state_i_();
  }

  SimpleRocksDBBatchingProcessorQueue::EnqueueResult
  SimpleRocksDBBatchingProcessorQueue::enqueue(Operations&& operations)
  {
    EnqueueResult result;
    if(operations.empty())
    {
      result.accepted = true;
      return result;
    }

    std::lock_guard guard(lock_);
    if(!accepting_.load(std::memory_order_acquire))
    {
      return result;
    }

    const bool was_empty = empty_i_();
    const std::size_t previous_read_size = read_operations_.size();
    const std::size_t previous_write_size = write_operations_.size();
    const Generics::Time now = Generics::Time::get_time_of_day();
    unsigned long operation_count = 0;

    auto it = operations.begin();
    while(it != operations.end())
    {
      auto current = it++;
      current->enqueue_time = now;
      account_operation_(result.counts, current->type);
      Operations& target = is_write_operation(current->type) ?
        write_operations_ : read_operations_;
      target.splice(target.end(), operations, current);
      ++operation_count;
    }

    pending_operations_.fetch_add(operation_count, std::memory_order_relaxed);
    result.accepted = true;

    const bool fills_read_batch = previous_read_size < batch_size_ &&
      read_operations_.size() >= batch_size_;
    const bool fills_write_batch = previous_write_size < batch_size_ &&
      write_operations_.size() >= batch_size_;
    if(was_empty || fills_read_batch || fills_write_batch)
    {
      result.ready_state = ready_state_i_();
    }

    return result;
  }

  void
  SimpleRocksDBBatchingProcessorQueue::start_batch() noexcept
  {
    active_workers_.fetch_add(1, std::memory_order_relaxed);
  }

  SimpleRocksDBBatchingProcessorQueue::ReadyState
  SimpleRocksDBBatchingProcessorQueue::collect_batch(
    Operations& batch,
    SelectedKeys& selected_keys) noexcept
  {
    std::lock_guard guard(lock_);
    const Operation* operation = oldest_ready_operation_i_();
    if(operation)
    {
      const bool collect_reads = !is_write_operation(operation->type);
      Operations& source = collect_reads ? read_operations_ : write_operations_;
      const unsigned long collected = collect_from_queue_i_(
        source,
        collect_reads,
        batch,
        selected_keys);
      pending_operations_.fetch_sub(collected, std::memory_order_relaxed);

      auto& in_flight_keys = collect_reads ? in_flight_read_keys_ : in_flight_write_keys_;
      for(const auto& batch_operation : batch)
      {
        ++in_flight_keys[batch_operation.key];
      }
    }

    return ready_state_i_();
  }

  SimpleRocksDBBatchingProcessorQueue::ReadyState
  SimpleRocksDBBatchingProcessorQueue::complete_batch(const Operations& batch) noexcept
  {
    std::lock_guard guard(lock_);
    auto& in_flight_keys =
      !batch.empty() && is_write_operation(batch.front().type) ?
      in_flight_write_keys_ : in_flight_read_keys_;
    for(const auto& operation : batch)
    {
      const auto it = in_flight_keys.find(operation.key);
      if(it != in_flight_keys.end() && --it->second == 0)
      {
        in_flight_keys.erase(it);
      }
    }
    return ready_state_i_();
  }

  void
  SimpleRocksDBBatchingProcessorQueue::finish_batch() noexcept
  {
    active_workers_.fetch_sub(1, std::memory_order_acq_rel);
  }

  bool
  SimpleRocksDBBatchingProcessorQueue::drained() const noexcept
  {
    return empty_i_() && active_workers_.load(std::memory_order_acquire) == 0;
  }

  void
  SimpleRocksDBBatchingProcessorQueue::account_operation_(
    OperationCounts& counts,
    OperationType type) const noexcept
  {
    switch(type)
    {
      case BaseQueue::OT_CHECK:
        ++counts.check;
        break;
      case BaseQueue::OT_GET:
        ++counts.get;
        break;
      case BaseQueue::OT_TOUCH:
        ++counts.touch;
        break;
      case BaseQueue::OT_SAVE:
        ++counts.save;
        break;
      case BaseQueue::OT_REMOVE:
        ++counts.remove;
        break;
    }
  }

  SimpleRocksDBBatchingProcessorQueue::ReadyState
  SimpleRocksDBBatchingProcessorQueue::ready_state_i_() noexcept
  {
    ReadyState result;
    result.generation = ++ready_generation_;
    const Operation* operation = oldest_ready_operation_i_();
    if(operation)
    {
      result.has_operation = true;
      result.write_operations = is_write_operation(operation->type);
      result.enqueue_time = operation->enqueue_time;
      result.ready_time = operation_ready_time_i_(*operation);
    }
    return result;
  }

  const SimpleRocksDBBatchingProcessorQueue::Operation*
  SimpleRocksDBBatchingProcessorQueue::oldest_ready_operation_i_() const noexcept
  {
    const Operation* read_operation = nullptr;
    for(const auto& operation : read_operations_)
    {
      if(in_flight_write_keys_.find(operation.key) == in_flight_write_keys_.end())
      {
        read_operation = &operation;
        break;
      }
    }

    const Operation* write_operation = nullptr;
    for(const auto& operation : write_operations_)
    {
      if(in_flight_read_keys_.find(operation.key) == in_flight_read_keys_.end() &&
        in_flight_write_keys_.find(operation.key) == in_flight_write_keys_.end())
      {
        write_operation = &operation;
        break;
      }
    }

    if(!read_operation)
    {
      return write_operation;
    }

    if(!write_operation)
    {
      return read_operation;
    }
    return read_operation->enqueue_time < write_operation->enqueue_time ?
      read_operation : write_operation;
  }

  Generics::Time
  SimpleRocksDBBatchingProcessorQueue::operation_ready_time_i_(
    const Operation& operation) const noexcept
  {
    const Operations& operations = is_write_operation(operation.type) ?
      write_operations_ : read_operations_;
    return !accepting_.load(std::memory_order_acquire) ||
      max_delay_ == Generics::Time::ZERO || operations.size() >= batch_size_ ?
      operation.enqueue_time : operation.enqueue_time + max_delay_;
  }

  unsigned long
  SimpleRocksDBBatchingProcessorQueue::collect_from_queue_i_(
    Operations& source,
    bool collect_reads,
    Operations& batch,
    SelectedKeys& selected_keys) noexcept
  {
    const std::size_t initial_batch_size = batch.size();
    auto it = source.begin();
    while(it != source.end())
    {
      const std::string_view key(it->key);
      const bool key_selected = selected_keys.find(key) != selected_keys.end();
      if(collect_reads && in_flight_write_keys_.find(it->key) != in_flight_write_keys_.end())
      {
        ++it;
        continue;
      }

      if(!collect_reads &&
        (in_flight_read_keys_.find(it->key) != in_flight_read_keys_.end() ||
        (!key_selected && in_flight_write_keys_.find(it->key) != in_flight_write_keys_.end())))
      {
        ++it;
        continue;
      }

      if(!key_selected)
      {
        if(selected_keys.size() >= batch_size_)
        {
          ++it;
          continue;
        }
        selected_keys.emplace(key);
      }

      auto current = it++;
      batch.splice(batch.end(), source, current);
    }
    return static_cast<unsigned long>(batch.size() - initial_batch_size);
  }

  bool
  SimpleRocksDBBatchingProcessorQueue::empty_i_() const noexcept
  {
    return pending_operations_.load(std::memory_order_acquire) == 0;
  }
}
