#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <grpcpp/grpcpp.h>

#include <Generics/AppUtils.hpp>
#include <Generics/ActiveObject.hpp>
#include <Generics/Time.hpp>
#include <Generics/Uuid.hpp>
#include <Logger/ActiveObjectCallback.hpp>
#include <Logger/StreamLogger.hpp>

#include <Commons/Grpc/GrpcExecutor.hpp>
#include <Commons/Grpc/GrpcSync.hpp>
#include <Commons/GrpcAlgs.hpp>
#include <Commons/BoostAsioContextRunActiveObject.hpp>
#include <UserBindControllerGrpc.grpc.pb.h>
#include <UserBindServerGrpc.grpc-client.hpp>
#include <UserInfoSvcs/UserBindClient/UserBindDistributedGrpcClient.hpp>

namespace
{
  namespace Proto = adserver::user_info_svcs::user_bind;
  namespace Controller = adserver::user_info_svcs::user_bind_controller;

  const std::chrono::seconds RPC_TIMEOUT(10);

  const char USAGE[] =
    "\nUsage: \nUserBindAdmin <command> <command arguments>\n\n"
    "UserBindAdmin print-bind-request <bind request id> "
      "-r[--reference=]<user_bind_controller_grpc_endpoint>|"
      "<user_bind_server_grpc_endpoint>\n"
    "UserBindAdmin add-user-id <external id> <user id> "
      "-r[--reference=]<grpc endpoint>\n"
    "UserBindAdmin get-user-id <external id> "
      "-r[--reference=]<grpc endpoint>\n\n";

  bool is_user_bind_controller_(const std::string& reference)
  {
    auto stub = Controller::UserBindControllerGrpc::NewStub(
      grpc::CreateChannel(reference, grpc::InsecureChannelCredentials()));

    grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + RPC_TIMEOUT);
    Controller::GetSessionDescriptionRequest request;
    Controller::GetSessionDescriptionResponse response;
    const auto status = stub->get_session_description(
      &context,
      request,
      &response);

    return status.ok() && !response.user_bind_servers().empty();
  }

  struct ClientHolder
  {
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor;
    std::shared_ptr<AdServer::Commons::BoostAsioContextRunActiveObject>
      coalesce_runner;
    std::shared_ptr<Generics::ActiveObject> active_object;
    std::shared_ptr<AdServer::UserInfoSvcs::UserBindServerGrpcAsyncClient>
      client;
  };

  ClientHolder create_client_(const std::string& reference)
  {
    ClientHolder result;
    result.grpc_executor = std::make_shared<AdServer::Grpc::GrpcExecutor>(1);
    result.grpc_executor->activate_object();
    Logging::Logger_var logger =
      new Logging::OStream::Logger(Logging::OStream::Config(std::cerr));
    result.coalesce_runner =
      std::make_shared<AdServer::Commons::BoostAsioContextRunActiveObject>(
        new Logging::ActiveObjectCallbackImpl(logger, "UserBindAdmin", "gRPC"),
        std::make_shared<boost::asio::io_service>(),
        1);
    result.coalesce_runner->activate_object();

    if (is_user_bind_controller_(reference))
    {
      auto client =
        std::make_shared<AdServer::UserInfoSvcs::UserBindDistributedGrpcClient>(
          std::vector<std::string>{reference},
          AdServer::Grpc::BatchingOptions(),
          result.grpc_executor,
          logger,
          result.coalesce_runner);
      result.client = client;
      result.active_object = client;
    }
    else
    {
      auto client = std::make_shared<
        AdServer::UserInfoSvcs::UserBindServerGrpcAsyncBatchingClient>(
          reference,
          result.grpc_executor,
          result.coalesce_runner,
          AdServer::Grpc::BatchingOptions());
      result.client = client;
      result.active_object = client;
    }

    result.active_object->activate_object();
    return result;
  }

  void shutdown_client_(ClientHolder& holder) noexcept
  {
    try
    {
      if (holder.active_object)
      {
        holder.active_object->deactivate_object();
        holder.active_object->wait_object();
      }
      if (holder.grpc_executor)
      {
        holder.grpc_executor->deactivate_object();
        holder.grpc_executor->wait_object();
      }
      if (holder.coalesce_runner)
      {
        holder.coalesce_runner->deactivate_object();
        holder.coalesce_runner->wait_object();
      }
    }
    catch (...)
    {}
  }

  void print_bind_request_(
    AdServer::UserInfoSvcs::UserBindServerGrpcAsyncClient* client,
    const std::string& bind_request_id)
  {
    Proto::GetBindRequestRequest request;
    request.set_request_id(bind_request_id);
    request.set_timestamp(GrpcAlgs::pack_time(Generics::Time::get_time_of_day()));

    const auto response =
      AdServer::Grpc::sync_call<Proto::GetBindRequestResponse>(
        [&](auto callback)
        {
          client->get_bind_request(request, std::move(callback));
        });

    for (const auto& user_id : response.bind_user_ids())
    {
      std::cout << user_id << std::endl;
    }
  }

  void add_user_id_(
    AdServer::UserInfoSvcs::UserBindServerGrpcAsyncClient* client,
    const std::string& external_id,
    const AdServer::Commons::UserId& user_id)
  {
    Proto::AddUserIdRequest request;
    request.set_id(external_id);
    request.set_timestamp(GrpcAlgs::pack_time(Generics::Time::get_time_of_day()));
    request.set_user_id(GrpcAlgs::pack_user_id(user_id));

    const auto response =
      AdServer::Grpc::sync_call<Proto::AddUserIdResponse>(
        [&](auto callback)
        {
          client->add_user_id(request, std::move(callback));
        });

    std::cout <<
      "merge_user_id = " <<
      GrpcAlgs::unpack_user_id(response.merge_user_id()) << std::endl <<
      "invalid_operation = " << response.invalid_operation() << std::endl;
  }

  void get_user_id_(
    AdServer::UserInfoSvcs::UserBindServerGrpcAsyncClient* client,
    const std::string& external_id)
  {
    const auto now = Generics::Time::get_time_of_day();
    Proto::GetUserIdRequest request;
    request.set_id(external_id);
    request.set_timestamp(GrpcAlgs::pack_time(now));
    request.set_silent(true);
    request.set_generate_user_id(false);
    request.set_for_set_cookie(false);
    request.set_create_timestamp(GrpcAlgs::pack_time(now));
    request.set_current_user_id(GrpcAlgs::pack_user_id(
      AdServer::Commons::UserId()));

    const auto response =
      AdServer::Grpc::sync_call<Proto::GetUserIdResponse>(
        [&](auto callback)
        {
          client->get_user_id(request, std::move(callback));
        });

    std::cout <<
      "user_id = " <<
      GrpcAlgs::unpack_user_id(response.user_id()).to_string() << std::endl <<
      "min_age_reached = " << response.min_age_reached() << std::endl <<
      "created = " << response.created() << std::endl <<
      "invalid_operation = " << response.invalid_operation() << std::endl <<
      "user_found = " << response.user_found() << std::endl;
  }
}

