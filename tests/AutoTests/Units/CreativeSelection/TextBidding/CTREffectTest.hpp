#ifndef _UNITTEST__CTRCALCULATIONLOGICTEST_
#define _UNITTEST__CTRCALCULATIONLOGICTEST_

#include <tests/AutoTests/Commons/Common.hpp>


class CTREffectTest: public BaseUnit
{
public:
 
  CTREffectTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var)
  {};
 
private:

  struct TestCaseRequest
  {
    std::string tag;
    AutoTest::NSLookupRequest::Member request_param_name;
    std::string request_param_value;
    std::string expected_ccids;
    std::string click_revenue;
  };

  bool run_test();

  // helper funcs
  template<size_t Count>
  void process_requests(const TestCaseRequest(&requests)[Count],
                        const std::string& prefix = std::string());

  // Test cases
  void regular_adjustment_();
  void invalid_ctr_();
  void ccg_concurrence_();
  void negative_ctr_();
  void time_of_week_();
};

#endif // _UNITTEST__CTRCALCULATIONLOGICTEST_
