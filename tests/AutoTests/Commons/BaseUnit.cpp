#include <eh/Errno.hpp>
#include <Logger/DistributorLogger.hpp>
#include <Generics/ArrayAutoPtr.hpp>
#include <tests/AutoTests/Commons/FailContext.hpp>
#include <tests/AutoTests/Commons/Common.hpp>

//
//  UnitConfig struct
//
UnitConfig::UnitConfig()
  : verbose(false),
    verbose_start(false)
{}

//
//  UnitStat struct
//
UnitStat::UnitStat(
  LockMutex& lock_guard,
  const UnitConfig& unit_config,
  unsigned long flags_var,
  std::string unit_name_var,
  std::string error_var,
  bool succeed_var)
  : lock(lock_guard),
    config(unit_config),
    start_time(Generics::Time::ZERO),
    stop_time(Generics::Time::ZERO),
    unit_name(unit_name_var),
    error(error_var),
    succeed(succeed_var),
    flags(flags_var)
{}

UnitStat::~UnitStat() noexcept
{}

Generics::Time UnitStat::duration () const
{ 
  return start_time !=
    Generics::Time::ZERO? stop_time - start_time:
      Generics::Time::ZERO;
}

void UnitStat::mark_begin ()
{
  start_time = Generics::Time::get_time_of_day();
  SyncPolicy::ReadGuard guard(lock);
  std::cout << unit_name;
  if(config.verbose_start)
  {
    std::cout << " start" << std::endl;
  }
  std::cout.flush();
}

void UnitStat::mark_ok ()
{
  stop_time = Generics::Time::get_time_of_day();
  SyncPolicy::ReadGuard guard(lock);
  if(config.verbose_start)
  {
    std::cout << unit_name;
  }
  std::cout << " ok" << std::endl;
  std::cout.flush();
}

void UnitStat::mark_error ()
{
  stop_time = Generics::Time::get_time_of_day();
  SyncPolicy::ReadGuard guard(lock);
  if(config.verbose_start)
  {
    std::cout << unit_name;
  }
  std::cout << " ! Error" << std::endl;
  std::cout.flush();
}

void UnitStat::mark_fault ()
{
  stop_time = Generics::Time::get_time_of_day();
  SyncPolicy::ReadGuard guard(lock);
  if(config.verbose_start)
  {
    std::cout << unit_name;
  }
  std::cout << " !!! Failed" << std::endl;
  std::cout.flush();
}

void UnitStat::dump_error ()
{
  if(config.verbose && !succeed)
  {
    std::cout << unit_name;
    std::cout << " Error:" << std::endl;
    std::cout << error << std::endl;
    std::cout.flush();
  }
}
 
//
//  BaseUnit class
//
BaseUnit::BaseUnit(
  UnitStat& stat_var,
  const char* task_name,
  XsdParams params_var)
  : stat_(stat_var),
    current_client_index_(0),
    task_name_(task_name),
    timeout_(
      AutoTest::GlobalSettings::instance().
      wait_timeout()),
    params_(params_var),
    test_descr_(),
    descr_counter_(1)
{
  AutoTest::Logger::thlog(add_logger(task_name));
  timeout_key_.set_data(&timeout_);
}
  
BaseUnit::~BaseUnit()
  noexcept
{
  try
  {
    checkers_.clear();
    timeout_key_.set_data(0);
  }
  catch(const eh::Exception& exc)
  {
    AutoTest::Logger::thlog().log(exc.what(), Logging::Logger::ERROR);
  }
  catch (...)
  {
    AutoTest::Logger::thlog().log("Unknown exception",
      Logging::Logger::ERROR);
  }
}

void 
BaseUnit::execute() 
  noexcept
{
  try 
  {
    try
    {
      AutoTest::Logger::thlog().stream(Logging::Logger::INFO, task_name_.c_str()) << "Unit '"
        << task_name_ << "' started" << std::endl;
      stat_.mark_begin ();
      stat_.succeed = run_test() && stat_.error.empty();
      if(stat_.succeed)
      {
        stat_.mark_ok ();
      }
      else
      {
        stat_.mark_fault ();
      }
    }
    catch(const eh::Exception& e)
    {
      stat_.error += "BaseUnit::execute() eh::Exception exception caught. : ";
      stat_.error += e.what();
      stat_.error += '\n';
      if (!test_descr_.str().empty())
      {
        stat_.error += "Test log:\n";
        stat_.error += test_descr_.str();
      }
      AutoTest::Logger::thlog().log(stat_.error, Logging::Logger::ERROR);
      stat_.mark_error ();
    }
    catch(...)
    {
      stat_.error = "BaseUnit::execute() Unknown exception caught\n";
      if (!test_descr_.str().empty())
      {
        stat_.error += "Test log:\n";
        stat_.error += test_descr_.str();
      }
      AutoTest::Logger::thlog().log(stat_.error, Logging::Logger::ERROR);
      stat_.mark_error ();
    }
    AutoTest::Logger::thlog().stream(Logging::Logger::INFO, task_name_.c_str()) << "Unit '"
      << task_name_ << "' finished" << std::endl;
  }
  catch(...)
  { }
}

