#include "BehavParamsGranularUpdateTest.hpp"

REFLECT_UNIT(BehavParamsGranularUpdateTest) (
  "GranularUpdate",
  AUTO_TEST_SLOW
);

namespace ORM = AutoTest::ORM;

bool
BehavParamsGranularUpdateTest::run()
{
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      get_config().check_service(CTE_ALL, STE_CAMPAIGN_SERVER)),
    "CampaignServer must set in the XML configuration file");
  
  AUTOTEST_CASE(
    empty(),
    "Empty behavioral parameters");
  
  AUTOTEST_CASE(
    assign(),
    "Assign behavioral parameters");
  
  AUTOTEST_CASE(
    change(),
    "Change behavioral parameters");

  AUTOTEST_CASE(
    remove(),
    "Remove a behavioral parameter");
  
//  The case requires specific channel_type
//  We possibly have to use MACRO channel ('L')
//  AUTOTEST_CASE(
//     add_list(),
//     "Add BP list");

  AUTOTEST_CASE(
    del_param(),
    "Clear behavioral parameters");
  
  return true;
}

void BehavParamsGranularUpdateTest::empty()
{
  const int channel_id = fetch_int("EMPTY/CHANNEL");
  ADD_WAIT_CHECKER(
    "Check behav_param_list",
    AutoTest::SimpleChannelChecker(
      this,
      channel_id,
      AutoTest::SimpleChannelChecker::Expected().
      channel_id(channel_id).
      behav_param_list_id("_")));
}

void BehavParamsGranularUpdateTest::assign()
{
  const int channel_id = fetch_int("ASSIGN/CHANNEL");
  ORM::ORMRestorer< ORM::PQ::BehavioralParameters>* bp_page =
    create<ORM::PQ::BehavioralParameters>();
  ORM::ORMRestorer< ORM::PQ::BehavioralParameters>* bp_search =
    create<ORM::PQ::BehavioralParameters>();

  FAIL_CONTEXT(
   AutoTest::wait_checker(
     AutoTest::SimpleChannelChecker(
       this,
       channel_id,
       AutoTest::SimpleChannelChecker::Expected().
       channel_id(channel_id).
       behav_param_list_id("_"))).check(),
   "Initial");

  bp_page->channel        = channel_id;
  bp_page->minimum_visits = 1;
  bp_page->time_from      = 0;
  bp_page->time_to        = 600;
  bp_page->trigger_type   = "P";
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      bp_page->insert()),
    "must insert BehavioralParameters of type 'P'");
  
  bp_search->channel        = channel_id;
  bp_search->minimum_visits = 1;
  bp_search->time_from      = 0;
  bp_search->time_to        = 1200;
  bp_search->trigger_type   = "S";
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      bp_search->insert()),
    "must insert BehavioralParameters of type 'S'");

  // it isn't good check, because using internal server key
  std::string bp_key = "_P0_600_1_1/S0_1200_1_1";
  ADD_WAIT_CHECKER(
    "Check behav_param_list",
    AutoTest::SimpleChannelChecker(
      this,
      channel_id,
      AutoTest::SimpleChannelChecker::Expected().
      channel_id(channel_id).
      behav_param_list_id(bp_key)));
}

void BehavParamsGranularUpdateTest::change()
{
  const int channel_id = fetch_int("CHANGE/CHANNEL");
  ORM::ORMRestorer< ORM::PQ::BehavioralParameters>* bp_page =
    create<ORM::PQ::BehavioralParameters>(fetch_int("CHANGE/PAGE"));
  ORM::ORMRestorer< ORM::PQ::BehavioralParameters>* bp_search =
    create<ORM::PQ::BehavioralParameters>(fetch_int("CHANGE/SEARCH"));

  // it isn't good check, because using internal server key
  std::string bp_key_initial = "_P0_600_1_1/S0_1200_1_1";
  std::string bp_key = "_P0_1200_1_1/S60_1200_2_1";

  FAIL_CONTEXT(
   AutoTest::wait_checker(
     AutoTest::SimpleChannelChecker(
       this,
       channel_id,
       AutoTest::SimpleChannelChecker::Expected().
       channel_id(channel_id).
       behav_param_list_id(bp_key_initial))).check(),
   "Initial");

  bp_page->time_to = 1200;
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      bp_page->update()),
    "should update 'bp_page' behav params");
  
  bp_search->time_from = 60;
  bp_search->minimum_visits = 2;
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      bp_search->update()),
    "should update 'bp_search' behav params");

  ADD_WAIT_CHECKER(
    "Check behav_param_list",
    AutoTest::SimpleChannelChecker(
      this,
      channel_id,
      AutoTest::SimpleChannelChecker::Expected().
      channel_id(channel_id).
      behav_param_list_id(bp_key)));
}

