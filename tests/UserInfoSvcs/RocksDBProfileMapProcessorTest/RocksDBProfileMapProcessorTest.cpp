#include <atomic>
#include <chrono>
#include <csignal>
#include <condition_variable>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

#include <rocksdb/env.h>

#include <Commons/AsyncMutex.hpp>
#include <Commons/Coro/StartableAwaitable.hpp>
#include <Commons/ExecutorPool.hpp>
#include <Commons/OrderedAsyncTaskWindow.hpp>
#include <Generics/MemBuf.hpp>
#include <Generics/Time.hpp>
#include <ProfilingCommons/ProfileMap/RocksDBBatchingProfileMap.hpp>
#include <ProfilingCommons/ProfileMap/RocksDBOptions.hpp>
#include <ProfilingCommons/ProfileMap/RocksDBProfileMapProcessor.hpp>
#include <ProfilingCommons/ProfileMap/TransactionProfileMap.hpp>

namespace
{
  using Processor = AdServer::ProfilingCommons::RocksDBProfileMapProcessor;
  using ProfileMap = AdServer::ProfilingCommons::RocksDBBatchingProfileMap<std::string>;
  using TransactionKey = Generics::StringHashAdapter;
  using TransactionBaseMap =
    AdServer::ProfilingCommons::RocksDBBatchingProfileMap<TransactionKey>;
  using TransactionMap = AdServer::ProfilingCommons::TransactionProfileMap<TransactionKey>;
  using TransactionMap_var = ReferenceCounting::SmartPtr<TransactionMap>;

  class NullActiveObjectCallback final:
    public virtual Generics::ActiveObjectCallback,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    void
    report_error(Severity, const String::SubString&, const char* = nullptr) noexcept override
    {}

