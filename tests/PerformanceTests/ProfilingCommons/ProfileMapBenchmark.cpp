// STD
#include <atomic>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>

// USERVER
#include <userver/engine/async.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/utils/async.hpp>
#include <engine/task/task_processor.hpp>

// THIS
#include <Logger/Logger.hpp>
#include <Logger/StreamLogger.hpp>
#include <ProfilingCommons/ProfileMap/AsyncRocksDBProfileMap.hpp>
#include <ProfilingCommons/ProfileMap/RocksDBProfileMap.hpp>
#include <UServerUtils/Component.hpp>
#include <UServerUtils/ComponentsBuilder.hpp>
#include <UServerUtils/Manager.hpp>
#include <UServerUtils/Utils.hpp>

using namespace AdServer::ProfilingCommons;

struct WriteStatistics final
{
  std::atomic<std::uint64_t> success{0};
  std::atomic<std::uint64_t> error{0};
};

struct ReadStatistics final
{
  std::atomic<std::uint64_t> success{0};
  std::atomic<std::uint64_t> error{0};
};

struct KeyStringAdapter
{
  std::string operator()(const std::string& key) const
  {
    return key;
  }
};

inline std::uint64_t number_digit(const std::uint64_t number) noexcept
{
  if (number < 100000)
  {
    if (number < 100)
    {
      if (number < 10)
      {
        return 1;
      }
      else
      {
        return 2;
      }
    }
    else
    {
      if (number < 1000)
      {
        return 3;
      }
      else
      {
        if (number < 10000)
        {
          return 4;
        }
        else
        {
          return 5;
        }
      }
    }
  }
  else
  {
    if (number < 10000000)
    {
      if (number < 1000000)
      {
        return 6;
      }
      else
      {
        return 7;
      }
    }
    else
    {
      if (number < 100000000)
      {
        return 8;
      }
      else
      {
        if (number < 1000000000)
        {
          return 9;
        }
        else
        {
          return 10;
        }
      }
    }
  }
}

class ProfileMapFactory : private Generics::Uncopyable
{
public:
  using ProfileMap_var =
    ReferenceCounting::SmartPtr<ProfileMap<std::string>>;

  ProfileMapFactory() = default;

  virtual ~ProfileMapFactory() = default;

  virtual ProfileMap_var create() = 0;
};

using ProfileMapFactoryPtr = std::unique_ptr<ProfileMapFactory>;

class ProfileMapRocksDBFactory final : public ProfileMapFactory
{
private:
  using ProfileMap = RocksDBProfileMap<std::string, KeyStringAdapter>;

public:
  explicit ProfileMapRocksDBFactory(
    const std::string& path_db,
    const Generics::Time& expire_time)
    : path_db_(path_db),
      expire_time_(expire_time)
  {
  }

  ~ProfileMapRocksDBFactory() override = default;

  ProfileMap_var create() override
  {
    return ReferenceCounting::SmartPtr<ProfileMap>(
      new ProfileMap(
        path_db_,
        expire_time_));
  }

private:
  const std::string path_db_;

  const Generics::Time expire_time_;
};

class ProfileMapIoUringRocksDBFactory final : public ProfileMapFactory
{
private:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using ProfileMap =
    AdServer::ProfilingCommons::RocksDB::RocksDBProfileMap<
      std::string,
      KeyStringAdapter>;
  using ManagerPoolConfig = UServerUtils::Grpc::RocksDB::Config;
  using DataBaseManagerPool = UServerUtils::Grpc::RocksDB::DataBaseManagerPool;
  using DataBaseManagerPoolPtr = std::shared_ptr<DataBaseManagerPool>;

public:
  explicit ProfileMapIoUringRocksDBFactory(
    Logger* logger,
    const std::uint64_t block_сache_size_mb,
    const std::string& path_db,
    const Generics::Time& expire_time,
    const std::optional<std::string> column_family_name)
    : logger_(ReferenceCounting::add_ref(logger)),
      block_сache_size_mb_(block_сache_size_mb),
      path_db_(path_db),
      expire_time_(expire_time),
      column_family_name_(column_family_name)
  {
    auto number_threads = std::thread::hardware_concurrency();
    if (number_threads == 0)
    {
      number_threads = 25;
    }

    ManagerPoolConfig config;
    config.event_queue_max_size = 10000000;
    config.io_uring_flags = IORING_SETUP_ATTACH_WQ | IORING_FEAT_FAST_POLL | IOSQE_ASYNC;
    config.io_uring_size = 1024 * 8;
    config.number_io_urings = 2 * number_threads;
    rocksdb_manager_pool_ = std::make_unique<DataBaseManagerPool>(
      config,
      logger_);
  }

