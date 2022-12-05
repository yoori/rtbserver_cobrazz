// $Id: ProcessControlVarsImpl.hpp 185978 2020-07-04 00:12:49Z jurij_kuznecov $

#ifndef _ADSERVER_COMMONS_PROCESS_CONTROL_VARS_IMPL_HPP_
#define _ADSERVER_COMMONS_PROCESS_CONTROL_VARS_IMPL_HPP_

#include <map>
#include <string>
#include <cerrno>
#include <cstdlib>

#include <Logger/Logger.hpp>
#include <Logger/StreamLogger.hpp>
#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/ProcessControlImpl.hpp>

namespace AdServer {
namespace Commons {

struct VarProcessor: public virtual ReferenceCounting::Interface
{
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  virtual bool set_value(const char *value, std::string& result)
    /*throw(Exception)*/ = 0;
};

typedef ReferenceCounting::SmartPtr<VarProcessor> VarProcessor_var;

struct LogLevelProcessor:
  public ReferenceCounting::AtomicImpl,
  public VarProcessor
{
  static const char VAR_NAME[];

  LogLevelProcessor(Logging::Logger* logger)
    : logger_(ReferenceCounting::add_ref(logger))
  {
  }

  bool set_value(const char *value, std::string& result) /*throw(Exception)*/
  {
    errno = 0;
    char *eptr = 0;
    int num_value = std::strtol(value, &eptr, 10);
    if (errno || value == eptr)
    {
      return false;
    }
    try
    {
      logger_->log_level(num_value);
      result = "Variable '";
      result += LogLevelProcessor::VAR_NAME;
      result += "' has been set to '";
      result += value;
      result += "'";
    }
    catch (const eh::Exception &ex)
    {
      throw Exception(ex.what());
    }
    catch (...)
    {
      throw Exception("LogLevelProcessor::set_value(): Unknown exception!!!");
    }
    return true;
  }

private:
  Logging::Logger_var logger_;
};

struct DbStateChanger: public virtual ReferenceCounting::Interface
{
  DECLARE_EXCEPTION(NotSupported, eh::DescriptiveException);

  virtual void change_db_state(bool) = 0;
  virtual bool get_db_state() /*throw(NotSupported)*/ = 0;
  static const char* print_db_state(bool db_state) noexcept;
};

typedef ReferenceCounting::SmartPtr<DbStateChanger> DbStateChanger_var;

/**
 * Delegate calls to specified object, DbStateChanger implementation
 * by default
 */
template <typename ModyfingObjectType>
struct SimpleDbStateChanger:
  public DbStateChanger,
  public ReferenceCounting::AtomicImpl
{
  /**
   * @param object The smart pointer to object where will be
   * delegated calls change_db_state
   */
  SimpleDbStateChanger(ModyfingObjectType& object)
    : object_(object)
  {}

  /**
   * @param new_state The new value of DB state to be set
   */
  void
  change_db_state(bool new_state)
  {
    object_->change_db_state(new_state);
  }

  /**
   * @return The state of DB
   */
  bool
  get_db_state() /*throw(NotSupported)*/
  {
    return object_->get_db_state();
  }
  
protected:
  virtual
  ~SimpleDbStateChanger() noexcept
  {}

private:
  ModyfingObjectType object_;
};

/**
 * Autodetect type facility (make function)
 */
template <typename ModyfingObjectType>
DbStateChanger*
new_simple_db_state_changer(ModyfingObjectType& object)
  /*throw(eh::Exception)*/
{
  return new SimpleDbStateChanger<ModyfingObjectType>(object);
}

struct DbStateProcessor:
  public ReferenceCounting::AtomicImpl,
  public VarProcessor
{
  static const char VAR_NAME[];

  DbStateProcessor(const DbStateChanger_var &db_state_changer)
  :
    db_state_changer_(db_state_changer)
  {
  }

  bool set_value(const char *value, std::string& result) /*throw(Exception)*/
  {
    errno = 0;
    char *eptr = 0;
    int num_value = std::strtol(value, &eptr, 10);
    if (errno || value == eptr)
    {
      return false;
    }
    try
    {
      if(num_value == -1)
      {
        try
        {
          result =
            DbStateChanger::print_db_state(db_state_changer_->get_db_state());
        }
        catch(const DbStateChanger::NotSupported&)
        {//no db connection in this instance
        } 
      }
      else
      {
        db_state_changer_->change_db_state(num_value);
        result = "Variable '";
        result += DbStateProcessor::VAR_NAME;
        result +="' has been set to '";
        result += value;
        result += "'";
      }
    }
    catch (const eh::Exception &ex)
    {
      throw Exception(ex.what());
    }
    return true;
  }

private:
  DbStateChanger_var db_state_changer_;
};

class ProcessControlVarsImpl: public ReferenceCounting::AtomicImpl
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  ProcessControlVarsImpl(): var_proc_map_() {}

  void
  add_var_processor(const char *var_name, const VarProcessor_var &var_proc)
  {
    var_proc_map_[var_name] = var_proc;
  }