  protected:
    ~NullActiveObjectCallback() noexcept override = default;
  };

  AdServer::Commons::StartableAwaitable<void>
  co_get_profile(ProfileMap& profile_map, const std::string& key)
  {
    co_await profile_map.co_get_profile(key);
  }

  Generics::ConstSmartMemBuf_var
  make_profile(const std::string& value)
  {
    return Generics::ConstSmartMemBuf_var(
      new Generics::ConstSmartMemBuf(value.data(), value.size()));
  }

  void
  run_background_threads_configuration_test(const std::filesystem::path& root)
  {
    rocksdb::Options options;
    AdServer::ProfilingCommons::configure_rocksdb_profile_map_options(options, 12, 4, 2, 7);
    if (options.max_background_compactions != 12 || options.max_background_flushes != 2 ||
      options.max_subcompactions != 4 || options.max_write_buffer_number != 7)
    {
      throw std::runtime_error("RocksDB background job configuration mismatch");
    }

    auto processor = std::make_shared<Processor>(1, 32, 0, 256, 12, 4, 2, 7);
    ProfileMap profile_map(
      processor,
      String::SubString(root.string()),
      Generics::Time::ZERO,
      32,
      Generics::Time::ZERO,
      true);

    auto* env = rocksdb::Env::Default();
    if (env->GetBackgroundThreads(rocksdb::Env::LOW) != 12 ||
      env->GetBackgroundThreads(rocksdb::Env::HIGH) != 2)
    {
      throw std::runtime_error("RocksDB background thread configuration mismatch");
    }
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
    for (const auto& entry : std::filesystem::directory_iterator("/proc/self/task"))
    {
      std::ifstream name_file(entry.path() / "comm");
      std::string name;
      std::getline(name_file, name);
      if (name.rfind("rdb-batch", 0) == 0)
      {
        ++result;
      }
    }
    return result;
  }

  AdServer::Commons::StartableAwaitable<void>
  co_update_profile(
    std::shared_ptr<AdServer::Commons::ExecutorPool> executor_pool,
    TransactionMap& transaction_map,
    Generics::ConstSmartMemBuf_var profile,
    std::atomic<unsigned long>& transactions_acquired,
    std::atomic<unsigned long>& reads_completed,
    std::atomic<unsigned long>& saves_started,
    std::atomic<unsigned long>& failures,
    std::atomic<unsigned long>& injected_io_errors)
  {
    try
    {
      co_await AdServer::Commons::ExecutorPool::reschedule(std::move(executor_pool));
      auto transaction = co_await transaction_map.co_get_transaction(
        TransactionKey("shared"), false);
      transactions_acquired.fetch_add(1, std::memory_order_relaxed);
      co_await transaction->co_get_profile();
      reads_completed.fetch_add(1, std::memory_order_relaxed);
      saves_started.fetch_add(1, std::memory_order_relaxed);
      co_await transaction->co_save_profile(profile.in());
    }
    catch (const std::exception& ex)
    {
      if (std::string_view(ex.what()).find("File too large") != std::string_view::npos)
      {
        injected_io_errors.fetch_add(1, std::memory_order_relaxed);
      }
      failures.fetch_add(1, std::memory_order_relaxed);
      throw;
    }
    catch (...)
    {
      failures.fetch_add(1, std::memory_order_relaxed);
      throw;
    }
  }

  [[noreturn]] void
  run_write_failure_test_child(const std::filesystem::path& path)
  {
    try
    {
      if (std::signal(SIGXFSZ, SIG_IGN) == SIG_ERR)
      {
        throw std::runtime_error("can't ignore SIGXFSZ");
      }

      auto processor = std::make_shared<Processor>(1, 32, 0);
      processor->activate_object();
      auto profile_map = std::make_unique<TransactionBaseMap>(
        processor,
        String::SubString(path.string()),
        Generics::Time::ZERO,
        128,
        Generics::Time(0, 200000),
        false);
      profile_map->activate_object();
      TransactionMap_var transaction_map = new TransactionMap(profile_map.get());
      Generics::ActiveObjectCallback_var callback(new NullActiveObjectCallback());
      auto executor_pool = std::make_shared<AdServer::Commons::ExecutorPool>(
        callback,
        4,
        AdServer::Commons::ExecutorPool::ResumeStrategy::AnyContext,
        "rdb-failure");
      executor_pool->activate_object();

      // Limit is process-wide, so the failure is isolated in this watchdog-controlled child.
      struct rlimit original_limit;
      if (::getrlimit(RLIMIT_FSIZE, &original_limit) != 0)
      {
        throw std::runtime_error("getrlimit(RLIMIT_FSIZE) failed");
      }

      struct rlimit failure_limit = original_limit;
      failure_limit.rlim_cur = 1;
      if (::setrlimit(RLIMIT_FSIZE, &failure_limit) != 0)
      {
        throw std::runtime_error("setrlimit(RLIMIT_FSIZE) failed");
      }

      constexpr unsigned long operation_count = 1024;
      std::atomic<unsigned long> transactions_acquired{0};
      std::atomic<unsigned long> reads_completed{0};
      std::atomic<unsigned long> saves_started{0};
      std::atomic<unsigned long> failures{0};
      std::atomic<unsigned long> injected_io_errors{0};
      const auto initial_mutex_stats = AdServer::Commons::AsyncMutex::stats();
      auto profile = make_profile(std::string(4096, 'x'));
      AdServer::Commons::OrderedAsyncTaskWindow window(0, operation_count);

      for (unsigned long i = 0; i < operation_count; ++i)
      {
        window.start(
          i,
          co_update_profile(
            executor_pool,
            *transaction_map,
            profile,
            transactions_acquired,
            reads_completed,
            saves_started,
            failures,
            injected_io_errors));
      }

      window.wait_progress();
      bool operation_failed = false;
      try
      {
        window.rethrow_exception();
      }
      catch (const eh::Exception&)
      {
        operation_failed = true;
      }

      if (::setrlimit(RLIMIT_FSIZE, &original_limit) != 0)
      {
        throw std::runtime_error("can't restore RLIMIT_FSIZE");
      }

      const auto mutex_stats = AdServer::Commons::AsyncMutex::stats();
      const auto processor_stats = processor->stats();
      const auto executor_stats = executor_pool->stats();
      if (!operation_failed || failures.load(std::memory_order_relaxed) != operation_count)
      {
        throw std::runtime_error("not all failed coroutines completed");
      }

      if (injected_io_errors.load(std::memory_order_relaxed) != operation_count)
      {
        throw std::runtime_error("coroutines didn't receive the injected I/O error");
      }

      if (transactions_acquired.load(std::memory_order_relaxed) != operation_count ||
        reads_completed.load(std::memory_order_relaxed) != 1 ||
        saves_started.load(std::memory_order_relaxed) != 1)
      {
        throw std::runtime_error("unexpected transaction failure path");
      }

      if (mutex_stats.current_waiters != initial_mutex_stats.current_waiters ||
        mutex_stats.contended_locks - initial_mutex_stats.contended_locks != operation_count - 1)
      {
        throw std::runtime_error("transaction mutex waiters weren't released");
      }

      if (processor_stats.get_total != 1 || processor_stats.save_total != 1 ||
        processor_stats.write_batch_total != 1 || processor_stats.pending_operations != 0 ||
        processor_stats.active_workers != 0 || processor_stats.failed_batch_total != 1 ||
        processor_stats.failed_operation_total != 1 ||
        processor_stats.failed_callback_expected != operation_count ||
        processor_stats.failed_callback_completed != operation_count ||
        processor_stats.cache_entries != 0)
      {
        throw std::runtime_error("unexpected processor state after injected write failure");
      }

      if (executor_stats.resumes_scheduled == 0 ||
        executor_stats.resumes_scheduled != executor_stats.resumes_executed ||
        executor_stats.resume_schedule_failures != 0)
      {
        throw std::runtime_error("executor didn't run all scheduled coroutine resumes");
      }

      // A failed RocksDB stays in background-error state until its retry period expires.
      ::_exit(0);
    }
    catch (const std::exception& ex)
    {
      std::cerr << "write failure test child: " << ex.what() << std::endl;
      ::_exit(1);
    }
  }

  void
  run_write_failure_test(const std::filesystem::path& path)
  {
    const pid_t child = ::fork();
    if (child == -1)
    {
      throw std::runtime_error("fork failed");
    }

    if (child == 0)
    {
      run_write_failure_test_child(path);
    }

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    int status = 0;
    while (std::chrono::steady_clock::now() < deadline)
    {
      const pid_t result = ::waitpid(child, &status, WNOHANG);
      if (result == child)
      {
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
        {
          throw std::runtime_error("write failure test child failed");
        }
        return;
      }

      if (result == -1)
      {
        throw std::runtime_error("waitpid failed");
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    ::kill(child, SIGKILL);
    ::waitpid(child, &status, 0);
    throw std::runtime_error("write failure callbacks timed out");
  }

  void
  run_cache_test(const std::filesystem::path& path)
  {
    constexpr std::size_t cache_limit = 16 * 1024;
    auto processor = std::make_shared<Processor>(1, 8, cache_limit);
    processor->activate_object();
    auto profile_map = std::make_unique<ProfileMap>(
      processor,
      String::SubString(path.string()),
      Generics::Time::ZERO,
      128,
      Generics::Time(0, 200000),
      true);
    auto second_profile_map = std::make_unique<ProfileMap>(
      processor,
      String::SubString(path.string() + "-second"),
      Generics::Time::ZERO,
      128,
      Generics::Time(0, 200000),
      true);
    profile_map->activate_object();
    second_profile_map->activate_object();

    std::mutex completion_lock;
    std::condition_variable completion_condition;
    std::atomic<unsigned long> saves_completed{0};
    auto profile = make_profile("cached-before-rocksdb-write");
    profile_map->save_profile_async(
      "cached-key",
      profile.in(),
      Generics::Time::get_time_of_day(),
      [&](std::optional<std::string> error)
      {
        if (error)
        {
          return;
        }
        saves_completed.fetch_add(1, std::memory_order_relaxed);
        completion_condition.notify_all();
      });

    const auto cached = profile_map->get_profile("cached-key");
    if (!cached || cached->membuf().size() != profile->membuf().size() ||
      std::memcmp(
        cached->membuf().data(),
        profile->membuf().data(),
        profile->membuf().size()) != 0)
    {
      throw std::runtime_error("pending save wasn't visible through cache");
    }

    const auto pending_stats = processor->stats();
    if (saves_completed.load(std::memory_order_relaxed) != 1 ||
      pending_stats.pending_operations == 0 || pending_stats.cache_hits == 0 ||
      pending_stats.cache_entries != 1 || pending_stats.cache_limit != cache_limit)
    {
      throw std::runtime_error("cached save wasn't completed on queue admission");
    }

    if (!wait_count(completion_condition, completion_lock, saves_completed, 1))
    {
      throw std::runtime_error("cached save timed out");
    }

    auto second_profile = make_profile("second-map-value");
    second_profile_map->save_profile(
      "cached-key",
      second_profile.in(),
      Generics::Time::get_time_of_day());
    const auto first_cached = profile_map->get_profile("cached-key");
    const auto second_cached = second_profile_map->get_profile("cached-key");
    if (!first_cached || !second_cached ||
      first_cached->membuf().size() != profile->membuf().size() ||
      second_cached->membuf().size() != second_profile->membuf().size() ||
      std::memcmp(
        first_cached->membuf().data(),
        profile->membuf().data(),
        profile->membuf().size()) != 0 ||
      std::memcmp(
        second_cached->membuf().data(),
        second_profile->membuf().data(),
        second_profile->membuf().size()) != 0)
    {
      throw std::runtime_error("shared cache mixed different profile maps");
    }

    std::atomic<unsigned long> removes_completed{0};
    profile_map->remove_profile_async(
      "cached-key",
      AdServer::ProfilingCommons::OP_RUNTIME,
      [&](bool result, std::optional<std::string> error)
      {
        if (result && !error)
        {
          removes_completed.fetch_add(1, std::memory_order_relaxed);
        }
      });
    if (profile_map->check_profile("cached-key"))
    {
      throw std::runtime_error("pending remove wasn't visible through cache");
    }

    if (removes_completed.load(std::memory_order_relaxed) != 1)
    {
      throw std::runtime_error("cached remove wasn't completed on queue admission");
    }

    constexpr unsigned long writer_count = 8;
    constexpr unsigned long writes_per_writer = 100;
    constexpr unsigned long concurrent_writes = writer_count * writes_per_writer;
    std::atomic<unsigned long> concurrent_completed{0};
    std::atomic<unsigned long> concurrent_errors{0};
    std::vector<std::thread> writers;
    writers.reserve(writer_count);
    for (unsigned long writer = 0; writer < writer_count; ++writer)
    {
      writers.emplace_back(
        [&, writer]()
        {
          for (unsigned long write = 0; write < writes_per_writer; ++write)
          {
            auto write_profile = make_profile(
              "writer-" + std::to_string(writer) + "-" + std::to_string(write));
            try
            {
              profile_map->save_profile_async(
                "concurrent-key",
                write_profile.in(),
                Generics::Time::get_time_of_day(),
                [&](std::optional<std::string> error)
                {
                  if (error)
                  {
                    concurrent_errors.fetch_add(1, std::memory_order_relaxed);
                  }
                  concurrent_completed.fetch_add(1, std::memory_order_relaxed);
                  completion_condition.notify_all();
                });
            }
            catch (...)
            {
              concurrent_errors.fetch_add(1, std::memory_order_relaxed);
              concurrent_completed.fetch_add(1, std::memory_order_relaxed);
              completion_condition.notify_all();
            }
          }
        });
    }

    for (auto& writer : writers)
    {
      writer.join();
    }

    const auto concurrent_cached = profile_map->get_profile("concurrent-key");
    if (!concurrent_cached)
    {
      throw std::runtime_error("concurrent writes disappeared from cache");
    }
    const std::string concurrent_expected(
      static_cast<const char*>(concurrent_cached->membuf().data()),
      concurrent_cached->membuf().size());

    if (!wait_count(
      completion_condition,
      completion_lock,
      concurrent_completed,
      concurrent_writes) ||
      concurrent_errors.load(std::memory_order_relaxed) != 0)
    {
      throw std::runtime_error("concurrent cached writes failed");
    }

    std::atomic<unsigned long> eviction_completed{0};
    std::atomic<unsigned long> eviction_errors{0};
    for (unsigned long i = 0; i < 64; ++i)
    {
      auto large_profile = make_profile(std::string(1024, static_cast<char>('a' + i % 26)));
      profile_map->save_profile_async(
        "eviction-key-" + std::to_string(i),
        large_profile.in(),
        Generics::Time::get_time_of_day(),
        [&](std::optional<std::string> error)
        {
          if (error)
          {
            eviction_errors.fetch_add(1, std::memory_order_relaxed);
          }
          eviction_completed.fetch_add(1, std::memory_order_relaxed);
          completion_condition.notify_all();
        });
    }

    if (!wait_count(completion_condition, completion_lock, eviction_completed, 64) ||
      eviction_errors.load(std::memory_order_relaxed) != 0)
    {
      throw std::runtime_error("cached eviction writes failed");
    }

    const auto persistence_deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(10);
    auto cache_stats = processor->stats();
    while ((cache_stats.pending_operations != 0 || cache_stats.active_workers != 0) &&
      std::chrono::steady_clock::now() < persistence_deadline)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      cache_stats = processor->stats();
    }

    if (cache_stats.pending_operations != 0 || cache_stats.active_workers != 0 ||
      cache_stats.cache_size > cache_limit || cache_stats.cache_evictions == 0 ||
      saves_completed.load(std::memory_order_relaxed) != 1 ||
      removes_completed.load(std::memory_order_relaxed) != 1)
    {
      throw std::runtime_error("cached writes weren't persisted exactly once");
    }

    profile_map->deactivate_object();
    profile_map->wait_object();
    profile_map.reset();

    auto reopened_profile_map = std::make_unique<ProfileMap>(
      processor,
      String::SubString(path.string()),
      Generics::Time::ZERO,
      128,
      Generics::Time::ZERO,
      true);
    reopened_profile_map->activate_object();
    const auto concurrent_persisted = reopened_profile_map->get_profile("concurrent-key");
    if (!concurrent_persisted ||
      concurrent_persisted->membuf().size() != concurrent_expected.size() ||
      std::memcmp(
        concurrent_persisted->membuf().data(),
        concurrent_expected.data(),
        concurrent_expected.size()) != 0)
    {
      throw std::runtime_error("cache and persistent state diverged after concurrent writes");
    }
    reopened_profile_map->deactivate_object();
    reopened_profile_map->wait_object();
    reopened_profile_map.reset();

    auto surviving_profile = make_profile("surviving-map-value");
    second_profile_map->save_profile(
      "surviving-key",
      surviving_profile.in(),
      Generics::Time::get_time_of_day());
    if (processor->stats().cache_entries == 0)
    {
      throw std::runtime_error("unregistering one map cleared the shared cache");
    }

    second_profile_map->deactivate_object();
    second_profile_map->wait_object();
    second_profile_map.reset();
    if (processor->stats().cache_entries != 0)
    {
      throw std::runtime_error("unregistered map remained in cache");
    }

    processor->deactivate_object();
    processor->wait_object();
  }
}

