/**
 * @file RequestInfoSvcs/RequestLogLoaderTest.cpp
 * Test generate set of Log files and load this files
 */

#include <sstream>

#include <Generics/AppUtils.hpp>
#include <Generics/DirSelector.hpp>
#include <LogCommons/LogCommons.hpp>
#include <Logger/StreamLogger.hpp>

#include <RequestInfoSvcs/RequestInfoManager/RequestLogLoader.hpp>
#include <RequestInfoSvcs/RequestInfoManager/RequestOutLogger.hpp>

using namespace AdServer::RequestInfoSvcs;

namespace
{
  const std::size_t TEST_RECORDS_COUNT = 128;
  const char DEFAULT_ROOT_PATH[] = "./RequestLogLoaderTestFolder";
  Generics::AppUtils::Option<std::string> root_path(DEFAULT_ROOT_PATH);

  const char USAGE[] =
    "RequestLogLoaderTest [OPTIONS]\n"
    "OPTIONS:\n"
    "  -p, --path : path to folder with temporary files.\n"
    "  -h, --help : show this message.\n";

  void
  read_dir(
    const std::string& path,
    std::set<std::string>& entries,
    bool regular = true)
    /*throw(std::exception)*/
  {
    DIR *dir = opendir(path.c_str());

    if (!dir)
    {
      throw std::runtime_error("can't open dir " + path);
    }

    dirent* ent;

    while ((ent = readdir(dir)))
    {
      const std::string full_path = path + '/' + ent->d_name;
      struct stat st;

      if (ent->d_name[0] != '.' &&
          stat(full_path.c_str(), &st) == 0)
      {
        if (S_ISREG(st.st_mode) || !regular)
        {
          entries.insert(ent->d_name);
        }
      }
    }

    closedir (dir);
  }

  std::string
  find_log_file(
    const std::string& path,
    const std::string& base_name)
    /*throw(std::exception)*/
  {
    std::set<std::string> entries;
    read_dir(path, entries);

    for (std::set<std::string>::const_iterator ci = entries.begin();
         ci != entries.end(); ++ci)
    {
      if (!ci->compare(0, base_name.size(), base_name))
      {
        return *ci;
      }
    }

    throw std::runtime_error("find_log_file: file not found");
  }

  void
  make_commit_file(
    const std::string& path,
    const std::string& file_name)
    /*throw(eh::Exception)*/
  {
    std::ofstream ofs((path + "/~" + file_name + ".commit").c_str());
  }

  void
  generate_log_files(
    const char* type,
    std::size_t records_count)
    /*throw(eh::Exception)*/
  {
    std::ostringstream ost;
    ost << "LogGenerator -g \"" << type << "\" "
     "-p " << *root_path << " -c " << records_count << " >/dev/null";
    system(ost.str().c_str());
  }
}

class ProcessHook
{
public:
  virtual
  ~ProcessHook() noexcept
  {}

  virtual void
  process_request() noexcept {}

  virtual void
  process_impression() /*throw(RequestContainerProcessor::Exception)*/ {}
};

class AdvActionPrinter :
  public AdvActionProcessor,
  public ReferenceCounting::AtomicImpl
{
public:
  AdvActionPrinter() noexcept
    : processed_requests_(0),
      processed_adv_actions_(0),
      processed_custom_actions_(0)
  {}

  virtual void
  process_request(const RequestInfo& /*request_info*/)
    /*throw(Exception)*/
  {
    Guard lock(lock_);
//    std::cout << "received request (for adv action processor):" << std::endl;
//    request_info.print(std::cout, " ");
    ++processed_requests_;
  }

  virtual void
  process_adv_action(
    const AdvActionProcessor::AdvActionInfo& /*adv_action_info*/)
    /*throw(Exception)*/
  {
    Guard lock(lock_);
//    std::cout << "received AdvertiserAction: ";
//    adv_action_info.print(std::cout, "  ");
//    std::cout << std::endl;
    ++processed_adv_actions_;
  }

  virtual void
  process_custom_action(
    const AdvActionProcessor::AdvExActionInfo& /*adv_ex_action_info*/)
    /*throw(Exception)*/
  {
    Guard lock(lock_);
//    std::cout << "received advertiser custom action (action_id not present): ";
//    adv_ex_action_info.print(std::cout, "  ");
//    std::cout << std::endl;
    ++processed_custom_actions_;
  }

  std::size_t
  requests() const noexcept
  {
    return processed_requests_;
  }

  std::size_t
  adv_actions() const noexcept
  {
    return processed_adv_actions_;
  }

  std::size_t
  custom_actions() const noexcept
  {
    return processed_custom_actions_;
  }

private:
  typedef Sync::PosixMutex Mutex;
  typedef Sync::PosixGuard Guard;

  Mutex lock_;
  std::size_t processed_requests_;
  std::size_t processed_adv_actions_;
  std::size_t processed_custom_actions_;
};