  ~ProfileMapIoUringRocksDBFactory() override = default;

  ProfileMap_var create() override
  {
    return ReferenceCounting::SmartPtr<ProfileMap>(
      new ProfileMap(
        new Logging::OStream::Logger(
          Logging::OStream::Config(
            std::cerr,
            Logging::Logger::ERROR)),
        rocksdb_manager_pool_,
        path_db_,
        AdServer::ProfilingCommons::RocksDB::RocksDBParams(
          block_сache_size_mb_,
          expire_time_.microseconds() / 1000000,
          rocksdb::kCompactionStyleLevel),
        column_family_name_));
  }

private:
  const Logger_var logger_;

  const std::uint64_t block_сache_size_mb_;

  const std::string path_db_;

  const Generics::Time expire_time_;

  const std::optional<std::string> column_family_name_;

  DataBaseManagerPoolPtr rocksdb_manager_pool_;
};

class Benchmark : public UServerUtils::Component
{
public:
  using ProfileMapT = ProfileMap<std::string>;
  using ProfileMap_var = ReferenceCounting::SmartPtr<ProfileMapT>;
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;

public:
  explicit Benchmark(
    Logger* logger,
    ProfileMapFactoryPtr&& profile_map_factory,
    WriteStatistics& write_statistics,
    ReadStatistics& read_statistics,
    const std::uint64_t db_size_mb,
    const std::string& key,
    const std::string& value)
    : logger_(ReferenceCounting::add_ref(logger)),
      profile_map_factory_(std::move(profile_map_factory)),
      write_statistics_(write_statistics),
      read_statistics_(read_statistics),
      db_size_bytes_(db_size_mb * 1024 * 1024),
      key_(key),
      value_(value)
  {
  }

  ~Benchmark() override = default;

  auto create_profile_map()
  {
    return profile_map_factory_->create();
  }

  void stop() noexcept
  {
    is_strop_ = true;
  }

protected:
  void write_benchmark(ProfileMapT* profile_map)
  {
    std::uint64_t size = 0;
    const std::uint64_t key_size = key_.size();
    const std::uint64_t value_size = value_.size();

    const std::size_t max_buffer_size =
      value_size + std::numeric_limits<std::uint64_t>::max_digits10;
    Generics::SmartMemBuf_var buffer(new Generics::SmartMemBuf(max_buffer_size));

    while (size < db_size_bytes_ && !is_strop_.load(std::memory_order_relaxed))
    {
      const auto i = counter_.fetch_add(1, std::memory_order_relaxed);
      size = current_size_.fetch_add(
        key_size + value_size + 2 * number_digit(i),
        std::memory_order_relaxed);

      try
      {
        const std::string value = value_ + std::to_string(i);
        buffer->membuf().assign(value.data(), value.size());
        profile_map->save_profile(
          key_ + std::to_string(i),
          Generics::transfer_membuf(buffer));
        write_statistics_.success.fetch_add(1, std::memory_order_relaxed);
        buffer->membuf().clear();
      }
      catch (...)
      {
        write_statistics_.error.fetch_add(1, std::memory_order_relaxed);
      }

      if (size_print_.load(std::memory_order_relaxed) <= size)
      {
        size_print_.fetch_add(diff_size_print_, std::memory_order_relaxed);
        std::stringstream stream;
        stream << "Number write date[%]="
               << (100 * size_print_.load(std::memory_order_relaxed) / db_size_bytes_);
        logger_->info(stream.str());
      }
    }
  }

