#ifndef _AUTOTEST__MULTILANGCHANNELMATCHING_
#define _AUTOTEST__MULTILANGCHANNELMATCHING_
#include <tests/AutoTests/Commons/Common.hpp>

typedef std::list <std::string> ChannelList;

/**
 * @class MultiLangChannelMatching
 * @brief tests multilanguages in channel matching
 */
class MultiLangChannelMatching: public BaseUnit
{
public:

  enum KeywordCaseEnum
  {
    KCE_USE_FT = 1,
    KCE_USE_KN = 2,
  };

  struct KeywordChannelCase
  {
    std::string description;
    const char* referer_kw;
    const char* search_kw;
    const char* expected_channels;
    const char* unexpected_channels;
    const char* expected_search_phrase;
    unsigned long flags;
  };

  typedef AutoTest::AdClient AdClient;

 
  MultiLangChannelMatching(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var),
    test_client_(AutoTest::AdClient::create_user(this))
  {};
 
  virtual ~MultiLangChannelMatching() noexcept
  {};
 
private:

  AutoTest::AdClient test_client_; // AdServer client

  virtual bool run_test();// test cases

  template<size_t Count>
  void keyword_channels(
    AdClient& client,
    const KeywordChannelCase(&tests)[Count],
    unsigned long colo = 0);
  
  void url_channels();
};

#endif //_AUTOTEST__MULTILANGCHANNELMATCHING_
