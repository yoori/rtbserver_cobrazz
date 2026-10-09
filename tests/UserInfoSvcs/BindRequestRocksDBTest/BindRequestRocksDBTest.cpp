#include <chrono>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <stdexcept>
#include <thread>

#include <sys/wait.h>
#include <sys/resource.h>
#include <unistd.h>

#include <UserInfoSvcs/UserBindServer/BindRequestContainer.hpp>
#include <UserInfoSvcs/UserBindServer/UserBindContainer.hpp>

namespace
{
  namespace fs = std::filesystem;
  using namespace AdServer::UserInfoSvcs;
  using AdServer::Commons::sync_wait;
  using BindRequest = BindRequestProcessor::BindRequest;

  const String::SubString MISSING("missing");
  const String::SubString KEY("key");
  const String::SubString EMPTY("empty");
  const String::SubString OVERWRITE("overwrite");
  const String::SubString EXPIRED("expired");
  const String::SubString TTL("ttl");
  const String::SubString WAL("wal");

  void require(bool condition, const char* message)
  {
    if (!condition)
    {
      throw std::runtime_error(message);
    }
  }

  class TemporaryDirectory
  {
  public:
    TemporaryDirectory()
    {
      char name[] = "/tmp/BindRequestRocksDBTest-XXXXXX";
      require(::mkdtemp(name) != nullptr, "mkdtemp failed");
      path = name;
    }

    ~TemporaryDirectory()
    {
      std::error_code error;
      fs::remove_all(path, error);
    }

    fs::path path;
  };

  class ProcessorGuard
  {
  public:
    explicit ProcessorGuard(std::size_t cache_size = 0)
      : processor(std::make_shared<AdServer::ProfilingCommons::RocksDBProfileMapProcessor>(
          2, 32, cache_size, 256, 2, 1, 2, 2))
    {
      processor->activate_object();
    }

    ~ProcessorGuard()
    {
      processor->deactivate_object();
      processor->wait_object();
    }

    std::shared_ptr<AdServer::ProfilingCommons::RocksDBProfileMapProcessor> processor;
  };

  BindRequestContainer_var open_container(
    const ProcessorGuard& processor,
    const fs::path& root,
    unsigned long count = 4,
    const Generics::Time& ttl = Generics::Time::ONE_DAY)
  {
    BindRequestContainer::ChunkPathMap folders;
    for (unsigned long i = 0; i < count; ++i)
    {
      const auto path = root / ("Chunk_" + std::to_string(i));
      fs::create_directories(path);
      folders.emplace(i, path.string());
    }

    return BindRequestContainer_var(
      new BindRequestContainer(processor.processor, count, folders, "BindRequest", ttl));
  }

  AdServer::Commons::StartableAwaitable<void> co_roundtrip(
    BindRequestContainer* container, std::string id, Generics::Time now)
  {
    BindRequest request{{id, "second"}};
    co_await container->co_add_bind_request(String::SubString(id), request, now);
    const auto result = co_await container->co_get_bind_request(String::SubString(id), now);
    require(result.bind_user_ids == request.bind_user_ids, "parallel roundtrip mismatch");
  }

