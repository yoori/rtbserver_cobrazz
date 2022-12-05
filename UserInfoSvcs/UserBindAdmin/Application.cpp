#include <list>
#include <vector>
#include <iterator>
#include <iostream>

#include <Generics/MemBuf.hpp>
#include <Generics/AppUtils.hpp>
#include <Generics/Uuid.hpp>
#include <CORBACommons/CorbaAdapters.hpp>
#include <Logger/StreamLogger.hpp>

#include <Commons/CorbaAlgs.hpp>

#include "Application.hpp"

namespace
{
  const char TIME_FORMAT[] = "%d-%m-%Y:%H-%M-%S.%q";
  const char DAY_TIME_FORMAT[] = "%H-%M-%S.%q";
}

namespace
{
  const char USAGE[] =
    "\nUsage: \nUserBindAdmin <command> <command arguments>\n\n"
    "Synopsis 1:\n"
    "UserBindAdmin print-bind-request --uid=<user id in base64 format> "
      "--page-channels=<channel ids separated by ','> "
      "-r[--reference=]<user_bind_corba_ref>|"
      "<user_info_manager_controller_corba_ref> "
      "(-r[--reference=]<user_info_manager_corba_ref>) \n\n";
}

Application_::Application_()
  noexcept
{}

Application_::~Application_() noexcept
{}

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
Application_::main(int& argc, char** argv)
  /*throw(eh::Exception)*/
{
  Generics::AppUtils::CheckOption opt_help;
  Generics::AppUtils::Option<std::string> opt_user_bind_mapper_ref("");

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
  
  args.parse(argc - 1, argv + 1);
 
  Generics::AppUtils::Args::CommandList commands = args.commands();

  if(commands.empty() || opt_help.enabled() || *commands.begin() == "help")
  {
    std::cout << USAGE << std::endl;
    return;
  }

  const std::string command = commands.front();
  commands.pop_front();

  if(!opt_user_bind_mapper_ref.installed())
  {
    Stream::Error ostr;
    ostr << "Not defined parameters: reference to the manager or controller must be specifed.";
    throw Exception(ostr);
  }

  CORBACommons::CorbaClientAdapter_var corba_client_adapter(
    new CORBACommons::CorbaClientAdapter());

  if(command == "print-bind-request")
  {
    AdServer::UserInfoSvcs::UserBindMapper_var user_bind_mapper;

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

      print_bind_request_(
        user_bind_mapper,
        bind_request_id);
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


