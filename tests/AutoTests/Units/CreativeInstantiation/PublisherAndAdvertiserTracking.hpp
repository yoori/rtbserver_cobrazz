
#ifndef _AUTOTEST__PUBLISHERANDADVERTISERTRACKING_
#define _AUTOTEST__PUBLISHERANDADVERTISERTRACKING_
  
#include <tests/AutoTests/Commons/Common.hpp>
  

class PublisherAndAdvertiserTracking : public BaseUnit
{

  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef NSLookupRequest::NSLookupParam NSLookupParam;

public:

  struct TestRequest
  {
    const char* format;
    const char* tid;
    const char* colo;
    const char* testrequest;
    const char* ccid;
    const char* track_pixel;
    const char* cradvtrackpixel;
    const char* publ_tag_track_pixel;
  };
  
public:
  PublisherAndAdvertiserTracking(
    UnitStat& stat_var,
    const char* task_name,
    XsdParams params_var) :
    BaseUnit(stat_var, task_name, params_var)
  {};

  virtual ~PublisherAndAdvertiserTracking() noexcept
  {};

private:

  virtual bool run_test();

  // Utils
  void set_request_param(
    NSLookupParam& request_param,
    const char* param_name);
  
  void prepare_expected_body(
    std::ostringstream& body,
    const std::string& impression_url,
    const TestRequest& test);
};

#endif // _AUTOTEST__PUBLISHERANDADVERTISERTRACKING_
