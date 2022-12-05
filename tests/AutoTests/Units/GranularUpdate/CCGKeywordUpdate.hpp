
#ifndef _AUTOTEST__CCGKEYWORDUPDATE_
#define _AUTOTEST__CCGKEYWORDUPDATE_
  
#include <tests/AutoTests/Commons/Common.hpp>

namespace ORM = ::AutoTest::ORM;

class CCGKeywordUpdate : public BaseDBUnit
{
public:
  CCGKeywordUpdate(
    UnitStat& stat_var,
    const char* task_name,
    XsdParams params_var) :
    BaseDBUnit(stat_var, task_name, params_var)
  { }

  virtual ~CCGKeywordUpdate() noexcept
  { }

private:

  virtual void
  set_up();

  virtual void
  tear_down();

  virtual bool
  run();

  void
  add_ccgkeyword_();

  void
  activate_ccgkeyword_();

  void
  deactivate_ccgkeyword_();

  void
  deactivate_channel_();

  void
  change_ccgkeyword_();
  
};

#endif // _AUTOTEST__CCGKEYWORDUPDATE_
