
#ifndef _UNITTEST__NOADVNOTRACKTEST_
#define _UNITTEST__NOADVNOTRACKTEST_

#include <tests/AutoTests/Commons/Common.hpp>

class NoAdvNoTrackTest: public BaseUnit
{
  typedef AutoTest::AdClient AdClient;

public:

  NoAdvNoTrackTest(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var);
 
  virtual ~NoAdvNoTrackTest() noexcept;
 
private:
  std::string no_track_words;
  std::string no_track_url_words;
  std::string no_track_urls;
  std::string no_adv_words;
  std::string no_adv_url_words;
  std::string no_adv_urls;
  std::string uid;

  virtual bool run_test();

  // Test cases
  void
  no_adv_case_(
    AdClient& client);

  void
  no_track_case_(
    AdClient& client);

  void
  no_adv_and_no_track_case_(
    AdClient& client);

  void
  session_info_case_();

  void
  passback_case_();
};

#endif  // _UNITTEST__NOADVNOTRACKTEST_