  void test_roundtrip(const fs::path& root)
  {
    fs::create_directories(root / "Chunk_0");
    const auto legacy_path = root / "Chunk_0" / "BindRequest.20261008.000000.00000000";
    {
      std::ofstream legacy(legacy_path);
      legacy << "invalid legacy snapshot";
    }

    const auto now = Generics::Time::get_time_of_day();
    const BindRequest request{{"", "a|b\n^c\td", std::string("a\0b", 3), std::string(70000, 'x')}};
    ProcessorGuard processor;
    auto container = open_container(processor, root);
    require(container->get_bind_request(MISSING, now).bind_user_ids.empty(), "missing key found");
    container->add_bind_request(KEY, request, now);
    require(container->get_bind_request(KEY, now).bind_user_ids == request.bind_user_ids,
      "binary strings did not survive roundtrip");
    container->add_bind_request(EMPTY, BindRequest{}, now);
    require(container->get_bind_request(EMPTY, now).bind_user_ids.empty(), "empty value found");

    const BindRequest updated{{"replacement"}};
    container->add_bind_request(OVERWRITE, request, now - Generics::Time(100));
    container->add_bind_request(OVERWRITE, updated, now);
    require(container->get_bind_request(OVERWRITE, now).bind_user_ids == updated.bind_user_ids,
      "overwrite returned an older value");

    container->add_bind_request(EXPIRED, request, now - Generics::Time::ONE_DAY);
    require(container->get_bind_request(EXPIRED, now).bind_user_ids.empty(), "expired key found");

    std::vector<std::future<void>> futures;
    for (unsigned long i = 0; i < 256; ++i)
    {
      auto completion = std::make_shared<std::promise<void>>();
      futures.push_back(completion->get_future());
      co_roundtrip(container, "parallel-" + std::to_string(i), now).start_detached(
        [completion](std::optional<std::exception_ptr> error)
        {
          if (error)
          {
            completion->set_exception(*error);
          }
          else
          {
            completion->set_value();
          }
        });
    }

    std::exception_ptr first_error;
    for (auto& future : futures)
    {
      try
      {
        future.get();
      }
      catch (...)
      {
        first_error = std::current_exception();
      }
    }

    if (first_error)
    {
      std::rethrow_exception(first_error);
    }

    const auto stats = container->rocksdb_stats();
    require(stats.workers == 2 && stats.queue_count == 4, "processor is not shared across chunks");
    container.reset();
    container = open_container(processor, root);
    require(container->get_bind_request(KEY, now).bind_user_ids == request.bind_user_ids,
      "value lost after reopening without dump");
    require(container->get_bind_request(OVERWRITE, now).bind_user_ids == updated.bind_user_ids,
      "overwrite lost after reopening");
    require(fs::exists(legacy_path), "legacy file was removed");
  }

  void test_shared_processor(const fs::path& root)
  {
    ProcessorGuard processor(1024 * 1024);
    fs::create_directories(root / "shared");
    const BindRequestContainer::ChunkPathMap folders{{0, (root / "shared").string()}};
    auto requests = BindRequestContainer_var(
      new BindRequestContainer(processor.processor, 1, folders, "BindRequest", Generics::Time(60)));
    UserBindContainer_var users = new UserBindContainer(
      nullptr, 1, folders, "", "", Generics::Time(15), Generics::Time(15),
      std::nullopt, 100, 1, false, 0, 1, 2, 1024 * 1024, 2, 1, 2, 2, processor.processor);
    const auto now = Generics::Time::get_time_of_day();
    const auto user_id = AdServer::Commons::UserId::create_random_based();
    sync_wait(users->co_add_user_id(KEY, user_id, now, true, false, false));
    requests->add_bind_request(KEY, BindRequest{{"shared"}}, now);
    const auto loaded_user = sync_wait(
      users->co_get_user_id(KEY, user_id, now, true, Generics::Time::ZERO, false, false));
    require(loaded_user.user_id == user_id, "shared processor changed user bind");
    const auto loaded_request = requests->get_bind_request(KEY, now);
    require(loaded_request.bind_user_ids == std::vector<std::string>{"shared"},
      "shared processor mixed the maps");
    const auto stats = processor.processor->stats();
    require(stats.workers == 2 && stats.queue_count == 2 && stats.cache_limit == 1024 * 1024,
      "shared processor duplicated worker or cache budgets");
    require(stats.save_total == 2 && users->rocksdb_stats().save_total == 2 &&
      requests->rocksdb_stats().save_total == 2, "shared statistics omit an operation");

    fs::create_directories(root / "partial");
    bool failed = false;
    try
    {
      BindRequestContainer_var invalid = new BindRequestContainer(
        processor.processor, 2, {{0, (root / "partial").string()}, {2, root.string()}},
        "BindRequest", Generics::Time(60));
    }
    catch (const eh::Exception&)
    {
      failed = true;
    }

    require(failed && processor.processor->active(), "failed container stopped the processor");
    require(processor.processor->stats().queue_count == 2, "failed container leaked a map");
    requests.reset();
    require(processor.processor->stats().queue_count == 1, "removed request map stayed registered");
    sync_wait(users->co_add_user_id(OVERWRITE, user_id, now, true, false, false));
    const auto after_remove = sync_wait(
      users->co_get_user_id(OVERWRITE, user_id, now, true, Generics::Time::ZERO, false, false));
    require(after_remove.user_id == user_id, "removing BindRequest stopped UserBind");
    users.reset();
    require(processor.processor->stats().queue_count == 0, "UserBind map stayed registered");
    require(processor.processor->active(), "removing UserBind stopped the borrowed processor");
    requests = new BindRequestContainer(
      processor.processor, 1, folders, "BindRequest", Generics::Time(60));
    requests->add_bind_request(KEY, BindRequest{{"after user bind"}}, now);
  }

