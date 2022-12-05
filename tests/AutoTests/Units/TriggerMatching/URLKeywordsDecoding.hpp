
#ifndef _AUTOTEST__URLKEYWORDSDECODING_
#define _AUTOTEST__URLKEYWORDSDECODING_
  
#include <tests/AutoTests/Commons/Common.hpp>

class URLKeywordsDecoding : public BaseUnit
{
public:
  URLKeywordsDecoding(
    UnitStat& stat_var,
    const char* task_name,
    XsdParams params_var) :
    BaseUnit(stat_var, task_name, params_var)
  { }

  virtual ~URLKeywordsDecoding() noexcept
  { }

private:

  virtual bool run_test();
};

#endif // _AUTOTEST__URLKEYWORDSDECODING_
