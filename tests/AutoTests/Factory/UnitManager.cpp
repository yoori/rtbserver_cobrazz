#include "UnitManager.hpp"
#include <Logger/ActiveObjectCallback.hpp>
#include <String/Tokenizer.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Generics/AppUtils.hpp>
#include <tests/AutoTests/Commons/Request/BaseRequest.hpp>

#include <algorithm>
#include <string.h>
#include <libgen.h>

///////////
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
//////////

namespace
{
  static const char* default_config   = "./Config/Config.xml";
  static const char* default_params   = "./Config/LocalParams.xml";

  void from_str(
    const std::string& str,
    AutoTestSpeedGroup& val)
  {
    if (str == "slow")
    {
      val = AUTO_TEST_SLOW;
    }
    else if (str == "quiet")
    {
      val = AUTO_TEST_QUIET;
    }
    else
    {
      val = AUTO_TEST_FAST;
    }
  }

  void from_str(
    const std::string& str,
    std::string& val)
  {
    val = str;
  }

  template <typename Type>
  void
  parse_string_list(
    const char* str,
    std::set<Type>& seq)
  {
    String::SubString s(str);
    String::StringManip::SplitComma tok(s);
    for (String::SubString token; tok.get_token(token);)
    {
      String::StringManip::trim(token);
      if (!token.empty())
      {
        Type v;
        from_str(token.str(), v);
        seq.insert(v);
      }
    }
  }

  bool compare_exec_time(const UnitStat* l , const UnitStat* r)
  {
    return l->duration() < r->duration();
  }

  bool basic_ud_sort (const UnitDescriptor* i, const UnitDescriptor* j)
  {
    /* Execution order:
         - serialized
         - fast;
         - quit;
         - slow;
         - serialized after.
    */
    int i_num = (10 * i->serialize) + i->group;
    int j_num = (10 * j->serialize) + j->group;
    return (i_num < j_num);
  }
}

/**
 * @class UnitManagerCallback
 */

class UnitManagerCallback :
  public virtual Generics::ActiveObjectCallback,
  public virtual ReferenceCounting::AtomicImpl
{
public:

  UnitManagerCallback(UnitManager& manager)
    : manager_(manager)
  { }

  virtual
  void
  report_error(
    Generics::ActiveObjectCallback::Severity severity,
    const String::SubString& description,
    const char*)
    noexcept
  {
    try
    {
      bool is_active = manager_.active();

      if(severity != ActiveObjectCallback::WARNING)
      {
        manager_.stop(false);
      }

      if(is_active)
      {
        try
        {
          unsigned long log_level = 0;

          switch(severity)
          {
          case Generics::ActiveObjectCallback::CRITICAL_ERROR:
            log_level = Logging::Logger::EMERGENCY;
            break;
          case Generics::ActiveObjectCallback::ERROR:
            log_level = Logging::Logger::CRITICAL;
            break;
          case Generics::ActiveObjectCallback::WARNING:
            log_level = Logging::Logger::WARNING;
            break;
          }

          AutoTest::Logger::thlog().
            stream(log_level, "UnitManager") <<
            "UnitManager::report_error: severity " <<
            severity << ", description: '" <<
            description << "'";
        }
        catch(const eh::Exception& e)
        {
          Stream::Error error;
          error << "UnitManager::report_error: " <<
            "eh::Exception caught. :" << std::endl << e.what();

          AutoTest::Logger::thlog().log(
            error.str(),
            Logging::Logger::EMERGENCY);
        }
      }
    }
    catch(...)
    { }
  }

protected:
  virtual ~UnitManagerCallback() noexcept
  { }

private:
  UnitManager& manager_;
};

UnitManager::TaskTest::TaskTest(
  UnitManager* manager,
  UnitDescriptor* descriptor,
  const GlobalConfig& config,
  AllLocals locals) :
  manager_(manager),
  descriptor_(descriptor),
  config_(config),
  locals_(locals)
{ }

