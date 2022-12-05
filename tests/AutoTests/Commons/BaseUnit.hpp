/** $Id$
 * @file tests/AutoTests/Commons/BaseUnit.hpp
 * base AutoTest execution unit
 */
#ifndef __AUTOTESTS_COMMONS_BASEUNIT_HPP
#define __AUTOTESTS_COMMONS_BASEUNIT_HPP

#include <sstream>
#include <map>

#include <Generics/TaskRunner.hpp>
#include <Sync/SyncPolicy.hpp>

#include <tests/AutoTests/Commons/Utils.hpp>
#include <tests/AutoTests/Commons/Logger.hpp>
#include <tests/AutoTests/Commons/Connection.hpp>
#include <tests/AutoTests/Commons/Checkers/Checker.hpp>
#include <tests/AutoTests/Commons/Checkers/CompositeCheckers.hpp>

#include "AutoTestsXsd.hpp"

// only one argument allowed at ... places : const char* or std::string
#define ADD_WAIT_CHECKER(description, checker)          \
{ \
  std::ostringstream ostr; \
  std::string file_name; \
  AdServer::PathManip::split_path(__FILE__, 0, &file_name); \
  ostr << "'" << description << "'" << std::endl << \
    "  " << file_name << "#" <<  __LINE__ << ": " << __FUNCTION__ << "()"; \
  add_wait_checker( \
    ostr.str(), \
    checker); \
};

/**
 * @struct UnitConfig
 *
 * @brief Struct for saving test setting.
 *
 * Contains test settings.
 */
struct UnitConfig
{
  bool verbose;       // need verbose (dump test result to output).
  bool verbose_start; // need verbose start (dump test start/stop to output).

  /**
   * @brief Default constructor.
   */
  UnitConfig();
};

/**
 * @class UnitStat
 *
 * @brief Struct for saving unit's statistics.
 *
 * Contains statistics data.
 */
struct UnitStat: public virtual ReferenceCounting::DefaultImpl<>
{
  typedef Sync::Policy::PosixThread SyncPolicy;
  typedef SyncPolicy::Mutex LockMutex;

  enum UnitMode { UM_NO_DB = 1 };

  LockMutex&     lock;
  UnitConfig     config;      // test config.
  Generics::Time start_time;  // test start time.
  Generics::Time stop_time;   // test stop time.
  std::string    unit_name;   // test name.
  std::string    error;       // stored error (use for multicase tests).
  bool           succeed;     // test status (fail or succeed).
  unsigned long  flags;       // test flags


  /**
   * @brief Constructor.
   * @param test lock.
   * @param test config.
   * @param test mode (fast or slow).
   * @param test name.
   * @param error string.
   * @param test status (fail or succeed).
   */
  UnitStat(
    LockMutex& lock_guard,
    const UnitConfig& unit_config,
    unsigned long flags = 0,
    std::string unit_name_var = "Not set",
    std::string error_var = "",
    bool succeed_var = false);

  /**
   * @brief Destructor.
   */
  ~UnitStat() noexcept;


  /**
   * @brief Get test duration.
   * @return duration.
   */
  Generics::Time duration () const;

  /**
   * @brief Mark test as started.
   */
  void mark_begin ();

  /**
   * @brief Mark test as succeed.
   */
  void mark_ok    ();

  /**
   * @brief Mark test as failed (scenario error).
   */
  void mark_error ();

  /**
   * @brief Mark test as failed (fail on check).
   */
  void mark_fault ();

  /**
   * @brief Dump error to standard output.
   */
  void dump_error ();

  /**
   * @brief Adserver run in DB active mode.
   */
  bool db_active () const noexcept;
  
};

typedef ReferenceCounting::SmartPtr<UnitStat> UnitStat_var;
typedef const xsd::tests::AutoTests::UnitLocalDataElemType& DataElemObjectRef;
typedef const xsd::tests::AutoTests::UnitLocalDataElemType* DataElemObjectPtr;

/**
 * @class BaseUnit
 *
 * @brief Unit base class.
 *
 * Intended to be used as a base class for units. Contains data and
 * functionality common for all units. Also provides a task interface for units.
 */
class BaseUnit
{

  typedef std::tuple<std::string, AutoTest::Logger&, AutoTest::Internals::CheckerHolder_var>
    DescriptiveChecker;
  typedef std::list<DescriptiveChecker> DescriptiveCheckerList;

  typedef std::map<std::string, AutoTest::Logger_var> Loggers;
 
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  DECLARE_EXCEPTION(InvalidArgument, Exception);

  /**
   * @brief Constructor.
   * @param test statistics.
   * @param test name.
   * @param test config.
   */
  BaseUnit(
    UnitStat& stat_var,
    const char* task_name,
    XsdParams params_var);

  /**
   * @brief Destructor.
   */
  virtual ~BaseUnit() noexcept;
 
  /**
   * @brief Get config.
   * @return config object.
   */
  const GlobalConfig& get_config() const noexcept;

  /**
   * @brief Get global config.
   * @return global config object.
   */
  Params get_global_params() noexcept;


  /**
   * @brief Get local test config.
   * @return local test config object.
   */
  Locals get_local_params() noexcept;

  /**
   * @brief Get local test config element.
   * @param element name.
   * @return local test config element.
   */
  DataElemObjectRef get_object_by_name(
    const std::string& obj_name)
    /*throw(Exception, InvalidArgument)*/;

  /**
   * @brief Get local test config element as string.
   * @param element name.
   * @return element string value.
   */
  std::string fetch_string(const char* obj_name);

