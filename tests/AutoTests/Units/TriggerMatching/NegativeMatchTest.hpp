
#ifndef _UNITTEST__NEGATIVEMATCHTEST_
#define _UNITTEST__NEGATIVEMATCHTEST_

#include <tests/AutoTests/Commons/Common.hpp>

/**
 * @class NegativeMatchTest
 * @brief test negative match
 */

class NegativeMatchTest:  public BaseDBUnit
{
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::ORM::ChannelTriggerStats Stat;
  typedef AutoTest::ORM::StatsList<Stat> Stats;
  typedef Stat::Diffs Diff;
  typedef std::list<Diff> Diffs;

public:

  struct TestCase
  {
    const char* referer;
    const char* referer_kw;
    const char* matched;
    const char* unmatched;
    const char* ccid;
  };
  
public:
 
  NegativeMatchTest(UnitStat& stat_var, 
                    const char* task_name, 
                    XsdParams params_var)
    : BaseDBUnit(stat_var, task_name, params_var),
      conn_(open_pq())
  { }
 
  virtual ~NegativeMatchTest() noexcept
  { }
 
private:

  AutoTest::DBC::Conn conn_;

  Stats stats_;
  Diffs diffs_;

  void set_up();
  void pre_condition();
  bool run();
  void post_condition();
  void tear_down();

  template<size_t Count>
  void
  test_group(
    const std::string& description,
    const TestCase(&tests)[Count]);

  void
  test_case(
    const std::string& description,
    AdClient& client,
    const TestCase& test);

};

#endif // _UNITTEST__NEGATIVEMATCHTEST_

