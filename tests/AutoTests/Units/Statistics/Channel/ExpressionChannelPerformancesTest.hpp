#ifndef AUTOTESTS_EXPRESSIONCHANNELPERFORMANCE_HPP
#define AUTOTESTS_EXPRESSIONCHANNELPERFORMANCE_HPP

#include <tests/AutoTests/Commons/Common.hpp>

namespace ORM = ::AutoTest::ORM;
namespace DB = ::AutoTest::DBC;

class ExpressionChannelPerformancesTest: public BaseUnit
{
public:

  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef NSLookupRequest::NSLookupParam NSLookupParam;

  typedef AutoTest::AdClient AdClient;
  typedef std::pair<AdClient, std::string> RequestPair;
  typedef std::list<RequestPair> RequestList;
  
  struct TestDescription
  {
    const char* description;   // test description
    unsigned long iter_count;  // iteration count (number of repeatable subcases) 
    const char* colo;          // colocation parameter name
    const char* referer_kw;    // keyword parameter name 
    const char* referer;       // referer parameter name
    const char* tid;           // tag ID parameter name
    const char* format;        // format parameter value
    const char* ccids;         // expected ccid names
    int time_ofset;            // request debug time
    int check_url;             // check response URL flags
    bool store_urls;           // store_urls flag
  };
    
public: 
  ExpressionChannelPerformancesTest(
    UnitStat& stat_var, 
    const char* task_name, 
    XsdParams params_var) :
      BaseUnit(stat_var, task_name, params_var),
      conn(open_pq())
  {};
 
  virtual ~ExpressionChannelPerformancesTest() noexcept
  {};
 
private:
  virtual bool run_test();

  // utils
  void test_case(const TestDescription& test);

  template <class ParamType>
  void set_request_param(
    ParamType& request_param,
    const char* param_name);

  // Test parts
  void base_scenario();
  void latecomer_requests();

  // request storages
  RequestList impressions;
  RequestList clicks;
  RequestList actions;

  // Entitues
  std::string ch1;  // Channel#1
  std::string ch2;  // Channel#2 
  std::string ch3;  // Channel#3 
  std::string ch4;  // Channel#4 
  std::string ch5;  // Channel#5 

  std::string expr1;// Expression#1 channel
  std::string expr2;// Expression#2 channel
  std::string expr3;// Expression#3 channel

  unsigned long colo; // Colocation (colo_id != 1)
  unsigned long displaycc1; // dccid#1 (Display campaign#1 cc_id)
  unsigned long displaycc2; // dccid#2 (Display campaign#2 cc_id)
  unsigned long tacc1; // taccid#1 (TA campaign#1 cc_id) 
  unsigned long tacc2; // taccid#1 (TA campaign#2 cc_id)
  unsigned long displayccg1; // dccid#1 (Display campaign#1 cc_id)
  unsigned long displayccg2; // dccid#2 (Display campaign#2 cc_id)
  unsigned long taccg1; // taccid#1 (TA campaign#1 cc_id) 
  unsigned long taccg2; // taccid#1 (TA campaign#2 cc_id) 
  double displaycpa1; // Display campaign#1 cpa
  double displaycpa2; // Display campaign#2 cpa
  double tacpm1; // TA campaign#1 cpm
  double tacpm2; // TA campaign#2 cpm
  double rate;   // Exchange rate for display campaign#2 account

  AutoTest::Time currentDate;

  // Connection
  DB::Conn conn;
};

#endif
