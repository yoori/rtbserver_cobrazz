// @file RequestInfoSvcs/RequestOutLoggerTest.cpp

#include <Generics/AppUtils.hpp>
#include <Generics/Uuid.hpp>
#include <Generics/DirSelector.hpp>
#include <Logger/StreamLogger.hpp>
#include <Logger/ActiveObjectCallback.hpp>

#include <RequestInfoSvcs/RequestInfoManager/RequestOutLogger.hpp>

using namespace AdServer::RequestInfoSvcs;

namespace
{
  const char DEFAULT_ROOT_PATH[] = "./";
  Generics::AppUtils::Option<std::string> root_path(DEFAULT_ROOT_PATH);

  const char TEST_FOLDER[] = "/RequestOutLoggerTestDir/";

  const char USAGE[] =
    "RequestOutLoggerTest [OPTIONS]\n"
    "OPTIONS:\n"
    "  -p, --path : path to folder with temporary files.\n"
    "  -h, --help : show this message.\n";
}


int
request_out_logger_test(const char* tmp_dir)
{
  AdServer::LogProcessing::LogFlushTraits log_traits;
  log_traits.out_dir = tmp_dir;

  Logging::Logger_var logger =
    new Logging::OStream::Logger(Logging::OStream::Config(std::cerr));

  Logging::ActiveObjectCallbackImpl_var callback(
    new Logging::ActiveObjectCallbackImpl(
      logger,
      "RequestInfoManager::_()",
      "RequestInfoManager", "ADS-IMPL-?"));

  RequestOutLogger_var request_out_logger(
    new RequestOutLogger(
      0,
      callback,
      log_traits,
      log_traits,
      log_traits,
      log_traits,
      log_traits,
      log_traits,
      log_traits,
      log_traits,
      log_traits,
      log_traits,
      log_traits,
      log_traits,
      log_traits,
      log_traits,
      log_traits,
      log_traits,
      log_traits,
      log_traits,
      &log_traits,
      &log_traits,
      &log_traits,
      &log_traits,
      &log_traits,
      AdServer::Commons::LogReferrer::LR_PATH,
      1));

  Generics::Uuid uuid = Generics::Uuid::create_random_based();
  AdServer::Commons::RequestId req_id(uuid.to_string().c_str());
  RequestInfo request_info(req_id);
  request_info.user_status = 'I';
  request_info.pub_floor_cost = RevenueDecimal::ZERO;
  request_info.pub_bid_cost = RevenueDecimal::ZERO;
  request_info.adv_revenue.currency_rate = RevenueDecimal(1);
  request_info.pub_revenue.currency_rate = RevenueDecimal(1);
  request_info.isp_revenue.currency_rate = RevenueDecimal(1);

  request_out_logger->process_request(
    request_info,
    RequestActionProcessor::ProcessingState());
  request_out_logger->flush_if_required();

  request_out_logger->process_impression(
    request_info,
    ImpressionInfo(),
    RequestActionProcessor::ProcessingState());
  request_out_logger->flush_if_required();

  request_out_logger->process_click(
    request_info,
    RequestActionProcessor::ProcessingState());
  request_out_logger->flush_if_required();

  request_out_logger->process_action(request_info);
  request_out_logger->flush_if_required();

  return 0;
}

bool
init(int& argc, char**& argv) /*throw(eh::Exception)*/
{
  using namespace Generics::AppUtils;
  Args args;
  CheckOption opt_help;

  args.add(
    equal_name("path") ||
    short_name("p"),
    root_path);
  args.add(
    equal_name("help") ||
    short_name("h"),
    opt_help);

  args.parse(argc - 1, argv + 1);

  if (opt_help.enabled())
  {
    std::cout << USAGE << std::endl;
    return false;
  }
  return true;
}

int
main(int argc, char* argv[]) noexcept
{
  try
  {
    if (!init(argc, argv))
    {
      return 0;
    }
    system(("rm -r " + *root_path + TEST_FOLDER +
      " 2>/dev/null ; mkdir -p " + *root_path + TEST_FOLDER).c_str());
    return request_out_logger_test(
      (*root_path + TEST_FOLDER).c_str());
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << ex.what() << std::endl;
  }
  return -1;
}
