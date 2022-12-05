
#ifndef _AUTOTEST__BAIDUTEST_
#define _AUTOTEST__BAIDUTEST_
  
#include <tests/AutoTests/Commons/Common.hpp>
  
class BaiduTest : public BaseDBUnit
{
public:
  BaiduTest(
    UnitStat& stat_var,
    const char* task_name,
    XsdParams params_var) :
    BaseDBUnit(stat_var, task_name, params_var)
  { }

  virtual ~BaiduTest() noexcept
  { }

protected:

  virtual void set_up();
  virtual bool run();
  virtual void tear_down();

private:

  AutoTest::Time now_;

  void base_();

};

#endif // _AUTOTEST__BAIDUTEST_