void 
BaseUnit::add_descr_phrase(const String::SubString& phrase) 
  /*throw(Exception)*/
{
  try
  {
    std::string str = strof(descr_counter_++)+ ". ";
    str.append(phrase.data(), phrase.size());
    AutoTest::Logger::thlog().log(str, Logging::Logger::INFO);
    test_descr_ << str << '\n';
  }
  catch (eh::Exception& e)
  {
    Stream::Error error;
    error << "BaseUnit::add_descr_phrase(). eh::Exception exception caught. "
          << ": " << e.what();
    throw Exception(error);
  }
}

void 
BaseUnit::add_descr_phrase(const char* phrase)
  /*throw(Exception)*/
{
  add_descr_phrase(String::SubString(phrase));
}

DataElemObjectRef
BaseUnit::get_object_by_name(const std::string& obj_name)
  /*throw(Exception, InvalidArgument)*/
{
  try
  {
    Locals all_locals = params_.get_local_params();
    size_t locals_len = all_locals.DataElem().size();
    
    for (size_t ind = 0; ind < locals_len; ++ind)
    {
      if (all_locals.DataElem()[ind].Name() == obj_name) 
        return all_locals.DataElem()[ind];
    }
  }
  catch(eh::Exception& e)
  {
    Stream::Error error;
    error << "BaseUnit::get_object_by_name() eh::Exception exception caught. "
          << ": " << e.what();
    throw Exception(error);
  }
  
  Stream::Error error;
  error << "BaseUnit::get_object_by_name(). "
        << "Error: Got unexpected object name = " << obj_name;
  throw InvalidArgument(error);
}

unsigned long
BaseUnit::fetch_int(const char* obj_name)
  /*throw(Exception)*/
{
  unsigned long buffer;
  
  try
  {
    Stream::Parser istr(get_object_by_name(obj_name).Value());
    istr >> buffer;
    
    if (istr.bad() || istr.fail())
    {
      Stream::Error error;
      error << "BaseUnit::fetch_int() Cant fetch integer value from <"
            << obj_name << "> in LocalData.xml";
      throw Exception(error);
    }
  }
  catch (const Exception&)
  {
    throw;
  }
  catch (const eh::Exception& e)
  {
    Stream::Error error;
    error << "BaseUnit::fetch_integer_type() eh::Exception caught. "
          << ": " << e.what();
    throw Exception(error);
  }
  
  return buffer;
}

long double
BaseUnit::fetch_float(const char* obj_name)
  /*throw(Exception)*/
{
  long double buffer;
  
  try
  {
    Stream::Parser istr(get_object_by_name(obj_name).Value());
    istr >> buffer;
    
    if (istr.bad() || istr.fail())
    {
      Stream::Error error;
      error << "BaseUnit::fetch_float_type() Cant fetch float value from <"
            << obj_name << "> in LocalData.xml";
      throw Exception(error);
    }
  }
  catch (const Exception&)
  {
    throw;
  }
  catch (const eh::Exception& e)
  {
    Stream::Error error;
    error << "BaseUnit::fetch_float() eh::Exception caught. "
          << ": " << e.what();
    throw Exception(error);
  }
  
  return buffer;
}

bool
BaseUnit::next_list_item(DataElemObjectPtr& res, const std::string& list_name)
  /*throw(Exception, InvalidArgument)*/
{  
  try
  {
    Locals all_locals = params_.get_local_params();
    unsigned long locals_len = all_locals.DataElem().size();    
    std::map<std::string, unsigned long>::iterator cur_list_it = 
      xml_lists_.find(list_name);
    
    if (cur_list_it == xml_lists_.end())
    {
      unsigned long ind = 0;
      
      for (ind = 0; ind < locals_len; ++ind)
      {
        if (all_locals.DataElem()[ind].Name() == list_name)
          break;
      }
      
      if (ind == locals_len)
      {
        Stream::Error error;
        error << "BaseUnit::next_list_item(...). Error: "
              << "Got unexpected object name = " << list_name;
        throw InvalidArgument(error);
      }
      
      if (all_locals.DataElem()[ind + 1].Name() == list_name + "End")
        return false;
        
      cur_list_it = xml_lists_.insert(std::pair<std::string, unsigned long>(
                                                                            list_name, ind)).first;
    }
    
    ++cur_list_it->second;
    
    if (cur_list_it->second == locals_len)
    {
      Stream::Error error;
      error << "BaseUnit::next_list_item(...). Error: list '"      
            << list_name << "' is not properly ended.";
      throw InvalidArgument(error);
    }
    
    if (all_locals.DataElem()[cur_list_it->second].Name() == list_name + "End")
    {
      xml_lists_.erase(cur_list_it);
      return false;
    }
    
    res = &all_locals.DataElem()[cur_list_it->second];
    return true;
  }
  catch(InvalidArgument&)
  {
    throw;
  }
  catch(eh::Exception& e)
  {
    Stream::Error error;
    error << "BaseUnit::next_list_item(...). eh::Exception exception caught. "
          << ": " << e.what();
    throw Exception(error);
  }
}

