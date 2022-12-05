#include <sstream>

#include <Generics/AppUtils.hpp>
#include <Generics/DirSelector.hpp>
#include <LogCommons/LogCommons.hpp>
#include <Logger/StreamLogger.hpp>

#include <RequestInfoSvcs/RequestInfoManager/RequestOperationLoader.hpp>
#include <RequestInfoSvcs/RequestInfoManager/RequestOperationSaver.hpp>

using namespace AdServer::RequestInfoSvcs;

namespace
{
  const std::size_t TEST_RECORDS_COUNT = 128;
  const char DEFAULT_ROOT_PATH[] = "./RequestOperationLoaderTestFolder";
  Generics::AppUtils::Option<std::string> root_path(DEFAULT_ROOT_PATH);

  const char USAGE[] =
    "RequestOperationLoaderTest [OPTIONS]\n"
    "OPTIONS:\n"
    "  -p, --path : path to folder with temporary files.\n"
    "  -h, --help : show this message.\n";
}

class RequestOperationProcessorImpl:
  public RequestOperationProcessor,
  public ReferenceCounting::AtomicImpl
{
public:
  struct Counter
  {
    Counter()
      : errors(0),
        impressions(0),
        clicks(0),
        actions(0),
        change_requests(0)
    {}

    Counter(
      unsigned long errors_val,
      unsigned long impressions_val,
      unsigned long clicks_val,
      unsigned long actions_val,
      unsigned long change_requests_val)
      : errors(errors_val),
        impressions(impressions_val),
        clicks(clicks_val),
        actions(actions_val),
        change_requests(change_requests_val)
    {}

    bool
    operator==(const Counter& right) const
    {
      return errors == right.errors &&
        impressions == right.impressions &&
        clicks == right.clicks &&
        actions == right.actions &&
        change_requests == right.change_requests;
    }

    std::ostream&
    print(std::ostream& out) const
    {
      out << "(err=" << errors <<
        ",imp=" << impressions <<
        ",clk=" << clicks <<
        ",act=" << actions <<
        ",change-req=" << change_requests <<
        ")";
      return out;
    }

    unsigned long errors;
    unsigned long impressions;
    unsigned long clicks;
    unsigned long actions;
    unsigned long change_requests;
  };

  RequestOperationProcessorImpl() noexcept
    : request_id_(AdServer::Commons::RequestId::create_random_based()),
      user_id_(AdServer::Commons::UserId::create_random_based()),
      time_(1000)
  {
    Generics::SmartMemBuf_var etalon_request_profile(new Generics::SmartMemBuf(1));
    *static_cast<char*>(etalon_request_profile->membuf().data()) = 1;
    etalon_request_profile_ = Generics::transfer_membuf(etalon_request_profile);
  }

  virtual void
  process_impression(
    const ImpressionInfo& impression_info)
    /*throw(Exception)*/
  {
    const ImpressionInfo check_impession_info =
      etalon_impression_info();

    if(!(impression_info.request_id == check_impession_info.request_id) ||
       impression_info.time != check_impession_info.time ||
       impression_info.verify_impression != check_impession_info.verify_impression ||
       !(impression_info.user_id == check_impession_info.user_id) ||
       !(impression_info.pub_revenue == check_impession_info.pub_revenue))
    {
      counter_.errors += 1;
      std::cerr << "process_impression: impression is incorrect:" << std::endl;
      impression_info.print(std::cerr, "  ");
      std::cerr << std::endl << "Instead:" << std::endl;
      check_impession_info.print(std::cerr, "  ");
      std::cerr << std::endl;
    }

    counter_.impressions += 1;
  }

  virtual void
  process_action(
    const AdServer::Commons::UserId& new_user_id,
    RequestContainerProcessor::ActionType action_type,
    const Generics::Time& time,
    const AdServer::Commons::RequestId& request_id)
    /*throw(Exception)*/
  {
    if(!(new_user_id == user_id_))
    {
      counter_.errors += 1;
      std::cerr << "process_action: user_id is incorrect: " << new_user_id <<
        " instead " << user_id_ << std::endl;
    }

    if(!(request_id == request_id_))
    {
      counter_.errors += 1;
      std::cerr << "process_action: request_id is incorrect: " << request_id <<
        " instead " << request_id_ << std::endl;
    }

    if(time != time_)
    {
      counter_.errors += 1;
      std::cerr << "process_action: time is incorrect: " << time.gm_ft() <<
        " instead " << time_.gm_ft() << std::endl;
    }

    if(action_type == RequestContainerProcessor::AT_CLICK)
    {
      counter_.clicks += 1;
    }
    else if(action_type == RequestContainerProcessor::AT_ACTION)
    {
      counter_.actions += 1;
    }
    else
    {
      counter_.errors += 1;
      std::cerr << "process_action: unexpected action" << std::endl;
    }
  }

  virtual void
  process_impression_post_action(
    const AdServer::Commons::UserId&,
    const AdServer::Commons::RequestId&,
    const RequestPostActionInfo&)
    /*throw(Exception)*/
  {}

  virtual void
  change_request_user_id(
    const AdServer::Commons::UserId& new_user_id,
    const AdServer::Commons::RequestId& request_id,
    const Generics::ConstSmartMemBuf* request_profile)
    /*throw(Exception)*/
  {
    if(!(new_user_id == user_id_))
    {
      counter_.errors += 1;
      std::cerr << "change_request_user_id: user_id is incorrect: " << new_user_id <<
        " instead " << user_id_ << std::endl;
    }

    if(!(request_id == request_id_))
    {
      counter_.errors += 1;
      std::cerr << "change_request_user_id: request_id is incorrect: " << request_id <<
        " instead " << request_id_ << std::endl;
    }

    if(!request_profile || request_profile->membuf().size() != 1)
    {
      std::cerr << "change_request_user_id: incorrect request_profile: size = " <<
        request_profile->membuf().size() <<
        " instead " << etalon_request_profile_->membuf().size() << std::endl;
    }
    else if(::memcmp(
      request_profile->membuf().data(),
      etalon_request_profile_->membuf().data(),
      etalon_request_profile_->membuf().size()) != 0)
    {
      std::cerr << "change_request_user_id: incorrect request_profile content" <<
        std::endl;      
    }

    counter_.change_requests += 1;
  }

  ImpressionInfo
  etalon_impression_info()
    noexcept
  {
    ImpressionInfo imp_info;
    imp_info.request_id = request_id_;
    imp_info.time = time_;
    imp_info.verify_impression = true;
    imp_info.user_id = AdServer::Commons::UserId(); // saver skip this field
    ImpressionInfo::PubRevenue pub_revenue;
    pub_revenue.revenue_type = AdServer::CampaignSvcs::RT_SHARE;
    pub_revenue.impression = RevenueDecimal("1");
    imp_info.pub_revenue = pub_revenue;
    return imp_info;
  }

  Counter
  counter() const
  {
    return counter_;
  }

  const AdServer::Commons::RequestId&
  etalon_request_id() const
  {
    return request_id_;
  }
  
  const AdServer::Commons::UserId&
  etalon_user_id() const
  {
    return user_id_;
  }

  const Generics::Time&
  etalon_time() const
  {
    return time_;
  }

  Generics::ConstSmartMemBuf_var
  etalon_request_profile() const
  {
    return etalon_request_profile_;
  }

private:
  const AdServer::Commons::RequestId request_id_;
  const AdServer::Commons::UserId user_id_;
  const Generics::Time time_;
  Generics::ConstSmartMemBuf_var etalon_request_profile_;
  Counter counter_;
};

