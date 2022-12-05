
#ifndef __INVALIDCOOKIESTEST_HPP
#define __INVALIDCOOKIESTEST_HPP

#include <tests/AutoTests/Commons/Common.hpp>

/**
 * @class InvalidCookiesTest
 * @brief test for invalid cookies processing
 */
                                 
class InvalidCookiesTest: public BaseUnit
{
  
public:
 
  InvalidCookiesTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var),
    client(AutoTest::AdClient::create_user(this))
  {};
 
  virtual ~InvalidCookiesTest() noexcept
  {};
 
private:
  AutoTest::AdClient client;
  AutoTest::NSLookupRequest ns_request;
  AutoTest::OptOutRequest   optout_request;
    
  virtual bool run_test();
  //test utils
  std::string crack_cookie(const char *cookie_name,
                           unsigned short pos,
                           const char* extStr,
                           bool replacing = true);
  void
  crack_cookie_and_check_serv_behaviour(const char* cookie_name,
                                        unsigned short pos,
                                        const char* extStr,
                                        bool replacing = true,
                                        bool check_host_cookie_presents = true);
  //test cases
  void set_up();
  
  void invalid_base64_uid_test_case();
  void invalid_uid_test_case();

  void invalid_last_colo_id_test_case();
  void opt_out_test_case();
  void invalid_opt_out_status_test_case();
  void opt_out_after_crack_test_case();

};

#endif  // __INVALIDCOOKIESTEST_HPP
