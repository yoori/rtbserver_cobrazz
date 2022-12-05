#include <iostream>

#include <String/StringManip.hpp>
#include <Generics/AppUtils.hpp>

#include <Commons/UserInfoManip.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/UserChannelInventoryProfile.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/UserChannelInventoryProfileUtils.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/UserTriggerMatchProfile.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/UserTriggerMatchProfileUtils.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/RequestTriggerMatchProfile.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/RequestTriggerMatchProfileUtils.hpp>

#include <UtilCommons/Table.hpp>

#include "Application.hpp"

namespace
{
  const char USAGE[] =
    "[OPTIONS] ( help | print | daily-process | "
      "print-user-trigger-match | print-request-trigger-match | print-household-colo-reach )\n"
    "OPTIONS:\n"
    "  -r, --ref, --expression-matcher : ExpressionMatcher CORBA reference.\n"
    "  -i, --id, --user-id : user id (for print command).\n"
    "  -s, --sync : do daily processing synchroniously (for daily-process command).\n"
    "\n"
    "NOTES:\n"
    "  if uid value start with symbol \'-\' escape it with \'\\\'\n"
    "Sample:\n"
    "  ExpressionMatcherAdmin print ''"
    " -r corbaloc::xen.ocslab.com:12116/ExpressionMatcher\n";
}

namespace
{
  std::string
  remove_uuid_escape(const char* uuid)
  {
    if(uuid[0] == '\\')
    {
      return std::string(uuid + 1);
    }

    return uuid;
  }
}

/**/
Application_::Application_() noexcept
{
}

Application_::~Application_() noexcept
{
}

AdServer::RequestInfoSvcs::ExpressionMatcher*
Application_::resolve_expression_matcher(
  const CORBACommons::CorbaClientAdapter* corba_client_adapter,
  const CORBACommons::CorbaObjectRef& corba_object_ref)
  /*throw(eh::Exception, Exception, CORBA::SystemException)*/
{
  CORBA::Object_var expression_matcher_obj =
    corba_client_adapter->resolve_object(corba_object_ref);

  AdServer::RequestInfoSvcs::ExpressionMatcher_var
    expression_matcher =
      AdServer::RequestInfoSvcs::ExpressionMatcher::_narrow(
        expression_matcher_obj.in());

  if(CORBA::is_nil(expression_matcher.in()))
  {
    throw Exception(
      "_narrow for expression-matcher return nil reference.");
  }

  return expression_matcher._retn();
}

