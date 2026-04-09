#include <list>
#include <vector>
#include <iterator>
#include <memory>
#include <iostream>

#include <Generics/MemBuf.hpp>
#include <Generics/AppUtils.hpp>
#include <Generics/Uuid.hpp>
#include <CORBACommons/CorbaAdapters.hpp>
#include <Logger/StreamLogger.hpp>
#include <grpcpp/grpcpp.h>

#include <Commons/CorbaAlgs.hpp>
#include <Commons/GrpcAlgs.hpp>

#include <UserInfoSvcs/UserBindServer/UserBindServerGrpc.grpc.pb.h>

#include "Application.hpp"

namespace
{
  using GrpcClient = adserver::user_info_svcs::user_bind::UserBindServerGrpc;

  const char USAGE[] =
    "\nUsage: \nUserBindAdmin <command> <command arguments>\n\n"
    "Synopsis 1:\n"
    "UserBindAdmin print-bind-request <bind request id> "
      "(-r[--reference=]<user_bind_corba_ref>|<user_info_manager_controller_corba_ref> | "
      "-g[--grpc-endpoint=]<host:port>)"
      "\n"
    "UserBindAdmin add-user-id <external id> <user id>"
      "(-r[--reference=]<user_bind_corba_ref>|<user_info_manager_controller_corba_ref> | "
      "-g[--grpc-endpoint=]<host:port>)"
      "\n"
    "UserBindAdmin get-user-id <external id>"
      "(-r[--reference=]<user_bind_corba_ref>|<user_info_manager_controller_corba_ref> | "
      "-g[--grpc-endpoint=]<host:port>)"
      "\n"
    "\n";
}

Application_::Application_() noexcept
{}

Application_::~Application_() noexcept
{}

void
Application_::print_bind_request_grpc_(
  GrpcClient::Stub* user_bind_stub,
  const String::SubString& bind_request_id)
  noexcept
{
  try
  {
    grpc::ClientContext context;
    adserver::user_info_svcs::user_bind::GetBindRequestRequest request;
    request.set_request_id(bind_request_id.str());
    request.set_timestamp(GrpcAlgs::pack_time(Generics::Time::get_time_of_day()));

    adserver::user_info_svcs::user_bind::GetBindRequestResponse response;
    const auto status = user_bind_stub->get_bind_request(&context, request, &response);

    if(!status.ok())
    {
      std::cerr << "Caught grpc::Status: " << status.error_message() << std::endl;
      return;
    }

    for(const auto& user_id : response.bind_user_ids())
    {
      std::cout << user_id << std::endl;
    }
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "Caught eh::Exception: " << ex.what() << std::endl;
  }
}

void
Application_::print_bind_request_(
  AdServer::UserInfoSvcs::UserBindMapper* user_bind_mapper,
  const String::SubString& bind_request_id)
  noexcept
{
  try
  {
    AdServer::UserInfoSvcs::UserBindServer::BindRequestInfo_var bind_request_info =
      user_bind_mapper->get_bind_request(
        bind_request_id.str().c_str(),
        CorbaAlgs::pack_time(Generics::Time::get_time_of_day()));

    for(CORBA::ULong i = 0; i < bind_request_info->bind_user_ids.length(); ++i)
    {
      std::cout << bind_request_info->bind_user_ids[i] << std::endl;
    }
  }
  catch(const AdServer::UserInfoSvcs::UserBindMapper::NotReady& )
  {
    std::cerr << "Caught UserBindMapper::NotReady" << std::endl;
  }
  catch(const AdServer::UserInfoSvcs::UserBindMapper::ChunkNotFound& e)
  {
    std::cerr << "Caught UserBindMapper::ChunkNotFound: " <<
      e.description << std::endl;
  }
  catch(const AdServer::UserInfoSvcs::UserBindMapper::ImplementationException& e)
  {
    std::cerr << "Caught UserBindMapper::ImplementationException: " <<
      e.description << std::endl;
  }
  catch(const CORBA::SystemException& e)
  {
    std::cerr << "Caught CORBA::SystemException: " << e << std::endl;
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "Caught eh::Exception: " << ex.what() << std::endl;
  }
}