typedef ReferenceCounting::SmartPtr<AdvActionPrinter> AdvActionPrinter_var;

class ActionPrinter :
  public RequestContainerProcessor,
  public ReferenceCounting::AtomicImpl
{
public:
  ActionPrinter(ProcessHook* hook = 0) noexcept
    : processed_requests_(0),
      processed_actions_(0),
      processed_custom_actions_(0),
      hook_(hook)
  {}

  virtual void
  process_request(const RequestInfo& /*request_info*/)
    /*throw(Exception)*/
  {
    Guard lock(lock_);
//    std::cout << "received request:" << std::endl;
//    request_info.print(std::cout, " ");
    ++processed_requests_;

    if (hook_)
    {
      hook_->process_request();
    }
  }

  virtual void
  process_impression(const ImpressionInfo& /*imp_info*/)
    /*throw(Exception)*/
  {
    Guard lock(lock_);
//    std::cout << "received request:" << std::endl;
//    request_info.print(std::cout, " ");
    ++processed_actions_;

    if (hook_)
    {
      hook_->process_impression();
    }
  }

  virtual void
  process_action(
    ActionType /*action_type*/,
    const Generics::Time&,
    const AdServer::Commons::RequestId& /*request_id*/) /*throw(Exception)*/
  {
    Guard lock(lock_);
//    std::cout << "received_action: " << request_id << std::endl;
    ++processed_actions_;
  }

  virtual void
  process_custom_action(
    const AdServer::Commons::RequestId& /*request_id*/,
    const AdvCustomActionInfo& /*adv_custom_action_info*/)
    /*throw(Exception)*/
  {
    Guard lock(lock_);
//    std::cout << "received advertiser custom action: ";
//    adv_custom_action_info.print(std::cout, "  ");
//    std::cout << std::endl;
    ++processed_custom_actions_;
  }

  virtual void
  process_impression_post_action(
    const AdServer::Commons::RequestId& /*request_id*/,
    const AdServer::RequestInfoSvcs::RequestPostActionInfo& /*request_post_action_info*/)
    /*throw(Exception)*/
  {}

  std::size_t requests() const noexcept
  {
    return processed_requests_;
  }

  std::size_t actions() const noexcept
  {
    return processed_actions_;
  }

  std::size_t custom_actions() const noexcept
  {
    return processed_custom_actions_;
  }

private:
  typedef Sync::PosixMutex Mutex;
  typedef Sync::PosixGuard Guard;

  Mutex lock_;
  std::size_t processed_requests_;
  std::size_t processed_actions_;
  std::size_t processed_custom_actions_;

  ProcessHook* hook_;
};

typedef ReferenceCounting::SmartPtr<ActionPrinter> ActionPrinter_var;

class PassbackPrinter :
  public PassbackVerificationProcessor,
  public ReferenceCounting::AtomicImpl
{
public:
  PassbackPrinter() noexcept
    : passbacks_count_(0),
      passbacks_imp_count_(0)
  {}

  virtual void
  process_passback_request(
    const AdServer::Commons::RequestId& /*request_id*/,
    const Generics::Time& /*impression_time*/)
    /*throw(PassbackVerificationProcessor::Exception)*/
  {
    Sync::PosixGuard lock(lock_);
//    std::cout << "received passback impression: "
//      << impression_time  << std::endl;
    ++passbacks_imp_count_;
  }

  std::size_t
  passbacks() const noexcept
  {
    return passbacks_count_;
  }

  std::size_t
  imp_passbacks() const noexcept
  {
    return passbacks_imp_count_;
  }

private:
  Sync::PosixMutex lock_;
  std::size_t passbacks_count_;
  std::size_t passbacks_imp_count_;
};

typedef ReferenceCounting::SmartPtr<PassbackPrinter> PassbackPrinter_var;