void
UnitManager::TaskTest::execute() /*throw(eh::Exception)*/
{
  if (AutoTest::Shutdown::instance().get())
  {
    return;
  }

  if (0 != descriptor_->constructor)
  {
    if (descriptor_->dirty)
    {
      return;
    }

    UnitStat* stat(manager_->add_stat());

    descriptor_->dirty = true;
    std::ostringstream full_name;
    full_name << *descriptor_;

    long local_idx =
      AutoTest::find_local_params(
        locals_,
        descriptor_->name.c_str());

    stat->unit_name = full_name.str().c_str();


    try
    {
      if (local_idx < 0)
      {
        Stream::Error error;
        error << "Test '" << descriptor_->name << "' not found";
        throw InvalidArgument(error);
      }

      XsdParams params(
        config_,
        locals_.UnitLocalData()[local_idx]);

      BaseUnit_var(
        descriptor_->constructor(
          *stat,
          stat->unit_name.c_str(),
          params))->execute();
    }
    catch (const eh::Exception& e)
    {
      std::ostringstream error;
      error << "BaseUnit::execute() eh::Exception exception caught. : " <<
        e.what() << std::endl;
      stat->error += error.str();

      AutoTest::Logger_var log(
        new AutoTest::Logger(
          full_name.str().c_str()));

      log->log(
        stat->error,
        Logging::Logger::ERROR);
      stat->mark_error();
    }

  }
  else
  {
    Stream::Error error;
    error << "Test '" << descriptor_->name << "' invalid";
    throw InvalidArgument(error);
  }
}

UnitManager::TaskTest::~TaskTest() noexcept
{ }

typedef ReferenceCounting::QualPtr<UnitManagerCallback> UnitManagerCallback_var;

UnitManager::UnitManager()
  : logger_(),
    active_(false),
    result_(false),
    task_runner_(),
    start_time_(0),
    stop_time_(0),
    sort_performance_(false),
    config_(),
    locals_(),
    no_db_(false) // default mode is DB active
{
  XMLUtility::initialize();
}

UnitManager::~UnitManager()
  noexcept
{
  test_stats_.clear();
  XMLUtility::terminate();
}

void
UnitManager::run_serialized_(const TestFactory::UnitsList& serialized)
{
  for (TestFactory::UnitsList::const_iterator i = serialized.begin();
       i != serialized.end(); ++i)
  {

    if (AutoTest::Shutdown::instance().get())
    {
      return;
    }

    UnitDescriptor* descriptor = *i;

    Generics::Task_var(
      new TaskTest(
        this,
        descriptor,
        *config_,
        *locals_))->execute();
  }
}

void
UnitManager::run(int argc, const char* argv[])
  /*throw(InvalidOperationOrder, Exception, eh::Exception)*/
{
  if (!read_parameters_(argc, argv))
  {
    return;
  }

  init_();

  try
  {
    logger_->log(String::SubString("UnitManager started"));
    if (no_db_)
    {
      logger_->log(
        String::SubString("WARNING: post_condition tests parts will not run"));
    }
    else
    {
      logger_->log(
        String::SubString("ATTENTION: post_condition tests parts will run"));
    }

    TestFactory::UnitsList serialized_after;
    TestFactory::UnitsList serialized_before;
    start_time_ = Generics::Time::get_time_of_day();

    {
      AutoTest::Logger::thlog(*logger_);
      TestFactory::UnitsSeq units(
        test_factory_.units().begin(),
        test_factory_.units().end());
      std::sort(units.begin(), units.end(), basic_ud_sort);
      for (TestFactory::UnitsSeq::const_iterator i = units.begin();
           i != units.end(); ++i)
      {
        UnitDescriptor* descriptor = *i;
        if (descriptor->serialize)
        {
          if (AUTO_TEST_SERIALIZE_PRE == descriptor->serialize)
            serialized_before.push_back(descriptor);
          else
            serialized_after.push_back(descriptor);
        }
        else
        {
          task_runner_->enqueue_task(
            Generics::Task_var(
              new TaskTest(
                this,
                descriptor,
                *config_,
                *locals_)));
        }
      }
    }

    // 'Serialized before' tests block
    run_serialized_(serialized_before);
    logger_->log(String::SubString("Serialized before finished"));

    // Simple tests block
    if (!AutoTest::Shutdown::instance().get())
    {
      task_runner_->activate_object();
      task_runner_->wait_for_queue_exhausting();
      if(task_runner_.in() && task_runner_->active())
      {
        task_runner_->deactivate_object();
        task_runner_->wait_object();
      }
    }

    // 'Serialized after' tests block
    if (!AutoTest::Shutdown::instance().get())
    {
      logger_->log(String::SubString("Serialized after started"));
      run_serialized_(serialized_after);
    }

    stop_time_ = Generics::Time::get_time_of_day();

    AutoTest::Logger::thlog(*logger_);
  }
  catch(const Generics::TaskRunner::Exception& e)
  {
    Stream::Error ostr;
    ostr << "UnitManager::run: Generics::TaskRunner::Exception caught. "
      ":" << std::endl << e.what();

    throw Exception(ostr);
  }

  active_ = false;

  print_results();
}