void
Application_::add_user_id_grpc_(
  GrpcClient::Stub* user_bind_stub,
  std::string_view external_id,
  const AdServer::Commons::UserId& user_id)
  noexcept
{
  try
  {
    grpc::ClientContext context;
    adserver::user_info_svcs::user_bind::AddUserIdRequest request;
    request.set_id(std::string(external_id));
    request.set_timestamp(GrpcAlgs::pack_time(Generics::Time::get_time_of_day()));
    request.set_user_id(GrpcAlgs::pack_user_id(user_id));

    adserver::user_info_svcs::user_bind::AddUserIdResponse response;
    const auto status = user_bind_stub->add_user_id(&context, request, &response);

    if(!status.ok())
    {
      std::cerr << "Caught grpc::Status: " << status.error_message() << std::endl;
      return;
    }

    std::cout <<
      "merge_user_id = " <<
      GrpcAlgs::unpack_user_id(response.merge_user_id()) << std::endl <<
      "invalid_operation = " << response.invalid_operation() << std::endl;
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "Caught eh::Exception: " << ex.what() << std::endl;
  }
}

void
Application_::add_user_id_(
  AdServer::UserInfoSvcs::UserBindMapper* user_bind_mapper,
  std::string_view external_id,
  const AdServer::Commons::UserId& user_id)
  noexcept
{
  try
  {
    AdServer::UserInfoSvcs::UserBindMapper::AddUserRequestInfo add_user_request;
    add_user_request.id << std::string(external_id);
    add_user_request.user_id = CorbaAlgs::pack_user_id(user_id);
    add_user_request.timestamp = CorbaAlgs::pack_time(Generics::Time::get_time_of_day());

    AdServer::UserInfoSvcs::UserBindMapper::AddUserResponseInfo_var add_user_response_info =
      user_bind_mapper->add_user_id(add_user_request);

    std::cout <<
      "merge_user_id = " <<
      CorbaAlgs::unpack_user_id(add_user_response_info->merge_user_id) << std::endl <<
      "invalid_operation = " << add_user_response_info->invalid_operation << std::endl;
  }
  catch(const AdServer::UserInfoSvcs::UserBindMapper::NotReady& )
  {
    std::cerr << "Caught UserBindMapper::NotReady" << std::endl;
  }
  catch(const AdServer::UserInfoSvcs::UserBindMapper::ChunkNotFound& e)
  {
    std::cerr << "Caught UserBindMapper::ChunkNotFound: " <<
      e.description << std::endl;
  }
  catch(const AdServer::UserInfoSvcs::UserBindMapper::ImplementationException& e)
  {
    std::cerr << "Caught UserBindMapper::ImplementationException: " <<
      e.description << std::endl;
  }
  catch(const CORBA::SystemException& e)
  {
    std::cerr << "Caught CORBA::SystemException: " << e << std::endl;
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "Caught eh::Exception: " << ex.what() << std::endl;
  }
}

void
Application_::get_user_id_grpc_(
  GrpcClient::Stub* user_bind_stub,
  std::string_view external_id)
  noexcept
{
  try
  {
    auto now = Generics::Time::get_time_of_day();
    grpc::ClientContext context;
    adserver::user_info_svcs::user_bind::GetUserIdRequest request;
    request.set_id(std::string(external_id));
    request.set_timestamp(GrpcAlgs::pack_time(now));
    request.set_silent(true);
    request.set_generate_user_id(false);
    request.set_for_set_cookie(false);
    request.set_create_timestamp(GrpcAlgs::pack_time(now));
    request.set_current_user_id(GrpcAlgs::pack_user_id(AdServer::Commons::UserId()));

    adserver::user_info_svcs::user_bind::GetUserIdResponse response;
    const auto status = user_bind_stub->get_user_id(&context, request, &response);

    if(!status.ok())
    {
      std::cerr << "Caught grpc::Status: " << status.error_message() << std::endl;
      return;
    }

    std::cout <<
      "user_id = " <<
      GrpcAlgs::unpack_user_id(response.user_id()).to_string() << std::endl <<
      "min_age_reached = " << response.min_age_reached() << std::endl <<
      "created = " << response.created() << std::endl <<
      "invalid_operation = " << response.invalid_operation() << std::endl <<
      "user_found = " << response.user_found() << std::endl;
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "Caught eh::Exception: " << ex.what() << std::endl;
  }
}

