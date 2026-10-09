#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <future>
#include <iostream>
#include <stdexcept>
#include <thread>

#include <Logger/StreamLogger.hpp>
#include <UserInfoSvcs/UserBindServer/UserBindServerCore.hpp>

namespace
{
  using Core = AdServer::UserInfoSvcs::UserBindServerCore;
  using CorePtr = AdServer::UserInfoSvcs::UserBindServerCore_var;
  using AdServer::Commons::sync_wait;
  namespace fs = std::filesystem;

  void require(bool value, const char* message)
  {
    if (!value)
    {
      throw std::runtime_error(message);
    }
  }

  class Fixture
  {
  public:
    Fixture()
    {
      char path[] = "/tmp/UserBindServerStorageTest-XXXXXX";
      require(::mkdtemp(path) != nullptr, "mkdtemp failed");
      root = path;
      fs::create_directory(root / "Chunk_0_2");
      fs::create_directory(root / "Chunk_1_2");
      config.storage.chunks_root = root.string();
      config.storage.common_chunks_number = 2;
      config.storage.expire_time = Generics::Time(3600);
      config.storage.bound_expire_time = Generics::Time(3600);
      config.storage.rocksdb_batching_threads = 2;
      config.storage.rocksdb_cache_size = 1024 * 1024;
      config.storage.compaction_threads = 2;
      config.storage.per_compaction_threads = 1;
      config.storage.flush_threads = 2;
      config.storage.max_mem_tables = 2;
      config.bind_request_storage.prefix = "BindRequest";
      config.bind_request_storage.common_chunks_number = 2;
      config.bind_request_storage.expire_time = Generics::Time(3600);
      config.max_bad_event = 100;
      logger = new Logging::OStream::Logger(Logging::OStream::Config(std::cerr));
    }

    ~Fixture()
    {
      if (core)
      {
        core->deactivate_object();
        core->wait_object();
        core.reset();
      }

      std::error_code error;
      fs::remove_all(root, error);
    }

    void open()
    {
      core = new Core(config, logger);
      core->activate_object();
      const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
      while (std::chrono::steady_clock::now() < deadline)
      {
        auto users = core->user_bind_container()->get_accessor();
        auto requests = core->bind_request_container()->get_accessor();
        if (users.get() && requests.get())
        {
          return;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }

      throw std::runtime_error("core did not become ready");
    }

    fs::path root;
    Core::Config config;
    Logging::Logger_var logger;
    CorePtr core;
  };

  AdServer::Commons::StartableAwaitable<void> co_save(Core* core, std::string key)
  {
    const Core::BindRequestInfo request{{key}};
    co_await core->co_add_bind_request(key, request, Generics::Time::get_time_of_day());
  }
}

int main()
{
  try
  {
    Fixture fixture;
    fixture.open();
    auto* core = fixture.core.in();
    const auto initial = core->rocksdb_stats();
    require(initial.workers == 2 && initial.queue_count == 4 && initial.cache_limit == 1024 * 1024,
      "core duplicated processor budgets or omitted queues");

    Core::AddUserRequestInfo user;
    user.id = "user";
    user.timestamp = Generics::Time::get_time_of_day();
    user.user_id = AdServer::Commons::UserId::create_random_based();
    sync_wait(core->co_add_user_id(user));
    std::vector<std::future<void>> completions;
    for (unsigned long i = 0; i < 128; ++i)
    {
      auto promise = std::make_shared<std::promise<void>>();
      completions.push_back(promise->get_future());
      co_save(core, std::to_string(i)).start_detached(
        [promise](std::optional<std::exception_ptr> error)
        {
          if (error)
          {
            promise->set_exception(*error);
          }
          else
          {
            promise->set_value();
          }
        });
    }

    core->deactivate_object();
    core->wait_object();
    for (auto& completion : completions)
    {
      require(completion.wait_for(std::chrono::seconds(0)) == std::future_status::ready,
        "shutdown returned before a request completed");
      completion.get();
    }

    const auto stopped = core->rocksdb_stats();
    require(stopped.save_total == initial.save_total + 129, "core statistics lost writes");
    require(stopped.pending_operations == 0 && stopped.active_workers == 0,
      "core stopped before the processor drained");
    fixture.core.reset();
    fixture.open();
    for (unsigned long i = 0; i < 128; ++i)
    {
      const auto key = std::to_string(i);
      const auto loaded = fixture.core->get_bind_request(key, Generics::Time::get_time_of_day());
      require(loaded.bind_user_ids == std::vector<std::string>{key}, "shutdown lost a request");
    }

    std::cout << "UserBindServerStorageTest: passed\n";
    return 0;
  }
  catch (const std::exception& ex)
  {
    std::cerr << "UserBindServerStorageTest: " << ex.what() << '\n';
    return 1;
  }
}