void
UnitManager::init_()
  /*throw(InvalidArgument, Exception, eh::Exception)*/
{
  std::ostringstream ostr;
  ostr << "Initializing UnitManager.\n";

  rlimit limit;
  if(getrlimit(RLIMIT_NOFILE, &limit) == 0)
  {
    limit.rlim_cur = limit.rlim_max;
    if(setrlimit(RLIMIT_NOFILE, &limit) == 0)
    {
      ostr << "    Setting file descriptors number limit to maximum (" <<
        limit.rlim_max << ")" << std::endl;
    }
  }
  try
  {
    active_ = true;

    try
    {

      AutoTest::GlobalSettings::instance().
        initialize(
          *config_,
          *locals_);

      logger_ =
        new AutoTest::Logger("UnitManager");
      AutoTest::Logger::thlog(*logger_);

      const unsigned long threads_num = config_->get_params().ThreadsNum().value();
      unit_config.verbose_start = unit_config.verbose_start || threads_num > 1;

      task_runner_ = new Generics::TaskRunner(
        UnitManagerCallback_var(
          new UnitManagerCallback(*this)),
        threads_num,
        TASK_RUNNER_STACK_SIZE_);
    }
    catch(const Generics::TaskRunner::Exception& e)
    {
      Stream::Error ostr;
      ostr << "UnitManager::init: Generics::TaskRunner::Exception caught. "
        ":" << std::endl << e.what();

      throw Exception(ostr);
    }
    logger_->log(ostr.str());
  }
  catch(...)
  {
    stop(true);

    if(task_runner_.in())
    {
      task_runner_->wait_object();
    }

    throw;
  }
}

