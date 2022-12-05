#include "ChannelSearchAdminTest.hpp"

REFLECT_UNIT(ChannelSearchAdminTest) (
  "TriggerMatching",
  AUTO_TEST_FAST
);

namespace
{
  typedef AutoTest::ChannelSearchAdmin SearchAdmin;
  typedef AutoTest::ChannelSearchChecker ChannelSearchChecker;
  typedef AutoTest::ChannelMatchLog MatchAdmin;

  const ChannelSearchAdminTest::TestCase TEST_CASES[] =
  {
    {
      "ChannelSearchAdminTestTest3 "
      "ChannelSearchAdminTestTest4 "
      "ChannelSearchAdminTestTest5",
      "CHSMT-01-2,CHSMT-01-1"
    },
    {
      "ChannelSearchAdminTestTest3 "
      "ChannelSearchAdminTestTest5 "
      "ChannelSearchAdminTestTest4",
      "CHSMT-01-2,CHSMT-01-1"
    },
    {
      "ChannelSearchAdminTestTest5 "
      "ChannelSearchAdminTestTest1 "
      "ChannelSearchAdminTestTest2",
      "HTMT-01-2,HTMT-01-1,STMT-01-2,STMT-01-1"
    },
    {
      "ChannelSearchAdminTestTest2 "
      "ChannelSearchAdminTestTest1",
      "HTMT-01-2,HTMT-01-1,STMT-01-2,STMT-01-1"
    },
    {
      "ChannelSearchAdminTestTest12",
      "MCMT-03-2,MCMT-03-1"
    },
    {
      "ChannelSearchAdminTestTest13",
      "MCMT-05-2,MCMT-05-1"
    },
    {
      "ChannelSearchAdminTestTest14",
      "MCMT-06-2,MCMT-06-1,MCMT-07-2,MCMT-07-1"
    },
    {
      "http://dev.ocslab.com/services/nslookup?site-id=100&app=PS",
      "RMT-01-2,RMT-01-1"
    },
    {
      "http://dev.ocslab.com/services/nslookup",
      "RMT-01-2,RMT-01-1"
    },
    {
      "http://dev.ocslab.com/services/",
      "RMT-01-2,RMT-01-1"
    },
    {
      "ChannelSearchAdminTestTest15 "
      "ChannelSearchAdminTestTest16 "
      "ChannelSearchAdminTestTest17 "
      "ChannelSearchAdminTestTest18 "
      "ChannelSearchAdminTestTest17 "
      "ChannelSearchAdminTestTest19 "
      "ChannelSearchAdminTestTest20 "
      "ChannelSearchAdminTestTest16 "
      "ChannelSearchAdminTestTest21",
      "STMT-02-2,STMT-02-1,STMT-03-2,STMT-03-1"
    }
  };
  
}

bool 
ChannelSearchAdminTest::run_test()
{

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      get_config().check_service(CTE_ALL, STE_CHANNEL_SEARCH_SERVER)),
    "ChannelSearchServ must set in the XML configuration file");
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      get_config().check_service(CTE_ALL, STE_CHANNEL_SERVER)),
    "ChannelServer must set in the XML configuration file");

  for (unsigned int i=0; i < countof(TEST_CASES); ++i)
  {
    test_case(i, TEST_CASES[i]);
  }
      
 
  return true;
}


void ChannelSearchAdminTest::test_case(
    unsigned int index,
    const TestCase& test)
{
  add_descr_phrase("Scenario#" + strof(index+1));
  std::string ChannelSearchServer =
    get_config().get_service(CTE_ALL, STE_CHANNEL_SEARCH_SERVER).address;
  std::string ChannelServer =
    get_config().get_service(CTE_ALL, STE_CHANNEL_SERVER).address;
  
  SearchAdmin admin(ChannelSearchServer, test.phrase);

  MatchAdmin(ChannelServer.c_str(),
    std::string("\"") + test.phrase + "\"",
    AutoTest::ChannelServer).log(
      AutoTest::Logger::thlog(),
      Logging::Logger::TRACE);
 
  std::string names_(test.expected_channels);
  String::StringManip::SplitComma tokenizer(names_);
  String::SubString token;
  while (tokenizer.get_token(token))
  {
    String::StringManip::trim(token);

    FAIL_CONTEXT(
      ChannelSearchChecker(
        this,
        test.phrase,
        AutoTest::ChannelSearch,
        ChannelSearchChecker::Expected().
          channel_id(fetch_string(token.str()))).check(),
      "search check");
  }
  

}
