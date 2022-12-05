
#ifndef _AUTOTEST__TEXTADVERTISINGNETANDGROSSTEST_
#define _AUTOTEST__TEXTADVERTISINGNETANDGROSSTEST_
 
#include <tests/AutoTests/Commons/Common.hpp>

namespace ORM = AutoTest::ORM;

/**
 * @class TextAdNetAndGrossTest
 * @brief Test Net & gross campaigns behaviour
 * in the case of text advertising.
 */
class TextAdNetAndGrossTest: public BaseDBUnit
{
public:
 
  TextAdNetAndGrossTest(
    UnitStat& stat_var, 
    const char* task_name, 
    XsdParams params_var)
    : BaseDBUnit(stat_var, task_name, params_var)
  {};
 
  virtual ~TextAdNetAndGrossTest() noexcept
  {};
 
protected:
 
  virtual void pre_condition();
  virtual bool run();
  virtual void post_condition();
  virtual void tear_down();

  void case_tag_with_and_without_commission();
  void case_competition();
  void case_publisher_commission();

private:

  typedef ORM::StatsArray<ORM::HourlyStats, 6> Stats;
  Stats stats_;
  Generics::Time target_request_time_;
};
 
#endif //_AUTOTEST__TEXTADVERTISINGNETANDGROSSTEST_
