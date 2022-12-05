#ifndef _AUTOTEST__DYNAMICCREATIVECONTENTTEST_
#define _AUTOTEST__DYNAMICCREATIVECONTENTTEST_
 
#include <tests/AutoTests/Commons/Common.hpp>
 
 
class DynamicCreativeContentTest: public BaseUnit
{
public:
 
  DynamicCreativeContentTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var)
  {};
 
  virtual ~DynamicCreativeContentTest() noexcept
  {};
 
private:
  virtual bool run_test();

  void add_file_copy_descr_phrase_(
    const char* message, const char* src, const char* dst);
};

#endif //_AUTOTEST__DYNAMICCREATIVECONTENTTEST_