bool
UnitManager::read_parameters_(int argc,
                              const char* argv[])
  /*throw(InvalidArgument, Exception, eh::Exception)*/
{
  std::ostringstream usage;
  std::string app_name(argv[0]);
  size_t slash_pos = app_name.rfind('/');
  if ( slash_pos != std::string::npos)
  {
    app_name = app_name.substr(slash_pos+1);
  }
  usage << "Usage: " << app_name << " [OPTION]..." << std::endl
        << "-h, --help                          show this usage" << std::endl
        << "-c, --config PATH                   select config file" << std::endl
        << "                   default = " << default_config << std::endl
        << "-p, --params PATH                   select parameters file" << std::endl
        << "                   default = " << default_params << std::endl
        << "-C, --categories C1[,C2 ..]         select list of categories" << std::endl
        << "-G, --groups G1[,G2 ..]             select list of groups" << std::endl
        << "                   groups = fast, quiet, slow " << std::endl
        << "-S, --serialized yes | no           select|exclude serialized" << std::endl
        << "-E, --exclude-categories C1[,C2 ..] exclude categories" << std::endl
        << "-t, --tests T1[,T2 ..]              select tests" << std::endl
        << "-e, --exclude T1[,T2 ..]            exclude tests" << std::endl
        << "-v, --verbose                       verbose run" << std::endl
        << "-w, --verbose-start                 verbose start and stop" << std::endl
        << "-P, --performance                   show result list sorted by performance" << std::endl
        << "                                      useful for multithreaded run" << std::endl
        << "-l, --list                          show list of tests" << std::endl
       << "--nodb                               set NO DB mode" << std::endl
    ;
  Generics::AppUtils::CheckOption help;
  Generics::AppUtils::StringOption categories;
  Generics::AppUtils::StringOption groups;
  Generics::AppUtils::StringOption serialized;
  Generics::AppUtils::StringOption tests;
  Generics::AppUtils::StringOption exclude;
  Generics::AppUtils::StringOption caexclude;
  Generics::AppUtils::CheckOption nodb;
  Generics::AppUtils::CheckOption verbose;
  Generics::AppUtils::CheckOption verbose_start;
  Generics::AppUtils::CheckOption performance;
  Generics::AppUtils::CheckOption list_tests;
  Generics::AppUtils::StringOption config;
  Generics::AppUtils::StringOption params;
  Generics::AppUtils::Args args;

  args.add(
    Generics::AppUtils::short_name("h") ||
    Generics::AppUtils::equal_name("help"),
    help);

  args.add(
    Generics::AppUtils::short_name("C") ||
    Generics::AppUtils::equal_name("categories"),
    categories);

  args.add(
    Generics::AppUtils::short_name("G") ||
    Generics::AppUtils::equal_name("groups"),
    groups);

  args.add(
    Generics::AppUtils::short_name("S") ||
    Generics::AppUtils::equal_name("serialized"),
    serialized);

  args.add(
    Generics::AppUtils::short_name("t") ||
    Generics::AppUtils::equal_name("tests"),
    tests);

  args.add(
    Generics::AppUtils::short_name("e") ||
    Generics::AppUtils::equal_name("exclude"),
    exclude);

  args.add(
    Generics::AppUtils::short_name("E") ||
    Generics::AppUtils::equal_name("exclude-categories"),
    caexclude);

  args.add(
    Generics::AppUtils::equal_name("nodb"),
    nodb);

  args.add(
    Generics::AppUtils::short_name("v") ||
    Generics::AppUtils::equal_name("verbose"),
    verbose);

  args.add(
    Generics::AppUtils::short_name("w") ||
    Generics::AppUtils::equal_name("verbose-start"),
    verbose_start);

  args.add(
    Generics::AppUtils::short_name("P") ||
    Generics::AppUtils::equal_name("performance"),
    performance);

  args.add(
    Generics::AppUtils::short_name("l") ||
    Generics::AppUtils::equal_name("list"),
    list_tests);

  args.add(
    Generics::AppUtils::short_name("c") ||
    Generics::AppUtils::equal_name("config"),
    config);

  args.add(
    Generics::AppUtils::short_name("p") ||
    Generics::AppUtils::equal_name("params"),
    params);

  try
  {
    args.parse(argc - 1, argv + 1);
  }
  catch (const eh::Exception& exc)
  {
    std::cerr << "error: " << exc.what() << std::endl
              << usage.str();
    return false;
  }
  if (help.enabled())
  {
    std::cout << usage.str();
    return false;
  }
  no_db_ = nodb.enabled();
  if (categories.installed())
  {
    parse_string_list(categories->c_str(), categories_);
  }
  if (groups.installed())
  {
    parse_string_list(groups->c_str(), groups_);
  }
  if (tests.installed())
  {
    parse_string_list(tests->c_str(), tests_);
  }
  int select_serialized = 0;
  if (serialized.installed())
  {
    select_serialized = (0 == strcmp(serialized->c_str(), "yes") ? 2 : 1);
  }

  if (exclude.installed())
  {
    parse_string_list(exclude->c_str(), exclude_tests_);
  }

  if (caexclude.installed())
  {
    parse_string_list(caexclude->c_str(), exclude_categories_);
  }

  { // load units
    if (!categories.installed() &&
        !groups.installed() &&
        !serialized.installed() &&
        tests.installed())
    {
      test_factory_.filter(tests_);
    }
    else
    {
      test_factory_.filter(
        exclude_tests_,
        exclude_categories_,
        tests_,
        groups_,
        categories_,
        select_serialized);
    }
  }

  const TestFactory::UnitsList& units = test_factory_.units();
  if (list_tests.enabled())
  {
    for (TestFactory::UnitsList::const_iterator i = units.begin();
         i != units.end(); ++i)
    {
      UnitDescriptor* descriptor = *i;
      std::cout << *descriptor << std::endl;
    }
    std::cout << "--------------------------" << std::endl;
    std::cout << "Total: " << units.size() << std::endl;
    std::cout << "--------------------------" << std::endl;
    return false;
  }
  if (verbose.enabled())
  {
    unit_config.verbose = true;
  }
  if (verbose_start.enabled())
  {
    unit_config.verbose = true;
    unit_config.verbose_start = true;
  }
  if (performance.enabled())
  {
    sort_performance_ = true;
  }
  if (params.installed())
  {
    locals_ = Config::load_helper<LocalsPtr>(
      xsd::tests::AutoTests::LocalParams,
      params->c_str(),
      xml_schema::flags::dont_initialize);
  }
  else
  {
    struct stat buf;
    if (!stat(default_params, &buf))
    {
      locals_ = Config::load_helper<LocalsPtr>(
        xsd::tests::AutoTests::LocalParams,
        default_params,
        xml_schema::flags::dont_initialize);
    }
    else
    {
      Stream::Error ostr;
      ostr << "Error: params file not specified" << std::endl <<
        usage.str().c_str();

      throw InvalidArgument(ostr);
    }
  }
  if (config.installed())
  {
    config_ = GlobalConfigPtr(new GlobalConfig(config->c_str()));
    char* dir = strdup(config->c_str());
    config_path_ = dirname(dir);
    free(dir);
  }
  else
  {
    struct stat buf;
    if (!stat(default_config, &buf))
    {
      config_ = GlobalConfigPtr(new GlobalConfig(default_config));
      char* dir = strdup(default_config);
      config_path_ = dirname(dir);
      free(dir);
    }
    else
    {
      Stream::Error ostr;
      ostr << "Error: configuration file not specified" << std::endl <<
        usage.str().c_str();

      throw InvalidArgument(ostr);
    }
  }
  return !test_factory_.units().empty();
}