  virtual char*
  control(const char *var_name, const char *var_value)
    /*throw(Exception)*/;

protected:
  virtual ~ProcessControlVarsImpl() noexcept {}

private:
  typedef std::map<std::string, VarProcessor_var> VarProcessorMapT;

  VarProcessorMapT var_proc_map_;
};

typedef ::ReferenceCounting::SmartPtr<ProcessControlVarsImpl>
  ProcessControlVarsImpl_var;

class CommonProcessControlVarsImpl: public ProcessControlVarsImpl
{
public:
  CommonProcessControlVarsImpl(Logging::Logger* logger)
  {
    add_var_processor(LogLevelProcessor::VAR_NAME,
      new LogLevelProcessor(logger));
  }

protected:
  virtual ~CommonProcessControlVarsImpl() noexcept {}
};

typedef ::ReferenceCounting::SmartPtr<CommonProcessControlVarsImpl>
  CommonProcessControlVarsImpl_var;

/**
 * ALL services use CommonProcessControlVarsImpl
 */
class ProcessControlVarsLoggerImpl:
  public CORBACommons::ProcessControlWithLogger
{
public:
  /**
   * Construct ProcessControlVarsLoggerImpl
   * @param logger is initial logger will be used
   * @param message_prefix for callback
   * @param aspect for callback
   * @param code for callback
   * @param shutdowner for ProcessControlImpl
   */
  ProcessControlVarsLoggerImpl(
    const char* message_prefix = "ProcessControlVarsLoggerImpl",
    const char* aspect = 0,
    const char* code = 0,
    CORBACommons::OrbShutdowner* shutdowner = 0)
    /*throw(InvalidArgument, Exception, eh::Exception)*/;

  /**
   * Create remote state controller, indicate that service is up
   */
  void
  register_vars_controller() /*throw(eh::Exception)*/;

  /**
   * Add remote state variable with specified name
   * @param var_name The name for variable
   * @param var_proc The object with VarProcessor interface, that
   * able to work with variable
   */
  void
  add_var_processor(const char *var_name, const VarProcessor_var &var_proc);

  /**
   * Performs specific action on remote object
   * @param param_name action name or variable name to set
   * @param param_value additional action data
   * @return action result
   */
  virtual char*
  control(const char* param_name, const char* param_value)
    noexcept;

protected:
  /**
   * Destructor
   */
  virtual
  ~ProcessControlVarsLoggerImpl() noexcept;
private:
  CommonProcessControlVarsImpl_var proc_ctrl_vars_impl_;
};

typedef ::ReferenceCounting::SmartPtr<ProcessControlVarsLoggerImpl>
  ProcessControlVarsLoggerImpl_var;

} // namespace Commons
} // namespace AdServer

//
// Inlines implementation
//

namespace AdServer {
namespace Commons {

  inline const char*
  DbStateChanger::print_db_state(bool db_state) noexcept
  {
    static const char* ENABLED_DB_STATE = "DB connection is active";
    static const char* DISABLED_DB_STATE = "DB connection is inactive";
    if(db_state)
    {
      return ENABLED_DB_STATE;
    }
    return DISABLED_DB_STATE;
  }

  inline
  ProcessControlVarsLoggerImpl::ProcessControlVarsLoggerImpl(
    const char* message_prefix,
    const char* aspect,
    const char* code,
    CORBACommons::OrbShutdowner* shutdowner)
    /*throw(InvalidArgument, Exception, eh::Exception)*/
    : CORBACommons::ProcessControlWithLogger(
        Logging::Logger_var(new Logging::OStream::Logger(
          Logging::OStream::Config(std::cerr))),
        message_prefix, aspect, code, shutdowner)
    {
    }

  inline void
  ProcessControlVarsLoggerImpl::register_vars_controller()
    /*throw(eh::Exception)*/
  {
    proc_ctrl_vars_impl_ = new CommonProcessControlVarsImpl(logger());
  }

  inline void
  ProcessControlVarsLoggerImpl::add_var_processor(
    const char *var_name, const VarProcessor_var &var_proc)
  {
    proc_ctrl_vars_impl_->add_var_processor(var_name, var_proc);
  }

  inline char*
  ProcessControlVarsLoggerImpl::control(
    const char* param_name,
    const char* param_value)
    noexcept
  {
    try
    {
      if (!proc_ctrl_vars_impl_.in())
      {
        Stream::Error ostr;
        assert(dynamic_cast<Logging::ActiveObjectCallbackImpl*>(callback()));
        const char* aspect =
          dynamic_cast<Logging::ActiveObjectCallbackImpl*>(
            callback())->aspect();
        ostr << (aspect ? aspect : "Service") << " is not ready.";
        return CORBA::string_dup(ostr.str().str().c_str());
      }
      return proc_ctrl_vars_impl_->control(param_name,
        param_value);
    }
    catch (const eh::Exception &ex)
    {
      return CORBA::string_dup(ex.what());
    }
  }

  inline
  ProcessControlVarsLoggerImpl::~ProcessControlVarsLoggerImpl() noexcept
  {}

} // namespace Commons
} // namespace AdServer

#endif // _ADSERVER_COMMONS_PROCESS_CONTROL_VARS_IMPL_HPP_