void
Application_::get_user_id_(
  AdServer::UserInfoSvcs::UserBindMapper* user_bind_mapper,
  std::string_view external_id)
  noexcept
{
  try
  {
    auto now = Generics::Time::get_time_of_day();
    AdServer::UserInfoSvcs::UserBindMapper::GetUserRequestInfo get_user_request;
    get_user_request.id << std::string(external_id);
    get_user_request.timestamp = CorbaAlgs::pack_time(now);
    get_user_request.silent = true;
    get_user_request.generate_user_id = false;
    get_user_request.for_set_cookie = false;
    get_user_request.create_timestamp = CorbaAlgs::pack_time(now);
    get_user_request.current_user_id = CorbaAlgs::pack_user_id(AdServer::Commons::UserId());

    AdServer::UserInfoSvcs::UserBindMapper::GetUserResponseInfo_var get_user_response_info =
      user_bind_mapper->get_user_id(get_user_request);

    std::cout <<
      "user_id = " <<
      CorbaAlgs::unpack_user_id(get_user_response_info->user_id).to_string() << std::endl <<
      "min_age_reached = " << get_user_response_info->min_age_reached << std::endl <<
      "created = " << get_user_response_info->created << std::endl <<
      "invalid_operation = " << get_user_response_info->invalid_operation << std::endl <<
      "user_found = " << get_user_response_info->user_found << std::endl
      ;
  }
  catch(const AdServer::UserInfoSvcs::UserBindMapper::NotReady& )
  {
    std::cerr << "Caught UserBindMapper::NotReady" << std::endl;
  }
  catch(const AdServer::UserInfoSvcs::UserBindMapper::ChunkNotFound& e)
  {
    std::cerr << "Caught UserBindMapper::ChunkNotFound: " <<
      e.description << std::endl;
  }
  catch(const AdServer::UserInfoSvcs::UserBindMapper::ImplementationException& e)
  {
    std::cerr << "Caught UserBindMapper::ImplementationException: " <<
      e.description << std::endl;
  }
  catch(const CORBA::SystemException& e)
  {
    std::cerr << "Caught CORBA::SystemException: " << e << std::endl;
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "Caught eh::Exception: " << ex.what() << std::endl;
  }
}