class TagRequestPrinter :
  public TagRequestProcessor,
  public ReferenceCounting::AtomicImpl
{
public:
  TagRequestPrinter() noexcept
    : tag_request_count_(0)
  {}

  virtual void
  process_tag_request(const TagRequestInfo& /*pi*/)
    /*throw(Exception)*/
  {
    Sync::PosixGuard lock(lock_);
    ++tag_request_count_;
  }

  std::size_t requests() const noexcept
  {
    return tag_request_count_;
  }

private:
  Sync::PosixMutex lock_;
  std::size_t tag_request_count_;
};

typedef ReferenceCounting::SmartPtr<TagRequestPrinter> TagRequestPrinter_var;

class RequestOperationPrinter :
  public RequestOperationProcessor,
  public ReferenceCounting::AtomicImpl
{
public:
  RequestOperationPrinter() noexcept
    : op_count_(0)
  {}

  virtual void
  process_impression(
    const ImpressionInfo& /*imp_info*/)
    /*throw(Exception)*/
  {}

  virtual void
  process_action(
    const AdServer::Commons::UserId& /*new_user_id*/,
    RequestContainerProcessor::ActionType /*action_type*/,
    const Generics::Time&,
    const AdServer::Commons::RequestId& /*request_id*/)
    /*throw(Exception)*/
  {}

  virtual void
  process_impression_post_action(
    const AdServer::Commons::UserId&,
    const AdServer::Commons::RequestId&,
    const AdServer::RequestInfoSvcs::RequestPostActionInfo&)
    /*throw(Exception)*/
  {}

  virtual void
  change_request_user_id(
    const AdServer::Commons::UserId& /*new_user_id*/,
    const AdServer::Commons::RequestId& /*request_id*/,
    const Generics::ConstSmartMemBuf* /*request_profile*/)
    noexcept
  {
    Sync::PosixGuard lock(lock_);
    ++op_count_;
  }

  std::size_t operations() const noexcept
  {
    return op_count_;
  }

private:
  Sync::PosixMutex lock_;
  std::size_t op_count_;
};

void make_dir(const std::string& name)
{
  ::mkdir(name.c_str(), S_IRWXU | S_IRGRP | S_IXGRP);
}

struct Tester
{
  Tester(std::size_t threads = 5) noexcept
    : adv_action_printer_(new AdvActionPrinter),
      passback_processor_(new PassbackPrinter),
      tag_request_processor_(new TagRequestPrinter),
      request_operation_processor_(new RequestOperationPrinter),
      threads_(threads)
  {}

  template <typename Functor>
  int
  do_test(Functor& file_generator)
  {
    action_printer_ = new ActionPrinter(&file_generator),

    file_generator();
    std::ostringstream ostr;

    Logging::Logger_var logger =
      new Logging::OStream::Logger(Logging::OStream::Config(ostr));

    Logging::ActiveObjectCallbackImpl_var callback(
      new Logging::ActiveObjectCallbackImpl(
        logger, "RequestInfoManager::check_logs_()",
        "RequestInfoManager", "ADS-IMPL-3015"));
    const std::string tmp_dir(*root_path);

    InLogs in_logs;
    in_logs.advertiser_action.dir = tmp_dir + "/AdvertiserAction";
    in_logs.advertiser_action.priority = 2;
    in_logs.click.dir = tmp_dir + "/Click";
    in_logs.click.priority = 2;
    in_logs.impression.dir = tmp_dir + "/Impression";
    in_logs.impression.priority = 2;
    in_logs.request.dir = tmp_dir + "/Request";
    in_logs.request.priority = 2;

    in_logs.passback_impression.dir = tmp_dir + "/PassbackImpression";
    in_logs.passback_impression.priority = 1;
    in_logs.tag_request.dir = tmp_dir + "/TagRequest";
    in_logs.tag_request.priority = 1;
    in_logs.request_operation.dir = tmp_dir + "/RequestOperation";
    in_logs.request_operation.priority = 1;

    make_dir(in_logs.advertiser_action.dir);
    make_dir(in_logs.advertiser_action.dir + "/Intermediate");
    make_dir(in_logs.click.dir);
    make_dir(in_logs.click.dir + "/Intermediate");
    make_dir(in_logs.impression.dir);
    make_dir(in_logs.impression.dir + "/Intermediate");
    make_dir(in_logs.passback_impression.dir);
    make_dir(in_logs.passback_impression.dir + "/Intermediate");
    make_dir(in_logs.request.dir);
    make_dir(in_logs.request.dir + "/Intermediate");
    make_dir(in_logs.tag_request.dir);
    make_dir(in_logs.tag_request.dir + "/Intermediate");
    make_dir(in_logs.request_operation.dir);
    make_dir(in_logs.request_operation.dir + "/Intermediate");

    request_log_loader_ = new RequestLogLoader(
      callback,
      in_logs,
      UnmergedClickProcessor_var(new NullUnmergedClickProcessor),
      action_printer_,
      adv_action_printer_,
      passback_processor_,
      tag_request_processor_,
      request_operation_processor_,
      Generics::Time(1), // check period
      Generics::Time(1),
      threads_,
      0);

    try
    {
      request_log_loader_->activate_object();
      std::cout << "sleep(3)" << std::endl;
      ::sleep(3);
      request_log_loader_->deactivate_object();
      request_log_loader_->wait_object();
    }
    catch (const eh::Exception& ex)
    {
      request_log_loader_->deactivate_object();
      request_log_loader_->wait_object();

      file_generator.check(ostr);
      file_generator.check(
        tag_request_processor_->requests(),
        adv_action_printer_->requests(),
        adv_action_printer_->adv_actions(),
        adv_action_printer_->custom_actions(),
        action_printer_->requests(),
        action_printer_->actions(),
        action_printer_->custom_actions(),
        passback_processor_->imp_passbacks());
      throw;
    }

    // Bad file generators control non empty logs,
    // good generators control emptyness report
    file_generator.check(ostr);

    return file_generator.check(
      tag_request_processor_->requests(),
      adv_action_printer_->requests(),
      adv_action_printer_->adv_actions(),
      adv_action_printer_->custom_actions(),
      action_printer_->requests(),
      action_printer_->actions(),
      action_printer_->custom_actions(),
      passback_processor_->imp_passbacks());
  }

public:
  ActionPrinter_var action_printer_;
  AdvActionPrinter_var adv_action_printer_;
  PassbackPrinter_var passback_processor_;
  TagRequestPrinter_var tag_request_processor_;
  RequestOperationProcessor_var request_operation_processor_;
  RequestLogLoader_var request_log_loader_;

