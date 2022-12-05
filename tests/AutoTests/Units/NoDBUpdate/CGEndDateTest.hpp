
#ifndef _AUTOTEST__CGENDDATETEST_
#define _AUTOTEST__CGENDDATETEST_
 
#include <tests/AutoTests/Commons/Common.hpp>
 
 
class CGEndDateTest: public BaseUnit
{

  enum CCGCheck
  {
    CCG_INACTIVE = 1, // CCG 'I' status check
    CCG_ACTIVE = 2    // CCG 'A' status check
  };

  struct TestRequest
  {
    std::string kwd;
    unsigned long ccid;
    unsigned long ccgid;
    unsigned short flags;
  };
  
public:
 
  CGEndDateTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var)
  { }
 
  virtual ~CGEndDateTest() noexcept
  { }
 
private:
 
  virtual bool run_test();

  // Utils
  template <size_t COUNT>
  void
  process_requests_(
    const std::string& description,
    unsigned long tag,
    const TestRequest (&requests)[COUNT]);

  // Cases

  void
  gmt_case_();

};

#endif //_AUTOTEST__CGENDDATETEST_
