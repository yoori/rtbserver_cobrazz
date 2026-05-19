#include <cassert>
#include <chrono>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <Commons/Grpc/DistributedPartitionPool.hpp>

namespace
{
  struct TestObj
  {
    explicit TestObj(int id_val)
      : id(id_val)
    {}

    int id;
    bool enabled = true;
  };

  class TestController
  {
  public:
    using EndpointChunks = AdServer::Grpc::DistributedPartitionPool<
      class TestRef>::EndpointChunks;
    using EndpointChunksList = AdServer::Grpc::DistributedPartitionPool<
      class TestRef>::EndpointChunksList;

    void set_partition(
      std::string controller_url,
      EndpointChunksList refs)
    {
      std::lock_guard<std::mutex> lock(lock_);
      partitions_[std::move(controller_url)] = std::move(refs);
    }

    std::shared_ptr<TestObj> add_object(
      const std::string& endpoint,
      const int id)
    {
      auto object = std::make_shared<TestObj>(id);
      std::lock_guard<std::mutex> lock(lock_);
      objects_[endpoint] = object;
      return object;
    }

    std::shared_ptr<TestObj> object(const std::string& endpoint) const
    {
      std::lock_guard<std::mutex> lock(lock_);
      const auto it = objects_.find(endpoint);
      return it == objects_.end() ? nullptr : it->second;
    }

    std::optional<EndpointChunksList> resolve(const std::string& url)
    {
      std::lock_guard<std::mutex> lock(lock_);
      ++resolve_count_[url];
      const auto it = partitions_.find(url);
      if (it == partitions_.end())
      {
        return std::nullopt;
      }
      return it->second;
    }

    int resolve_count(const std::string& url) const
    {
      std::lock_guard<std::mutex> lock(lock_);
      const auto it = resolve_count_.find(url);
      return it == resolve_count_.end() ? 0 : it->second;
    }

    int total_resolve_count() const
    {
      std::lock_guard<std::mutex> lock(lock_);
      int result = 0;
      for (const auto& item : resolve_count_)
      {
        result += item.second;
      }
      return result;
    }

  private:
    mutable std::mutex lock_;
    std::map<std::string, EndpointChunksList> partitions_;
    std::map<std::string, std::shared_ptr<TestObj>> objects_;
    std::map<std::string, int> resolve_count_;
  };

  std::shared_ptr<TestController> current_controller;

  class TestRef:
    public Generics::SimpleActiveObject,
    public virtual AdServer::Grpc::Client
  {
  public:
    TestRef(
      const std::string& endpoint,
      std::shared_ptr<AdServer::Grpc::GrpcExecutor>,
      std::shared_ptr<AdServer::Commons::BoostAsioContextRunActiveObject>,
      AdServer::Grpc::BatchingOptions)
      : object_(current_controller->object(endpoint))
    {
      assert(object_);
    }

    std::shared_ptr<TestObj> object() const noexcept
    {
      return object_;
    }

  private:
    std::shared_ptr<TestObj> object_;
  };

  using Pool = AdServer::Grpc::DistributedPartitionPool<TestRef>;

  Pool::EndpointChunks
  endpoint_chunks(
    std::string endpoint,
    std::vector<unsigned long> chunks)
  {
    Pool::EndpointChunks result;
    result.endpoint = std::move(endpoint);
    result.chunk_ids = std::move(chunks);
    return result;
  }

  std::shared_ptr<Pool>
  make_pool(
    std::shared_ptr<TestController> controller,
    const Generics::Time& pool_timeout = Generics::Time(0, 50 * 1000),
    const Generics::Time& resolve_period = Generics::Time(0, 200 * 1000))
  {
    current_controller = controller;
    return std::make_shared<Pool>(
      "DistributedPartitionPoolTest",
      "Test",
      std::vector<std::string>{"controller-0", "controller-1"},
      AdServer::Grpc::BatchingOptions(),
      std::shared_ptr<AdServer::Grpc::GrpcExecutor>(),
      std::shared_ptr<AdServer::Commons::BoostAsioContextRunActiveObject>(),
      nullptr,
      [controller](const std::string& url)
        -> std::optional<Pool::EndpointChunksList>
      {
        return controller->resolve(url);
      },
      [](const std::string& key, const unsigned long partitions_number)
        -> unsigned long
      {
        return key == "primary-1" ? 1 % partitions_number : 0;
      },
      [](const std::string&, const unsigned long chunks_number)
        -> unsigned long
      {
        return 0 % chunks_number;
      },
      pool_timeout,
      resolve_period);
  }

