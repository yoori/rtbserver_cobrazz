#include <iostream>
#include <memory>
#include <string>

#include <boost/asio.hpp>

#include <eh/Exception.hpp>
#include <Generics/AppUtils.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/Time.hpp>
#include <Logger/ActiveObjectCallback.hpp>
#include <Logger/StreamLogger.hpp>
#include <Commons/BoostAsioContextRunActiveObject.hpp>
#include <Commons/Grpc/GrpcExecutor.hpp>
#include <Commons/Grpc/GrpcSync.hpp>
#include <Commons/GrpcAlgs.hpp>
#include <BillingServerGrpc.grpc-client.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>

namespace
{
  const char USAGE[] =
    "\nUsage:\n"
    "BillingAdmin <command> <options>\n\n"
    "Commands:\n"
    "  check-available-bid\n"
    "    -r, --reference=<billing server grpc endpoint host:port>\n"
    "    --account-id=<id>\n"
    "    --advertiser-id=<id>\n"
    "    --campaign-id=<id>\n"
    "    --ccg-id=<id>\n"
    "    --ctr=<decimal>\n"
    "    [--optimize-campaign-ctr]\n\n"
    "Example:\n"
    "  BillingAdmin check-available-bid "
    "-r localhost:10607 "
    "--account-id=1 --advertiser-id=2 --campaign-id=3 --ccg-id=4 --ctr=0.01\n";

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  DECLARE_EXCEPTION(InvalidArgument, Exception);

  typedef AdServer::CampaignSvcs::RevenueDecimal RevenueDecimal;
  namespace Proto = adserver::campaign_svcs::billing_server;

  void
  require_option(
    bool installed,
    const char* option_name)
    /*throw(InvalidArgument)*/
  {
    if(!installed)
    {
      Stream::Error ostr;
      ostr << "Not defined required option '" << option_name << "'";
      throw InvalidArgument(ostr);
    }
  }

  int
  run(int argc, char** argv)
    /*throw(Exception, InvalidArgument, eh::Exception)*/
  {
    Generics::AppUtils::CheckOption opt_help;
    Generics::AppUtils::CheckOption opt_optimize_campaign_ctr;
    Generics::AppUtils::Option<std::string> opt_reference;
    Generics::AppUtils::Option<unsigned long> opt_account_id;
    Generics::AppUtils::Option<unsigned long> opt_advertiser_id;
    Generics::AppUtils::Option<unsigned long> opt_campaign_id;
    Generics::AppUtils::Option<unsigned long> opt_ccg_id;
    Generics::AppUtils::StringOption opt_ctr;

    Generics::AppUtils::Args args(-1);

    args.add(
      Generics::AppUtils::equal_name("help") ||
      Generics::AppUtils::short_name("h"),
      opt_help);
    args.add(
      Generics::AppUtils::equal_name("reference") ||
      Generics::AppUtils::short_name("r"),
      opt_reference);
    args.add(Generics::AppUtils::equal_name("account-id"), opt_account_id);
    args.add(Generics::AppUtils::equal_name("advertiser-id"), opt_advertiser_id);
    args.add(Generics::AppUtils::equal_name("campaign-id"), opt_campaign_id);
    args.add(Generics::AppUtils::equal_name("ccg-id"), opt_ccg_id);
    args.add(Generics::AppUtils::equal_name("ctr"), opt_ctr);
    args.add(
      Generics::AppUtils::equal_name("optimize-campaign-ctr"),
      opt_optimize_campaign_ctr);

    args.parse(argc - 1, argv + 1);

    Generics::AppUtils::Args::CommandList commands = args.commands();

    if(commands.empty() || opt_help.enabled() || commands.front() == "help")
    {
      std::cout << USAGE << std::endl;
      return 0;
    }

    const std::string command = commands.front();
    commands.pop_front();

    if(command != "check-available-bid")
    {
      Stream::Error ostr;
      ostr << "Unknown command '" << command << "'";
      throw InvalidArgument(ostr);
    }

    require_option(opt_reference.installed(), "reference");
    require_option(opt_account_id.installed(), "account-id");
    require_option(opt_advertiser_id.installed(), "advertiser-id");
    require_option(opt_campaign_id.installed(), "campaign-id");
    require_option(opt_ccg_id.installed(), "ccg-id");
    require_option(opt_ctr.installed(), "ctr");

    Logging::Logger_var logger(new Logging::OStream::Logger(
      Logging::OStream::Config(std::cerr)));
    Generics::ActiveObjectCallback_var callback(
      new Logging::ActiveObjectCallbackImpl(logger, "BillingAdmin"));

    auto grpc_executor = std::make_shared<AdServer::Grpc::GrpcExecutor>(
      1,
      "billing-admin-grpc");
    auto coalesce_runner =
      std::make_shared<AdServer::Commons::BoostAsioContextRunActiveObject>(
        callback,
        std::make_shared<boost::asio::io_service>(),
        1,
        128 * 1024,
        "billing-admin-coalesce");
    auto billing_server =
      std::make_shared<AdServer::CampaignSvcs::BillingServerGrpcAsyncBatchingClient>(
        *opt_reference,
        grpc_executor,
        coalesce_runner);

    Generics::CompositeActiveObject active_objects;
    active_objects.add_child_object(grpc_executor);
    active_objects.add_child_object(coalesce_runner);
    active_objects.activate_object();

    Proto::CheckBidRequest request;
    Proto::BidInfo* bid = request.mutable_bid();
    bid->set_time(GrpcAlgs::pack_time(Generics::Time::get_time_of_day()));
    bid->set_account_id(*opt_account_id);
    bid->set_advertiser_id(*opt_advertiser_id);
    bid->set_campaign_id(*opt_campaign_id);
    bid->set_ccg_id(*opt_ccg_id);
    bid->set_ctr(GrpcAlgs::pack_decimal(RevenueDecimal(opt_ctr->c_str())));
    bid->set_optimize_campaign_ctr(opt_optimize_campaign_ctr.enabled());

    const Proto::BidResultResponse result =
      AdServer::Grpc::sync_call<Proto::BidResultResponse>(
        [&billing_server, &request](auto callback)
        {
          billing_server->check_available_bid(request, std::move(callback));
        },
        [](const grpc::Status& status)
        {
          Stream::Error ostr;
          ostr << "BillingServer gRPC check_available_bid failed: code=" <<
            status.error_code() << ", message=" << status.error_message();
          throw Exception(ostr);
        });

    const RevenueDecimal goal_ctr = result.goal_ctr().empty() ?
      RevenueDecimal::ZERO :
      GrpcAlgs::unpack_decimal<RevenueDecimal>(result.goal_ctr());

    std::cout << "available: " << (result.available() ? "true" : "false") <<
      "\n"
      "goal_ctr: " << goal_ctr.str() << std::endl;

    active_objects.deactivate_object();
    active_objects.wait_object();

    return 0;
  }
}

int
main(int argc, char** argv)
{
  try
  {
    return run(argc, argv);
  }
  catch(const InvalidArgument& ex)
  {
    std::cerr << "Invalid argument: " << ex.what() <<
      "\nRun 'BillingAdmin help' for usage details" << std::endl;
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "BillingAdmin: eh::Exception caught: " << ex.what() << std::endl;
  }
  return -1;
}
