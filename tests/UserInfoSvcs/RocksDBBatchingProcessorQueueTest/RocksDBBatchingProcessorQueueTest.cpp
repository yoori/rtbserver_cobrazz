#include <chrono>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <ProfilingCommons/ProfileMap/RocksDBBatchingProcessorQueue.hpp>

namespace
{
  using Queue = AdServer::ProfilingCommons::RocksDBBatchingProcessorQueue;

  Queue::Operation
  make_operation(Queue::OperationType type, std::string key)
  {
    Queue::Operation operation;
    operation.type = type;
    operation.key = std::move(key);
    return operation;
  }

  std::string
  key_for_bucket(std::size_t bucket_index, std::size_t bucket_count)
  {
    for (unsigned long i = 0; ; ++i)
    {
      std::string key = "bucket-key-" + std::to_string(bucket_index) + "-" + std::to_string(i);
      if (Generics::StringViewHashAdapter(key).hash() % bucket_count == bucket_index)
      {
        return key;
      }
    }
  }

  void
  require(bool condition, const char* message)
  {
    if (!condition)
    {
      throw std::runtime_error(message);
    }
  }

  void
  test_string_view_hash_adapter()
  {
    Generics::StringHashAdapter owned_key(std::string("adapter-key"));
    const Generics::StringViewHashAdapter key_view(owned_key);
    require(key_view.text() == owned_key.text(), "hash adapter text mismatch");
    require(key_view.hash() == owned_key.hash(), "hash adapter hash mismatch");

    const Generics::StringHashAdapter copied_key(key_view.hash(), key_view.text());
    require(copied_key.text() == owned_key.text(), "owned adapter text mismatch");
    require(copied_key.hash() == owned_key.hash(), "owned adapter hash mismatch");
  }

  void
  test_batch_threshold()
  {
    Queue queue(2, Generics::Time(1), 4);

    Queue::Operations first_operations;
    first_operations.emplace_back(make_operation(Queue::OT_GET, "first"));
    const auto first_result = queue.enqueue(std::move(first_operations));
    require(first_result.counts.get == 1, "first enqueue counter mismatch");
    require(first_result.ready_state.has_value(), "first ready state is absent");
    require(first_result.ready_state->has_operation, "first operation is not ready-indexed");
    require(
      first_result.ready_state->ready_time > first_result.ready_state->enqueue_time,
      "single operation did not use max delay");

    Queue::Operations second_operations;
    second_operations.emplace_back(make_operation(Queue::OT_CHECK, "second"));
    const auto second_result = queue.enqueue(std::move(second_operations));
    require(second_result.counts.check == 1, "second enqueue counter mismatch");
    require(second_result.ready_state.has_value(), "threshold ready state is absent");
    require(
      second_result.ready_state->ready_time == second_result.ready_state->enqueue_time,
      "full batch was not promoted");

    Queue::Operations batch;
    Queue::SelectedKeys selected_keys;
    queue.start_batch();
    const auto after_collect = queue.collect_batch(batch, selected_keys);
    require(batch.size() == 2, "unexpected read batch size");
    require(selected_keys.size() == 2, "unexpected selected key count");
    require(!after_collect.has_operation, "collected operations remain ready");

    const auto after_complete = queue.complete_batch(batch);
    queue.finish_batch();
    require(!after_complete.has_operation, "completed batch remains ready");
    require(queue.drained(), "queue is not drained after complete");
  }

  void
  test_mixed_operations_use_common_threshold()
  {
    Queue queue(2, Generics::Time(10), 1);

    Queue::Operations read_operations;
    read_operations.emplace_back(make_operation(Queue::OT_GET, "read"));
    const auto read_result = queue.enqueue(std::move(read_operations));
    require(read_result.ready_state.has_value(), "mixed read ready state is absent");
    require(
      read_result.ready_state->ready_time > read_result.ready_state->enqueue_time,
      "mixed read did not use max delay");

    Queue::Operations write_operations;
    write_operations.emplace_back(make_operation(Queue::OT_SAVE, "write"));
    const auto write_result = queue.enqueue(std::move(write_operations));
    require(write_result.ready_state.has_value(), "mixed threshold ready state is absent");
    require(
      write_result.ready_state->ready_time == write_result.ready_state->enqueue_time,
      "mixed operations did not use common threshold");

    for (unsigned int i = 0; i < 2; ++i)
    {
      Queue::Operations batch;
      Queue::SelectedKeys selected_keys;
      queue.start_batch();
      queue.collect_batch(batch, selected_keys);
      require(batch.size() == 1, "mixed batch has unexpected size");
      queue.complete_batch(batch);
      queue.finish_batch();
    }

    require(queue.drained(), "mixed threshold queue is not drained");
  }

