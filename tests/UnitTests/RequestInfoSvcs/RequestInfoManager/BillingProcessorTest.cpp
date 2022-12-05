#include <map>

#include <iostream>
#include <Generics/AppUtils.hpp>
#include <Generics/Uuid.hpp>
#include <Generics/Rand.hpp>
#include <Logger/StreamLogger.hpp>
#include <Logger/ActiveObjectCallback.hpp>

#include <RequestInfoSvcs/RequestInfoManager/BillingProcessor.hpp>

using namespace AdServer::RequestInfoSvcs;
using AdServer::Commons::UserId;
using AdServer::Commons::RequestId;

namespace
{
  const char USAGE[] =
    "BillingProcessorTest [OPTIONS]\n"
    "OPTIONS:\n"
    "  -h, --help : show this message.\n";
}

Generics::Uuid
generate_user_id()
{
  return Generics::Uuid::create_random_based();
}

Generics::Uuid
generate_request_id()
{
  return Generics::Uuid::create_random_based();
}

RequestInfo
init_request()
{
  RequestInfo request_info;
  request_info.time = Generics::Time::get_time_of_day();
  request_info.request_id = generate_request_id();
  request_info.user_id = generate_user_id();
  request_info.adv_account_id = 1;
  request_info.advertiser_id = 2;
  request_info.campaign_id = 3;
  request_info.ccg_id = 4;
  request_info.enabled_action_tracking = false;
  request_info.enabled_impression_tracking = false;
  request_info.enabled_notice = false;
  request_info.expression = "TEST-SHARE";

  request_info.adv_revenue.impression = RevenueDecimal("0.1");
  request_info.adv_revenue.click = RevenueDecimal("0.1");
  request_info.adv_revenue.action = RevenueDecimal("0.1");
  request_info.adv_revenue.currency_rate = RevenueDecimal(1);
  request_info.pub_revenue.impression = RevenueDecimal("0.1");
  request_info.pub_revenue.click = RevenueDecimal("0.1");
  request_info.pub_revenue.action = RevenueDecimal("0.1");
  request_info.pub_bid_cost = RevenueDecimal::ZERO;
  request_info.pub_floor_cost = RevenueDecimal::ZERO;

  request_info.pub_revenue.currency_rate = RevenueDecimal("1");
  request_info.isp_revenue.impression = RevenueDecimal("0.1");
  request_info.isp_revenue.click = RevenueDecimal("0.1");
  request_info.isp_revenue.action = RevenueDecimal("0.1");
  request_info.isp_revenue.currency_rate = RevenueDecimal(1);
  request_info.device_channel_id = 1;
  request_info.pub_commission = RevenueDecimal::ZERO;
  request_info.isp_revenue_share = RevenueDecimal::ZERO;

  return request_info;
}

struct RequestSenderTest: public BillingProcessor::RequestSender
{
public:
  RequestSenderTest(unsigned long server_count, unsigned long invalid_index)
    : server_count_(server_count),
      invalid_index_(invalid_index)
  {}

  virtual void
  send_requests(
    BillingProcessor::RequestArray& requests,
    unsigned long service_index)
    /*throw(BillingProcessor::RequestSender::ServerUnreachable,
      BillingProcessor::RequestSender::Exception)*/
  {
    assert(!requests.empty());

    bool invalid = service_index < invalid_index_;

    std::ostringstream ostr;
    ostr << "Server #" << service_index << (invalid ? "(invalid):" : ":") << std::endl;
    for(auto req_it = requests.begin();
      req_it != requests.end(); ++req_it)
    {
      (*req_it)->print(ostr, "  ");
    }

    std::cout << ostr.str();
    std::cout.flush();

    if(service_index < invalid_index_)
    {
      throw BillingProcessor::RequestSender::ServerUnreachable("");
    }

    requests.clear();
  }

  virtual unsigned long
  server_count() const noexcept
  {
    return server_count_;
  }

protected:
  unsigned long server_count_;
  unsigned long invalid_index_;
};

int
main(int argc, char* argv[]) noexcept
{
  try
  {
    using namespace Generics::AppUtils;

    CheckOption opt_help;

    Args args;
    args.add(equal_name("help") || short_name("h"), opt_help);

    args.parse(argc - 1, argv + 1);

    if (opt_help.enabled())
    {
      std::cerr << USAGE << std::endl;
      return 0;
    }

    Logging::Logger_var logger = new Logging::OStream::Logger(
      Logging::OStream::Config(std::cerr));

    Logging::ActiveObjectCallbackImpl_var callback(
      new Logging::ActiveObjectCallbackImpl(
        logger,
        "BillingProcessorTest::main()",
        "BillingProcessorTest",
        ""));

    CORBACommons::CorbaObjectRefList billing_server_refs;
    billing_server_refs.push_back(CORBACommons::CorbaObjectRef());
    billing_server_refs.push_back(CORBACommons::CorbaObjectRef());

    system("rm -r ./BillingProcessorTestDir/ 2>/dev/null ; mkdir -p ./BillingProcessorTestDir/");

    BillingProcessor_var billing_processor = new BillingProcessor(
      logger,
      callback,
      String::SubString("./BillingProcessorTestDir/Amounts"),
      BillingProcessor::RequestSender_var(new RequestSenderTest(2, 1)),
      5,
      Generics::Time(10),
      Generics::Time(10)
      );

    Generics::ActiveObject_var act_object = billing_processor;

    act_object->activate_object();

    billing_processor->process_impression(
      init_request(),
      ImpressionInfo(),
      RequestInfo::RS_DUPLICATE);

    sleep(15);

    act_object->deactivate_object();
    act_object->wait_object();

    return 0;
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << ex.what() << std::endl;
  }

  return -1;
}
