#include <iostream>
#include <string>

#include <eh/Exception.hpp>
#include <Generics/AppUtils.hpp>
#include <Generics/Time.hpp>
#include <CORBACommons/CorbaAdapters.hpp>

#include <Commons/CorbaAlgs.hpp>
#include <CampaignSvcs/BillingServer/BillingServer.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>

namespace
{
  const char USAGE[] =
    "\nUsage:\n"
    "BillingAdmin <command> <options>\n\n"
    "Commands:\n"
    "  check-available-bid\n"
    "    -r, --reference=<billing server corba ref>\n"
    "    --account-id=<id>\n"
    "    --advertiser-id=<id>\n"
    "    --campaign-id=<id>\n"
    "    --ccg-id=<id>\n"
    "    --ctr=<decimal>\n"
    "    [--optimize-campaign-ctr]\n\n"
    "Example:\n"
    "  BillingAdmin check-available-bid "
    "-r corbaloc::localhost:10108/BillingServer "
    "--account-id=1 --advertiser-id=2 --campaign-id=3 --ccg-id=4 --ctr=0.01\n";

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  DECLARE_EXCEPTION(InvalidArgument, Exception);

  typedef AdServer::CampaignSvcs::RevenueDecimal RevenueDecimal;
  typedef AdServer::CampaignSvcs::BillingServer BillingServer;
  typedef AdServer::CampaignSvcs::BillingServer_var BillingServer_var;

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
    /*throw(Exception, InvalidArgument, eh::Exception, CORBA::Exception)*/
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

    CORBACommons::CorbaClientAdapter_var corba_client_adapter(
      new CORBACommons::CorbaClientAdapter());

    CORBA::Object_var billing_server_obj =
      corba_client_adapter->resolve_object(
        CORBACommons::CorbaObjectRef(opt_reference->c_str()));

    BillingServer_var billing_server = BillingServer::_narrow(
      billing_server_obj.in());

    if(CORBA::is_nil(billing_server.in()))
    {
      Stream::Error ostr;
      ostr << "AdServer::CampaignSvcs::BillingServer::_narrow failed for '"
        << *opt_reference << "'";
      throw Exception(ostr);
    }

    BillingServer::CheckBidInfo check_bid_info;
    check_bid_info.time = CorbaAlgs::pack_time(Generics::Time::get_time_of_day());
    check_bid_info.account_id = *opt_account_id;
    check_bid_info.advertiser_id = *opt_advertiser_id;
    check_bid_info.campaign_id = *opt_campaign_id;
    check_bid_info.ccg_id = *opt_ccg_id;
    check_bid_info.ctr = CorbaAlgs::pack_decimal(RevenueDecimal(opt_ctr->c_str()));
    check_bid_info.optimize_campaign_ctr = opt_optimize_campaign_ctr.enabled();

    BillingServer::BidResultInfo_var result =
      billing_server->check_available_bid(check_bid_info);

    const RevenueDecimal goal_ctr =
      CorbaAlgs::unpack_decimal<RevenueDecimal>(result->goal_ctr);

    std::cout << "available: " << (result->available ? "true" : "false") <<
      "\n"
      "goal_ctr: " << goal_ctr.str() << std::endl;

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
  catch(const BillingServer::NotReady& ex)
  {
    std::cerr << "BillingAdmin: BillingServer::NotReady caught: " <<
      ex.description.in() << std::endl;
  }
  catch(const BillingServer::ImplementationException& ex)
  {
    std::cerr << "BillingAdmin: BillingServer::ImplementationException caught: " <<
      ex.description.in() << std::endl;
  }
  catch(const CORBA::Exception& ex)
  {
    std::cerr << "BillingAdmin: CORBA::Exception caught: " << ex << std::endl;
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "BillingAdmin: eh::Exception caught: " << ex.what() << std::endl;
  }
  return -1;
}
