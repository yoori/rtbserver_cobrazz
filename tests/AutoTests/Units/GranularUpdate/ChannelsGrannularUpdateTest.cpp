#include "ChannelsGrannularUpdateTest.hpp"

REFLECT_UNIT(ChannelsGrannularUpdateTest) (
  "GranularUpdate",
  AUTO_TEST_SLOW
);

namespace
{
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::ChannelSearchChecker ChannelSearchChecker;
  typedef AutoTest::ExpressionChannelChecker ExpressionChannelChecker;
  typedef AutoTest::MatchChecker MatchChecker;
};

void
ChannelsGrannularUpdateTest::set_up()
{
  add_descr_phrase("Setup");
  
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      get_config().check_service(CTE_ALL, STE_CHANNEL_SEARCH_SERVER)),
    "ChannelSearchServer must set in the XML configuration file");
}

void
ChannelsGrannularUpdateTest::create_trigger(ORM::BehavioralChannel* channel,
                                            const char* kwd_name)
{

  std::string keyword(fetch_string(kwd_name));
  std::string n_keyword(tolower(keyword));

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      channel->select()),
    "Can't select channel");

  // Trigger
  ORM::PQ::Triggers trigger(pq_conn_);
  if(channel->params.trigger_type.value().compare("U") == 0)
  {
    trigger.trigger_type = "U";
  }
  else
  {
    trigger.trigger_type = "K";
  }
  trigger.normalized_trigger = n_keyword;
  trigger.qa_status = "A";
  trigger.channel_type =
    channel->type.value() == "D"? "D": "A";
  trigger.country_code =
   channel->country_code.is_null()? "GB":
     channel->country_code.value();
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      trigger.insert()),
    "Can't insert trigger");
  triggers.push_back(trigger);

  // ChannelTrigger
  ORM::PQ::Channeltrigger ch_trigger(pq_conn_);
  ch_trigger.channel_id = channel->id();
  ch_trigger.trigger_id = trigger.trigger_id();
  ch_trigger.trigger_type =
    channel->params.trigger_type.value();
  ch_trigger.channel_type =
    channel->type.value() == "D"? "D": "A";
  ch_trigger.original_trigger = keyword;
  ch_trigger.qa_status = "A";
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      ch_trigger.insert()),
    "Can't insert channel trigger");
  channeltriggers.push_back(ch_trigger);

  channel->triggers_version.set_now();
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      channel->update()),
    "Can't update channel");
}

void
ChannelsGrannularUpdateTest::page_channel()
{
  chp.account               = fetch_int("Account");
  chp.name                  = fetch_string("ChannelName") + "p";
  chp.language              = "gn";
  chp.params.minimum_visits = 1;
  chp.triggers_status       = "A";
  chp.distinct_url_triggers_count = 0;

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      chp.insert()),
    "Must insert page channel");
  
  create_trigger(&chp, "Keyword1");

  std::string page_id(strof(chp.id())); 
  
  ADD_WAIT_CHECKER(
    "Search admin check",
    ChannelSearchChecker(
      this,
      fetch_string("Keyword1"),
      AutoTest::ChannelSearch,
      ChannelSearchChecker::Expected().
        channel_id(page_id)));

  ADD_WAIT_CHECKER(
    "Match check",
    MatchChecker(
      this,
      NSLookupRequest().
        referer_kw(fetch_string("Keyword1")),
      chp.id_with_suffix()));
}


void
ChannelsGrannularUpdateTest::search_channel()
{
  // Create search channel
  chs.name                  = fetch_string("ChannelName") + "s";
  chs.account               = fetch_int("Account");
  chs.language              = "gn";
  chs.params.minimum_visits = 1;
  chs.triggers_status       = "A";
  chs.distinct_url_triggers_count = 0;
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      chs.insert()),
    "Must create search channel");

  create_trigger(&chs, "Keyword3");

  std::string search_id(strof(chs.id())); 

  ADD_WAIT_CHECKER(
    "Search admin check",
    ChannelSearchChecker(
      this,
      fetch_string("Keyword3"),
      AutoTest::ChannelSearch,
      ChannelSearchChecker::Expected().
        channel_id(search_id)));

  ADD_WAIT_CHECKER(
    "Match check",      
    MatchChecker(this,
      NSLookupRequest().
        search(fetch_string("Keyword3")),
      chs.id_with_suffix()));
}

