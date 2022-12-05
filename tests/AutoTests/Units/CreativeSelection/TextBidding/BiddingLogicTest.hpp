#ifndef _UNITTEST__BIDDINGLOGICTEST_
#define _UNITTEST__BIDDINGLOGICTEST_

#include <tests/AutoTests/Commons/Common.hpp>
 
 
class BiddingLogicTest: public BaseUnit
{
  
public:
 
  BiddingLogicTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var)
  {};
 
  virtual ~BiddingLogicTest() noexcept
  {};
 
private:

  
  virtual bool run_test();

  void scenario1 ();
  void scenario2 ();
  void scenario3 ();
  void scenario4 ();
  void scenario5 ();
  void scenario6 ();
  void scenario7 ();
  void scenario8 ();
  void scenario9 ();

};

#endif
