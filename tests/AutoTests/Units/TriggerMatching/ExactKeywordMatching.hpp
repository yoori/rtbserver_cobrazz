
#ifndef _AUTOTEST__EXACTKEYWORDMATCHING_
#define _AUTOTEST__EXACTKEYWORDMATCHING_
  
#include <tests/AutoTests/Commons/Common.hpp>
  

class ExactKeywordMatching : public BaseUnit
{

  typedef AutoTest::AdClient AdClient;

public:
  
  struct TestCase
  {
    const char* search;
    const char* matched;
    const char* unmatched;
  };

public:
  ExactKeywordMatching(
    UnitStat& stat_var,
    const char* task_name,
    XsdParams params_var) :
    BaseUnit(stat_var, task_name, params_var),
    case_idx_(0)
  { }

  virtual ~ExactKeywordMatching() noexcept
  { }

private:

  unsigned long case_idx_;

  virtual bool run_test();

  template<size_t Count>
  void
  test_group_(
    AdClient& client,
    const TestCase(&tests)[Count],
    unsigned long flags = 0);

  void
  test_case_(
    AdClient& client,
    const TestCase&test,
    unsigned long flags);

};

#endif // _AUTOTEST__EXACTKEYWORDMATCHING_