void
UnitManager::stop(bool result)
  /*throw(Exception, eh::Exception)*/
{
  SyncPolicy::ReadGuard lock(lock_);

  if(!active_) return;

  active_ = false;

  try
  {
    if(task_runner_.in() && task_runner_->active())
    {
      task_runner_->deactivate_object();
    }
  }
  catch(const Generics::TaskRunner::Exception& e)
  {
    Stream::Error ostr;
    ostr << "UnitManager::stop: Generics::TaskRunner::Exception caught. "
      ":" << std::endl << e.what();

    throw Exception(ostr);
  }

  result_ = result;
}

UnitStat*
UnitManager::add_stat() /*throw(eh::Exception)*/
{
  SyncPolicy::ReadGuard lock(stat_lock_);
  UnitStat* stat(
    new UnitStat(
      lock_, unit_config,
        no_db_? UnitStat::UM_NO_DB: 0));
  test_stats_.push_back(stat);
  return stat;
}

void
UnitManager::print_results() /*throw(eh::Exception)*/
{
  AutoTest::Logger::thlog(*logger_);
  std::ostringstream ostr;
  ostr << "\n\n********************* TEST RESULTS *********************\n\n";

  result_ = true;

  Generics::Time total_duration;
  int failed = 0;
  int passed = 0;

  if (sort_performance_)
  {
    std::sort(test_stats_.begin(), test_stats_.end(), compare_exec_time);
  }
  unsigned long test_units_count = test_stats_.size();
  for (unsigned long i = 0; i < test_units_count; ++i)
  {
    result_ = result_ && test_stats_[i]->succeed;

    total_duration +=  test_stats_[i]->duration();
    if(test_stats_[i]->succeed)
      ++passed;
    else
      ++failed;

    ostr << "Unit '" << test_stats_[i]->unit_name
         << "' " << test_stats_[i]->duration()
         << (test_stats_[i]->succeed ? " SUCCEED\n" : " FAILED");
    if (!test_stats_[i]->error.empty())
    {
     ostr << " with error : \n"/*'"*/ << test_stats_[i]->error;/* << "'";*/
    }
    ostr << /*".*/"\n";
  }

  ostr << "\nExecution time: " << stop_time_ - start_time_ <<
    std::endl;

  if(unit_config.verbose && failed > 0)
  {
    std::cout << "==========================" << std::endl;
    std::cout << "Errors" << std::endl;
    for (unsigned long i = 0; i < test_units_count; ++i)
    {
      if(!test_stats_[i]->succeed)
      {
        std::cout << "--------------------------" << std::endl;
        test_stats_[i]->dump_error ();
        std::cout << "--------------------------" << std::endl;
      }
    }
  }

  logger_->log(ostr.str());

  std::cout << "==========================" << std::endl;
  std::cout << "Total duration: " << total_duration << "/" << stop_time_ - start_time_  << std::endl;
  std::cout << "==========================" << std::endl;
  std::cout << "Passed: " << passed << std::endl;
  std::cout << "Failed: " << failed << std::endl;
  std::cout << "==========================" << std::endl;
  std::cout << "Total:  " << test_stats_.size() << std::endl;
}