void
ChannelsGrannularUpdateTest::expression_channel()
{
  // Create expression channel
  // Case depend on page_channel & search_channel
  che.name       = fetch_string("ChannelName") + "e";
  che.account    = fetch_int("Account");
  che.language.null();
  che.expression = strof(chp.id()) + "&" + strof(chs.id());
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      che.insert()),
    "Must create expression channel");

  std::string expr_id(strof(che.id())); 

  ADD_WAIT_CHECKER(
    "Search admin check",      
    ChannelSearchChecker(
      this,
      fetch_string("Keyword1"),
      AutoTest::ChannelSearch,
      AutoTest::ChannelSearchAdmin::Expected().
        channel_id(expr_id)));
  
  std::string regexp =
    "\\(\\[" + strof(chp.id()) + "\\] & "  +
    "\\[" + strof(chs.id()) + "\\]\\)";

  ADD_WAIT_CHECKER(
    "Expression admin check",   
    ExpressionChannelChecker(
      this,
      che.id(),
      ExpressionChannelChecker::Expected().
        channel_id(strof(che.id())).
        expression(regexp).
        status("A")));
  
}

void
ChannelsGrannularUpdateTest::delete_channel_expression()
{
  {
     std::string regexp =
       "\\(\\[" + fetch_string("Channel2") + "\\] \\| "  +
       "\\[" + fetch_string("Channel3") + "\\]\\)";
  
     FAIL_CONTEXT(
       AutoTest::wait_checker(
         ExpressionChannelChecker(
           this,
           fetch_int("Expr"),
           ExpressionChannelChecker::Expected().
             channel_id(fetch_string("Expr")).
             expression(regexp).
             status("A"))).check(),
       "Initial check");
  }

  ORM::ORMRestorer<ORM::PQ::Channel>* channel =
    create<ORM::PQ::Channel>(fetch_int("Channel3"));

  channel->status = "D";
  channel->display_status_id = 5; // 5 = Deleted
  
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      channel->update()),
    "Must update channel state");

  ADD_WAIT_CHECKER(
    "Search channel deleted",     
    ExpressionChannelChecker(
      this,
      fetch_int("Channel3"),
      ExpressionChannelChecker::Expected().
        channel_id(fetch_string("Channel3")),
      AutoTest::AEC_NOT_EXISTS));

 // Check that expression changed
 {
   std::string regexp =
     "\\(\\[" + fetch_string("Channel2") + "\\] \\| NULL\\)";

  ADD_WAIT_CHECKER(
    "Expression changed",     
    ExpressionChannelChecker(
      this,
      fetch_int("Expr"),
      ExpressionChannelChecker::Expected().
        channel_id(fetch_string("Expr")).
        expression(regexp).
        status("A")));
 }
}

void
ChannelsGrannularUpdateTest::channel_rate_change()
{
  FAIL_CONTEXT(
    AutoTest::wait_checker(
      ExpressionChannelChecker(
        this,
        fetch_int("Channel4"),
        ExpressionChannelChecker::Expected().
          channel_id(fetch_string("Channel4")).
          imp_revenue("0.01").
          status("A"))).check(),
    "Initial check");

  ORM::ORMRestorer<ORM::PQ::Channelrate>* rate =
    create<ORM::PQ::Channelrate>(fetch_int("Channel4Rate"));

  rate->cpm = 5;
  
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      rate->update()),
    "Must update channel rate");
  
  ADD_WAIT_CHECKER(
    "Rate changed",     
    ExpressionChannelChecker(
      this,
      fetch_int("Channel4"),
      ExpressionChannelChecker::Expected().
        channel_id(fetch_string("Channel4")).
        imp_revenue("0.005").
        status("A")));
}

bool
ChannelsGrannularUpdateTest::run()
{
  AUTOTEST_CASE(
    page_channel(),
    "Page channel");
  
  AUTOTEST_CASE(
    search_channel(),
    "Search channel");

  AUTOTEST_CASE(
    expression_channel(),
    "Expression channel");
  
  AUTOTEST_CASE(
    delete_channel_expression(),
    "Delete channel of expression");
  
  AUTOTEST_CASE(
    channel_rate_change(),
    "Channel rate change");

  return true;
}

void
ChannelsGrannularUpdateTest::tear_down()
{
  add_descr_phrase("Tear down");

  // To restore country changed channels status

  NOSTOP_FAIL_CONTEXT(che.del());
  NOSTOP_FAIL_CONTEXT(chs.del());
  NOSTOP_FAIL_CONTEXT(chp.del());
  for (std::list<ORM::PQ::Channeltrigger>::iterator
         it = channeltriggers.begin();
       it != channeltriggers.end(); ++it)
  {
    NOSTOP_FAIL_CONTEXT(it->delet());
  }
  for (std::list<ORM::PQ::Triggers>::iterator it = triggers.begin();
       it != triggers.end(); ++it)
  {
    NOSTOP_FAIL_CONTEXT(it->delet());
  }
}
