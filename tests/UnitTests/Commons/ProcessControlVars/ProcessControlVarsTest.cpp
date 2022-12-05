/// @file ProcessControlVarsTest.cpp

#include <iostream>
#include <Logger/StreamLogger.hpp>
#include <Commons/ProcessControlVarsImpl.hpp>

using namespace AdServer::Commons;

const char ASPECT[] = "TEST_ASPECT";

struct CallContext
{
  const char* p1;
  const char* p2;
};

const CallContext TEST_CASES[] =
{
 {"name", "value"},
 {"name", "value"},
 {LogLevelProcessor::VAR_NAME, "value"},
 {LogLevelProcessor::VAR_NAME, "8"},
 {DbStateProcessor::VAR_NAME, "1"},
 {DbStateProcessor::VAR_NAME, "0"}
};

const std::size_t TEST_CASES_AMOUNT = sizeof(TEST_CASES) /
  sizeof(TEST_CASES[0]);

const char* RESULTS[] =
{
  "TEST_ASPECT is not ready.",
  "No processor found for variable: 'name'",
  "Failed to set variable 'LOG_LEVEL' to 'value'",
  "Variable 'LOG_LEVEL' has been set to '8'",
  "Variable 'DB' has been set to '1'",
  "Variable 'DB' has been set to '0'"
};

struct Test : public AdServer::Commons::ProcessControlVarsLoggerImpl
{
  Test() :
    AdServer::Commons::ProcessControlVarsLoggerImpl(
      "ProcessControlVarsTestApp_", ASPECT)
  {}
private:
  virtual
  ~Test() noexcept
  {}
};

class DbStateChangerImpl:
  public virtual ReferenceCounting::AtomicImpl,
  public AdServer::Commons::DbStateChanger
{
public:
  virtual void
  change_db_state(bool new_state)
  {
    state_ = new_state;
  }

  virtual bool
  get_db_state() noexcept
  {
    return state_;
  }

private:
  bool state_;
};

typedef ReferenceCounting::SmartPtr<DbStateChangerImpl>
  DbStateChangerImpl_var;

void
check_test_case(const ProcessControlVarsLoggerImpl_var& var, std::size_t index)
{
  CORBA::String_var result =
    var->control(TEST_CASES[index].p1, TEST_CASES[index].p2);
  if (strcmp(result, RESULTS[index]))
  {
    std::cerr << "FAIL: can't set " << TEST_CASES[index].p1
      << " to " << TEST_CASES[index].p2 << ": " << result << std::endl;
  }
  else
  {
    std::cout << index << ": " << result << std::endl;
  }
}

int
main()
{
  std::cout << "ProcessControlVarsTest started" << std::endl;
  std::size_t test_case_index = 0;
  {
    ProcessControlVarsLoggerImpl_var var = new Test;
    check_test_case(var, test_case_index++);
  }
  ProcessControlVarsLoggerImpl_var var = new Test;
  var->register_vars_controller();
  for (; test_case_index < 4; ++test_case_index)
  {
    check_test_case(var, test_case_index);
  }

  DbStateChangerImpl_var db_state = new DbStateChangerImpl;
  var->add_var_processor(DbStateProcessor::VAR_NAME,
    new DbStateProcessor(db_state));
  check_test_case(var, test_case_index++);
  if (!db_state->get_db_state())
  {
    std::cerr << "FAIL: DB state wasn't set to 1" << std::endl;
  }
  check_test_case(var, test_case_index++);
  if (db_state->get_db_state())
  {
    std::cerr << "FAIL: DB state wasn't set to 0" << std::endl;
  }
  std::cout << "ProcessControlVarsTest finished" << std::endl;
  return 0;
}
