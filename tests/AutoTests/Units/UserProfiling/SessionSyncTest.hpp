#ifndef _UNITTEST__SESSIONSYNCTEST_
#define _UNITTEST__SESSIONSYNCTEST_

#include <Generics/TaskRunner.hpp> 
#include <tests/AutoTests/Commons/Common.hpp>

/**
 * @class SessionSyncTest
 * @brief This test, can and must fault because race condition
 */
class SessionSyncTest: 
  public BaseUnit
{
public:
 
  SessionSyncTest(UnitStat& stat_var, 
    const char* task_name, 
    XsdParams params_var);
 
  virtual ~SessionSyncTest() noexcept
  {};
 
private:
  bool run_test_element(int count,
                        unsigned int minimum_visits, 
                        const std::string& keyword);
  virtual bool run_test();
  //
  Generics::ActiveObjectCallback_var callback_;
};

#endif
