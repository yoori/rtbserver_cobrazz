// UNIX_COMMONS
#include <Generics/AppUtils.hpp>
#include <Logger/StreamLogger.hpp>
#include <UServerUtils/Grpc/ComponentsBuilder.hpp>
#include <UServerUtils/Grpc/Manager.hpp>

// PROTO
#include "UserInfoSvcs/UserBindServer/proto/UserBindServer_client.cobrazz.pb.hpp"

// THIS
#include <Commons/GrpcAlgs.hpp>
#include <UserInfoSvcs/UserBindController/GrpcUserBindOperationDistributor.hpp>
#include "Application.hpp"

namespace
{

const char TIME_FORMAT[] = "%Y-%m-%d";

const char USAGE[] =
  "\nUsage: \nUserBindAdmin <command> <command arguments>\n\n"
  "Schemes:\n"
  "UserBindAdmin add-user-id --external_id=<value> --timestamp=<%Y-%m-%d - '2005-10-15'> "
  "--user_id=<value> --endpoint_grpc=<host:port>[--reference_corba=<user_bind_corba_ref>]\n"
  "UserBindAdmin get_user_id --external_id=<value> --current_user_id=<value> "
  "--timestamp=<%Y-%m-%d - '2005-10-15'> --create_timestamp=<%Y-%m-%d - '2005-10-15'> "
  "--silent=<true/false> --generate_user_id=<true/false> --for_set_cookie=<true/false> "
  "--endpoint_grpc=<host:port>[--reference_corba=<user_bind_corba_ref>]\n\n";

void print_error(const AdServer::UserInfoSvcs::Error& error)
{
  std::stringstream stream;
  stream << "Error occure. Type=";

  switch (error.type())
  {
    case AdServer::UserInfoSvcs::Error_Type_NotReady:
    {
      stream << "NotReady";
      break;
    }
    case AdServer::UserInfoSvcs::Error_Type_ChunkNotFound:
    {
      stream << "ChunkNotFound";
      break;
    }
    case AdServer::UserInfoSvcs::Error_Type_Implementation:
    {
      stream << "Implementation";
      break;
    }
    default:
    {
      stream << "Unknown";
      break;
    }
  }

  stream << ". Description="
         << error.description();
  std::cerr << stream.str();
}

} // namespace

using namespace Generics::AppUtils;
using namespace UServerUtils::Grpc::Core::Client;
using namespace AdServer::UserInfoSvcs;
using namespace UServerUtils::Grpc;