  std::size_t threads_;
};

/*
 * Test cases
 */

struct AllFilesGood : public ProcessHook
{
  AllFilesGood(Tester&) noexcept
  {}

  virtual
  ~AllFilesGood() noexcept
  {}

  void
  operator ()() const /*throw(eh::Exception)*/
  {
    generate("Request");
    generate("Impression");
    generate("Click");
    generate("AdvertiserAction");
    generate("PassbackImpression");
    generate("TagRequest");
  }

  void
  generate(const char* type) const /*throw(eh::Exception)*/
  {
    generate_log_files(type, TEST_RECORDS_COUNT);
  }

  void
  check(const std::ostringstream& ostr) const noexcept
  {
    if (!ostr.str().empty())
    {
      std::cerr << "FAIL: logged troubles while processing files:\n"
         << ostr.str() << std::endl;
    }
  }

  /**
   * @return Error level, 0 is none errors
   */
  int
  check(
    std::size_t tag_request_processor_requests,
    std::size_t adv_action_printer_requests,
    std::size_t adv_action_printer_adv_actions,
    std::size_t adv_action_printer_custom_actions,
    std::size_t action_printer_requests,
    std::size_t action_printer_actions,
    std::size_t action_printer_custom_actions,
    std::size_t passback_processor_imp_passbacks) const noexcept
  {
    struct TestCase
    {
      const char* NAME;
      std::size_t standard;
      std::size_t result;
    };

    const TestCase TESTS[] =
    {
      {"TagRequest", TEST_RECORDS_COUNT,
        tag_request_processor_requests},
      {"AdvAction requests", 0,
        adv_action_printer_requests},
      {"AdvAction actions", 10,
        adv_action_printer_adv_actions},
      {"AdvAction custom actions", TEST_RECORDS_COUNT,
        adv_action_printer_custom_actions},
      {"Requests", 2 * TEST_RECORDS_COUNT, action_printer_requests},
      // Impression records count + Click records count
      {"Actions", 2 * TEST_RECORDS_COUNT,
        action_printer_actions},
      {"Custom actions", 0,
        action_printer_custom_actions},
      {"PassbackImpression", TEST_RECORDS_COUNT,
        passback_processor_imp_passbacks},
    };

    std::size_t fails = 0;

    for (std::size_t i = 0; i < sizeof(TESTS) / sizeof(TESTS[0]); ++i)
    {
      if (TESTS[i].standard != TESTS[i].result)
      {
        std::cerr << "FAIL: " << TESTS[i].NAME
          << ": result=" << TESTS[i].result
          << ", standard=" << TESTS[i].standard << std::endl;
        ++fails;
      }
    }

    return fails;
  }
};

