
#ifndef _AUTOTEST__MAXPUBLISHERSHARETEST_
#define _AUTOTEST__MAXPUBLISHERSHARETEST_
 
#include <tests/AutoTests/Commons/Common.hpp>

namespace ORM = ::AutoTest::ORM;
 
class MaxPublisherShareTest: public BaseDBUnit
{
public:
 
  MaxPublisherShareTest(
    UnitStat& stat_var, 
    const char* task_name, 
    XsdParams params_var)
    : BaseDBUnit(stat_var, task_name, params_var),
      check_count_(0)
  { }
 
  virtual ~MaxPublisherShareTest() noexcept
  { }
 
private:

  int check_count_;
 
  void
  set_up();

  void
  tear_down();

  bool
  run();

  virtual
  void
  check() /*throw(eh::Exception)*/;

  virtual
  bool
  checker_call(
    const std::string& description,
    AutoTest::Checker* checker)
    /*throw(eh::Exception)*/;

  // Cases
  void
  share_expiring_();

  void
  increase_budget_();

  void
  increase_share_();

  void
  three_sites_part_1_(
    const AutoTest::Time& new_day);

  void
  three_sites_part_2_(
    const AutoTest::Time& new_day);

  void
  text_static_();

  void
  text_dynamic_();
  
  void
  text_daily_part_1_(
    const AutoTest::Time& new_day);

  void
  text_daily_part_2_(
    const AutoTest::Time& new_day);

  void
  text_daily_part_3_(
    const AutoTest::Time& new_day);

};

#endif //_AUTOTEST__MAXPUBLISHERSHARETEST_