int
main()
{
  std::string root_template = "/tmp/RocksDBProfileMapProcessorTest.XXXXXX";
  const char* const root_name = ::mkdtemp(root_template.data());
  if (!root_name)
  {
    std::cerr << "RocksDBProfileMapProcessorTest: FAIL: mkdtemp failed" << std::endl;
    return 1;
  }
  const std::filesystem::path root(root_name);

  std::shared_ptr<Processor> processor;
  std::unique_ptr<ProfileMap> first;
  std::unique_ptr<ProfileMap> second;

  try
  {
    run_background_threads_configuration_test(root / "background-threads");
    processor = std::make_shared<Processor>(1);
    auto* env = rocksdb::Env::Default();
    if (env->GetBackgroundThreads(rocksdb::Env::LOW) != 32 ||
      env->GetBackgroundThreads(rocksdb::Env::HIGH) != 32)
    {
      throw std::runtime_error("default RocksDB background thread configuration mismatch");
    }

    const auto disabled_cache_stats = processor->stats();
    if (disabled_cache_stats.cache_limit != 0 || disabled_cache_stats.cache_size != 0 ||
      disabled_cache_stats.cache_entries != 0 || disabled_cache_stats.cache_hits != 0 ||
      disabled_cache_stats.cache_misses != 0 || disabled_cache_stats.cache_evictions != 0)
    {
      throw std::runtime_error("cache isn't disabled by default");
    }

    run_write_failure_test(root / "write-failure");
    run_cache_test(root / "cache");

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

    const auto initial_processor_stats = processor->stats();
    if (initial_processor_stats.workers != 1 || initial_processor_stats.queue_count != 2 ||
      initial_processor_stats.pending_operations != 0 ||
      initial_processor_stats.active_workers != 0)
    {
      throw std::runtime_error("initial processor queue stats mismatch");
    }

    for (unsigned int i = 0; i < 100 && rdb_batch_thread_count() != 1; ++i)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    if (rdb_batch_thread_count() != 1)
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

    if (processor->stats().pending_operations == 0)
    {
      throw std::runtime_error("pending operation is missing from processor stats");
    }

    const auto enqueue_started = std::chrono::steady_clock::now();
    second->save_profile_async(
      "isolation",
      isolation_profile.in(),
      Generics::Time::get_time_of_day(),
      [&](std::optional<std::string> error)
      {
        if (error)
        {
          errors.fetch_add(1, std::memory_order_relaxed);
        }
        isolation_done.fetch_add(1, std::memory_order_relaxed);
        completion_condition.notify_all();
      });
    const auto enqueue_elapsed = std::chrono::steady_clock::now() - enqueue_started;
    if (enqueue_elapsed >= std::chrono::milliseconds(500))
    {
      throw std::runtime_error("one map max_delay blocked another map enqueue");
    }

    if (!wait_count(
      completion_condition,
      completion_lock,
      isolation_done,
      1,
      std::chrono::seconds(1)))
    {
      throw std::runtime_error("one map max_delay blocked another ready map");
    }

    for (unsigned long i = 0; i < operations_per_map; ++i)
    {
      auto first_profile = make_profile("first-value-" + std::to_string(i));
      first->save_profile_async(
        "first-key-" + std::to_string(i),
        first_profile.in(),
        Generics::Time::get_time_of_day(),
        [&](std::optional<std::string> error)
        {
          if (error)
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
          if (error)
          {
            errors.fetch_add(1, std::memory_order_relaxed);
          }
          second_done.fetch_add(1, std::memory_order_relaxed);
          completion_condition.notify_all();
        });
    }

    first->deactivate_object();
    first->wait_object();
    if (first_done.load(std::memory_order_relaxed) != operations_per_map)
    {
      throw std::runtime_error("wait_unregister returned before callbacks completed");
    }

    std::atomic<unsigned long> rejected_callbacks{0};
    const auto count_rejected = [&rejected_callbacks](const std::optional<std::string>& error)
    {
      if (!error)
      {
        throw std::runtime_error("rejected async operation completed without error");
      }
      rejected_callbacks.fetch_add(1, std::memory_order_relaxed);
    };
    auto rejected_profile = make_profile("rejected");
    first->check_profile_async(
      "rejected",
      [&count_rejected](bool, std::optional<std::string> error)
      {
        count_rejected(error);
      });
    first->get_profile_async(
      "rejected",
      [&count_rejected](Generics::ConstSmartMemBuf_var, std::optional<std::string> error)
      {
        count_rejected(error);
      });
    first->get_own_profile_async(
      "rejected",
      [&count_rejected](Generics::SmartMemBuf_var, std::optional<std::string> error)
      {
        count_rejected(error);
      });
    first->save_profile_async(
      "rejected",
      rejected_profile.in(),
      Generics::Time::get_time_of_day(),
      [&count_rejected](std::optional<std::string> error)
      {
        count_rejected(error);
      });
    first->remove_profile_async(
      "rejected",
      AdServer::ProfilingCommons::OP_RUNTIME,
      [&count_rejected](bool, std::optional<std::string> error)
      {
        count_rejected(error);
      });
    if (rejected_callbacks.load(std::memory_order_relaxed) != 5)
    {
      throw std::runtime_error("rejected async operations didn't complete callbacks");
    }

    bool rejected_coroutine = false;
    try
    {
      AdServer::Commons::sync_wait(co_get_profile(*first, "rejected"));
    }
    catch (const eh::Exception&)
    {
      rejected_coroutine = true;
    }

    if (!rejected_coroutine)
    {
      throw std::runtime_error("rejected coroutine operation didn't receive an error");
    }

    bool rejected = false;
    try
    {
      first->save_profile_async(
        "rejected",
        rejected_profile.in(),
        Generics::Time::get_time_of_day());
    }
    catch (const eh::Exception&)
    {
      rejected = true;
    }

    if (!rejected)
    {
      throw std::runtime_error("operation was accepted after unregister");
    }
    first.reset();

    if (!wait_count(completion_condition, completion_lock, second_done, operations_per_map))
    {
      throw std::runtime_error("second map operations timed out");
    }

    auto profile = make_profile("still-active");
    second->save_profile("still-active", profile.in(), Generics::Time::get_time_of_day());
    const auto loaded = second->get_profile("still-active");
    if (!loaded.in() || loaded->membuf().size() != std::string("still-active").size())
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
    third->save_profile("third", profile.in(), Generics::Time::get_time_of_day());
    if (!third->check_profile("third"))
    {
      throw std::runtime_error("saved profile wasn't found");
    }

    if (!third->remove_profile("third"))
    {
      throw std::runtime_error("saved profile wasn't removed");
    }
    third->deactivate_object();
    third->wait_object();
    third.reset();

    processor->deactivate_object();
    processor->wait_object();

    const auto first_processor_stats = processor->stats();
    if (first_processor_stats.check_total != 1 ||
      first_processor_stats.get_total != 1 ||
      first_processor_stats.touch_total != 0 ||
      first_processor_stats.save_total != 2 * operations_per_map + 4 ||
      first_processor_stats.remove_total != 1 ||
      first_processor_stats.read_batch_total != 2 ||
      first_processor_stats.read_batch_total_time == 0 ||
      first_processor_stats.write_batch_total == 0 ||
      first_processor_stats.write_batch_total >
        first_processor_stats.save_total + first_processor_stats.remove_total ||
      first_processor_stats.write_batch_total_time == 0 ||
      first_processor_stats.workers != 1 ||
      first_processor_stats.queue_count != 0 ||
      first_processor_stats.pending_operations != 0 ||
      first_processor_stats.active_workers != 0)
    {
      throw std::runtime_error("first processor stats mismatch");
    }

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
      const unsigned long index = same_key_sent.fetch_add(1, std::memory_order_relaxed);
      if (index >= same_key_operations)
      {
        return;
      }

      first->get_profile_async(
        "shared",
        [&, index](Generics::ConstSmartMemBuf_var, std::optional<std::string> error)
        {
          if (error)
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
              if (error)
              {
                errors.fetch_add(1, std::memory_order_relaxed);
              }
              same_key_done.fetch_add(1, std::memory_order_relaxed);
              completion_condition.notify_all();
              enqueue_same_key();
            });
        });
    };

    for (unsigned long i = 0; i < same_key_inflight; ++i)
    {
      enqueue_same_key();
    }

    if (!wait_count(completion_condition, completion_lock, same_key_done, same_key_operations))
    {
      throw std::runtime_error("same-key operations timed out");
    }

    first->deactivate_object();
    first->wait_object();
    first.reset();

    constexpr unsigned long lifecycle_maps = 20;
    constexpr unsigned long lifecycle_operations = 200;
    bool processor_deactivated = false;
    for (unsigned long map_index = 0; map_index < lifecycle_maps; ++map_index)
    {
      auto map = std::make_unique<ProfileMap>(
        processor,
        String::SubString((root / ("lifecycle-" + std::to_string(map_index))).string()),
        Generics::Time::ZERO,
        8,
        Generics::Time(1),
        true);
      map->activate_object();

      auto lifecycle_done = std::make_shared<std::atomic<unsigned long>>(0);
      for (unsigned long i = 0; i < lifecycle_operations; ++i)
      {
        auto lifecycle_profile = make_profile("lifecycle-value");
        map->save_profile_async(
          "key-" + std::to_string(i),
          lifecycle_profile.in(),
          Generics::Time::get_time_of_day(),
          [&, lifecycle_done](std::optional<std::string> error)
          {
            if (error)
            {
              errors.fetch_add(1, std::memory_order_relaxed);
            }
            lifecycle_done->fetch_add(1, std::memory_order_relaxed);
          });
      }

      map->deactivate_object();
      if (map_index + 1 == lifecycle_maps)
      {
        processor->deactivate_object();
        processor_deactivated = true;
      }

      map->wait_object();
      if (lifecycle_done->load(std::memory_order_relaxed) != lifecycle_operations)
      {
        throw std::runtime_error("map destroyed while selected workers were still active");
      }
    }

    if (!processor_deactivated)
    {
      processor->deactivate_object();
    }
    processor->wait_object();

    const auto second_processor_stats = processor->stats();
    if (second_processor_stats.check_total != 0 ||
      second_processor_stats.get_total != same_key_operations ||
      second_processor_stats.touch_total != 0 ||
      second_processor_stats.save_total !=
        same_key_operations + lifecycle_maps * lifecycle_operations ||
      second_processor_stats.remove_total != 0 ||
      second_processor_stats.read_batch_total == 0 ||
      second_processor_stats.read_batch_total > second_processor_stats.get_total ||
      second_processor_stats.read_batch_total_time == 0 ||
      second_processor_stats.write_batch_total == 0 ||
      second_processor_stats.write_batch_total > second_processor_stats.save_total ||
      second_processor_stats.write_batch_total_time == 0 ||
      second_processor_stats.workers != 4 ||
      second_processor_stats.queue_count != 0 ||
      second_processor_stats.pending_operations != 0 ||
      second_processor_stats.active_workers != 0)
    {
      throw std::runtime_error("second processor stats mismatch");
    }

    if (errors.load(std::memory_order_relaxed) != 0)
    {
      throw std::runtime_error("background operations failed");
    }

    std::filesystem::remove_all(root);
    std::cout << "RocksDBProfileMapProcessorTest: PASS" << std::endl;
    return 0;
  }
  catch (const std::exception& ex)
  {
    if (first && first->active())
    {
      first->deactivate_object();
      first->wait_object();
    }

    if (second && second->active())
    {
      second->deactivate_object();
      second->wait_object();
    }

    if (processor && processor->active())
    {
      processor->deactivate_object();
      processor->wait_object();
    }
    std::filesystem::remove_all(root);
    std::cerr << "RocksDBProfileMapProcessorTest: FAIL: " << ex.what() << std::endl;
    return 1;
  }
}
