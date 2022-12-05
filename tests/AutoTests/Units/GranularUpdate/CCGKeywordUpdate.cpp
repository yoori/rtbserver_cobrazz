
#include "CCGKeywordUpdate.hpp"

REFLECT_UNIT(CCGKeywordUpdate) (
  "GranularUpdate",
  AUTO_TEST_SLOW);

namespace
{
  typedef AutoTest::CCGKeywordChecker CCGKeywordChecker;
}

void
CCGKeywordUpdate::set_up()
{
  add_descr_phrase("Setup.");
}

void
CCGKeywordUpdate::tear_down()
{
  add_descr_phrase("Tear down.");
}
  

bool
CCGKeywordUpdate::run()
{
  AUTOTEST_CASE(
    add_ccgkeyword_(),
    "Add CCGKeyword");

  AUTOTEST_CASE(
    activate_ccgkeyword_(),
    "Activate CCGKeyword");

  AUTOTEST_CASE(
    deactivate_ccgkeyword_(),
    "Deactivate CCGKeyword");
  
  AUTOTEST_CASE(  
    deactivate_channel_(),
    "Deactivate keyword channel");
  
  AUTOTEST_CASE(  
    change_ccgkeyword_(),
    "Change CCGKeyword");

  return true;
}

void
CCGKeywordUpdate::add_ccgkeyword_()
{
  // Create channel
  ORM::ORMRestorer<ORM::BehavioralChannel>* channel =
    create<ORM::BehavioralChannel>();

  channel->type = "K";
  channel->visibility = "PRI";
  channel->trigger_type = "S";
  channel->account = fetch_int("ADD/Account");
  channel->name = fetch_string("ADD/CHANNEL_NAME");
  channel->qa_status = "A";
  channel->status = "A";
  channel->country_code = "GN";
  channel->channel_namespace = "K";
  channel->language = "gn";
  channel->params.trigger_type = "P";
  channel->params.minimum_visits = 2;
  channel->params.time_from = 0;
  channel->params.time_to = 3600; 

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      channel->insert()),
    "Create channel");

  // CCGKeyword
  ORM::ORMRestorer<ORM::PQ::CCGKeyword>* ccgkeyword =
    create<ORM::PQ::CCGKeyword>();
  ccgkeyword->ccg = fetch_int("ADD/CCG");;
  ccgkeyword->original_keyword = fetch_string("ADD/KWD");
  ccgkeyword->status = "A";
  ccgkeyword->channel_id = channel->id();
  ccgkeyword->max_cpc_bid = 10.0;
  ccgkeyword->click_url = "http://test.com";
  ccgkeyword->trigger_type = "S";

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      ccgkeyword->insert()),
    "Create CCGKeyword");

  ADD_WAIT_CHECKER(
    "Check changes",
    CCGKeywordChecker(
      this,
      ccgkeyword->id(),
      AutoTest::ChannelManagerController,
      CCGKeywordChecker::Expected().
        ccg_keyword_id(strof(ccgkeyword->id())).
        ccg_id(fetch_string("ADD/CCG")).
        channel_id(strof(channel->id()))));
}

void
CCGKeywordUpdate::activate_ccgkeyword_()
{
  unsigned long ccgkeyword_id =
    fetch_int("ACTIVATE/CCGKeyword");

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      CCGKeywordChecker(
        this,
        ccgkeyword_id,
        AutoTest::ChannelManagerController,
        CCGKeywordChecker::Expected(),
        AutoTest::AEC_NOT_EXISTS)).check(),
    "Initial check");

  ORM::ORMRestorer<ORM::PQ::CCGKeyword>* ccgkeyword =
    create<ORM::PQ::CCGKeyword>(ccgkeyword_id);

  ccgkeyword->status = "A";

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      ccgkeyword->update()),
    "Update CCGKeyword");

  ADD_WAIT_CHECKER(
    "Chack changes",
    CCGKeywordChecker(
      this,
      ccgkeyword_id,
      AutoTest::ChannelManagerController,
      CCGKeywordChecker::Expected().
        ccg_id(fetch_string("ACTIVATE/CCG"))));
}

