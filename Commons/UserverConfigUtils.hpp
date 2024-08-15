#ifndef _COMMONS_USERVERCONFIGUTILS_HPP_
#define _COMMONS_USERVERCONFIGUTILS_HPP_

// THIS
#include <Logger/Logger.hpp>
#include <UServerUtils/Grpc/Client/ConfigPoolCoro.hpp>
#include <UServerUtils/Grpc/Server/ServerBuilder.hpp>
#include <UServerUtils/TaskProcessorContainerBuilder.hpp>
#include <xsd/AdServerCommons/AdServerCommons.hpp>

namespace Config
{

inline
UServerUtils::TaskProcessorContainerBuilderPtr
create_task_processor_container_builder(
  Logging::Logger* logger,
  const xsd::AdServer::Configuration::CoroutineType& coro_config)
{
  using CoroPoolConfig = UServerUtils::CoroPoolConfig;
  using EventThreadPoolConfig = UServerUtils::EventThreadPoolConfig;
  using TaskProcessorConfig = UServerUtils::TaskProcessorConfig;
  using TaskProcessorContainerBuilder = UServerUtils::TaskProcessorContainerBuilder;
  using OverloadActionType = xsd::AdServer::Configuration::OverloadActionType;

  const auto& coro_pool = coro_config.CoroPool();
  CoroPoolConfig coro_pool_config;
  coro_pool_config.initial_size = coro_pool.initial_size();
  coro_pool_config.max_size = coro_pool.max_size();
  coro_pool_config.stack_size = coro_pool.stack_size();

  const auto& event_thread_pool = coro_config.EventThreadPool();
  EventThreadPoolConfig event_thread_pool_config;
  event_thread_pool_config.threads = event_thread_pool.number_threads();
  event_thread_pool_config.thread_name = event_thread_pool.name();
  event_thread_pool_config.defer_events = event_thread_pool.defer_events();
  event_thread_pool_config.ev_default_loop_disabled = event_thread_pool.ev_default_loop_disabled();

  const auto& main_task_processor = coro_config.MainTaskProcessor();
  TaskProcessorConfig main_task_processor_config;
  main_task_processor_config.name = main_task_processor.name();
  main_task_processor_config.should_guess_cpu_limit = main_task_processor.should_guess_cpu_limit();
  main_task_processor_config.worker_threads = main_task_processor.number_threads();

  switch (main_task_processor.overload_action()) {
    case OverloadActionType::cancel:
      main_task_processor_config.overload_action =
        TaskProcessorConfig::OverloadAction::Cancel;
      break;
    case OverloadActionType::ignore:
      main_task_processor_config.overload_action =
        TaskProcessorConfig::OverloadAction::Ignore;
      break;
  }

  main_task_processor_config.wait_queue_length_limit =
    main_task_processor.wait_queue_length_limit();
  main_task_processor_config.wait_queue_time_limit =
    std::chrono::microseconds(main_task_processor.wait_queue_time_limit());
  main_task_processor_config.sensor_wait_queue_time_limit =
    std::chrono::microseconds(main_task_processor.sensor_wait_queue_time_limit());

  return std::make_unique<TaskProcessorContainerBuilder>(
    logger,
    coro_pool_config,
    event_thread_pool_config,
    main_task_processor_config);
}

inline
UServerUtils::Grpc::Server::ServerBuilderPtr
create_grpc_server_builder(
  Logging::Logger* logger,
  const xsd::AdServer::Configuration::GrpcServerType& server_config)
{
  using ServerConfig = UServerUtils::Grpc::Server::ConfigCoro;
  using ServerBuilder = UServerUtils::Grpc::Server::ServerBuilder;

  ServerConfig config;
  const auto& num_threads = server_config.num_threads();
  if (num_threads)
  {
    config.num_threads = *server_config.num_threads();
  }
  config.ip = server_config.ip();
  config.port = server_config.port();

  const auto& channel_args = server_config.ChannelArgs();
  auto& channel_arg = channel_args.ChannelArg();
  for (auto& arg : channel_arg)
  {
    config.channel_args[arg.key()] = arg.value();
  }

  return std::make_unique<ServerBuilder>(config, logger);
}

inline
auto create_pool_client_config(
  const xsd::AdServer::Configuration::GrpcClientPoolType& config_client)
{
  UServerUtils::Grpc::Client::ConfigPoolCoro config;

  std::stringstream stream;
  stream << config_client.ip()
         << ":"
         << config_client.port();
  config.endpoint = stream.str();

  if (config.number_threads.has_value())
  {
    config.number_threads = config.number_threads.value();
  }

  const auto& channel_args = config_client.ChannelArgs();
  auto& channel_arg = channel_args.ChannelArg();
  for (auto& arg : channel_arg)
  {
    config.channel_args[arg.key()] = arg.value();
  }

  config.number_async_client = config_client.num_clients();
  config.number_channels = config_client.num_channels();

  return std::pair{config, config_client.timeout()};
}

} // namespace Config

#endif /*_COMMONS_USERVERCONFIGUTILS_HPP_*/
