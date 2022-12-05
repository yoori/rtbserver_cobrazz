#ifndef _AUTOTEST__GEOCHANNELSMATCHING_
#define _AUTOTEST__GEOCHANNELSMATCHING_
  
#include <tests/AutoTests/Commons/Common.hpp>
  

class GEOChannelsMatching : public BaseUnit
{
public:
  typedef AutoTest::NSLookupRequest NSLookupRequest;

  struct LocTestRequest
  {
    std::string description;
    const char* location;
    const char* expected;
    const char* unexpected;
    NSLookupRequest::Member param;
  };

  struct IPTestRequest
  {
    std::string description;
    const char* ip;
    const char* expected_location;
    const char* expected_channels;
  };

  
public:
  GEOChannelsMatching(
    UnitStat& stat_var,
    const char* task_name,
    XsdParams params_var) :
    BaseUnit(stat_var, task_name, params_var)
  {};

  virtual ~GEOChannelsMatching() noexcept
  {};

private:

  virtual bool
  run_test();

  void
  location_name_case(
    const LocTestRequest& testcase);

  void
  ip_case(
    const IPTestRequest& testcase);

};

#endif // _AUTOTEST__GEOCHANNELSMATCHING_
