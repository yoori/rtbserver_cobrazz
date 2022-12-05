
#ifndef _AUTOTEST__URLENCODINGTEST_
#define _AUTOTEST__URLENCODINGTEST_
  
#include <tests/AutoTests/Commons/Common.hpp>
  
typedef AutoTest::NSLookupRequest NSLookupRequest;

class URLEncodingTest : public BaseUnit
{
public:
  URLEncodingTest(
    UnitStat& stat_var,
    const char* task_name,
    XsdParams params_var) :
    BaseUnit(stat_var, task_name, params_var)
  {};

  virtual ~URLEncodingTest() noexcept
  {};

private:

  void
  passback_encoding(
    const char* tag,
    const char* passback,
    NSLookupRequest::Member param,
    bool with_frontend,
    const char* expected_passback);

  void
  tag_passback_encoding(
    const char* tag,
    const char* expected_passback);
  
  void
  clickurl_encoding();
  void
  clickurl_relocate_encoding();
  void
  optout_redirect();

  void
  clickurl_preclick_encoding(
    const char* preclick,
    const char* expected_redirect);

  virtual bool run_test();
};

#endif // _AUTOTEST__URLENCODINGTEST_