void
Application_::print(
  AdServer::RequestInfoSvcs::ExpressionMatcher* expression_matcher,
  const char* user_id_str)
  noexcept
{
  try
  {
    AdServer::Commons::UserId user_id(user_id_str);

    AdServer::RequestInfoSvcs::UserInventoryProfile_var inv_profile;

    if(expression_matcher->get_inventory_profile(
         user_id_str,
         inv_profile.out()))
    {
      AdServer::RequestInfoSvcs::UserChannelInventoryProfileReader
        inv_profile_reader(
          inv_profile->get_buffer(),
          inv_profile->length());

      AdServer::RequestInfoSvcs::print_user_channel_inventory_profile(
        std::cout, user_id_str, inv_profile_reader);
    }
    else
    {
      std::cout << "Profile not found." << std::endl;
    }
  }
  catch(const AdServer::RequestInfoSvcs::
        ExpressionMatcher::ImplementationException& e)
  {
    std::cerr <<
      "Caught ExpressionMatcher::ImplementationException: " <<
      e.description << std::endl;
  }
  catch(const AdServer::RequestInfoSvcs::
        ExpressionMatcher::NotReady& e)
  {
    std::cerr << "Caught ExpressionMatcher::NotReady." << std::endl;
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
Application_::print_user_trigger_match(
  AdServer::RequestInfoSvcs::ExpressionMatcher* expression_matcher,
  const char* user_id_str,
  bool temporary)
  noexcept
{
  try
  {
    AdServer::Commons::UserId user_id(user_id_str);

    AdServer::RequestInfoSvcs::UserTriggerMatchProfile_var profile;

    if(expression_matcher->get_user_trigger_match_profile(
         user_id_str,
         temporary,
         profile.out()))
    {
      AdServer::RequestInfoSvcs::UserTriggerMatchReader
        profile_reader(
          profile->get_buffer(),
          profile->length());

      AdServer::RequestInfoSvcs::print_user_trigger_match_profile(
        std::cout,
        user_id_str,
        profile_reader);
    }
    else
    {
      std::cout << "Profile not found." << std::endl;
    }
  }
  catch(const AdServer::RequestInfoSvcs::
        ExpressionMatcher::ImplementationException& e)
  {
    std::cerr
      << "Caught ExpressionMatcher::ImplementationException: "
      << e.description << std::endl;
  }
  catch(const AdServer::RequestInfoSvcs::
        ExpressionMatcher::NotReady& e)
  {
    std::cerr << "Caught ExpressionMatcher::NotReady." << std::endl;
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
Application_::print_request_trigger_match(
  AdServer::RequestInfoSvcs::ExpressionMatcher* expression_matcher,
  const char* request_id_str)
  noexcept
{
  try
  {
    AdServer::Commons::UserId request_id(request_id_str);

    AdServer::RequestInfoSvcs::RequestTriggerMatchProfile_var profile;

    if(expression_matcher->get_request_trigger_match_profile(
         request_id_str,
         profile.out()))
    {
      AdServer::RequestInfoSvcs::RequestTriggerMatchReader
        profile_reader(
          profile->get_buffer(),
          profile->length());

      AdServer::RequestInfoSvcs::print_request_trigger_match_profile(
        std::cout,
        request_id_str,
        profile_reader);
    }
    else
    {
      std::cout << "Profile not found." << std::endl;
    }
  }
  catch(const AdServer::RequestInfoSvcs::
        ExpressionMatcher::ImplementationException& e)
  {
    std::cerr <<
      "Caught ExpressionMatcher::ImplementationException: " <<
      e.description << std::endl;
  }
  catch(const AdServer::RequestInfoSvcs::
        ExpressionMatcher::NotReady& e)
  {
    std::cerr << "Caught ExpressionMatcher::NotReady." << std::endl;
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
Application_::print_household_colo_reach(
  AdServer::RequestInfoSvcs::ExpressionMatcher* expression_matcher,
  const char* user_id_str)
  noexcept
{
  try
  {
    AdServer::Commons::UserId user_id(user_id_str);

    AdServer::RequestInfoSvcs::HouseholdColoReachProfile_var profile;

    if(expression_matcher->get_household_colo_reach_profile(
         user_id_str,
         profile.out()))
    {
      AdServer::RequestInfoSvcs::UserColoReachProfileReader
        profile_reader(
          profile->get_buffer(),
          profile->length());

      AdServer::RequestInfoSvcs::print_user_colo_reach_profile(
        std::cout,
        user_id_str,
        profile_reader);
    }
    else
    {
      std::cout << "Profile not found." << std::endl;
    }
  }
  catch(const AdServer::RequestInfoSvcs::
        ExpressionMatcher::ImplementationException& e)
  {
    std::cerr <<
      "Caught ExpressionMatcher::ImplementationException: " <<
      e.description << std::endl;
  }
  catch(const AdServer::RequestInfoSvcs::
        ExpressionMatcher::NotReady& e)
  {
    std::cerr << "Caught ExpressionMatcher::NotReady." << std::endl;
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
Application_::main(int& argc, char** argv) noexcept
{
  try
  {
    if (!::setlocale(LC_CTYPE, "en_US.utf8"))
    {
      std::cout << "Cannot set locale." << std::endl;
    }

    Generics::AppUtils::CheckOption opt_help;
    Generics::AppUtils::CheckOption opt_sync;
    Generics::AppUtils::Option<std::string> opt_expression_matcher;
    Generics::AppUtils::Option<std::string> opt_user_id;
    Generics::AppUtils::CheckOption opt_temporary;
    Generics::AppUtils::Args args(-1);

    try
    {
      args.add(
        Generics::AppUtils::equal_name("help") ||
        Generics::AppUtils::short_name("h"),
        opt_help);

      args.add(
        Generics::AppUtils::equal_name("expression-matcher") ||
        Generics::AppUtils::equal_name("ref") ||
        Generics::AppUtils::short_name("r"),
        opt_expression_matcher);

      args.add(
        Generics::AppUtils::equal_name("id") ||
        Generics::AppUtils::equal_name("user_id") ||
        Generics::AppUtils::short_name("i"),
        opt_user_id);

      args.add(
        Generics::AppUtils::equal_name("sync") ||
        Generics::AppUtils::short_name("s"),
        opt_sync);

      args.add(
        Generics::AppUtils::equal_name("temporary") ||
        Generics::AppUtils::short_name("t"),
        opt_temporary);

      args.parse(argc - 1, argv + 1);
    }
    catch(const eh::Exception& e)
    {
      std::cerr << "Caught exception on parsing arguments:"
        << e.what() << std::endl;
    }

    const Generics::AppUtils::Args::CommandList& commands = args.commands();

    if(commands.empty() || opt_help.enabled())
    {
      std::cout << USAGE << std::endl;
      return;
    }

    std::string command = *commands.begin();

    if (command == "print" ||
        command == "print-est" ||
        command == "print-user-trigger-match" ||
        command == "print-request-trigger-match" ||
        command == "print-household-colo-reach" ||
        command == "daily-process")
    {
      if(!opt_expression_matcher.installed())
      {
        std::cout << "Can't do print, daily-process commands without request info manager reference." <<
          std::endl << USAGE << std::endl;
        return;
      }

      CORBACommons::CorbaClientAdapter_var corba_client_adapter(
        new CORBACommons::CorbaClientAdapter());

      CORBACommons::CorbaObjectRef corba_object_ref(
        opt_expression_matcher->c_str());

      AdServer::RequestInfoSvcs::ExpressionMatcher_var expression_matcher =
        resolve_expression_matcher(
          corba_client_adapter.in(), corba_object_ref);

      if(command == "daily-process")
      {
        try
        {
          expression_matcher->run_daily_processing(opt_sync.enabled());
        }
        catch(const AdServer::RequestInfoSvcs::
              ExpressionMatcher::ImplementationException& e)
        {
          std::cerr <<
            "Caught ExpressionMatcher::ImplementationException: " <<
            e.description << std::endl;
        }
        catch(const AdServer::RequestInfoSvcs::
              ExpressionMatcher::NotReady& e)
        {
          std::cerr << "Caught ExpressionMatcher::NotReady." << std::endl;
        }
        catch(const CORBA::SystemException& e)
        {
          std::cerr << "Caught CORBA::SystemException: " << e << std::endl;
        }
        catch(const eh::Exception& ex)
        {
          std::cerr << "Caught eh::Exception: " << ex.what() << std::endl;
        }
      } // print
      else
      {
        if(!(opt_user_id.installed() || commands.size() > 1))
        {
          std::cout << "Can't do print command without user id." << std::endl
                    << USAGE << std::endl;
          return;
        }

        if(command == "print")
        {
          print(
            expression_matcher,
            remove_uuid_escape(
              commands.size() > 1 ?
                (++commands.begin())->c_str():
                opt_user_id->c_str()).c_str());
        }
        else if(command == "print-user-trigger-match")
        {
          print_user_trigger_match(
            expression_matcher,
            remove_uuid_escape(
              commands.size() > 1 ?
                (++commands.begin())->c_str():
                opt_user_id->c_str()).c_str(),
            opt_temporary.enabled());
        }
        else if(command == "print-request-trigger-match")
        {
          print_request_trigger_match(
            expression_matcher,
            remove_uuid_escape(
              commands.size() > 1 ?
                (++commands.begin())->c_str():
                opt_user_id->c_str()).c_str());
        }
        else if(command == "print-household-colo-reach")
        {
          print_household_colo_reach(
            expression_matcher,
            remove_uuid_escape(
              commands.size() > 1 ?
                (++commands.begin())->c_str():
                opt_user_id->c_str()).c_str());
        }
      }
    }
    else
    {
      std::cout << "Unknown command '" << command <<
        "'. See usage:" << USAGE << std::endl;
    }
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

  app->main(argc, argv);
}