void
Application_::main(int& argc, char** argv)
  /*throw(eh::Exception)*/
{
  Generics::AppUtils::CheckOption opt_help;
  Generics::AppUtils::Option<std::string> opt_user_bind_mapper_ref("");
  Generics::AppUtils::Option<std::string> opt_user_bind_grpc_endpoint("");

  Generics::AppUtils::Args args(-1);

  Generics::AppUtils::Option<unsigned long> opt_portion;

  args.add(
    Generics::AppUtils::equal_name("help") ||
    Generics::AppUtils::short_name("h"),
    opt_help);

  args.add(
    Generics::AppUtils::equal_name("reference") ||
    Generics::AppUtils::short_name("r"),
    opt_user_bind_mapper_ref);

  args.add(
    Generics::AppUtils::equal_name("grpc-endpoint") ||
    Generics::AppUtils::short_name("g"),
    opt_user_bind_grpc_endpoint);

  args.parse(argc - 1, argv + 1);

  Generics::AppUtils::Args::CommandList commands = args.commands();

  if(commands.empty() || opt_help.enabled() || *commands.begin() == "help")
  {
    std::cout << USAGE << std::endl;
    return;
  }

  const std::string command = commands.front();
  commands.pop_front();

  const bool use_grpc = opt_user_bind_grpc_endpoint.installed();
  if(!opt_user_bind_mapper_ref.installed() && !use_grpc)
  {
    Stream::Error ostr;
    ostr << "Not defined parameters: reference (-r/--reference) or grpc endpoint "
      "(-g/--grpc-endpoint) must be specified.";
    throw Exception(ostr);
  }

  if(command == "print-bind-request" || command == "add-user-id" ||
    command == "get-user-id")
  {
    std::unique_ptr<GrpcClient::Stub> user_bind_grpc_stub;
    AdServer::UserInfoSvcs::UserBindMapper_var user_bind_mapper;

    if(use_grpc)
    {
      user_bind_grpc_stub = GrpcClient::NewStub(
        grpc::CreateChannel(
          *opt_user_bind_grpc_endpoint,
          grpc::InsecureChannelCredentials()));
    }
    else
    {
      CORBACommons::CorbaClientAdapter_var corba_client_adapter(
        new CORBACommons::CorbaClientAdapter());

      bool success = false;

      try
      {
        CORBA::Object_var user_user_bind_mapper_obj =
          corba_client_adapter->resolve_object(
            CORBACommons::CorbaObjectRef(opt_user_bind_mapper_ref->c_str()));

        user_bind_mapper = AdServer::UserInfoSvcs::UserBindServer::_narrow(
          user_user_bind_mapper_obj);

        if(!CORBA::is_nil(user_bind_mapper.in()))
        {
          success = true;
        }
      }
      catch(const CORBA::SystemException& ex)
      {
        Stream::Error ostr;
        ostr << "Caught CORBA::SystemException: " << ex;
        throw Exception(ostr);
      }

      if(!success)
      {
        try
        {
          AdServer::UserInfoSvcs::UserBindOperationDistributor::
            ControllerRefList controller_groups;

          {
            AdServer::UserInfoSvcs::UserBindOperationDistributor::
              ControllerRef controller_ref_group;

            CORBACommons::CorbaObjectRef ref;
            ref.object_ref = opt_user_bind_mapper_ref->c_str();
            ref.type = CORBACommons::CorbaObjectRef::CT_NON_SECURE;

            controller_ref_group.push_back(ref);
            controller_groups.push_back(controller_ref_group);
          }

          Logging::Logger_var logger = new Logging::OStream::Logger(
            Logging::OStream::Config(std::cerr));

          AdServer::UserInfoSvcs::UserBindOperationDistributor_var distributor =
            new AdServer::UserInfoSvcs::UserBindOperationDistributor(
              logger,
              controller_groups,
              corba_client_adapter);

          user_bind_mapper = ReferenceCounting::add_ref(distributor);

          distributor->activate_object();

          success = true;
        }
        catch(const CORBACommons::CorbaClientAdapter::Exception&)
        {}
        catch(const CORBA::SystemException&)
        {}
        catch(const eh::Exception&)
        {}
      }
    }

    if(command == "print-bind-request")
    {
      if(commands.empty())
      {
        Stream::Error ostr;
        ostr << "print-bind-request: not defined bind request id.";
        throw Exception(ostr);
      }

      const std::string bind_request_id = commands.front();
      commands.pop_front();

      if(use_grpc)
      {
        print_bind_request_grpc_(user_bind_grpc_stub.get(), bind_request_id);
      }
      else
      {
        print_bind_request_(user_bind_mapper, bind_request_id);
      }
    }
    else if(command == "add-user-id")
    {
      if(commands.size() != 2)
      {
        Stream::Error ostr;
        ostr << "add-user-id: expected arguments: <external id> <user id>.";
        throw Exception(ostr);
      }

      const std::string external_id = commands.front();
      commands.pop_front();

      const std::string user_id_str = commands.front();
      commands.pop_front();

      if(use_grpc)
      {
        add_user_id_grpc_(
          user_bind_grpc_stub.get(),
          external_id,
          AdServer::Commons::UserId(user_id_str));
      }
      else
      {
        add_user_id_(
          user_bind_mapper,
          external_id,
          AdServer::Commons::UserId(user_id_str));
      }
    }
    else if(command == "get-user-id")
    {
      if(commands.size() != 1)
      {
        Stream::Error ostr;
        ostr << "get-user-id: expected arguments: <external id>";
        throw Exception(ostr);
      }

      const std::string external_id = commands.front();
      commands.pop_front();

      if(use_grpc)
      {
        get_user_id_grpc_(user_bind_grpc_stub.get(), external_id);
      }
      else
      {
        get_user_id_(user_bind_mapper, external_id);
      }
    }
  }
  else
  {
    std::cerr << "Unknown command '" << command << "'. "
      "See help for more info." << std::endl;
  }
}

int
main(int argc, char** argv)
{
  Application_* app = 0;

  try
  {
    app = &Application::instance();
  }
  catch (...)
  {
    std::cerr << "main(): Critical: Got exception while "
      "creating application object.\n";
    return -1;
  }

  if (app == 0)
  {
    std::cerr << "main(): Critical: got NULL application object.\n";
    return -1;
  }

  try
  {
    app->main(argc, argv);
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "Caught eh::Exception: " << ex.what() << std::endl;
    return -1;
  }

  return 0;
}