  void
  test_key_serialization()
  {
    constexpr std::size_t bucket_count = 4;
    const std::string same_key = "same-key";
    const std::size_t bucket_index =
      Generics::StringViewHashAdapter(same_key).hash() % bucket_count;
    const std::string other_key = key_for_bucket(bucket_index, bucket_count);

    Queue queue(16, Generics::Time::ZERO, bucket_count);

    Queue::Operations operations;
    operations.emplace_back(make_operation(Queue::OT_SAVE, same_key));
    operations.emplace_back(make_operation(Queue::OT_GET, same_key));
    operations.emplace_back(make_operation(Queue::OT_GET, other_key));
    const auto enqueue_result = queue.enqueue(std::move(operations));
    require(enqueue_result.counts.save == 1, "write counter mismatch");
    require(enqueue_result.counts.get == 2, "read counter mismatch");

    Queue::Operations write_batch;
    Queue::SelectedKeys write_keys;
    queue.start_batch();
    const auto after_write_collect = queue.collect_batch(write_batch, write_keys);
    require(write_batch.size() == 1, "unexpected write batch size");
    require(Queue::is_write_operation(write_batch.front().type), "first batch is not write");
    require(after_write_collect.has_operation, "independent read was not left ready");

    Queue::Operations read_batch;
    Queue::SelectedKeys read_keys;
    queue.start_batch();
    const auto after_read_collect = queue.collect_batch(read_batch, read_keys);
    require(read_batch.size() == 1, "read blocked by write was collected");
    require(read_batch.front().key.text() == other_key, "wrong independent read collected");
    require(after_read_collect.has_operation, "pending read check was not scheduled");

    Queue::Operations blocked_batch;
    Queue::SelectedKeys blocked_keys;
    queue.start_batch();
    const auto after_blocked_collect = queue.collect_batch(blocked_batch, blocked_keys);
    queue.finish_batch();
    require(blocked_batch.empty(), "blocked read was collected while write is in flight");
    require(!after_blocked_collect.has_operation, "blocked read check was rescheduled");

    const auto after_read_complete = queue.complete_batch(read_batch);
    queue.finish_batch();
    read_batch.clear();
    read_keys.clear();

    if (after_read_complete.has_operation)
    {
      queue.start_batch();
      const auto after_blocked_retry = queue.collect_batch(blocked_batch, blocked_keys);
      queue.finish_batch();
      require(blocked_batch.empty(), "blocked read was collected after unrelated completion");
      require(!after_blocked_retry.has_operation, "blocked read retry was rescheduled");
    }

    const auto after_write_complete = queue.complete_batch(write_batch);
    queue.finish_batch();
    require(after_write_complete.has_operation, "same-key read was not unblocked");

    queue.start_batch();
    const auto after_final_collect = queue.collect_batch(read_batch, read_keys);
    require(read_batch.size() == 1, "same-key read was not collected after write complete");
    require(!after_final_collect.has_operation, "queue still has ready operations");

    queue.complete_batch(read_batch);
    queue.finish_batch();
    require(queue.drained(), "serialized queue is not drained");
  }

  void
  test_same_key_coalescing()
  {
    Queue queue(2, Generics::Time::ZERO, 1);

    Queue::Operations operations;
    operations.emplace_back(make_operation(Queue::OT_GET, "A"));
    operations.emplace_back(make_operation(Queue::OT_GET, "B"));
    operations.emplace_back(make_operation(Queue::OT_GET, "C"));
    operations.emplace_back(make_operation(Queue::OT_GET, "A"));
    operations.emplace_back(make_operation(Queue::OT_GET, "B"));
    queue.enqueue(std::move(operations));

    Queue::Operations batch;
    Queue::SelectedKeys selected_keys;
    queue.start_batch();
    const auto after_collect = queue.collect_batch(batch, selected_keys);
    require(batch.size() == 4, "same-key operations were not coalesced");
    require(selected_keys.size() == 2, "coalesced batch has unexpected key count");
    require(after_collect.has_operation, "unselected key disappeared after coalescing");

    unsigned long a_count = 0;
    unsigned long b_count = 0;
    for (const auto& operation : batch)
    {
      a_count += operation.key == "A";
      b_count += operation.key == "B";
      require(operation.key != "C", "unselected key was added to coalesced batch");
    }
    require(a_count == 2 && b_count == 2, "coalesced operation counts mismatch");

    queue.complete_batch(batch);
    queue.finish_batch();
    selected_keys.clear();
    batch.clear();

    queue.start_batch();
    const auto after_final_collect = queue.collect_batch(batch, selected_keys);
    require(batch.size() == 1 && batch.front().key == "C", "remaining key mismatch");
    require(!after_final_collect.has_operation, "queue remains ready after final collection");
    queue.complete_batch(batch);
    queue.finish_batch();
    require(queue.drained(), "coalescing test queue is not drained");
  }

