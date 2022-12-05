#ifndef _AUTOTEST__TEXTCHANNELANDKEYWORDCREATIVESCOMPETITION_
#define _AUTOTEST__TEXTCHANNELANDKEYWORDCREATIVESCOMPETITION_
  
#include <tests/AutoTests/Commons/Common.hpp>
  

class TextChannelAndKeywordCreativesCompetition : public BaseUnit
{
public:
  TextChannelAndKeywordCreativesCompetition(
    UnitStat& stat_var,
    const char* task_name,
    XsdParams params_var) :
    BaseUnit(stat_var, task_name, params_var)
  {};

  virtual ~TextChannelAndKeywordCreativesCompetition() noexcept
  {};

private:

  virtual bool run_test();

  void scenario1();
  void scenario2();
  void scenario3();
  void scenario4();
  void scenario5();
  void scenario6();
};

#endif // _AUTOTEST__TEXTCHANNELANDKEYWORDCREATIVESCOMPETITION_
