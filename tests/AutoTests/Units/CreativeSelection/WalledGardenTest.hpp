#ifndef _UNITTEST__WALLEDGARDENTEST_
#define _UNITTEST__WALLEDGARDENTEST_
 
#include <tests/AutoTests/Commons/Common.hpp> 

namespace ORM = AutoTest::ORM;
 
class WalledGardenTest:
  public BaseDBUnit
{

public:

  enum CCShownEnum
  {
    CSE_WALLED_GARDEN = 1,     // Walled garden campaign creative
    CSE_OIX = 2,               // 'OIX' campaign creative
    CSE_NON_WALLED_GARDEN = 4, // Non walled garden campaign creative
    CSE_ALL = 8               // 'ALL' campaign creative
  };

  enum CCGType
  {
    CT_CPM, // CPM CCG
    CT_CPC  // CPC CCG
  };
  
  struct TestCase
  {
    const char* case_name;
    CCShownEnum tagtype;
    unsigned short flags;
    bool skip_priority_check;
    const char* description;
  };

  struct CCSelection
  {
    const char* kwd;
    const char* ccid;
    const char* revenue;
    CCShownEnum cctype;
    CCGType ccgtype;
  };
  
public: 
  WalledGardenTest(
    UnitStat& stat_var, 
    const char* task_name, 
    XsdParams params_var): 
    BaseDBUnit(stat_var, task_name, params_var)
  {};

  virtual ~WalledGardenTest() noexcept
  {};
 
private:
  typedef ORM::StatsArray<ORM::AdvertiserStatsDaily, 9> AdvStats;
  typedef ORM::StatsArray<ORM::PublisherStatsDaily, 10> PubStats;
  typedef ORM::AdvertiserStatsDaily::Diffs AdvDiff;
  typedef ORM::PublisherStatsDaily::Diffs PubDiff;

  virtual void pre_condition();
  virtual bool run();
  virtual void post_condition();
  virtual void tear_down();

  bool non_wg_shown(
    const TestCase& testcase,
    const CCSelection& creative,
    const CCSelection& non_wg_creative);
 
  void test_case(
    const TestCase& testcase);

  void test_priority(
    const TestCase& testcase,
    const CCSelection& non_wg_creative);

  template <size_t COUNT>
    void wg_foros_competition_case (
      const std::string& referer_kw, 
      std::string (&ccids)[COUNT]);

private:
  AdvStats advstats;
  PubStats pubstats;
  AutoTest::Time today_;
};

#endif
