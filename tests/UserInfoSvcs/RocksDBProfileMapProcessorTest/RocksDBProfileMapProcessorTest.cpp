#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

#include <Generics/MemBuf.hpp>
#include <Generics/Time.hpp>
#include <ProfilingCommons/ProfileMap/RocksDBBatchingProfileMap.hpp>
#include <ProfilingCommons/ProfileMap/RocksDBProfileMapProcessor.hpp>

namespace
{
  using Processor =
    AdServer::ProfilingCommons::RocksDBProfileMapProcessor;
  using ProfileMap =
    AdServer::ProfilingCommons::RocksDBBatchingProfileMap<std::string>;

  Generics::ConstSmartMemBuf_var
  make_profile(const std::string& value)
  {
    return Generics::ConstSmartMemBuf_var(
      new Generics::ConstSmartMemBuf(value.data(), value.size()));
  }

  bool
  wait_count(
    std::condition_variable& condition,
    std::mutex& lock,
    const std::atomic<unsigned long>& count,
    unsigned long expected,
    std::chrono::milliseconds timeout = std::chrono::seconds(10))
  {
    std::unique_lock<std::mutex> guard(lock);
    return condition.wait_for(
      guard,
      timeout,
      [&count, expected]()
      {
        return count.load(std::memory_order_relaxed) == expected;
      });
  }

  unsigned long
  rdb_batch_thread_count()
  {
    unsigned long result = 0;
    for(const auto& entry : std::filesystem::directory_iterator("/proc/self/task"))
    {
      std::ifstream name_file(entry.path() / "comm");
      std::string name;
      std::getline(name_file, name);
      if(name.rfind("rdb-batch", 0) == 0)
      {
        ++result;
      }
    }
    return result;
  }
}