  void
  test_overlapping_reads_block_write()
  {
    Queue queue(1, Generics::Time::ZERO, 1);

    Queue::Operations first_read_operations;
    first_read_operations.emplace_back(make_operation(Queue::OT_GET, "same-key"));
    queue.enqueue(std::move(first_read_operations));

    Queue::Operations first_read_batch;
    Queue::SelectedKeys first_read_keys;
    queue.start_batch();
    queue.collect_batch(first_read_batch, first_read_keys);
    require(first_read_batch.size() == 1, "first read was not collected");

    Queue::Operations second_read_operations;
    second_read_operations.emplace_back(make_operation(Queue::OT_GET, "same-key"));
    queue.enqueue(std::move(second_read_operations));

    Queue::Operations second_read_batch;
    Queue::SelectedKeys second_read_keys;
    queue.start_batch();
    queue.collect_batch(second_read_batch, second_read_keys);
    require(second_read_batch.size() == 1, "second overlapping read was not collected");

    Queue::Operations write_operations;
    write_operations.emplace_back(make_operation(Queue::OT_SAVE, "same-key"));
    queue.enqueue(std::move(write_operations));

    Queue::Operations write_batch;
    Queue::SelectedKeys write_keys;
    queue.start_batch();
    queue.collect_batch(write_batch, write_keys);
    queue.finish_batch();
    require(write_batch.empty(), "write was collected while reads are in flight");

    const auto after_first_read = queue.complete_batch(first_read_batch);
    queue.finish_batch();
    require(after_first_read.has_operation, "pending write was not scheduled after first read");

    queue.start_batch();
    queue.collect_batch(write_batch, write_keys);
    queue.finish_batch();
    require(write_batch.empty(), "write was collected after only one read completed");

    const auto after_second_read = queue.complete_batch(second_read_batch);
    queue.finish_batch();
    require(after_second_read.has_operation, "write was not unblocked after both reads completed");

    queue.start_batch();
    queue.collect_batch(write_batch, write_keys);
    require(write_batch.size() == 1, "unblocked write was not collected");
    queue.complete_batch(write_batch);
    queue.finish_batch();
    require(queue.drained(), "overlapping read test queue is not drained");
  }

  void
  test_write_group_tail()
  {
    Queue queue(16, Generics::Time::ZERO, 1);

    Queue::Operations operations;
    operations.emplace_back(make_operation(Queue::OT_SAVE, "save-before-touch"));
    operations.emplace_back(make_operation(Queue::OT_TOUCH, "save-before-touch"));
    operations.emplace_back(make_operation(Queue::OT_TOUCH, "touch-before-remove"));
    operations.emplace_back(make_operation(Queue::OT_REMOVE, "touch-before-remove"));
    operations.emplace_back(make_operation(Queue::OT_REMOVE, "remove-before-save"));
    operations.emplace_back(make_operation(Queue::OT_SAVE, "remove-before-save"));
    queue.enqueue(std::move(operations));

    Queue::Operations batch;
    Queue::SelectedKeys selected_keys;
    queue.start_batch();
    queue.collect_batch(batch, selected_keys);

    Queue::OperationType save_before_touch = Queue::OT_TOUCH;
    Queue::OperationType touch_before_remove = Queue::OT_TOUCH;
    Queue::OperationType remove_before_save = Queue::OT_TOUCH;
    for (const auto& operation : batch)
    {
      if (operation.key == "save-before-touch")
      {
        save_before_touch = operation.type;
      }

      if (operation.key == "touch-before-remove")
      {
        touch_before_remove = operation.type;
      }

      if (operation.key == "remove-before-save")
      {
        remove_before_save = operation.type;
      }
    }

    require(save_before_touch == Queue::OT_SAVE, "touch displaced save at group tail");
    require(touch_before_remove == Queue::OT_REMOVE, "remove is not at group tail");
    require(remove_before_save == Queue::OT_SAVE, "latest save is not at group tail");

    queue.complete_batch(batch);
    queue.finish_batch();
    require(queue.drained(), "write tail test queue is not drained");
  }

  void
  test_flush_pending_removes_delay()
  {
    Queue queue(16, Generics::Time(10), 2);

    Queue::Operations operations;
    operations.emplace_back(make_operation(Queue::OT_GET, "key"));
    const auto enqueue_result = queue.enqueue(std::move(operations));
    require(enqueue_result.ready_state.has_value(), "delayed ready state is absent");
    require(
      enqueue_result.ready_state->ready_time > enqueue_result.ready_state->enqueue_time,
      "operation was not delayed");

    const auto stopped_state = queue.flush_pending();
    require(stopped_state.has_operation, "pending operation disappeared on flush");
    require(
      stopped_state.ready_time == stopped_state.enqueue_time,
      "flush did not remove delay");

    Queue::Operations batch;
    Queue::SelectedKeys selected_keys;
    queue.start_batch();
    queue.collect_batch(batch, selected_keys);
    queue.complete_batch(batch);
    queue.finish_batch();
    queue.wait_drained();
  }

