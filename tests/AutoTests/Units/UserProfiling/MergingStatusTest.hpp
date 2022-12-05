#ifndef _UNITTEST__MERGINGSTATUSTEST_
#define _UNITTEST__MERGINGSTATUSTEST_

#include <tests/AutoTests/Commons/Common.hpp>

typedef AutoTest::NSLookupRequest  NSLookupRequest;
typedef AutoTest::AdClient AdClient;
typedef AutoTest::TemporaryAdClient TemporaryAdClient;
 
class MergingStatusTest: public BaseUnit
{
public:
 
  MergingStatusTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var),
    client(AdClient::create_user(this))
  {};
 
  virtual ~MergingStatusTest() noexcept
  {};
 
private:
  virtual bool run_test();
  AdClient client;
  std::string uid;

  void set_up();
  // test cases
  void double_merging();
  void unknown_tuid();
  // utils
  std::string create_temporary_profile();
  
};

#endif
