
#ifndef _AUTOTEST__FRAUDPROTECTIONFEATURETEST_
#define _AUTOTEST__FRAUDPROTECTIONFEATURETEST_
 
#include <tests/AutoTests/Commons/Common.hpp>
 



class FraudProtectionFeatureTest: public BaseDBUnit
{

  typedef std::map<std::string, bool> RequestList;
  typedef std::map<std::string, bool> UserList;
  typedef AutoTest::AdClient AdClient;

public:
  struct TestRequest
  {
    const char* referer_kw;
    const char* tid;
    const char* format;
    unsigned long time_ofset;
    const char* expected_channels;
    const char* expected_ccs;
    unsigned long count;
    unsigned long flags;
  };
  
public:
 
  FraudProtectionFeatureTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseDBUnit(stat_var, task_name, params_var)
  { }
 
  virtual ~FraudProtectionFeatureTest() noexcept
  { }

protected:

  virtual bool run();
  virtual void set_up();
  virtual void tear_down();
 
private:
 
  // helper methods
  template<size_t Count>
  void
  process_request(
    AdClient& client,
    const TestRequest (&data)[Count],
    const AutoTest::Time& dtime = Generics::Time::ZERO);
  
  // Scenario
  void click_fraud_();
  void genuine_user_();
  void mixed_();
  void cpa_part_1_(
    AdClient& client);
  void cpa_part_2_(
    AdClient& client);
  void no_fraud_();
  void ta_no_fraud_();
  void ta_channel_fraud_();
  void ta_text_fraud_();
  void unconfirmed_imps_();
  void merging_();
  void impression_fraud_(
    const std::string& suffix,
    bool wait_fraud);
  void fraud_override_();
  void reversed_order_fraud_part_1_(
    AdClient& client);
  void reversed_order_fraud_part_2_(
    AdClient& client);
  void delayed_clicks_();

  AutoTest::Time today;    // 'today' timestamp
};

#endif //_AUTOTEST__FRAUDPROTECTIONFEATURETEST_