void Application::run(int argc, char** argv)
{
  using ConfigPoolClient = UServerUtils::Grpc::Core::Client::ConfigPoolCoro;
  using ManagerCoro = UServerUtils::Grpc::Manager;
  using ManagerCoro_var = UServerUtils::Grpc::Manager_var;

  CheckOption opt_help;
  Option<std::string> opt_reference_corba("");
  Option<std::string> opt_endpoint_grpc("");
  Option<std::string> opt_external_id("");
  Option<std::string> opt_user_id("");
  Option<std::string> opt_timestamp("");
  Option<bool> opt_silent(true);
  Option<bool> opt_generate_user_id(true);
  Option<bool> opt_for_set_cookie(true);
  Option<std::string> opt_create_timestamp("");
  Option<std::string> opt_current_user_id("");

  Args args(-1);

  Option<unsigned long> opt_portion;
  
  args.add(
    equal_name("help") ||
    short_name("h"),
    opt_help);

  args.add(
    equal_name("reference_corba"),
    opt_reference_corba);

  args.add(
    equal_name("endpoint_grpc"),
    opt_endpoint_grpc);

  args.add(
    equal_name("external_id"),
    opt_external_id);

  args.add(
    equal_name("user_id"),
    opt_user_id);

  args.add(
    equal_name("timestamp"),
    opt_timestamp);

  args.add(
    equal_name("create_timestamp"),
    opt_create_timestamp);

  args.add(
    equal_name("silent"),
    opt_silent);

  args.add(
    equal_name("generate_user_id"),
    opt_generate_user_id);

  args.add(
    equal_name("for_set_cookie"),
    opt_for_set_cookie);

  args.add(
    equal_name("current_user_id"),
    opt_current_user_id);
  
  args.parse(argc - 1, argv + 1);

  Args::CommandList commands = args.commands();
  if(commands.empty() || opt_help.enabled() || *commands.begin() == "help")
  {
    std::cout << USAGE << std::endl;
    return;
  }

  const std::string command = commands.front();
  commands.pop_front();

  if(!opt_reference_corba.installed() && !opt_endpoint_grpc.installed())
  {
    Stream::Error stream;
    stream << "Endpoint to grpc server or reference to"
              " the manager controller must be specifed";
    throw Exception(stream);
  }

  Logging::Logger_var logger = new Logging::OStream::Logger(
    Logging::OStream::Config(std::cerr));

  CoroPoolConfig coro_pool_config;
  EventThreadPoolConfig event_thread_pool_config;
  TaskProcessorConfig main_task_processor_config;
  main_task_processor_config.name = "main_task_processor";
  main_task_processor_config.worker_threads = 3;
  main_task_processor_config.thread_name = "main_tskpr";

  ConfigPoolClient config_grpc_client;
  config_grpc_client.number_async_client = 5;
  config_grpc_client.number_channels = 5;

  auto init_func = [] (
    TaskProcessorContainer& task_processor_container) {
      return std::make_unique<ComponentsBuilder>();
  };
  auto task_processor_container_builder =
    std::make_unique<TaskProcessorContainerBuilder>(
      logger.in(),
      coro_pool_config,
      event_thread_pool_config,
      main_task_processor_config);

  ManagerCoro_var manager_coro(
    new ManagerCoro(
      std::move(task_processor_container_builder),
      std::move(init_func),
      logger.in()));
  manager_coro->activate_object();

  GrpcUserBindOperationDistributor_var grpc_user_bind_distributor;
  std::unique_ptr<GrpcCobrazzPoolClientFactory> factory;

  std::exception_ptr eptr;
  try
  {
    if (opt_reference_corba.installed())
    {
      GrpcUserBindOperationDistributor::ControllerRefList controller_groups;
      GrpcUserBindOperationDistributor::ControllerRef controller_ref_group;
      CORBACommons::CorbaClientAdapter_var corba_client_adapter(
        new CORBACommons::CorbaClientAdapter());

      CORBACommons::CorbaObjectRef ref;
      ref.object_ref = opt_reference_corba->c_str();
      ref.type = CORBACommons::CorbaObjectRef::CT_NON_SECURE;
      controller_ref_group.push_back(ref);
      controller_groups.push_back(controller_ref_group);

      UServerUtils::Grpc::Core::Common::SchedulerPtr scheduler =
        UServerUtils::Grpc::Core::Common::Utils::create_scheduler(
          3,
          logger.in());

      grpc_user_bind_distributor =
        GrpcUserBindOperationDistributor_var(
          new GrpcUserBindOperationDistributor(
            logger.in(),
            manager_coro->get_main_task_processor(),
            scheduler,
            controller_groups,
            corba_client_adapter,
            config_grpc_client,
            1500));
      grpc_user_bind_distributor->activate_object();
    }
    else
    {
      config_grpc_client.endpoint = *opt_endpoint_grpc;
      factory = std::make_unique<GrpcCobrazzPoolClientFactory>(
        logger.in(),
        config_grpc_client);
    }

    if (command == "add-user-id")
    {
      if (!opt_external_id.installed())
      {
        throw Exception ("Missing option --external_id=<value>");
      }
      if (!opt_user_id.installed())
      {
        throw Exception ("Missing option --user_id=<value>");
      }
      if (!opt_timestamp.installed())
      {
        throw Exception ("Missing option --timestamp=<%Y-%m-%d>");
      }

      const std::string external_id = *opt_external_id;
      const Generics::Time timestamp(*opt_timestamp, "%Y-%m-%d");
      const std::string user_id = *opt_user_id;

      std::unique_ptr<AddUserIdResponse> response;
      if (grpc_user_bind_distributor)
      {
        response = grpc_user_bind_distributor->add_user_id(
          external_id,
          timestamp,
          user_id);
      }
      else
      {
        std::unique_ptr<AddUserIdRequest> request =
          std::make_unique<AddUserIdRequest>();
        request->set_user_id(user_id);
        request->set_id(external_id);
        request->set_timestamp(GrpcAlgs::pack_time(timestamp));
        auto client = factory->create<UserBindService_add_user_id_ClientPool>(
          manager_coro->get_main_task_processor());
        auto result = client->write(std::move(request), 1500);
        if (result.status == Status::Ok)
        {
          response = std::move(result.response);
        }
        else if (result.status == Status::Timeout)
        {
          std::cerr << "Timeout is reached\n";
        }
        else if (result.status == Status::InternalError)
        {
          std::cerr << "Internal error occure\n";
        }
      }

      if (!response)
      {
        std::cout << "Response: is null" << std::endl;
      }
      else if (response->has_error())
      {
        const auto& error = response->error();
        print_error(error);
      }
      else
      {
        const auto& info = response->info();
        std::cout << "\nResponse: "
                  << "\n  invalid_operation=" << info.invalid_operation()
                  << "\n  merge_user_id=" << info.merge_user_id()
                  << "\n\n"
                  << std::endl;
      }
    }
    else if (command == "get-user-id")
    {
      if (!opt_external_id.installed())
      {
        throw Exception ("Missing option --external_id=<value>");
      }
      if (!opt_current_user_id.installed())
      {
        throw Exception ("Missing option --current_user_id=<value>");
      }
      if (!opt_timestamp.installed())
      {
        throw Exception ("Missing option --timestamp=<%Y-%m-%d>");
      }
      if (!opt_create_timestamp.installed())
      {
        throw Exception ("Missing option --create_timestamp=<%Y-%m-%d>");
      }
      if (!opt_silent.installed())
      {
        throw Exception ("Missing option --silent=<true/false>");
      }
      if (!opt_generate_user_id.installed())
      {
        throw Exception ("Missing option --generate_user_id=<true/false>");
      }
      if (!opt_for_set_cookie.installed())
      {
        throw Exception ("Missing option --for_set_cookie=<true/false>");
      }

      const std::string external_id = *opt_external_id;
      const std::string current_user_id = *opt_current_user_id;
      const Generics::Time timestamp(*opt_timestamp, "%Y-%m-%d");
      const Generics::Time create_timestamp(*opt_create_timestamp, "%Y-%m-%d");
      const bool silent = *opt_silent;
      const bool generate_user_id = *opt_generate_user_id;
      const bool for_set_cookie = *opt_for_set_cookie;

      std::unique_ptr<GetUserIdResponse> response;
      if (grpc_user_bind_distributor)
      {
        response = grpc_user_bind_distributor->get_user_id(
          external_id,
          current_user_id,
          timestamp,
          create_timestamp,
          silent,
          generate_user_id,
          for_set_cookie);
      }
      else
      {
        std::unique_ptr<GetUserIdRequest> request =
          std::make_unique<GetUserIdRequest>();
        request->set_id(external_id);
        request->set_current_user_id(current_user_id);
        request->set_timestamp(GrpcAlgs::pack_time(timestamp));
        request->set_create_timestamp(GrpcAlgs::pack_time(create_timestamp));
        request->set_silent(silent);
        request->set_generate_user_id(generate_user_id);
        request->set_for_set_cookie(for_set_cookie);

        auto client = factory->create<UserBindService_get_user_id_ClientPool>(
          manager_coro->get_main_task_processor());
        auto result = client->write(std::move(request), 1000);
        if (result.status == Status::Ok)
        {
          response = std::move(result.response);
        }
        else if (result.status == Status::Timeout)
        {
          std::cerr << "Timeout is reached\n";
        }
        else if (result.status == Status::InternalError)
        {
          std::cerr << "Internal error occure\n";
        }
      }

      if (!response)
      {
        std::cerr << "Response: is null";
      }
      else if (response->has_error())
      {
        print_error(response->error());
      }
      else
      {
        const auto& info = response->info();
        std::cout << "\nResponse: "
                  << "\n  user_id=" << info.user_id()
                  << "\n  min_age_reached=" << info.min_age_reached()
                  << "\n  created=" << info.created()
                  << "\n  invalid_operation=" << info.invalid_operation()
                  << "\n  user_found=" << info.user_found()
                  << "\n\n"
                  << std::endl;
      }
    }
  }
  catch (...)
  {
    eptr = std::current_exception();
  }

  if (grpc_user_bind_distributor)
  {
    try
    {
      grpc_user_bind_distributor->deactivate_object();
      grpc_user_bind_distributor->wait_object();
    }
    catch (...)
    {
    }
  }

  try
  {
    manager_coro->deactivate_object();
    manager_coro->wait_object();
  }
  catch (...)
  {
  }

  if (eptr)
  {
    std::rethrow_exception(eptr);
  }
}