  void read_benchmark(ProfileMapT* profile_map)
  {
    const auto max_id = counter_.load(std::memory_order_relaxed);

    std::random_device random_device;
    std::mt19937 gen(random_device());
    std::uniform_int_distribution<std::uint64_t> dist(1, max_id - 1);

    while (!is_strop_.load(std::memory_order_relaxed))
    {
      const auto number = dist(gen);
      const auto result = profile_map->get_profile(key_ + std::to_string(number));
      if (!result)
      {
        read_statistics_.error.fetch_add(1, std::memory_order_relaxed);
        continue;
      }

      const auto& membuf = result->membuf();
      if (std::string_view(static_cast<const char*>(membuf.data()), membuf.size()) == value_ + std::to_string(number))
      {
        read_statistics_.success.fetch_add(1, std::memory_order_relaxed);
      }
      else
      {
        read_statistics_.error.fetch_add(1, std::memory_order_relaxed);
      }
    }
  }

private:
  const Logger_var logger_;

  const ProfileMapFactoryPtr profile_map_factory_;

  WriteStatistics& write_statistics_;

  ReadStatistics& read_statistics_;

  const std::uint64_t db_size_bytes_ = 0;

  const std::string key_;

  const std::string value_;

  std::atomic<bool> is_strop_{false};

  std::atomic<std::uint64_t> counter_;

  std::atomic<std::uint64_t> current_size_;

  const std::uint64_t diff_size_print_ = db_size_bytes_ / 100;

  std::atomic<std::uint64_t> size_print_{diff_size_print_};
};

class PrinterStatistics final
{
public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using Thread = std::unique_ptr<std::jthread>;

public:
  PrinterStatistics(
    Logger* logger,
    ReadStatistics& read_statistics,
    const std::size_t time_interval)
    : logger_(ReferenceCounting::add_ref(logger)),
      read_statistics_(read_statistics),
      time_interval_(time_interval)
  {
    thread_ = std::make_unique<std::jthread>([this] (std::stop_token token) {
      while (!token.stop_requested())
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(time_interval_ * 1000));
        const auto success_read =
          read_statistics_.success.exchange(0, std::memory_order_relaxed);
        const auto error_read =
          read_statistics_.error.exchange(0, std::memory_order_relaxed);

        logger_->info(std::string("--------------------"));
        std::ostringstream stream;
        stream << "\n"
               << "Success read[count/s] = "
               << success_read / time_interval_
               << "\n"
               << "Error read[count/s] = "
               << error_read / time_interval_
               << "\n";
        logger_->info(stream.str());
      }
    });
  }

  ~PrinterStatistics()
  {
    thread_->request_stop();
  }

private:
  Logger_var logger_;

  ReadStatistics& read_statistics_;

  const std::size_t time_interval_;

  std::unique_ptr<std::jthread> thread_;
};