struct FilesGoodAndBad : AllFilesGood
{
  FilesGoodAndBad(Tester& t) noexcept
    : AllFilesGood(t)
  {}

  virtual
  ~FilesGoodAndBad() noexcept
  {}

  void
  operator ()() const /*throw(eh::Exception)*/
  {
    AllFilesGood::operator ()();
    const char* PREFIXES[] =
    {
      "Request", "AdvertiserAction", "Click", "Impression",
      "PassbackImpression", "TagRequest",
    };
    // Bad content files
    for (std::size_t j = 0; j < sizeof(PREFIXES) / sizeof(PREFIXES[0]); ++j)
    {
      const std::string path = *root_path + '/' + PREFIXES[j];

      for (std::size_t i = 0; i < 10; ++i)
      {
        AdServer::LogProcessing::LogFileNameInfo name_info(PREFIXES[j]);
        const std::string file_name =
          AdServer::LogProcessing::make_log_file_name(name_info, path);
        std::ofstream file(file_name.c_str());
        file << file_name;

        make_commit_file(
          path,
          Generics::DirSelect::file_name(file_name.c_str()));
      }
    }
  }

  void
  check(const std::ostringstream& ostr) const noexcept
  {
    if (ostr.str().empty())
    {
      std::cerr << "FAIL: do not logged troubles while processing bad files."
        << std::endl;
    }
  }

  int
  check(
    std::size_t tag_request_processor_requests,
    std::size_t adv_action_printer_requests,
    std::size_t adv_action_printer_adv_actions,
    std::size_t adv_action_printer_custom_actions,
    std::size_t action_printer_requests,
    std::size_t action_printer_actions,
    std::size_t action_printer_custom_actions,
    std::size_t passback_processor_imp_passbacks) const noexcept
  {
    return AllFilesGood::check(
      tag_request_processor_requests,
      adv_action_printer_requests,
      adv_action_printer_adv_actions,
      adv_action_printer_custom_actions,
      action_printer_requests,
      action_printer_actions,
      action_printer_custom_actions,
      passback_processor_imp_passbacks);
  }
};

struct InterruptFileProcess : public ProcessHook
{
  static const std::size_t INTERRUPT_SIZE = 3000;

  InterruptFileProcess(Tester& t) noexcept
    : tester(t)
  {}

  virtual
  ~InterruptFileProcess() noexcept
  {}

  virtual void
  process_request() noexcept
  {
    if (tester.action_printer_->requests() == INTERRUPT_SIZE)
    {
      tester.request_log_loader_->deactivate_object();
    }
  }

  void
  operator ()() /*throw(std::exception)*/
  {
    generate("Request");
    file_name = find_log_file(*root_path + "/Request/", "Request") + ".3000";
  }

  void
  generate(const char* type) const /*throw(eh::Exception)*/
  {
    const std::size_t BIG_FILE_RECORD_COUNT = 5000;
    generate_log_files(type, BIG_FILE_RECORD_COUNT);
  }

  void
  check(const std::ostringstream& ostr) const noexcept
  {
    if (!ostr.str().empty())
    {
      std::cerr << "FAIL: logged troubles while processing files:\n"
         << ostr.str() << std::endl;
    }
  }

  int
  check(
    std::size_t,
    std::size_t,
    std::size_t,
    std::size_t,
    std::size_t action_printer_requests,
    std::size_t,
    std::size_t,
    std::size_t) const
    /*throw(std::exception)*/
  {
    std::size_t fails = 0;

    if (action_printer_requests != INTERRUPT_SIZE)
    {
      std::cerr << "FAIL: Requests : result=" << action_printer_requests
        << ", standard=" << INTERRUPT_SIZE << std::endl;
      ++fails;
    }

    const std::string moved_back_file = find_log_file(
      *root_path + "/Request/Intermediate",
      "Request");

    if (file_name != moved_back_file)
    {
      std::cerr << "FAIL: moved back file '" << moved_back_file <<
        "' not equals '" << file_name << "'" << std::endl;
      ++fails;
    }

    return fails;
  }

  std::string file_name;
  Tester& tester;
};

