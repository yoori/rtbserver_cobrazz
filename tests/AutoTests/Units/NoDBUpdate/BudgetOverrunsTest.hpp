#ifndef _AUTOTEST__BUDGETOVERRUNSTEST_
#define _AUTOTEST__BUDGETOVERRUNSTEST_
 
#include <tests/AutoTests/Commons/Common.hpp>
  
class BudgetOverrunsTest: public BaseDBUnit
{
public:

  enum TestCaseFlag
  {
    TCF_NOT_SPENT = 0x1,
    TCF_CPC_RATE = 0x2
  };

  typedef void (BudgetOverrunsTest::* DynamicCaseFunc)(const std::string&);

  struct TestCase
  {
    std::string prefix;
    unsigned int flags;
    unsigned int requests_count;
    DynamicCaseFunc dynamic_part;
  };

  BudgetOverrunsTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseDBUnit(stat_var, task_name, params_var)
  { }
 
  virtual ~BudgetOverrunsTest() noexcept
  { }
 
private:

  static const TestCase TEST_CASES[];

  void process_case(size_t i);
  void campaign_zero_budget();
 
  bool run();
  void post_condition();
  void tear_down();

  // Dynamic part
  void decrease_budget_of_ccg(const std::string&);
  void decrease_budget_of_campaign(const std::string&);
  void spent_the_rest_budget(const std::string&);
  void increase_budget_of_ccg(const std::string&);
  void increase_budget_of_campaign(const std::string&);
  void set_unlim_budget_for_campaign(const std::string&);
  void set_unlim_budget_for_ccg(const std::string&);
};

#endif //_AUTOTEST__BUDGETOVERRUNSTEST_
