#ifndef _AUTOTEST__GEOCREATIVESELECTION_
#define _AUTOTEST__GEOCREATIVESELECTION_
  
#include <tests/AutoTests/Commons/Common.hpp>
  

class GEOCreativeSelection : public BaseUnit
{
public:
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  
  struct TestRequest
  {
    const char* keyword;
    const char* location;
    const char* tid;
    const char* expected_channels;
    const char* expected_ccids;
  };
      
public:
  GEOCreativeSelection(
    UnitStat& stat_var,
    const char* task_name,
    XsdParams params_var) :
    BaseUnit(stat_var, task_name, params_var)
  {};

  virtual ~GEOCreativeSelection() noexcept
  {};

private:

  virtual bool run_test();


  template <size_t COUNT>
  void testcase(
    const std::string& description,
    const TestRequest (&requests)[COUNT]);
  
  void set_param(
    NSLookupRequest::NSLookupParam& param,
    const char* param_name,
    const char* separator = ",");
};

#endif // _AUTOTEST__GEOCREATIVESELECTION_
