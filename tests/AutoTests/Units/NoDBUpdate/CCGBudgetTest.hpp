
#ifndef _AUTOTEST__PAUSINGCGREACHINGTEST_
#define _AUTOTEST__PAUSINGCGREACHINGTEST_
 
#include <tests/AutoTests/Commons/Common.hpp>

namespace
{
  typedef AutoTest::ORM::PQ::CampaignCreativeGroup CCG;
  typedef AutoTest::ORM::ORMFloat CCG::* CCGMember;
}
 
class CCGBudgetTest: public BaseDBUnit
{
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::CampaignChecker CampaignChecker;

public:
  
  enum CaseFlags
  {
    CF_BUDGET_NOT_SPENT = 0x1,  // CCG budget don't spent during test
    CF_BUDGET_DYNAMIC = 0x2,    // CCG have dynamic budget
    CF_CPC = 0x4,               // Use CPC revenue instead CPM
    CF_SPEND_REST_BUDGET = 0x8  // whether to run additional case to spent rest budget
  };

  typedef void (CCGBudgetTest::* DynamicFunction)(const std::string&);
  struct TestCase
  {
    std::string description;// Case description
    std::string prefix;     // Case prefix 
    unsigned long flags;     
    unsigned long interval; // Requests time interval (in seconds)
    unsigned long requests; // Count of test requests
                            // (0 used for dynamic calculation)
    long debug_time_shift;  //
    DynamicFunction dynamic_part; // ptr to func that will be called in db mode case
  };

public:
  CCGBudgetTest(
    UnitStat& stat_var, 
    const char* task_name, 
    XsdParams params_var)
    : BaseDBUnit(stat_var, task_name, params_var)
  {}
 
  virtual ~CCGBudgetTest() noexcept
  {}

protected:

  virtual
  bool
  checker_call(
    const std::string& description,
    AutoTest::Checker* checker)
    /*throw(eh::Exception)*/;

  virtual void tear_down();
  virtual bool run();
  virtual void post_condition();
 
private:

  static const TestCase GMT_CASES[];
  static const TestCase GMT_PLUS_3_CASES[];
  static const TestCase GMT_MINUS_3_CASES[];

  template<size_t SIZE>
  void process_cases(
    const TestCase (&cases)[SIZE],
    bool initial);

  template<size_t SIZE>
  void process_dynamic_cases(
    const TestCase (&cases)[SIZE]);

  void process_case(
    const TestCase& test,
    double& realized_budget,
    bool initial);

  std::map<std::string, double> budget_reached;
  std::map<std::string, Generics::Time> campaign_last_request;

  Generics::Time get_switch_date_time(
    const Generics::Time& time,
    const char* tzname);
  Generics::Time get_time_in_tz(const char* tzname);

  void spend_rest_budget(const std::string& prefix);

  template<CCGMember member, int coef>
  void increase_budget(const std::string& prefix);

  void prolong_lifetime(const std::string& prefix);
};

#endif //_AUTOTEST__PAUSINGCGREACHINGTEST_
