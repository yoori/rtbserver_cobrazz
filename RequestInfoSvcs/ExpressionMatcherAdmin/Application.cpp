#include "Application.hpp"

#include <iostream>
#include <locale.h>
#include <string>

#include <grpcpp/grpcpp.h>

#include <Generics/AppUtils.hpp>

#include <Commons/UserInfoManip.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/RequestTriggerMatchProfile.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/RequestTriggerMatchProfileUtils.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/UserChannelInventoryProfile.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/UserChannelInventoryProfileUtils.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/UserTriggerMatchProfile.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/UserTriggerMatchProfileUtils.hpp>

namespace
{
  namespace Proto = adserver::request_info_svcs::expression_matcher;

  const char USAGE[] =
    "[OPTIONS] ( help | print | print-est | daily-process | "
      "print-user-trigger-match | print-request-trigger-match | "
      "print-household-colo-reach )\n"
    "OPTIONS:\n"
    "  -r, --ref, --expression-matcher : ExpressionMatcher gRPC endpoint.\n"
    "  -i, --id, --user-id : user id (for print command).\n"
    "  -s, --sync : do daily processing synchronously.\n"
    "  -t, --temporary : print temporary user trigger profile.\n"
    "\n"
    "NOTES:\n"
    "  if uid value starts with symbol '-' escape it with '\\'\n"
    "Sample:\n"
    "  ExpressionMatcherAdmin print -i <user-id> -r host:port\n";

  std::string
  remove_uuid_escape(const char* uuid)
  {
    return uuid[0] == '\\' ? std::string(uuid + 1) : std::string(uuid);
  }

  std::string
  normalize_endpoint(std::string endpoint)
  {
    constexpr std::string_view corbaloc_prefix = "corbaloc::";
    constexpr std::string_view iiop_prefix = "corbaloc:iiop:";
    if(endpoint.compare(0, corbaloc_prefix.size(), corbaloc_prefix) == 0)
    {
      endpoint.erase(0, corbaloc_prefix.size());
    }
    else if(endpoint.compare(0, iiop_prefix.size(), iiop_prefix) == 0)
    {
      endpoint.erase(0, iiop_prefix.size());
    }

    const std::size_t object_key_pos = endpoint.find('/');
    if(object_key_pos != std::string::npos)
    {
      endpoint.resize(object_key_pos);
    }
    return endpoint;
  }

  bool
  check_status(const grpc::Status& status)
  {
    if(status.ok())
    {
      return true;
    }

    std::cerr << "ExpressionMatcher gRPC call failed: code=" <<
      status.error_code() << ", message=" << status.error_message() << '\n';
    return false;
  }

  const char*
  command_id(
    const Generics::AppUtils::Args::CommandList& commands,
    const Generics::AppUtils::Option<std::string>& option)
  {
    return commands.size() > 1 ? (++commands.begin())->c_str() : option->c_str();
  }
}

Application_::Application_() noexcept = default;
Application_::~Application_() noexcept = default;

void
Application_::print(Stub& expression_matcher, const char* user_id_str) noexcept
{
  try
  {
    AdServer::Commons::UserId user_id(user_id_str);
    Proto::ProfileRequest request;
    request.set_id(user_id_str);
    Proto::ProfileResponse response;
    grpc::ClientContext context;
    if(!check_status(expression_matcher.get_inventory_profile(
         &context, request, &response)))
    {
      return;
    }

    if(!response.found())
    {
      std::cout << "Profile not found." << std::endl;
      return;
    }

    const std::string& profile = response.profile();
    AdServer::RequestInfoSvcs::UserChannelInventoryProfileReader reader(
      profile.data(), profile.size());
    AdServer::RequestInfoSvcs::print_user_channel_inventory_profile(
      std::cout, user_id_str, reader);
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "Caught eh::Exception: " << ex.what() << std::endl;
  }
}

void
Application_::print_user_trigger_match(
  Stub& expression_matcher,
  const char* user_id_str,
  bool temporary) noexcept
{
  try
  {
    AdServer::Commons::UserId user_id(user_id_str);
    Proto::UserTriggerMatchProfileRequest request;
    request.set_user_id(user_id_str);
    request.set_temporary_user(temporary);
    Proto::ProfileResponse response;
    grpc::ClientContext context;
    if(!check_status(expression_matcher.get_user_trigger_match_profile(
         &context, request, &response)))
    {
      return;
    }

    if(!response.found())
    {
      std::cout << "Profile not found." << std::endl;
      return;
    }

    const std::string& profile = response.profile();
    AdServer::RequestInfoSvcs::UserTriggerMatchReader reader(
      profile.data(), profile.size());
    AdServer::RequestInfoSvcs::print_user_trigger_match_profile(
      std::cout, user_id_str, reader);
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "Caught eh::Exception: " << ex.what() << std::endl;
  }
}