class BenchmarkRocksDB final
  : public Benchmark,
    public ReferenceCounting::AtomicImpl
{
private:
  using Threads = std::vector<std::jthread>;
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;

public:
  BenchmarkRocksDB(
    Logger* logger,
    WriteStatistics& write_statistics,
    ReadStatistics& read_statistics,
    const std::size_t time_interval,
    const std::uint64_t db_size_mb,
    const std::string& key,
    const std::string& value,
    const std::string& path_db,
    const Generics::Time& expire_time,
    const std::uint64_t number_threads)
    : Benchmark(
        logger,
        std::make_unique<ProfileMapRocksDBFactory>(path_db, expire_time),
        write_statistics,
        read_statistics,
        db_size_mb,
        key,
        value),
      logger_(ReferenceCounting::add_ref(logger)),
      write_statistics_(write_statistics),
      read_statistics_(read_statistics),
      time_interval_(time_interval),
      number_threads_(number_threads)
  {
  }

  ~BenchmarkRocksDB() override = default;

protected:
  void activate_object_() override
  {
    helper_thread_ = std::make_unique<std::jthread>([this] () {
      std::ostringstream stream;
      stream << "Start write to RocksDB";
      logger_->info(stream.str());
      const auto time_start = std::chrono::high_resolution_clock::now();

      threads_.reserve(number_threads_);
      auto profile_map = create_profile_map();
      for (std::uint32_t i = 1; i <= number_threads_; ++i)
      {
        threads_.emplace_back([this, profile_map = profile_map] () {
          write_benchmark(profile_map.in());
        });
      }

      threads_.clear();

      auto time_finish = std::chrono::high_resolution_clock::now();
      auto elapsed_time = std::chrono::duration<double, std::milli>(time_finish - time_start).count();
      stream = std::ostringstream{};
      stream << "\n\nWWrite to RocksDB is finished"
             << "\nElapsed write time[s]="
             << (static_cast<std::uint64_t>(elapsed_time) / 1000)
             << "\nSuccess write="
             << write_statistics_.success
             << "\nError write="
             << write_statistics_.error;

      profile_map.reset();

      time_finish = std::chrono::high_resolution_clock::now();
      elapsed_time = std::chrono::duration<double, std::milli>(time_finish - time_start).count();
      stream << "\n\nRocksDB is closed"
             << "\nElapsed total time[s]="
             << (static_cast<std::uint64_t>(elapsed_time) / 1000);
      logger_->info(stream.str());

      logger_->info(std::string("\n\nStart read benchmark"));
      auto priter_statistics = std::make_unique<PrinterStatistics>(
        logger_.in(),
        read_statistics_,
        time_interval_);
      profile_map = create_profile_map();
      for (std::uint32_t i = 1; i <= number_threads_; ++i)
      {
        threads_.emplace_back([this, profile_map = profile_map] () {
          read_benchmark(profile_map.in());
        });
      }
      threads_.clear();
      priter_statistics.reset();
      profile_map.reset();
      logger_->info(std::string("\n\nRead benchmark is finished"));
    });
  }

  void deactivate_object_() override
  {
    stop();
  }

  void wait_object_() override
  {
    helper_thread_.reset();
  }

private:
  const Logger_var logger_;

  WriteStatistics& write_statistics_;

  ReadStatistics& read_statistics_;

  const std::size_t time_interval_;

  const std::uint64_t number_threads_;

  Threads threads_;

  std::unique_ptr<std::jthread> helper_thread_;
};