int main(int argc, char** argv)
{
  try
  {
    Generics::AppUtils::CheckOption opt_help;
    Generics::AppUtils::Option<std::string> opt_reference;
    Generics::AppUtils::Args args(-1);

    args.add(
      Generics::AppUtils::equal_name("help") ||
      Generics::AppUtils::short_name("h"),
      opt_help);
    args.add(
      Generics::AppUtils::equal_name("reference") ||
      Generics::AppUtils::short_name("r") ||
      Generics::AppUtils::equal_name("grpc-endpoint") ||
      Generics::AppUtils::short_name("g"),
      opt_reference);
    args.parse(argc - 1, argv + 1);

    auto commands = args.commands();
    if (commands.empty() || opt_help.enabled() || commands.front() == "help")
    {
      std::cout << USAGE << std::endl;
      return 0;
    }

    if (!opt_reference.installed())
    {
      throw std::runtime_error("reference to gRPC server or controller is required");
    }

    const std::string command = commands.front();
    commands.pop_front();

    ClientHolder holder = create_client_(*opt_reference);
    auto holder_guard = std::unique_ptr<ClientHolder, void(*)(ClientHolder*)>(
      &holder,
      [](ClientHolder* value)
      {
        shutdown_client_(*value);
      });

    if (command == "print-bind-request")
    {
      if (commands.size() != 1)
      {
        throw std::runtime_error(
          "print-bind-request: expected argument: <bind request id>");
      }
      print_bind_request_(holder.client.get(), commands.front());
    }
    else if (command == "add-user-id")
    {
      if (commands.size() != 2)
      {
        throw std::runtime_error(
          "add-user-id: expected arguments: <external id> <user id>");
      }
      const std::string external_id = commands.front();
      commands.pop_front();
      add_user_id_(
        holder.client.get(),
        external_id,
        AdServer::Commons::UserId(commands.front()));
    }
    else if (command == "get-user-id")
    {
      if (commands.size() != 1)
      {
        throw std::runtime_error("get-user-id: expected argument: <external id>");
      }
      get_user_id_(holder.client.get(), commands.front());
    }
    else
    {
      std::cerr << "Unknown command '" << command << "'. See help for more info." <<
        std::endl;
      return 1;
    }

    return 0;
  }
  catch (const std::exception& ex)
  {
    std::cerr << "Caught std::exception: " << ex.what() << std::endl;
    return 1;
  }
}
