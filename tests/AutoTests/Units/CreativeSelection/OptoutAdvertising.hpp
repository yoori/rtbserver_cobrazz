#ifndef _AUTOTEST__OPTOUTADVERTISING_
#define _AUTOTEST__OPTOUTADVERTISING_
  
#include <tests/AutoTests/Commons/Common.hpp>
  

class OptoutAdvertising : public BaseUnit
{
  
public:

  struct TestCase {
    const char* colo;
    const char* optin_ccid;
    const char* undef_ccid;
    const char* optout_ccid;
    unsigned short flags;
  };

  typedef AutoTest::NSLookupRequest NSLookupRequest;
  
public:
  OptoutAdvertising(
    UnitStat& stat_var,
    const char* task_name,
    XsdParams params_var) :
    BaseUnit(stat_var, task_name, params_var)
  {};

  virtual ~OptoutAdvertising() noexcept
  {};

private:

  virtual bool run_test();

  template <size_t Count>
  void
  run_test_case(
    const TestCase(&testcases)[Count],
    const NSLookupRequest& base_request);

  void optout_click_and_impression();
  
};

#endif // _AUTOTEST__OPTOUTADVERTISING_

