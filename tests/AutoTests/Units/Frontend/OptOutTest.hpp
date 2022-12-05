#ifndef _UNITTEST__OPTOUTTEST_
#define _UNITTEST__OPTOUTTEST_

#include <tests/AutoTests/Commons/Common.hpp>

namespace
{
  typedef AutoTest::AdClient AdClient;
}

class OptOutTest: public BaseUnit
{
  
public:

  OptOutTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var)
  { }
 
  virtual ~OptOutTest() noexcept
  { }
 
private:

  virtual bool run_test();

  void base_scenario ();

  //============================================================
  //Scenarios & utils adding by changes in ADSC-53 at 12/02/2008
  //============================================================
  void optout_status_redirect_scenario();
  
  //============================================================
  //scenario for testing correct OptOutFrontend redirection 
  //by getting invalid uid
  //============================================================
  void incorrect_uid_opt_out_scenario ();
 
  //============================================================
  //scenario for testing correct no cookies client matching
  //in the different colo cases
  //============================================================
  void client_without_cookes_scenario();

  //============================================================
  //scenario for testing cookies expiration on OO request
  //ce parameter used
  //============================================================
  void cookie_expiration();

};

#endif
