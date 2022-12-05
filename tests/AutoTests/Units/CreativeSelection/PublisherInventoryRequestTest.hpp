#ifndef _UNITTEST__PUBLISHERINVENTORYREQUESTTEST_
#define _UNITTEST__PUBLISHERINVENTORYREQUESTTEST_

#include <tests/AutoTests/Commons/Common.hpp>

typedef AutoTest::AdClient AdClient;

class PublisherInventoryRequestTest: public BaseUnit
{
public:
 
  PublisherInventoryRequestTest(
              UnitStat& stat_var, 
              const char* task_name, 
              XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var)
  {};
 
  virtual ~PublisherInventoryRequestTest() noexcept
  {};
 
private:
 
  virtual bool run_test();

};

#endif
