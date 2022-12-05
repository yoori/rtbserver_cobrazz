
#ifndef _AUTOTEST__FREQCAPMERGINGTEST_
#define _AUTOTEST__FREQCAPMERGINGTEST_
 
#include <tests/AutoTests/Commons/Common.hpp>

/**
 * @class FreqCapMergingTest
 * @brief Test that persistent client's frequency caps policy 
 * isn't remove after merging with temporary client
 */
class FreqCapMergingTest: public BaseUnit
{
public:

  enum RequestEnum
  {
    RE_TEMPORARY,
    RE_PERSISTENT,
    RE_MERGE
  };
      
  struct TestRequest
  {
    unsigned long time_ofset;
    RequestEnum request_type;
    const char* referer_kw;
    const char* tid;
    const char* expected_history;
    const char* expected_ccid;
  };
  
public:
  FreqCapMergingTest(
    UnitStat& stat_var, 
    const char* task_name, 
    XsdParams params_var)
    : BaseUnit(stat_var, task_name, params_var)
  {};

  virtual ~FreqCapMergingTest() noexcept
  {};

private:
  virtual bool run_test();

  template <size_t COUNT>
  void check(
    const std::string& description,
    const TestRequest (&requests)[COUNT]);
};
 
#endif //_AUTOTEST__FREQCAPMERGINGTEST_
