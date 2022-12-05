#ifndef _UNITTEST__USERPROFILEMERGINGTEST_
#define _UNITTEST__USERPROFILEMERGINGTEST_
 
#include <tests/AutoTests/Commons/Common.hpp>  
 
class UserProfileMergingTest: 
  public BaseUnit
{

  typedef AutoTest::NSLookupRequest  NSLookupRequest;
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::TemporaryAdClient TemporaryAdClient;
  
public:
  enum RequestEnum
  {
    RE_TEMPORARY,
    RE_PERSISTENT,
    RE_MERGING
  };

  struct ExpectedType
  {
    const char* triggers;
    const char* history;    
  };

  struct RequestType
  {
    long time_ofset;
    RequestEnum req_type;
    const char* referer_kw;
    ExpectedType expected;
    ExpectedType unexpected;
  };
  
public:
  UserProfileMergingTest(
    UnitStat& stat_var, 
    const char* task_name, 
    XsdParams params_var)
    : BaseUnit(stat_var, task_name, params_var)
  {};
 
  virtual ~UserProfileMergingTest() noexcept
  {};

  void check(
    const std::string& description,
    const RequestType* requests,
    size_t requests_size,
    const AutoTest::Time& base_time = Generics::Time::ZERO);

  void log_admin_output(
    AdClient& pclient,
    TemporaryAdClient& tclient,
    RequestEnum request_type);
 
private:
  virtual bool run_test();

private:
  AutoTest::Time base_time;
};

#endif