  std::shared_ptr<TestController>
  make_controller()
  {
    auto controller = std::make_shared<TestController>();
    controller->add_object("ref-0", 0);
    controller->add_object("ref-1", 1);
    controller->set_partition(
      "controller-0",
      Pool::EndpointChunksList{endpoint_chunks("ref-0", {0})});
    controller->set_partition(
      "controller-1",
      Pool::EndpointChunksList{endpoint_chunks("ref-1", {0})});
    return controller;
  }

  int ref_id(const Pool::Ref& ref)
  {
    return ref->client->object()->id;
  }

  bool ref_enabled(const Pool::Ref& ref)
  {
    return ref->client->object()->enabled;
  }

  void test_resolve_is_not_more_frequent_than_period()
  {
    auto controller = make_controller();
    auto pool = make_pool(
      controller,
      Generics::Time(0, 50 * 1000),
      Generics::Time(0, 250 * 1000));
    pool->activate_object();

    assert(controller->resolve_count("controller-0") == 1);
    assert(controller->resolve_count("controller-1") == 1);

    for (int i = 0; i < 50; ++i)
    {
      auto ref = pool->get_ref("primary-0");
      assert(ref.has_value());
      assert(ref_id(*ref) == 0);
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    assert(controller->total_resolve_count() == 2);

    pool->deactivate_object();
    pool->wait_object();
  }

  void test_same_key_uses_same_primary_ref()
  {
    auto controller = make_controller();
    auto pool = make_pool(controller);
    pool->activate_object();

    for (int i = 0; i < 20; ++i)
    {
      auto ref = pool->get_ref("primary-0");
      assert(ref.has_value());
      assert(ref_id(*ref) == 0);
    }

    for (int i = 0; i < 20; ++i)
    {
      auto ref = pool->get_ref("primary-1");
      assert(ref.has_value());
      assert(ref_id(*ref) == 1);
    }

    pool->deactivate_object();
    pool->wait_object();
  }

  void test_primary_fallback_and_return_after_probe()
  {
    auto controller = make_controller();
    auto primary = controller->object("ref-0");
    assert(primary);

    auto pool = make_pool(
      controller,
      Generics::Time(0, 50 * 1000),
      Generics::Time(0, 500 * 1000));
    pool->activate_object();

    primary->enabled = false;
    {
      auto ref = pool->get_ref("primary-0");
      assert(ref.has_value());
      assert(ref_id(*ref) == 0);
      assert(!ref_enabled(*ref));
      ref->mark_as_bad(
        Generics::Time::get_time_of_day() + Generics::Time(0, 50 * 1000));
    }

    {
      auto ref = pool->get_ref("primary-0");
      assert(ref.has_value());
      assert(ref_id(*ref) == 1);
    }

    primary->enabled = true;
    std::this_thread::sleep_for(std::chrono::milliseconds(80));

    {
      auto ref = pool->get_ref("primary-0");
      assert(ref.has_value());
      assert(ref_id(*ref) == 0);
      assert(ref_enabled(*ref));
    }

    {
      auto ref = pool->get_ref("primary-0");
      assert(ref.has_value());
      assert(ref_id(*ref) == 0);
    }

    pool->deactivate_object();
    pool->wait_object();
  }
}

int
main()
{
  test_resolve_is_not_more_frequent_than_period();
  test_same_key_uses_same_primary_ref();
  test_primary_fallback_and_return_after_probe();

  std::cout << "DistributedPartitionPoolTest passed" << std::endl;
  return 0;
}