void
Application_::print_request_trigger_match(
  Stub& expression_matcher,
  const char* request_id_str) noexcept
{
  try
  {
    AdServer::Commons::RequestId request_id(request_id_str);
    Proto::ProfileRequest request;
    request.set_id(request_id_str);
    Proto::ProfileResponse response;
    grpc::ClientContext context;
    if(!check_status(expression_matcher.get_request_trigger_match_profile(
         &context, request, &response)))
    {
      return;
    }

    if(!response.found())
    {
      std::cout << "Profile not found." << std::endl;
      return;
    }

    const std::string& profile = response.profile();
    AdServer::RequestInfoSvcs::RequestTriggerMatchReader reader(
      profile.data(), profile.size());
    AdServer::RequestInfoSvcs::print_request_trigger_match_profile(
      std::cout, request_id_str, reader);
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "Caught eh::Exception: " << ex.what() << std::endl;
  }
}

void
Application_::print_household_colo_reach(
  Stub& expression_matcher,
  const char* user_id_str) noexcept
{
  try
  {
    AdServer::Commons::UserId user_id(user_id_str);
    Proto::ProfileRequest request;
    request.set_id(user_id_str);
    Proto::ProfileResponse response;
    grpc::ClientContext context;
    if(!check_status(expression_matcher.get_household_colo_reach_profile(
         &context, request, &response)))
    {
      return;
    }

    if(!response.found())
    {
      std::cout << "Profile not found." << std::endl;
      return;
    }

    const std::string& profile = response.profile();
    AdServer::RequestInfoSvcs::UserColoReachProfileReader reader(
      profile.data(), profile.size());
    AdServer::RequestInfoSvcs::print_user_colo_reach_profile(
      std::cout, user_id_str, reader);
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
    if(!::setlocale(LC_CTYPE, "en_US.utf8"))
    {
      std::cout << "Cannot set locale." << std::endl;
    }

    Generics::AppUtils::CheckOption opt_help;
    Generics::AppUtils::CheckOption opt_sync;
    Generics::AppUtils::CheckOption opt_temporary;
    Generics::AppUtils::Option<std::string> opt_expression_matcher;
    Generics::AppUtils::Option<std::string> opt_user_id;
    Generics::AppUtils::Args args(-1);

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

    const Generics::AppUtils::Args::CommandList& commands = args.commands();
    if(commands.empty() || opt_help.enabled() || *commands.begin() == "help")
    {
      std::cout << USAGE << std::endl;
      return;
    }

    const std::string& command = *commands.begin();
    if(command != "print" &&
       command != "print-est" &&
       command != "print-user-trigger-match" &&
       command != "print-request-trigger-match" &&
       command != "print-household-colo-reach" &&
       command != "daily-process")
    {
      std::cout << "Unknown command '" << command << "'. See usage:\n" <<
        USAGE << std::endl;
      return;
    }

    if(!opt_expression_matcher.installed())
    {
      std::cout << "ExpressionMatcher gRPC endpoint isn't specified.\n" <<
        USAGE << std::endl;
      return;
    }

    const std::string endpoint = normalize_endpoint(*opt_expression_matcher);
    auto channel = grpc::CreateChannel(
      endpoint,
      grpc::InsecureChannelCredentials());
    std::unique_ptr<Stub> expression_matcher =
      Proto::ExpressionMatcherGrpc::NewStub(channel);

    if(command == "daily-process")
    {
      Proto::RunDailyProcessingRequest request;
      request.set_synchronous(opt_sync.enabled());
      Proto::RunDailyProcessingResponse response;
      grpc::ClientContext context;
      check_status(expression_matcher->run_daily_processing(
        &context, request, &response));
      return;
    }

    // Kept for compatibility with existing scripts. The legacy command did
    // not issue an ExpressionMatcher request either.
    if(command == "print-est")
    {
      return;
    }

    if(!(opt_user_id.installed() || commands.size() > 1))
    {
      std::cout << "Profile id isn't specified.\n" << USAGE << std::endl;
      return;
    }

    const std::string id = remove_uuid_escape(command_id(commands, opt_user_id));
    if(command == "print")
    {
      print(*expression_matcher, id.c_str());
    }
    else if(command == "print-user-trigger-match")
    {
      print_user_trigger_match(
        *expression_matcher,
        id.c_str(),
        opt_temporary.enabled());
    }
    else if(command == "print-request-trigger-match")
    {
      print_request_trigger_match(*expression_matcher, id.c_str());
    }
    else
    {
      print_household_colo_reach(*expression_matcher, id.c_str());
    }
  }
  catch(const std::exception& ex)
  {
    std::cerr << "Caught std::exception: " << ex.what() << std::endl;
  }
}

int
main(int argc, char** argv)
{
  Application::instance().main(argc, argv);
}