std::string
BaseUnit::map_objects(
  const char* obj_names,
  const char* separator)
  /*throw(Exception)*/
{
  std::string value;
  const String::AsciiStringManip::CharCategory SEP(separator);
  String::StringManip::CharSplitter tokenizer(
    String::SubString(obj_names), SEP);
  String::SubString token;
  while (tokenizer.get_token(token))
  {
    if (!value.empty())
    {
      value+=separator;
    }
    try
    {
      value += fetch_string(token.str()); 
    }
    catch (const BaseUnit::InvalidArgument&)
    {
      value += token.str(); 
    }
  }
  return value;
}

unsigned long BaseUnit::get_client_index()
{
  return current_client_index_++;
}

AutoTest::DBC::IConn*
BaseUnit::open_pq() /*throw(eh::Exception)*/
{
  return
    AutoTest::DBC::Conn::open_pq(
      get_global_params().PGDBConnection().user(),
      get_global_params().PGDBConnection().password(),
      get_global_params().PGDBConnection().host(),
      get_global_params().PGDBConnection().db());
}

AutoTest::Logger&
BaseUnit::add_logger(
  const std::string& log_name)
{
  Loggers::iterator it = loggers_.find(log_name);
  if (it != loggers_.end())
  {
    return *it->second;
  }

  AutoTest::Logger* logger =
    new AutoTest::Logger(log_name.c_str());
  
  loggers_[log_name] = logger;

  return *logger;
}

Sync::Key<int> BaseUnit::timeout_key_;

int
BaseUnit::timeout()
{
  int* t = timeout_key_.get_data();
  return *t;
}

void BaseUnit::check(bool clear_checkers)  /*throw(eh::Exception)*/
{
  timeout_ =
    AutoTest::GlobalSettings::instance().wait_timeout();
  bool case_ok = true;
  for (DescriptiveCheckerList::iterator
    it = checkers_.begin(); it != checkers_.end(); ++it)
  {
    AutoTest::Time start_time;

    AutoTest::Logger& case_logger = std::get<1>(*it);

    // Switch case
    if(&case_logger != &AutoTest::Logger::thlog())
    {
      case_ok = true;
    }
    // Switch logger
    AutoTest::LoggerSwitcher guard(case_logger);
    if (case_ok)
    {
      bool checker_call_result = false;
      // checker_call method can be overrided in specific test unit,
      // and it can throw any exception. We must clear checker list (if needed)
      // before rethrow this exception.
      try
      {
        checker_call_result =
          checker_call(std::get<0>(*it), std::get<2>(*it));
      }
      catch (const eh::Exception& e)
      {
        if (clear_checkers)
        {
          checkers_.clear();
        }
        throw;
      }
      if (!checker_call_result)
      {
        case_ok = false;
        // Empty logger - fail case sign (for multipart cases)
        // AUTOTEST_CASE don't run case with empty logger.
        // loggers_.size() == 1, mean that test don't use case logic. 
        if (loggers_.size() > 1)
        {
          case_logger.clear_loggers();
        }
      }
    }
    if (timeout_ >= 0)
    {
      timeout_ -= (AutoTest::Time() - start_time).tv_sec;
    }
  }
  if (clear_checkers)
  {
    checkers_.clear();
  }
}

bool
BaseUnit::checker_call(
  const std::string& description,
  AutoTest::Checker* checker) /*throw(eh::Exception)*/
{
  try
  {
    checker->check();
  }
  catch (const eh::Exception& exc)
  {
    std::string error;
    error.append(description);
    error.append(": ");
    error.append(exc.what());
    AutoTest::Logger::thlog().log(
      error,
      Logging::Logger::ERROR);
    stat_.error.append(error);
    stat_.error.push_back('\n');
    return false;
  }

  return true;

}