  /**
   * @brief Get local test config element as string.
   * @param element name.
   * @return element string value.
   */
  std::string fetch_string(const std::string& obj_name);

  /**
   * @brief Get local test config element as integer.
   * @param element name.
   * @return element integer value.
   */
  unsigned long fetch_int(const char* obj_name)
    /*throw(Exception)*/;

  /**
   * @brief Get local test config element as integer.
   * @param element name.
   * @return element integer value.
   */
  unsigned long fetch_int(const std::string& obj_name)
    /*throw(Exception)*/;

  /**
   * @brief Get local test config element as float.
   * @param element name.
   * @return element float value.
   */
  long double fetch_float(const char* obj_name)
    /*throw(Exception)*/;

  /**
   * @brief Get local test config element as float.
   * @param element name.
   * @return element float value.
   */
  long double fetch_float(const std::string& obj_name)
    /*throw(Exception)*/;

  /**
   * @brief Map object names to object values.
   * @param object names as separated string.
   * @param separator.
   * @return object values as separated string.
   */
  std::string
  map_objects(
    const char* obj_names,
    const char* separator = ",")
    /*throw(Exception)*/;

  /**
   * @brief Get element as sequence.
   * @param sequence iterator.
   * @param object names as separated string.
   * @param separator.
   */
  template<typename InsertIterator>
  InsertIterator fetch_int_sequence(
    InsertIterator ins_it,
    const std::string& obj_name,
    const char* separator = ",")
    /*throw(Exception)*/;

  /**
   * @brief Get sequence of elements.
   * @param sequence iterator.
   * @param element name.
   * @param values separator.
   */
  template<typename InsertIterator>
  InsertIterator fetch_objects(
    InsertIterator ins_it,
    const char* obj_names,
    const char* separator  = ",")
    /*throw(Exception)*/;
  
  /**
   * @brief Add description to test log.
   *        Using for mark test parts.
   * @param description.
   */
  void
  add_descr_phrase(const String::SubString& phrase)
    /*throw(Exception)*/;

  /**
   * @brief Add description to test log.
   *        Using for mark test parts.
   * @param description.
   */
  void
  add_descr_phrase(const char* phrase)
    /*throw(Exception)*/;

  /**
   * @brief Get next element from local config lists.
   * @param[out] element.
   * @param list name.
   * @return true if next exists.
   */
  bool next_list_item(DataElemObjectPtr& res, const std::string& list_name)
    /*throw(Exception, InvalidArgument)*/;

  /**
   * @brief Get client index.
   *        Using for mark test HTTP clients.
   * @return unique index for test HTTP client.
   */
  unsigned long get_client_index();

  /**
   * @brief Execute test.
   */
  virtual void execute() noexcept;

  /**
   * @brief Open POSTGRES connection
   * @return POSTGRES connection
   */
  AutoTest::DBC::IConn*
  open_pq() /*throw(eh::Exception)*/;

  /**
   * @brief Add checker to unit.
   * @param description.
   * @param checker
   */
  template<typename CheckerType>
  void
  add_checker(
    const std::string& description,
    const CheckerType& checker)
    /*throw(eh::Exception)*/;

  template<class T>
  void
  add_wait_checker(
    const std::string& description,
    const T& checker) /*throw(eh::Exception)*/;

  /**
   * @brief Get test timeout (used in WaitChecker).
   * @param timeout decrement.
   * @return current test timeout.
   */
  static int timeout();

protected:

  /**
   * @brief Add logger by name.
   * @param logger name.   
   * @return exists or new logger with name.
   */
  AutoTest::Logger&
  add_logger(
    const std::string& log_name);
  
  /**
   * @brief Get element as float.
   * @param [out] element float value.   
   * @param element name.
   */
  void fetch(
    long double& v,
    const std::string& obj_name)
    /*throw(Exception)*/;

  /**
   * @brief Get element as integer.
   * @param [out] element integer value.   
   * @param element name.
   */
  void fetch(
    unsigned long& v,
    const std::string& obj_name)
    /*throw(Exception)*/;

  /**
   * @brief Get element as string.
   * @param [out] element string value.   
   * @param element name.
   */
  void fetch(
    std::string& v,
    const std::string& obj_name)
    /*throw(Exception)*/;

  /**
   * @brief Get element by container type.
   * @param container iterator.   
   * @param element name.
   */
  template < 
    template <class T, class A> class container,
    template <class T> class allocator,
    class Value>
  void
  fetch(
    std::insert_iterator< container<Value, allocator<Value> > > &iterator,
    const std::string& obj_name)
    /*throw(Exception)*/;

  /**
   * @brief Run checkers.
   */
  virtual void check(bool clear_checkers = true) /*throw(eh::Exception)*/;

  /**
   * @brief Run a checker.
   *
   * @param description
   * @param checker
   */
  virtual bool
  checker_call(
    const std::string& description,
    AutoTest::Checker* checker) /*throw(eh::Exception)*/;

private:
  virtual bool run_test() = 0;

protected:
  UnitStat& stat_;
  unsigned long current_client_index_;
  Loggers loggers_;
  std::string task_name_;
  int timeout_;

private:
  
  XsdParams params_;
  std::ostringstream test_descr_;
  unsigned long descr_counter_;
  std::map<std::string, unsigned long> xml_lists_;
  static Sync::Key<int> timeout_key_;
  DescriptiveCheckerList checkers_;
};

#include "BaseUnit.ipp"
#include "BaseUnit.tpp"

#endif //__AUTOTESTS_COMMONS_BACEUNIT_HPP