struct ProcessedRowNumberInErrorFileName : public ProcessHook
{
  static const std::size_t THROW_EXCEPTION_SIZE = 3000;

  ProcessedRowNumberInErrorFileName(Tester& t) noexcept
    : tester(t)
  {}

  virtual
  ~ProcessedRowNumberInErrorFileName() noexcept
  {}

  virtual void
  process_impression() /*throw(RequestContainerProcessor::Exception)*/
  {
    if (tester.action_printer_->actions() == THROW_EXCEPTION_SIZE)
    {
      throw RequestContainerProcessor::Exception("Emulated error");
    }
  }

  void
  operator ()() /*throw(std::exception)*/
  {
    generate("Impression");
    const std::string original_file_name = find_log_file(*root_path + "/Impression/", "Impression");

    if (::rename(
         (*root_path + "/Impression/" + original_file_name).c_str(),
         (*root_path + "/Impression/" + original_file_name + ".1000").c_str()))
    {
      throw std::runtime_error("can't rename " + original_file_name +
        " to " + original_file_name + ".1000");
    }

    make_commit_file(*root_path + "/Impression/", original_file_name + ".1000");
    file_name = original_file_name + ".3999";
  }

  void
  generate(const char* type) const noexcept
  {
    const size_t BIG_FILE_RECORD_COUNT = 5000;
    std::ostringstream ost;
    ost << "LogGenerator -g \"" << type << "\" "
     "-p " << *root_path << " -c " << BIG_FILE_RECORD_COUNT << " >/dev/null";
    system(ost.str().c_str());
  }

  void
  check(const std::ostringstream& ostr) const noexcept
  {
    if (ostr.str().empty())
    {
      std::cerr << "FAIL: do not logged troubles while processing bad files."
        << std::endl;
    }
  }

  int
  check(
    std::size_t,
    std::size_t,
    std::size_t,
    std::size_t,
    std::size_t,
    std::size_t action_printer_actions,
    std::size_t,
    std::size_t) const
    /*throw(std::exception)*/
  {
    std::size_t fails = 0;

    if (action_printer_actions != THROW_EXCEPTION_SIZE)
    {
      std::cerr << "FAIL: Actions : result=" << action_printer_actions
        << ", standard=" << THROW_EXCEPTION_SIZE << std::endl;
      ++fails;
    }

    std::set<std::string> entries;
    read_dir(*root_path + "/Impression/Error/Current", entries, false);

    if (entries.empty())
    {
      throw std::runtime_error(*root_path + "/Impression/Error/Current is empty");
    }

    const std::string error_file = find_log_file(
      *root_path + "/Impression/Error/Current/" + *entries.begin(), "Impression");

    if (file_name != error_file)
    {
      std::cerr << "FAIL: error file '" << error_file <<
        "' not equals '" << file_name << "'" << std::endl;
      ++fails;
    }

    return fails;
  }

  std::string file_name;
  Tester& tester;
};

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

template<typename TestType>
int execute_test(const char* TEST_NAME, unsigned long threads_count)
  noexcept
{
  int local_result = 1;

  try
  {
    system(("rm -r " + *root_path +
      " 2>/dev/null ; mkdir -p " + *root_path).c_str());

    Tester test_one_thread(threads_count);
    TestType test_type(test_one_thread);
    local_result = test_one_thread.do_test(test_type);
    if(local_result)
    {
      std::cerr << TEST_NAME << ": fail" << std::endl;
    }
    else
    {
      std::cout << TEST_NAME << ": success" << std::endl;
    }
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << TEST_NAME << ": fail, caught eh::Exception: " <<
      ex.what() << std::endl;
  }

  return local_result;
}

int
main(int argc, char* argv[]) noexcept
{
  const char* tmp_dir = getenv("tmp_dir");

  if (tmp_dir)
  {
    root_path.set_value(std::string(tmp_dir));
  }

  int result = 0;

  try
  {
    if (!init(argc, argv))
    {
      return 0;
    }

    result += execute_test<AllFilesGood>("One thread loading", 1);
    result += execute_test<AllFilesGood>("Few thread loading", 5);
    result += execute_test<FilesGoodAndBad>("One thread loading + bad files", 1);
    result += execute_test<FilesGoodAndBad>("Few thread loading + bad files", 5);
    result += execute_test<InterruptFileProcess>("Interrupt file process", 1);
    result += execute_test<ProcessedRowNumberInErrorFileName>("Processed row number in error file name", 1);

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