class BenchmarkIoUringRocksDB final
  : public Benchmark,
    public ReferenceCounting::AtomicImpl
{
private:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using Task = userver::engine::TaskWithResult<void>;

public:
  BenchmarkIoUringRocksDB(
    Logger* logger,
    WriteStatistics& write_statistics,
    ReadStatistics& read_statistics,
    const std::size_t time_interval,
    const std::uint64_t db_size_mb,
    const std::string& key,
    const std::string& value,
    const std::uint64_t block_сache_size_mb,
    const std::string& path_db,
    const Generics::Time& expire_time,
    const std::size_t number_coroutines)
    : Benchmark(
        logger,
        std::make_unique<ProfileMapIoUringRocksDBFactory>(
          logger,
          block_сache_size_mb,
          path_db,
          expire_time,
          rocksdb::kDefaultColumnFamilyName),
      write_statistics,
      read_statistics,
      db_size_mb,
      key,
      value),
      logger_(ReferenceCounting::add_ref(logger)),
      write_statistics_(write_statistics),
      read_statistics_(read_statistics),
      time_interval_(time_interval),
      number_coroutines_(number_coroutines)
  {
  }

  ~BenchmarkIoUringRocksDB() override = default;

protected:
  void activate_object_() override
  {
    auto& current_task_processor = userver::engine::current_task::GetTaskProcessor();
    task_ = std::make_unique<Task>(
      userver::utils::Async(
        current_task_processor,
        "benchmark_task",
        &BenchmarkIoUringRocksDB::run,
        this));
  }

  void deactivate_object_() override
  {
    stop();
  }

  void wait_object_() override
  {
    try
    {
      if (task_->IsValid())
      {
        task_->Get();
      }
    }
    catch (const eh::Exception& exc)
    {
      std::ostringstream stream;
      stream << FNS
             << exc.what();
      logger_->error(stream.str());
    }
  }

private:
  void run()
  {
    std::ostringstream stream;
    stream << "Start write to RocksDB";
    logger_->info(stream.str());
    const auto time_start = std::chrono::high_resolution_clock::now();

    auto profile_map = create_profile_map();
    auto& current_task_processor = userver::engine::current_task::GetTaskProcessor();
    std::vector<Task> tasks;
    for (std::size_t i = 1; i <= number_coroutines_; ++i)
    {
      tasks.emplace_back(
        userver::utils::Async(
          current_task_processor,
          "write_benchmark",
          [this, profile_map = profile_map] () {
            write_benchmark(profile_map.in());
          }));
    }
    std::for_each(
      std::begin(tasks),
      std::end(tasks),
      [] (auto& t) {t.Get();});
    tasks.clear();

    auto time_finish = std::chrono::high_resolution_clock::now();
    auto elapsed_time = std::chrono::duration<double, std::milli>(time_finish - time_start).count();
    stream = std::ostringstream{};
    stream << "\n\nWrite to RocksDB is finished"
           << "\nElapsed write time[s]="
           << (static_cast<std::uint64_t>(elapsed_time) / 1000)
           << "\nSuccess write="
           << write_statistics_.success
           << "\nError write="
           << write_statistics_.error;

    profile_map.reset();

    time_finish = std::chrono::high_resolution_clock::now();
    elapsed_time = std::chrono::duration<double, std::milli>(time_finish - time_start).count();
    stream << "\n\nRocksDB is closed"
           << "\nElapsed total time[s]="
           << (static_cast<std::uint64_t>(elapsed_time) / 1000);
    logger_->info(stream.str());

    logger_->info(std::string("\n\nStart read benchmark"));
    profile_map = create_profile_map();
    auto priter_statistics = std::make_unique<PrinterStatistics>(
      logger_.in(),
      read_statistics_,
      time_interval_);
    for (std::size_t i = 1; i <= number_coroutines_; ++i)
    {
      tasks.emplace_back(
        userver::utils::Async(
          current_task_processor,
          "read_benchmark",
          [this, profile_map = profile_map] () {
            read_benchmark(profile_map.in());
          }));
    }
    std::for_each(
      std::begin(tasks),
      std::end(tasks),
      [] (auto& t) {t.Get();});
    tasks.clear();
    profile_map.reset();
    priter_statistics.reset();
    logger_->info(std::string("\n\nRead benchmark is finished"));
  }

private:
  const Logger_var logger_;

  WriteStatistics& write_statistics_;

  ReadStatistics& read_statistics_;

  const std::size_t time_interval_;

  std::unique_ptr<Task> task_;

  const std::size_t number_coroutines_;
};

enum class TestType
{
  RocksDb,
  IoUringRocksDb
};

