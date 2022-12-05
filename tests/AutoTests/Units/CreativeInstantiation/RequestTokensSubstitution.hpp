
#ifndef _AUTOTEST__REQUESTTOKENSSUBSTITUTION_
#define _AUTOTEST__REQUESTTOKENSSUBSTITUTION_
  
#include <tests/AutoTests/Commons/Common.hpp>
  

class RequestTokensSubstitution : public BaseUnit
{
public:
  enum CreativeEnum
  {
    CE_SIMPLE,
    CE_SIMPLE_AGENCY
  };
  
public:
  RequestTokensSubstitution(
    UnitStat& stat_var,
    const char* task_name,
    XsdParams params_var) :
    BaseUnit(stat_var, task_name, params_var)
  {};

  virtual ~RequestTokensSubstitution() noexcept
  {};

private:

  virtual bool run_test();

  std::string get_creative_content(
    CreativeEnum creative,
    bool empty_tokens,
    const AutoTest::AdClient& client);
};

#endif // _AUTOTEST__REQUESTTOKENSSUBSTITUTION_
