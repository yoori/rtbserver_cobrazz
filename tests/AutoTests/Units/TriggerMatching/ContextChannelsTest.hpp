#ifndef _UNITTEST__CONTEXTCHANNELSTEST_
#define _UNITTEST__CONTEXTCHANNELSTEST_

#include <tests/AutoTests/Commons/Common.hpp>
 
typedef std::list <std::string> ChannelList;

class ContextChannelsTest: public BaseUnit
{
public:
 
  ContextChannelsTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var): 
    BaseUnit(stat_var, task_name, params_var),
    test_client_(AutoTest::AdClient::create_user(this)),
    req_count_(0)
  {};
 
  virtual ~ContextChannelsTest() noexcept
  {};
protected:

  AutoTest::AdClient test_client_; // AdServer client
private:

  unsigned long req_count_;                     // Client request count
  
  virtual bool run_test();

  // test cases
  void url_request_test_case();
  void page_request_test_case();
  void search_request_test_case();
  void composite_request_test_case();
};

#endif