  void test_ttl(const fs::path& root)
  {
    const auto now = Generics::Time::get_time_of_day();
    ProcessorGuard uncached_processor;
    ProcessorGuard cached_processor(1024 * 1024);
    auto uncached = open_container(uncached_processor, root / "uncached", 1, Generics::Time(4));
    auto cached = open_container(cached_processor, root / "cached", 1, Generics::Time(4));
    const BindRequest request{{"ttl"}};
    uncached->add_bind_request(TTL, request, now);
    cached->add_bind_request(TTL, request, now);
    for (int i = 0; i < 6; ++i)
    {
      // Keep the logical read time fixed to also exercise expiration inside the map and cache.
      uncached->get_bind_request(TTL, now);
      cached->get_bind_request(TTL, now);
      if (i != 5)
      {
        std::this_thread::sleep_for(std::chrono::seconds(1));
      }
    }

    require(uncached->get_bind_request(TTL, now).bind_user_ids.empty(), "read extended disk TTL");
    require(cached->get_bind_request(TTL, now).bind_user_ids.empty(), "read extended cache TTL");
    require(uncached->rocksdb_stats().touch_total == 0, "disk reads scheduled TTL writes");
    require(cached->rocksdb_stats().touch_total == 0, "cache reads scheduled TTL writes");

    cached->add_bind_request(TTL, request, Generics::Time::get_time_of_day());
    require(cached->get_bind_request(TTL, Generics::Time::get_time_of_day()).bind_user_ids ==
      request.bind_user_ids, "expired value could not be replaced");
  }

  void test_write_failure(const fs::path& root)
  {
    ProcessorGuard processor;
    auto container = open_container(processor, root, 1);
    struct rlimit saved;
    require(::getrlimit(RLIMIT_FSIZE, &saved) == 0, "getrlimit failed");
    const auto handler = std::signal(SIGXFSZ, SIG_IGN);
    require(handler != SIG_ERR, "signal failed");
    auto limit = saved;
    limit.rlim_cur = 0;
    require(::setrlimit(RLIMIT_FSIZE, &limit) == 0, "setrlimit failed");
    bool rejected = false;
    try
    {
      container->add_bind_request(KEY, BindRequest{{std::string(65536, 'x')}},
        Generics::Time::get_time_of_day());
    }
    catch (const BindRequestProcessor::Exception&)
    {
      rejected = true;
    }

    require(::setrlimit(RLIMIT_FSIZE, &saved) == 0, "restoring rlimit failed");
    std::signal(SIGXFSZ, handler);
    require(rejected, "failed WAL write was acknowledged");
    container.reset();
  }

  void test_child(const fs::path& root, const char* mode)
  {
    const auto child = ::fork();
    require(child != -1, "fork failed");
    if (child == 0)
    {
      ::execl("/proc/self/exe", "BindRequestRocksDBTest", mode, root.c_str(), nullptr);
      ::_exit(127);
    }

    int status = 0;
    require(::waitpid(child, &status, 0) == child, "waitpid failed");
    require(WIFEXITED(status) && WEXITSTATUS(status) == 0, "child process failed");
  }

  void test_abrupt_exit(const fs::path& root)
  {
    test_child(root, "--writer");
    ProcessorGuard processor;
    auto container = open_container(processor, root, 1);
    require(container->get_bind_request(WAL, Generics::Time::get_time_of_day()).bind_user_ids ==
      std::vector<std::string>{"saved"}, "acknowledged write lost after abrupt exit");
  }
}

int main(int argc, char** argv)
{
  try
  {
    if (argc == 3 && std::string(argv[1]) == "--write-failure")
    {
      test_write_failure(argv[2]);
      return 0;
    }

    if (argc == 3 && std::string(argv[1]) == "--writer")
    {
      ProcessorGuard processor;
      auto container = open_container(processor, argv[2], 1);
      container->add_bind_request(WAL, BindRequest{{"saved"}}, Generics::Time::get_time_of_day());
      ::_exit(0);
    }

    TemporaryDirectory directory;
    test_child(directory.path / "write-failure", "--write-failure");
    test_abrupt_exit(directory.path / "wal");
    test_roundtrip(directory.path / "roundtrip");
    test_ttl(directory.path / "ttl");
    test_shared_processor(directory.path / "shared");
    std::cout << "BindRequestRocksDBTest: passed\n";
    return 0;
  }
  catch (const std::exception& ex)
  {
    std::cerr << "BindRequestRocksDBTest: " << ex.what() << '\n';
    return 1;
  }
}