void BehavParamsGranularUpdateTest::remove()
{
  const int channel_id = fetch_int("REMOVE/CHANNEL");
  ORM::ORMRestorer< ORM::PQ::BehavioralParameters>* bp_search =
    create<ORM::PQ::BehavioralParameters>(fetch_int("REMOVE/SEARCH"));

  // it isn't good check, because using internal server key
  std::string bp_key_initial = "_P0_1200_1_1/S0_1200_1_1";
  std::string bp_key = "_P0_1200_1_1";

  FAIL_CONTEXT(
   AutoTest::wait_checker(
     AutoTest::SimpleChannelChecker(
       this,
       channel_id,
       AutoTest::SimpleChannelChecker::Expected().
       channel_id(channel_id).
       behav_param_list_id(bp_key_initial))).check(),
   "Initial");
    
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      bp_search->delet()),
    "must delete behav params");

  ADD_WAIT_CHECKER(
    "Check behav_param_list",
    AutoTest::SimpleChannelChecker(
      this,
      channel_id,
      AutoTest::SimpleChannelChecker::Expected().
      channel_id(channel_id).
      behav_param_list_id(bp_key)));
}

void BehavParamsGranularUpdateTest::add_list()
{
  const int channel_id = fetch_int("LIST/CHANNEL");
  ORM::ORMRestorer< ORM::PQ::Behavioralparameterslist>* bp_list =
    create<ORM::PQ::Behavioralparameterslist>();
  ORM::ORMRestorer< ORM::PQ::Channel>* channel =
    create<ORM::PQ::Channel>(channel_id);
  ORM::ORMRestorer< ORM::PQ::BehavioralParameters>* bp_page =
    create<ORM::PQ::BehavioralParameters>(fetch_int("LIST/PAGE"));

  // it isn't good check, because using internal server key
  std::string bp_key_initial = "_P0_1200_1_1"; 

  FAIL_CONTEXT(
   AutoTest::wait_checker(
     AutoTest::SimpleChannelChecker(
       this,
       channel_id,
       AutoTest::SimpleChannelChecker::Expected().
       channel_id(channel_id).
       behav_param_list_id(bp_key_initial))).check(),
   "Initial");
    
  bp_list->name = fetch_string("BPListName");
  bp_list->threshold = 0;
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      bp_list->insert()),
    "should insert behavioral parameters list");
  
  bp_page->channel.null();
  bp_page->behav_params_list_id = bp_list->id();
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      bp_page->update()),
    "should update 'bp_page' behav params");
  
  channel->behav_params_list_id = bp_list->id();
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      channel->update()),
    "should update channel");
  
  ADD_WAIT_CHECKER(
    "Check behav_param_list",
    AutoTest::SimpleChannelChecker(
      this,
      channel_id,
      AutoTest::SimpleChannelChecker::Expected().
      channel_id(channel_id).
      behav_param_list_id(strof(bp_list->id()))));
}

void BehavParamsGranularUpdateTest::del_param()
{
  const int channel_id = fetch_int("DEL/CHANNEL");
  ORM::ORMRestorer< ORM::PQ::Channel>* channel =
    create<ORM::PQ::Channel>(channel_id);
  ORM::ORMRestorer< ORM::PQ::BehavioralParameters>* bp_page =
    create<ORM::PQ::BehavioralParameters>(fetch_int("DEL/PAGE"));

  // it isn't good check, because using internal server key
  std::string bp_key_initial = "_P0_1200_1_1"; 

  FAIL_CONTEXT(
   AutoTest::wait_checker(
     AutoTest::SimpleChannelChecker(
       this,
       channel_id,
       AutoTest::SimpleChannelChecker::Expected().
       channel_id(channel_id).
       behav_param_list_id(bp_key_initial))).check(),
   "Initial");

  channel->behav_params_list_id.null();
  bp_page->delet();
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      channel->update()),
    "should update channel");
  
  ADD_WAIT_CHECKER(
    "Check behav_param_list",
    AutoTest::SimpleChannelChecker(
      this,
      channel_id,
      AutoTest::SimpleChannelChecker::Expected().
      channel_id(channel_id).
      behav_param_list_id("_")));
}