typedef ReferenceCounting::SmartPtr<RequestOperationProcessorImpl>
  RequestOperationProcessorImpl_var;

/*
 * Test cases
 */

int
save_load_test()
  noexcept
{
  static const char* TEST_NAME = "save_load_test";

  int local_result = 0;

  try
  {
    system(("rm -r " + *root_path +
      " 2>/dev/null ; mkdir -p " + *root_path).c_str());

    RequestOperationProcessorImpl_var test_processor =
      new RequestOperationProcessorImpl();

    Logging::Logger_var logger =
      new Logging::OStream::Logger(Logging::OStream::Config(std::cerr));

    RequestOperationSaver_var saver = new RequestOperationSaver(
      logger,
      root_path->c_str(),
      "Operation",
      32,
      Generics::Time(100000));

    for(unsigned long i = 0; i < 10; ++i)
    {
      saver->process_impression(test_processor->etalon_impression_info());
    }

    for(unsigned long i = 0; i < 10; ++i)
    {
      saver->process_action(
        test_processor->etalon_user_id(),
        RequestContainerProcessor::AT_CLICK,
        test_processor->etalon_time(),
        test_processor->etalon_request_id());
    }

    for(unsigned long i = 0; i < 10; ++i)
    {
      saver->process_action(
        test_processor->etalon_user_id(),
        RequestContainerProcessor::AT_ACTION,
        test_processor->etalon_time(),
        test_processor->etalon_request_id());
    }

    for(unsigned long i = 0; i < 10; ++i)
    {
      saver->change_request_user_id(
        test_processor->etalon_user_id(),
        test_processor->etalon_request_id(),
        test_processor->etalon_request_profile());
    }

    RequestOperationSaver::FileNameList files;
    saver->flush(&files);

    RequestOperationLoader_var loader = new RequestOperationLoader(
      test_processor);

    for(RequestOperationSaver::FileNameList::const_iterator fit =
          files.begin();
        fit != files.end(); ++fit)
    {
      unsigned long lines = 0;
      loader->process_file(
        lines,
        fit->c_str(),
        0);
    }

    // check counter
    RequestOperationProcessorImpl::Counter etalon_counter(0, 10, 10, 10, 10);

    if(!(test_processor->counter() == etalon_counter))
    {
      std::cerr << TEST_NAME << ": Incorrect result counter: ";
      test_processor->counter().print(std::cerr);
      std::cerr << " Instead ";
      etalon_counter.print(std::cerr);
      std::cerr << std::endl;
      local_result = 1;
    }
  }
  catch(const eh::Exception& ex)
  {
    local_result = 1;
    std::cerr << TEST_NAME << ": fail, caught eh::Exception: " <<
      ex.what() << std::endl;
  }

  return local_result;
}

int
main(int argc, char* argv[]) noexcept
{
  const char* tmp_dir = getenv("tmp_dir");

  using namespace Generics::AppUtils;
  Args args;
  CheckOption opt_help;

  if (tmp_dir)
  {
    root_path.set_value(std::string(tmp_dir));
  }

  args.add(equal_name("path") || short_name("p"),
    root_path);

  args.add(equal_name("help") || short_name("h"),
    opt_help);

  args.parse(argc - 1, argv + 1);

  if (opt_help.enabled())
  {
    std::cout << USAGE << std::endl;
    return 0;
  }

  int result = 0;

  try
  {
    result += save_load_test();
    return result;
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << ex.what() << std::endl;
  }
  catch (...)
  {
    std::cerr << "unknown exception" << std::endl;
  }

  return -1;
}