void
CCGKeywordUpdate::deactivate_ccgkeyword_()
{
  unsigned long ccgkeyword_id =
    fetch_int("DEACTIVATE/CCGKeyword");

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      CCGKeywordChecker(
        this,
        ccgkeyword_id,
        AutoTest::ChannelManagerController,
        CCGKeywordChecker::Expected().
          ccg_id(fetch_string("DEACTIVATE/CCG")))).check(),
    "Initial check");

  ORM::ORMRestorer<ORM::PQ::CCGKeyword>* ccgkeyword =
    create<ORM::PQ::CCGKeyword>(ccgkeyword_id);

  ccgkeyword->status = "D";

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      ccgkeyword->update()),
    "Update CCGKeyword");

  ADD_WAIT_CHECKER(
    "Check changes",
    CCGKeywordChecker(
      this,
      ccgkeyword_id,
      AutoTest::ChannelManagerController,
      CCGKeywordChecker::Expected(),
      AutoTest::AEC_NOT_EXISTS));
}

void
CCGKeywordUpdate::deactivate_channel_()
{
  unsigned long ccgkeyword_id =
    fetch_int("DEACTIVATECHANNEL/CCGKeyword");

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      CCGKeywordChecker(
        this,
        ccgkeyword_id,
        AutoTest::ChannelManagerController,
        CCGKeywordChecker::Expected().
          ccg_id(fetch_string("DEACTIVATECHANNEL/CCG")))).check(),
    "Initial check");

  ORM::ORMRestorer<ORM::PQ::Channel>* channel =
    create<ORM::PQ::Channel>(
      fetch_int("DEACTIVATECHANNEL/Channel"));

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      channel->set_display_status(
        ORM::DS_NOT_LIVE_BY_OIX)),
    "Deactivate channel");

  ADD_WAIT_CHECKER(
    "Check changes",
    CCGKeywordChecker(
      this,
      ccgkeyword_id,
      AutoTest::ChannelManagerController,
      CCGKeywordChecker::Expected(),
      AutoTest::AEC_NOT_EXISTS));
}

void
CCGKeywordUpdate::change_ccgkeyword_()
{
  unsigned long ccgkeyword_id =
    fetch_int("CHANGE/CCGKeyword");
  
  FAIL_CONTEXT(
    AutoTest::wait_checker(
      CCGKeywordChecker(
        this,
        ccgkeyword_id,
        AutoTest::ChannelManagerController,
        CCGKeywordChecker::Expected().
          max_cpc(fetch_float("CHANGE/MAXCPC")).
          click_url(fetch_string("CHANGE/CLICK")).
          original_keyword(fetch_string("CHANGE/KWD")))).check(),
    "Initial check");

  ORM::ORMRestorer<ORM::PQ::CCGKeyword>* ccgkeyword =
    create<ORM::PQ::CCGKeyword>(ccgkeyword_id);

  ccgkeyword->max_cpc_bid = fetch_float("CHANGE/NEW_MAXCPC");
  ccgkeyword->click_url = fetch_string("CHANGE/NEW_CLICK");
  ccgkeyword->original_keyword = fetch_string("CHANGE/NEW_KWD");

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      ccgkeyword->update()),
    "Update CCGKeyword");

  ADD_WAIT_CHECKER(
    "Check changes",
    CCGKeywordChecker(
      this,
      ccgkeyword_id,
      AutoTest::ChannelManagerController,
      CCGKeywordChecker::Expected().
        max_cpc(fetch_float("CHANGE/NEW_MAXCPC")).
        click_url(fetch_string("CHANGE/NEW_CLICK")).
        original_keyword(fetch_string("CHANGE/NEW_KWD"))));

}
