// GTEST
#include <gtest/gtest.h>

// STD
#include <future>

// USERVER
#include <userver/concurrent/background_task_storage.hpp>

// THIS
#include <Logger/Logger.hpp>
#include <Logger/StreamLogger.hpp>
#include <UServerUtils/ComponentsBuilder.hpp>
#include <UServerUtils/Manager.hpp>

TEST(Overload, Cancel)
{
  const std::size_t queue_length_limit = 3;

  Logging::Logger_var logger = new Logging::OStream::Logger(
    Logging::OStream::Config(
      std::cerr,
      Logging::Logger::CRITICAL));

  UServerUtils::CoroPoolConfig coro_pool_config;
  UServerUtils::EventThreadPoolConfig event_thread_pool_config;

  UServerUtils::TaskProcessorConfig main_task_processor_config;
  main_task_processor_config.name = "main_task_processor";
  main_task_processor_config.worker_threads = 2;
  main_task_processor_config.thread_name = "main_tskpr";
  main_task_processor_config.overload_action =
    UServerUtils::TaskProcessorConfig::OverloadAction::Cancel;
  main_task_processor_config.wait_queue_length_limit = queue_length_limit;

  UServerUtils::TaskProcessorContainerBuilderPtr task_processor_container_builder =
    std::make_unique<UServerUtils::TaskProcessorContainerBuilder>(
      logger.in(),
      coro_pool_config,
      event_thread_pool_config,
      main_task_processor_config);

  auto init_func = [] (UServerUtils::TaskProcessorContainer& task_processor_container) {
    auto& main_task_processor = task_processor_container.get_main_task_processor();
    std::vector<userver::engine::TaskWithResult<int>> tasks;
    const std::size_t task_number = queue_length_limit * 1000;
    tasks.reserve(task_number);

    std::promise<void> promise;
    auto future = promise.get_future();
    auto task_block = userver::engine::AsyncNoSpan(
      main_task_processor,
      [&future] () {
        future.wait();
      });

    for (std::size_t i = 1; i <= task_number; i += 1)
    {
      tasks.emplace_back(userver::engine::AsyncNoSpan(
        main_task_processor,
        [] () {
          EXPECT_NO_THROW(userver::engine::InterruptibleSleepFor(std::chrono::seconds(100)));
          EXPECT_TRUE(userver::engine::current_task::IsCancelRequested());
          EXPECT_TRUE(userver::engine::current_task::ShouldCancel());

          return 1;
      }));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    promise.set_value();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::size_t number_overload_cancel = 0;
    for (auto& task : tasks)
    {
      EXPECT_NO_THROW(task.SyncCancel());

      try
      {
        const auto result = task.Get();
        EXPECT_TRUE(result == 1);
      }
      catch (userver::engine::TaskCancelledException&)
      {
        number_overload_cancel += 1;
      }
      catch (...)
      {
        EXPECT_TRUE(false);
      }
    }

    EXPECT_TRUE(number_overload_cancel != 0);

    return std::make_unique<UServerUtils::ComponentsBuilder>();
  };

  UServerUtils::Manager_var manager = new UServerUtils::Manager(
    std::move(task_processor_container_builder),
    std::move(init_func),
    logger.in());
  manager->activate_object();
  manager->deactivate_object();
  manager->wait_object();
}

TEST(Overload, Ignore)
{
  const std::size_t queue_length_limit = 3;

  Logging::Logger_var logger = new Logging::OStream::Logger(
    Logging::OStream::Config(
      std::cerr,
      Logging::Logger::CRITICAL));

  UServerUtils::CoroPoolConfig coro_pool_config;
  UServerUtils::EventThreadPoolConfig event_thread_pool_config;

  UServerUtils::TaskProcessorConfig main_task_processor_config;
  main_task_processor_config.name = "main_task_processor";
  main_task_processor_config.worker_threads = 2;
  main_task_processor_config.thread_name = "main_tskpr";
  main_task_processor_config.overload_action =
  UServerUtils::TaskProcessorConfig::OverloadAction::Ignore;
  main_task_processor_config.wait_queue_length_limit = queue_length_limit;

  UServerUtils::TaskProcessorContainerBuilderPtr task_processor_container_builder =
    std::make_unique<UServerUtils::TaskProcessorContainerBuilder>(
      logger.in(),
      coro_pool_config,
      event_thread_pool_config,
      main_task_processor_config);

  auto init_func = [] (UServerUtils::TaskProcessorContainer& task_processor_container) {
    auto& main_task_processor = task_processor_container.get_main_task_processor();
    std::vector<userver::engine::TaskWithResult<int>> tasks;
    const std::size_t task_number = queue_length_limit * 1000;
    tasks.reserve(task_number);

    std::promise<void> promise;
    auto future = promise.get_future();
    auto task_block = userver::engine::AsyncNoSpan(
      main_task_processor,
      [&future] () {
        future.wait();
      });

    for (std::size_t i = 1; i <= task_number; i += 1)
    {
      tasks.emplace_back(userver::engine::AsyncNoSpan(
        main_task_processor,
        [] () {
          EXPECT_NO_THROW(userver::engine::InterruptibleSleepFor(std::chrono::seconds(100)));
          EXPECT_TRUE(userver::engine::current_task::IsCancelRequested());
          EXPECT_TRUE(userver::engine::current_task::ShouldCancel());

          return 1;
        }));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    promise.set_value();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    for (auto& task : tasks)
    {
      EXPECT_NO_THROW(task.SyncCancel());

      const auto result = task.Get();
      EXPECT_TRUE(result == 1);
    }

    return std::make_unique<UServerUtils::ComponentsBuilder>();
  };

  UServerUtils::Manager_var manager = new UServerUtils::Manager(
    std::move(task_processor_container_builder),
    std::move(init_func),
    logger.in());
  manager->activate_object();
  manager->deactivate_object();
  manager->wait_object();
}

TEST(BackgroundTaskStorage, test)
{
  Logging::Logger_var logger = new Logging::OStream::Logger(
    Logging::OStream::Config(
      std::cerr,
      Logging::Logger::CRITICAL));

  UServerUtils::CoroPoolConfig coro_pool_config;
  UServerUtils::EventThreadPoolConfig event_thread_pool_config;

  UServerUtils::TaskProcessorConfig main_task_processor_config;
  main_task_processor_config.name = "main_task_processor";
  main_task_processor_config.worker_threads = 2;
  main_task_processor_config.thread_name = "main_tskpr";
  main_task_processor_config.overload_action =
    UServerUtils::TaskProcessorConfig::OverloadAction::Ignore;
  main_task_processor_config.wait_queue_length_limit = 10000000;

  UServerUtils::TaskProcessorContainerBuilderPtr task_processor_container_builder =
    std::make_unique<UServerUtils::TaskProcessorContainerBuilder>(
      logger.in(),
      coro_pool_config,
      event_thread_pool_config,
      main_task_processor_config);

  auto init_func = [] (UServerUtils::TaskProcessorContainer& task_processor_container) {
    auto& main_task_processor = task_processor_container.get_main_task_processor();

    userver::concurrent::BackgroundTaskStorage background_task_storage;

    std::vector<userver::engine::TaskWithResult<void>> tasks;
    const std::size_t task_number = 1000;
    tasks.reserve(task_number);

    std::atomic<std::size_t> counter{0};
    for (std::size_t i = 1; i <= task_number; i += 1)
    {
      background_task_storage.Detach(
        userver::engine::AsyncNoSpan(
          main_task_processor,
          [&counter] () {
            EXPECT_NO_THROW(userver::engine::InterruptibleSleepFor(std::chrono::seconds(100)));
            EXPECT_TRUE(userver::engine::current_task::IsCancelRequested());
            EXPECT_TRUE(userver::engine::current_task::ShouldCancel());
            counter.fetch_add(1);
          }));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    background_task_storage.CancelAndWait();
    EXPECT_EQ(counter.load(), task_number);

    return std::make_unique<UServerUtils::ComponentsBuilder>();
  };

  UServerUtils::Manager_var manager = new UServerUtils::Manager(
    std::move(task_processor_container_builder),
    std::move(init_func),
    logger.in());
  manager->activate_object();
  manager->deactivate_object();
  manager->wait_object();
}