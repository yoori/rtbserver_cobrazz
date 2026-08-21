#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
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
      if (std::hash<std::string_view>{}(key) % bucket_count == bucket_index)
      {
        return key;
      }
    }
  }

  void
  require(bool condition, const char* message)
  {
    if(!condition)
    {
      throw std::runtime_error(message);
    }
  }

  void
  test_activation_and_batch_threshold()
  {
    Queue queue(2, Generics::Time(1), 4);

    Queue::Operations inactive_operations;
    inactive_operations.emplace_back(make_operation(Queue::OT_GET, "inactive"));
    require(!queue.enqueue(std::move(inactive_operations)).accepted, "inactive enqueue accepted");

    queue.activate();

    Queue::Operations first_operations;
    first_operations.emplace_back(make_operation(Queue::OT_GET, "first"));
    const auto first_result = queue.enqueue(std::move(first_operations));
    require(first_result.accepted, "first enqueue rejected");
    require(first_result.counts.get == 1, "first enqueue counter mismatch");
    require(first_result.ready_state.has_value(), "first ready state is absent");
    require(first_result.ready_state->has_operation, "first operation is not ready-indexed");
    require(
      first_result.ready_state->ready_time > first_result.ready_state->enqueue_time,
      "single operation did not use max delay");

    Queue::Operations second_operations;
    second_operations.emplace_back(make_operation(Queue::OT_CHECK, "second"));
    const auto second_result = queue.enqueue(std::move(second_operations));
    require(second_result.accepted, "second enqueue rejected");
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
  test_key_serialization()
  {
    Queue queue(16, Generics::Time::ZERO, 4);
    queue.activate();

    Queue::Operations operations;
    operations.emplace_back(make_operation(Queue::OT_SAVE, "same-key"));
    operations.emplace_back(make_operation(Queue::OT_GET, "same-key"));
    operations.emplace_back(make_operation(Queue::OT_GET, "other-key"));
    const auto enqueue_result = queue.enqueue(std::move(operations));
    require(enqueue_result.accepted, "mixed enqueue rejected");
    require(enqueue_result.counts.save == 1, "write counter mismatch");
    require(enqueue_result.counts.get == 2, "read counter mismatch");

    Queue::Operations write_batch;
    Queue::SelectedKeys write_keys;
    queue.start_batch();
    const auto after_write_collect = queue.collect_batch(write_batch, write_keys);
    require(write_batch.size() == 1, "unexpected write batch size");
    require(Queue::is_write_operation(write_batch.front().type), "first batch is not write");
    require(after_write_collect.has_operation, "independent read was not left ready");
    require(!after_write_collect.write_operations, "ready batch has unexpected type");

    Queue::Operations read_batch;
    Queue::SelectedKeys read_keys;
    queue.start_batch();
    const auto after_read_collect = queue.collect_batch(read_batch, read_keys);
    require(read_batch.size() == 1, "read blocked by write was collected");
    require(read_batch.front().key == "other-key", "wrong independent read collected");
    require(!after_read_collect.has_operation, "blocked read unexpectedly ready");

    queue.complete_batch(read_batch);
    queue.finish_batch();
    read_batch.clear();
    read_keys.clear();

    const auto after_write_complete = queue.complete_batch(write_batch);
    queue.finish_batch();
    require(after_write_complete.has_operation, "same-key read was not unblocked");
    require(!after_write_complete.write_operations, "unblocked batch has unexpected type");

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
    queue.activate();

    Queue::Operations operations;
    operations.emplace_back(make_operation(Queue::OT_GET, "A"));
    operations.emplace_back(make_operation(Queue::OT_GET, "B"));
    operations.emplace_back(make_operation(Queue::OT_GET, "C"));
    operations.emplace_back(make_operation(Queue::OT_GET, "A"));
    operations.emplace_back(make_operation(Queue::OT_GET, "B"));
    require(queue.enqueue(std::move(operations)).accepted, "coalescing enqueue rejected");

    Queue::Operations batch;
    Queue::SelectedKeys selected_keys;
    queue.start_batch();
    const auto after_collect = queue.collect_batch(batch, selected_keys);
    require(batch.size() == 4, "same-key operations were not coalesced");
    require(selected_keys.size() == 2, "coalesced batch has unexpected key count");
    require(after_collect.has_operation, "unselected key disappeared after coalescing");

    unsigned long a_count = 0;
    unsigned long b_count = 0;
    for(const auto& operation : batch)
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
  test_write_group_tail()
  {
    Queue queue(16, Generics::Time::ZERO, 1);
    queue.activate();

    Queue::Operations operations;
    operations.emplace_back(make_operation(Queue::OT_SAVE, "save-before-touch"));
    operations.emplace_back(make_operation(Queue::OT_TOUCH, "save-before-touch"));
    operations.emplace_back(make_operation(Queue::OT_TOUCH, "touch-before-remove"));
    operations.emplace_back(make_operation(Queue::OT_REMOVE, "touch-before-remove"));
    operations.emplace_back(make_operation(Queue::OT_REMOVE, "remove-before-save"));
    operations.emplace_back(make_operation(Queue::OT_SAVE, "remove-before-save"));
    require(queue.enqueue(std::move(operations)).accepted, "write enqueue rejected");

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
  test_deactivate_removes_delay()
  {
    Queue queue(16, Generics::Time(10), 2);
    queue.activate();

    Queue::Operations operations;
    operations.emplace_back(make_operation(Queue::OT_GET, "key"));
    const auto enqueue_result = queue.enqueue(std::move(operations));
    require(enqueue_result.ready_state.has_value(), "delayed ready state is absent");
    require(
      enqueue_result.ready_state->ready_time > enqueue_result.ready_state->enqueue_time,
      "operation was not delayed");

    const auto stopped_state = queue.deactivate();
    require(stopped_state.has_operation, "pending operation disappeared on deactivate");
    require(
      stopped_state.ready_time == stopped_state.enqueue_time,
      "deactivate did not remove delay");

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
    queue.activate();

    Queue::Operations read_operations;
    read_operations.emplace_back(make_operation(Queue::OT_GET, "read"));
    require(queue.enqueue(std::move(read_operations)).accepted, "read enqueue rejected");

    Queue::Operations write_operations;
    write_operations.emplace_back(make_operation(Queue::OT_SAVE, "write"));
    require(queue.enqueue(std::move(write_operations)).accepted, "write enqueue rejected");

    Queue::Operations read_batch;
    Queue::SelectedKeys read_keys;
    queue.start_batch();
    const auto after_read_collect = queue.collect_batch(read_batch, read_keys);
    require(read_batch.size() == 1, "unexpected first batch size");
    require(!Queue::is_write_operation(read_batch.front().type), "older read was displaced");
    require(after_read_collect.has_operation, "write was not scheduled after read collection");
    require(after_read_collect.write_operations, "wrong second batch type");

    Queue::Operations write_batch;
    Queue::SelectedKeys write_keys;
    queue.start_batch();
    const auto after_write_collect = queue.collect_batch(write_batch, write_keys);
    require(write_batch.size() == 1, "unexpected second batch size");
    require(Queue::is_write_operation(write_batch.front().type), "write was not collected second");
    require(!after_write_collect.has_operation, "queue still has ready operations");

    queue.complete_batch(read_batch);
    queue.finish_batch();
    queue.complete_batch(write_batch);
    queue.finish_batch();
    require(queue.drained(), "fairness test queue is not drained");
  }

  void
  test_bucket_rotation()
  {
    constexpr std::size_t bucket_count = 4;
    Queue queue(1, Generics::Time::ZERO, bucket_count);
    queue.activate();

    std::vector<std::string> keys;
    keys.reserve(bucket_count);
    for (std::size_t i = 0; i < bucket_count; ++i)
    {
      keys.emplace_back(key_for_bucket(i, bucket_count));

      Queue::Operations operations;
      operations.emplace_back(make_operation(Queue::OT_GET, keys.back()));
      require(queue.enqueue(std::move(operations)).accepted, "bucket enqueue rejected");
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
}

int
main()
{
  try
  {
    test_activation_and_batch_threshold();
    test_key_serialization();
    test_same_key_coalescing();
    test_write_group_tail();
    test_deactivate_removes_delay();
    test_queue_fairness();
    test_bucket_rotation();
    std::cout << "RocksDBBatchingProcessorQueueTest: PASS" << std::endl;
    return 0;
  }
  catch(const std::exception& ex)
  {
    std::cerr << "RocksDBBatchingProcessorQueueTest: FAIL: " << ex.what() << std::endl;
    return 1;
  }
}