int
main()
{
  std::string root_template = "/tmp/RocksDBProfileMapProcessorTest.XXXXXX";
  const char* const root_name = ::mkdtemp(root_template.data());
  if(!root_name)
  {
    std::cerr << "RocksDBProfileMapProcessorTest: FAIL: mkdtemp failed" << std::endl;
    return 1;
  }
  const std::filesystem::path root(root_name);

  auto processor = std::make_shared<Processor>(1);
  std::unique_ptr<ProfileMap> first;
  std::unique_ptr<ProfileMap> second;

  try
  {
    processor->activate_object();
    first = std::make_unique<ProfileMap>(
      processor,
      String::SubString((root / "first").string()),
      Generics::Time::ZERO,
      32,
      Generics::Time(2),
      true);
    second = std::make_unique<ProfileMap>(
      processor,
      String::SubString((root / "second").string()),
      Generics::Time::ZERO,
      32,
      Generics::Time::ZERO,
      true);
    first->activate_object();
    second->activate_object();

    for(unsigned int i = 0; i < 100 && rdb_batch_thread_count() != 1; ++i)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    if(rdb_batch_thread_count() != 1)
    {
      throw std::runtime_error("shared processor worker count mismatch");
    }

    constexpr unsigned long operations_per_map = 500;
    std::atomic<unsigned long> first_done{0};
    std::atomic<unsigned long> second_done{0};
    std::atomic<unsigned long> isolation_done{0};
    std::atomic<unsigned long> errors{0};
    std::mutex completion_lock;
    std::condition_variable completion_condition;

    auto isolation_profile = make_profile("isolation");
    first->save_profile_async(
      "isolation",
      isolation_profile.in(),
      Generics::Time::get_time_of_day());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    const auto enqueue_started = std::chrono::steady_clock::now();
    second->save_profile_async(
      "isolation",
      isolation_profile.in(),
      Generics::Time::get_time_of_day(),
      [&](std::optional<std::string> error)
      {
        if(error)
        {
          errors.fetch_add(1, std::memory_order_relaxed);
        }
        isolation_done.fetch_add(1, std::memory_order_relaxed);
        completion_condition.notify_all();
      });
    const auto enqueue_elapsed = std::chrono::steady_clock::now() - enqueue_started;
    if(enqueue_elapsed >= std::chrono::milliseconds(500))
    {
      throw std::runtime_error("one map max_delay blocked another map enqueue");
    }
    if(!wait_count(
      completion_condition,
      completion_lock,
      isolation_done,
      1,
      std::chrono::seconds(1)))
    {
      throw std::runtime_error("one map max_delay blocked another ready map");
    }

    for(unsigned long i = 0; i < operations_per_map; ++i)
    {
      auto first_profile = make_profile("first-value-" + std::to_string(i));
      first->save_profile_async(
        "first-key-" + std::to_string(i),
        first_profile.in(),
        Generics::Time::get_time_of_day(),
        [&](std::optional<std::string> error)
        {
          if(error)
          {
            errors.fetch_add(1, std::memory_order_relaxed);
          }
          first_done.fetch_add(1, std::memory_order_relaxed);
          completion_condition.notify_all();
        });

      auto second_profile = make_profile("second-value-" + std::to_string(i));
      second->save_profile_async(
        "second-key-" + std::to_string(i),
        second_profile.in(),
        Generics::Time::get_time_of_day(),
        [&](std::optional<std::string> error)
        {
          if(error)
          {
            errors.fetch_add(1, std::memory_order_relaxed);
          }
          second_done.fetch_add(1, std::memory_order_relaxed);
          completion_condition.notify_all();
        });
    }

    first->deactivate_object();
    first->wait_object();
    if(first_done.load(std::memory_order_relaxed) != operations_per_map)
    {
      throw std::runtime_error("wait_unregister returned before callbacks completed");
    }

    bool rejected = false;
    try
    {
      auto profile = make_profile("rejected");
      first->save_profile_async(
        "rejected",
        profile.in(),
        Generics::Time::get_time_of_day());
    }
    catch(const eh::Exception&)
    {
      rejected = true;
    }
    if(!rejected)
    {
      throw std::runtime_error("operation was accepted after unregister");
    }
    first.reset();

    if(!wait_count(
      completion_condition,
      completion_lock,
      second_done,
      operations_per_map))
    {
      throw std::runtime_error("second map operations timed out");
    }

    auto profile = make_profile("still-active");
    second->save_profile(
      "still-active",
      profile.in(),
      Generics::Time::get_time_of_day());
    const auto loaded = second->get_profile("still-active");
    if(!loaded.in() || loaded->membuf().size() != std::string("still-active").size())
    {
      throw std::runtime_error("second map stopped with the first map");
    }

    second->deactivate_object();
    second->wait_object();
    second.reset();

    auto third = std::make_unique<ProfileMap>(
      processor,
      String::SubString((root / "third").string()),
      Generics::Time::ZERO,
      8,
      Generics::Time::ZERO,
      true);
    third->activate_object();
    profile = make_profile("third");
    third->save_profile(
      "third",
      profile.in(),
      Generics::Time::get_time_of_day());
    third->deactivate_object();
    third->wait_object();
    third.reset();

    processor->deactivate_object();
    processor->wait_object();

    processor = std::make_shared<Processor>(4);
    processor->activate_object();
    first = std::make_unique<ProfileMap>(
      processor,
      String::SubString((root / "same-key").string()),
      Generics::Time::ZERO,
      128,
      Generics::Time(0, 1000),
      true);
    first->activate_object();

    constexpr unsigned long same_key_operations = 10000;
    constexpr unsigned long same_key_inflight = 1000;
    std::atomic<unsigned long> same_key_sent{0};
    std::atomic<unsigned long> same_key_done{0};
    std::function<void()> enqueue_same_key;
    enqueue_same_key = [&]()
    {
      const unsigned long index =
        same_key_sent.fetch_add(1, std::memory_order_relaxed);
      if(index >= same_key_operations)
      {
        return;
      }

      first->get_profile_async(
        "shared",
        [&, index](
          Generics::ConstSmartMemBuf_var,
          std::optional<std::string> error)
        {
          if(error)
          {
            errors.fetch_add(1, std::memory_order_relaxed);
          }

          auto write_profile = make_profile("value-" + std::to_string(index));
          first->save_profile_async(
            "shared",
            write_profile.in(),
            Generics::Time::get_time_of_day(),
            [&, write_profile](std::optional<std::string> error)
            {
              if(error)
              {
                errors.fetch_add(1, std::memory_order_relaxed);
              }
              same_key_done.fetch_add(1, std::memory_order_relaxed);
              completion_condition.notify_all();
              enqueue_same_key();
            });
        });
    };

    for(unsigned long i = 0; i < same_key_inflight; ++i)
    {
      enqueue_same_key();
    }

    if(!wait_count(
      completion_condition,
      completion_lock,
      same_key_done,
      same_key_operations))
    {
      throw std::runtime_error("same-key operations timed out");
    }

    first->deactivate_object();
    first->wait_object();
    first.reset();
    processor->deactivate_object();
    processor->wait_object();

    if(errors.load(std::memory_order_relaxed) != 0)
    {
      throw std::runtime_error("background operations failed");
    }

    std::filesystem::remove_all(root);
    std::cout << "RocksDBProfileMapProcessorTest: PASS" << std::endl;
    return 0;
  }
  catch(const std::exception& ex)
  {
    if(first && first->active())
    {
      first->deactivate_object();
      first->wait_object();
    }
    if(second && second->active())
    {
      second->deactivate_object();
      second->wait_object();
    }
    if(processor->active())
    {
      processor->deactivate_object();
      processor->wait_object();
    }
    std::filesystem::remove_all(root);
    std::cerr << "RocksDBProfileMapProcessorTest: FAIL: " << ex.what() << std::endl;
    return 1;
  }
}
