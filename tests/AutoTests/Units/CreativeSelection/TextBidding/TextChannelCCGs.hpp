
#ifndef _AUTOTEST__TEXTCHANNELCCGS_
#define _AUTOTEST__TEXTCHANNELCCGS_
  
#include <tests/AutoTests/Commons/Common.hpp>
  

class TextChannelCCGs : public BaseUnit
{
public:
  TextChannelCCGs(
    UnitStat& stat_var,
    const char* task_name,
    XsdParams params_var) :
    BaseUnit(stat_var, task_name, params_var)
  {};

  virtual ~TextChannelCCGs() noexcept
  {};

private:

  virtual bool run_test();

  void scenario1();
  void scenario2();
  void scenario3();
  void scenario4();
  void scenario5();
};

#endif // _AUTOTEST__TEXTCHANNELCCGS_