  void
  test_queue_fairness()
  {
    Queue queue(16, Generics::Time::ZERO, 4);

    Queue::Operations read_operations;
    read_operations.emplace_back(make_operation(Queue::OT_GET, "read"));
    queue.enqueue(std::move(read_operations));

    Queue::Operations write_operations;
    write_operations.emplace_back(make_operation(Queue::OT_SAVE, "write"));
    queue.enqueue(std::move(write_operations));

    Queue::Operations first_batch;
    Queue::SelectedKeys first_keys;
    queue.start_batch();
    const auto after_first_collect = queue.collect_batch(first_batch, first_keys);
    require(first_batch.size() == 1, "unexpected first batch size");
    require(after_first_collect.has_operation, "second operation was not scheduled");

    Queue::Operations second_batch;
    Queue::SelectedKeys second_keys;
    queue.start_batch();
    const auto after_second_collect = queue.collect_batch(second_batch, second_keys);
    require(second_batch.size() == 1, "unexpected second batch size");
    require(
      Queue::is_write_operation(first_batch.front().type) !=
        Queue::is_write_operation(second_batch.front().type),
      "round-robin did not alternate operation types");
    require(!after_second_collect.has_operation, "queue still has ready operations");

    queue.complete_batch(first_batch);
    queue.finish_batch();
    queue.complete_batch(second_batch);
    queue.finish_batch();
    require(queue.drained(), "fairness test queue is not drained");
  }

  void
  test_bucket_rotation()
  {
    constexpr std::size_t bucket_count = 4;
    Queue queue(1, Generics::Time::ZERO, bucket_count);

    std::vector<std::string> keys;
    keys.reserve(bucket_count);
    for (std::size_t i = 0; i < bucket_count; ++i)
    {
      keys.emplace_back(key_for_bucket(i, bucket_count));

      Queue::Operations operations;
      operations.emplace_back(make_operation(Queue::OT_GET, keys.back()));
      queue.enqueue(std::move(operations));
    }

    for (std::size_t i = 0; i < bucket_count; ++i)
    {
      Queue::Operations batch;
      Queue::SelectedKeys selected_keys;
      queue.start_batch();
      queue.collect_batch(batch, selected_keys);
      require(batch.size() == 1, "unexpected rotating batch size");
      require(batch.front().key == keys[i], "bucket rotation order mismatch");
      queue.complete_batch(batch);
      queue.finish_batch();
    }

    require(queue.drained(), "rotating queue is not drained");
  }

  void
  test_min_enqueue_time_after_collect()
  {
    constexpr std::size_t bucket_count = 4;
    Queue queue(2, Generics::Time(10), bucket_count);

    for (std::size_t bucket_index = 0; bucket_index < 3; ++bucket_index)
    {
      Queue::Operations operations;
      operations.emplace_back(make_operation(
        Queue::OT_GET,
        key_for_bucket(bucket_index, bucket_count)));
      queue.enqueue(std::move(operations));
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    Queue::Operations batch;
    Queue::SelectedKeys selected_keys;
    queue.start_batch();
    const auto after_collect = queue.collect_batch(batch, selected_keys);
    require(batch.size() == 2, "unexpected delayed batch size");
    require(after_collect.has_operation, "remaining delayed operation was not scheduled");
    require(
      after_collect.enqueue_time > batch.back().enqueue_time,
      "minimum enqueue time still points to a collected operation");
    require(
      after_collect.ready_time == after_collect.enqueue_time + Generics::Time(10),
      "remaining operation lost max delay");

    queue.complete_batch(batch);
    queue.finish_batch();
    batch.clear();
    selected_keys.clear();

    queue.start_batch();
    queue.collect_batch(batch, selected_keys);
    require(batch.size() == 1, "remaining delayed operation was not collected");
    queue.complete_batch(batch);
    queue.finish_batch();
    require(queue.drained(), "delayed queue is not drained");
  }
}

int
main()
{
  try
  {
    test_string_view_hash_adapter();
    test_batch_threshold();
    test_mixed_operations_use_common_threshold();
    test_key_serialization();
    test_same_key_coalescing();
    test_overlapping_reads_block_write();
    test_write_group_tail();
    test_flush_pending_removes_delay();
    test_queue_fairness();
    test_bucket_rotation();
    test_min_enqueue_time_after_collect();
    std::cout << "RocksDBBatchingProcessorQueueTest: PASS" << std::endl;
    return 0;
  }
  catch(const std::exception& ex)
  {
    std::cerr << "RocksDBBatchingProcessorQueueTest: FAIL: " << ex.what() << std::endl;
    return 1;
  }
}