class Application final
  : public Generics::CompositeActiveObject,
    public ReferenceCounting::AtomicImpl
{
private:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using Benchmark_var = ReferenceCounting::SmartPtr<Benchmark>;

public:
  Application(
    const TestType test_type,
    const std::size_t time_interval,
    const std::string& key,
    const std::string& value,
    const std::uint32_t db_size_mb,
    const std::string& path_db,
    const Generics::Time expire_time,
    const std::size_t number_threads,
    const std::uint32_t block_сache_size_mb,
    const std::size_t number_coroutines)
    : test_type_(test_type),
      logger_(new Logging::OStream::Logger(
        Logging::OStream::Config(
          std::cerr,
          Logging::Logger::INFO))),
      time_interval_(time_interval)
  {
    std::filesystem::remove_all(std::filesystem::path(path_db));

    std::ostringstream stream;
    stream << "\n\nType="
           << (test_type == TestType::RocksDb ? "Old RocksDB" : "IoUring RocksDB")
           << "\nkey size[bytes]=" << key.size()
           << "\nvalue size[bytes]=" << value.size()
           << "\nDb size[mb]=" << db_size_mb;
    if (test_type == TestType::RocksDb)
    {
      stream << "\nNumber threads="
             << number_threads
             << "\n\n";
    }
    else
    {
      stream << "\nNumber coroutines="
             << number_coroutines
             << "\nBlock сache size[mb]="
             << block_сache_size_mb;
    }
    logger_->info(std::string(stream.str()));

    if (test_type == TestType::IoUringRocksDb)
    {
      benchmark_ = new BenchmarkIoUringRocksDB(
        logger_.in(),
        write_statistics_,
        read_statistics_,
        time_interval_,
        db_size_mb,
        key,
        value,
        block_сache_size_mb,
        path_db,
        expire_time,
        number_coroutines);
    }
    else
    {
      benchmark_ = new BenchmarkRocksDB(
        logger_.in(),
        write_statistics_,
        read_statistics_,
        time_interval_,
        db_size_mb,
        key,
        value,
        path_db,
        expire_time,
        number_threads);
    }
  }

  void run()
  {
    if (test_type_ == TestType::IoUringRocksDb)
    {
      Logger_var logger_userver(new Logging::OStream::Logger(
        Logging::OStream::Config(
          std::cerr,
          Logging::Logger::ERROR)));

      UServerUtils::CoroPoolConfig coro_pool_config;
      coro_pool_config.initial_size = 100000;
      coro_pool_config.max_size = 1000000;

      UServerUtils::EventThreadPoolConfig event_thread_pool_config;
      event_thread_pool_config.threads = 4;

      UServerUtils::TaskProcessorConfig main_task_processor_config;
      main_task_processor_config.name = "main_task_processor";
      main_task_processor_config.worker_threads =
        std::thread::hardware_concurrency();
      main_task_processor_config.thread_name = "main_tskpr";
      main_task_processor_config.should_guess_cpu_limit = false;
      main_task_processor_config.wait_queue_length_limit = 200000;

      UServerUtils::TaskProcessorContainerBuilderPtr task_processor_container_builder(
        new UServerUtils::TaskProcessorContainerBuilder(
          logger_userver.in(),
          coro_pool_config,
          event_thread_pool_config,
          main_task_processor_config));

      auto init_func =
        [benchmark = std::move(benchmark_)] (
          UServerUtils::TaskProcessorContainer& task_processor_container) {
          UServerUtils::ComponentsBuilderPtr components_builder(
            new UServerUtils::ComponentsBuilder);
          components_builder->add_user_component(
            "Benchmark",
            benchmark.in());
          return components_builder;
        };

      UServerUtils::Manager_var manager = new UServerUtils::Manager(
        std::move(task_processor_container_builder),
        std::move(init_func),
        logger_userver.in());
      add_child_object(manager);
    }
    else
    {
      add_child_object(benchmark_);
    }

    activate_object();

    userver::utils::SignalCatcher signal_catcher{SIGINT, SIGTERM, SIGQUIT};
    signal_catcher.Catch();

    deactivate_object();
    wait_object();
  }

private:
  WriteStatistics write_statistics_;

  ReadStatistics read_statistics_;

  const TestType test_type_;

  const Logger_var logger_;

  const std::size_t time_interval_;

  Benchmark_var benchmark_;
};

int main(int /*argc*/, char** /*argv*/)
{
  try
  {
    const TestType type = TestType::IoUringRocksDb;
    const std::size_t time_interval = 5;
    const std::string key = "key";
    const std::string value = "value";
    const std::uint32_t db_size_mb = 1024;
    const std::string path_db = "/u03/test/profile_map_test";
    const Generics::Time expire_time(100000);

    // TestType::RocksDb
    const std::size_t number_threads = 300;

    // TestType::IoUringRocksDb
    const std::uint32_t block_сache_size_mb = 256;
    const std::size_t number_coroutines = 10000;

    ReferenceCounting::SmartPtr<Application> application =
      new Application(
        type,
        time_interval,
        key,
        value,
        db_size_mb,
        path_db,
        expire_time,
        number_threads,
        block_сache_size_mb,
        number_coroutines);
    application->run();
  }
  catch (const std::exception& exc)
  {
    std::cerr << "Benchmark is failed. Reason: "
              << exc.what();
  }
  catch (...)
  {
    std::cerr << "Benchmark is failed. Unknown error";
  }

  return EXIT_SUCCESS;
}