#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

#include <Commons/Grpc/BatchRequestBuffer.hpp>

namespace
{
  using AdServer::Grpc::Detail::BatchRequestBuffer;
  using adserver::grpc::BatchRequest;

  void require(bool condition, const char* message)
  {
    if (!condition)
    {
      throw std::runtime_error(message);
    }
  }

  void test_reuse_and_clear()
  {
    BatchRequestBuffer buffer;
    buffer.clear();
    auto& first = buffer.get();
    first.set_batch_id(42);
    first.add_response_time_steps_us(1000);
    first.add_response_time_steps_us(2000);
    auto* first_item = first.add_items();
    first_item->set_request_id(42);
    first_item->set_full_method("/test.First");
    first_item->set_payload(std::string(4096, 'a'));
    first.add_items()->set_payload(std::string(1024, 'b'));

    buffer.clear();
    auto& second = buffer.get();
    require(&second == &first, "small batches must reuse their message");
    require(second.ByteSizeLong() == 0, "old batch fields must be cleared");
    require(second.items().ClearedCount() == 2, "item storage must be retained");

    auto* second_item = second.add_items();
    require(second_item == first_item, "item objects must be reused");
    require(second_item->ByteSizeLong() == 0, "reused item must have no stale fields");
    require(second_item->payload().capacity() >= 4096, "string capacity must be retained");
    second.set_batch_id(43);
    second_item->set_request_id(43);
    second_item->set_full_method("/test.Second");
    second_item->set_payload("new payload");

    BatchRequest expected;
    expected.set_batch_id(43);
    auto* expected_item = expected.add_items();
    expected_item->set_request_id(43);
    expected_item->set_full_method("/test.Second");
    expected_item->set_payload("new payload");
    require(second.SerializeAsString() == expected.SerializeAsString(),
      "reused batch must serialize identically to a fresh batch");
  }

  void test_large_string_capacity()
  {
    BatchRequestBuffer buffer(16 * 1024);
    auto& request = buffer.get();
    std::string payload(64 * 1024, 'a');
    payload.resize(1);
    request.add_items()->set_payload(std::move(payload));
    require(request.ByteSizeLong() < 100, "test must use a small wire payload");
    require(request.GetArena()->SpaceAllocated() < 16 * 1024,
      "large string buffer must be outside arena blocks");

    buffer.clear();
    auto& next = buffer.get();
    require(next.items().ClearedCount() == 0, "oversized string capacity must be released");
    require(next.ByteSizeLong() == 0, "reset buffer must be empty");
  }

  void test_inactive_items_count_towards_limit()
  {
    BatchRequestBuffer buffer(16 * 1024);
    auto& first = buffer.get();
    first.add_items()->set_payload(std::string(6000, 'a'));
    first.add_items()->set_payload(std::string(6000, 'b'));
    buffer.clear();

    auto& second = buffer.get();
    require(second.items().ClearedCount() == 2, "first batch must fit the retention limit");
    second.add_items()->set_payload(std::string(11000, 'c'));
    require(second.items_size() == 1 && second.items().ClearedCount() == 1,
      "test must retain an inactive item from the previous batch");
    buffer.clear();

    require(buffer.get().items().ClearedCount() == 0,
      "inactive string buffers must count towards the retention limit");
  }
}

int main()
{
  try
  {
    test_reuse_and_clear();
    test_large_string_capacity();
    test_inactive_items_count_towards_limit();
    std::cout << "BatchRequestBufferTest passed\n";
    return 0;
  }
  catch (const std::exception& ex)
  {
    std::cerr << "BatchRequestBufferTest: " << ex.what() << '\n';
    return 1;
  }
}
