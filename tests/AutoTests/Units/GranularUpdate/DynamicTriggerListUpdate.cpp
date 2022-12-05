
#include "DynamicTriggerListUpdate.hpp"

REFLECT_UNIT(DynamicTriggerListUpdate) (
  "GranularUpdate",
  AUTO_TEST_SLOW,
  AUTO_TEST_SERIALIZE
);

namespace
{
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::TriggerChecker TriggerChecker;
  typedef AutoTest::MatchChecker MatchChecker;
  typedef AutoTest::SelectedCreativeChecker SelectedCreativeChecker;
  typedef AutoTest::SpecialEffectsChecker SpecialEffectsChecker;
}

DynamicTriggerListUpdate::DynamicTriggerListUpdate(
  UnitStat& stat_var, 
  const char* task_name, 
  XsdParams params_var)
  : BaseDBUnit(stat_var, task_name, params_var)
{ }

DynamicTriggerListUpdate::~DynamicTriggerListUpdate() noexcept
{ }

bool 
DynamicTriggerListUpdate::run()
{
  AUTOTEST_CASE(
    change_trigger(),
    "Change trigger");
  
  AUTOTEST_CASE(
    no_adv(),
    "No Advertising");

  AUTOTEST_CASE(  
    no_track(),
    "No Track");
  
  return true;
}

void DynamicTriggerListUpdate::change_trigger()
{
  FAIL_CONTEXT(
    AutoTest::wait_checker(
      TriggerChecker(
        this,
        fetch_string("CHANGETRIGGER/Channel"),
        AutoTest::ChannelManagerController,
        TriggerChecker::Expected().
          page_word(
            AutoTest::ComparableStringList(
              AutoTest::parse_list(
                tolower(fetch_string("CHANGETRIGGER/KeywordOrig"))))))).check(),
    "Initial");

  FAIL_CONTEXT(
    MatchChecker(
      this,
      NSLookupRequest().
        referer_kw(
          fetch_string(
            "CHANGETRIGGER/KeywordOrig")),
      "CHANGETRIGGER/BP").check(),
    "Initial matching (expected)");

  FAIL_CONTEXT(
    MatchChecker(
      this,
      NSLookupRequest().
        referer_kw(
          fetch_string(
            "CHANGETRIGGER/KeywordChanged")),
      "CHANGETRIGGER/BP",
      AutoTest::SCE_NOT_ENTRY).check(),
    "Initial matching (unexpected)");


  ORM::ORMRestorer<ORM::TriggerChannel>* trigger1 =
    create<ORM::TriggerChannel>(
      fetch_int("CHANGETRIGGER/ChannelTrigger/1"));
  
  ORM::ORMRestorer<ORM::TriggerChannel>* trigger2 =
    create<ORM::TriggerChannel>(
      fetch_int("CHANGETRIGGER/ChannelTrigger/2"));

  trigger1->qa_status = "D";
  trigger2->qa_status = "A";
  
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      trigger1->update() && trigger2->update()),
    "Can't update trigger");

  ADD_WAIT_CHECKER(
    "Check changes loaded",
    TriggerChecker(
      this,
      fetch_string("CHANGETRIGGER/Channel"),
      AutoTest::ChannelManagerController,    
      TriggerChecker::Expected().
        page_word(
          AutoTest::ComparableStringList(
            AutoTest::parse_list(
              tolower(fetch_string("CHANGETRIGGER/KeywordChanged")))))));

  ADD_WAIT_CHECKER(
    "Check matching (unexpected)",
    MatchChecker(
      this,
      NSLookupRequest().
        referer_kw(
          fetch_string(
            "CHANGETRIGGER/KeywordOrig")),
      "CHANGETRIGGER/BP",
      AutoTest::SCE_NOT_ENTRY));


  ADD_WAIT_CHECKER(
    "Check matching (expected)",
    MatchChecker(
      this,
      NSLookupRequest().
        referer_kw(
          fetch_string(
            "CHANGETRIGGER/KeywordChanged")),
      "CHANGETRIGGER/BP"));
}

void DynamicTriggerListUpdate::no_adv()
{

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      TriggerChecker(
        this,
        fetch_string("NOADV/Channel"),
        AutoTest::ChannelManagerController,
        TriggerChecker::Expected().
          url(
            AutoTest::ComparableStringList(
              AutoTest::parse_list(fetch_string("NOADV/Urls")),
              false)).
          url_keyword(
            AutoTest::ComparableStringList(
              AutoTest::parse_list(fetch_string("NOADV/UrlWords")),
              false)).
          page_word(
            AutoTest::ComparableStringList(
              AutoTest::parse_list(fetch_string("NOADV/Words")),
              false)))).check(),
    "Initial");

  AdClient client(AdClient::create_user(this));

  FAIL_CONTEXT(
    SpecialEffectsChecker(
      client,
      NSLookupRequest().
        referer_kw(fetch_string("NOADV/KWD")).
        tid(fetch_string("Tag")),
      fetch_string("CC")).check(),
    "Initial ADV request");

  FAIL_CONTEXT(
    SpecialEffectsChecker(
      client,
      NSLookupRequest().
        search(fetch_string("NOADV/URLKWD")).
        tid(fetch_string("Tag")),
      fetch_string("CC")).check(),
    "Initial ADV request");

  FAIL_CONTEXT(
    SpecialEffectsChecker(
      client,
      NSLookupRequest().
        referer(fetch_string("NOADV/REF")).
        tid(fetch_string("Tag")),
      "0",
      SpecialEffectsChecker::SE_NO_ADV).check(),
    "Initial NO ADV request");

  {
    // Keyword trigger
    ORM::ORMRestorer<ORM::PQ::Triggers>* trigger =
      create<ORM::PQ::Triggers>();
    
    trigger->trigger_type = "K";
    trigger->normalized_trigger =
      tolower(fetch_string("NOADV/NewWords"));
    trigger->qa_status = "A";
    trigger->channel_type = "S";
    trigger->country_code = "";
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        trigger->insert()),
      "Can't insert NO ADV trigger");
    
    ORM::ORMRestorer<ORM::TriggerChannel>* channel_trigger_kwd =
      create<ORM::TriggerChannel>();

    channel_trigger_kwd->channel_type = "S";
    channel_trigger_kwd->trigger_type = "P";
    channel_trigger_kwd->channel_id =  fetch_int("NOADV/Channel");
    channel_trigger_kwd->trigger_id = trigger->trigger_id();
    channel_trigger_kwd->original_trigger = fetch_string("NOADV/NewWords");
    channel_trigger_kwd->qa_status = "A";

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        channel_trigger_kwd->insert()),
      "Can't insert NO ADV channel trigger");

  }

  {
    // URL keyword trigger
    ORM::ORMRestorer<ORM::PQ::Triggers>* trigger =
      create<ORM::PQ::Triggers>();
    
    trigger->trigger_type = "K";
    trigger->normalized_trigger =
      tolower(fetch_string("NOADV/NewUrlWords"));
    trigger->qa_status = "A";
    trigger->channel_type = "S";
    trigger->country_code = "";
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        trigger->insert()),
      "Can't insert NO ADV trigger");
    
    ORM::ORMRestorer<ORM::TriggerChannel>* channel_trigger_kwd =
      create<ORM::TriggerChannel>();

    channel_trigger_kwd->channel_type = "S";
    channel_trigger_kwd->trigger_type = "R";
    channel_trigger_kwd->channel_id =  fetch_int("NOADV/Channel");
    channel_trigger_kwd->trigger_id = trigger->trigger_id();
    channel_trigger_kwd->original_trigger = fetch_string("NOADV/NewUrlWords");
    channel_trigger_kwd->qa_status = "A";

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        channel_trigger_kwd->insert()),
      "Can't insert NO ADV channel trigger");
  }
  
  ORM::ORMRestorer<ORM::TriggerChannel>* channel_trigger_old =
    create<ORM::TriggerChannel>(
      fetch_int("NOADV/ChannelTriggerUrl"));
  
  ORM::ORMRestorer<ORM::TriggerChannel>* channel_trigger_new =
    create<ORM::TriggerChannel>(
      fetch_int("NOADV/ChannelTriggerUrlNew"));

  channel_trigger_old->log();
  channel_trigger_new->log();
  
  channel_trigger_old->qa_status = "D";
  channel_trigger_new->qa_status = "A";

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      channel_trigger_old->update() &&
        channel_trigger_new->update()),
    "Can't update NO ADV trigger");

  ADD_WAIT_CHECKER(
    "Check changes loaded",
    TriggerChecker(
        this,
        fetch_string("NOADV/Channel"),
        AutoTest::ChannelManagerController,    
        TriggerChecker::Expected().
          channel_id(
            fetch_string("NOADV/Channel")).
          url(
            AutoTest::ComparableStringList(
              AutoTest::parse_list(fetch_string("NOADV/ExpectedUrls")),
              false)).
          url_keyword(
            AutoTest::ComparableStringList(
              AutoTest::parse_list(fetch_string("NOADV/ExpectedUrlWords")),
              false)).
          page_word(
            AutoTest::ComparableStringList(
              AutoTest::parse_list(fetch_string("NOADV/ExpectedWords")),
              false))));

  ADD_WAIT_CHECKER(
    "Check new NO ADV",
    SpecialEffectsChecker(
      client,
      NSLookupRequest().
        search(fetch_string("NOADV/URLKWD")).
        tid(fetch_string("Tag")),
      "0",
      SpecialEffectsChecker::SE_NO_ADV));

  ADD_WAIT_CHECKER(
    "Check new NO ADV",
    SpecialEffectsChecker(
      client,
      NSLookupRequest().
        referer_kw(fetch_string("NOADV/KWD")).
        tid(fetch_string("Tag")),
      "0",
      SpecialEffectsChecker::SE_NO_ADV));

  ADD_WAIT_CHECKER(
    "Check new ADV",
    SpecialEffectsChecker(
      client,
      NSLookupRequest().
        referer(fetch_string("NOADV/REF")).
        tid(fetch_string("Tag")),
      fetch_string("CC")));
}

void DynamicTriggerListUpdate::no_track()
{

  AdClient client(AdClient::create_user(this));

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      TriggerChecker(
        this,
        fetch_string("NOTRACK/Channel"),
        AutoTest::ChannelManagerController,
        TriggerChecker::Expected().
          url(
            AutoTest::ComparableStringList(
              AutoTest::parse_list(fetch_string("NOTRACK/Urls")),
              false)).
          url_keyword(
            AutoTest::ComparableStringList(
              AutoTest::parse_list(fetch_string("NOTRACK/UrlWords")),
              false)).
          page_word(
            AutoTest::ComparableStringList(
              AutoTest::parse_list(fetch_string("NOTRACK/Words")),
              false)))).check(),
    "Initial");

  FAIL_CONTEXT(
    SpecialEffectsChecker(
      client,
      NSLookupRequest().
        referer_kw(fetch_string("NOTRACK/KWD2")).
        referer(fetch_string("NOTRACK/REF")).
        tid(fetch_string("Tag")),
      fetch_string("CC")).check(),
    "Initial TRACK request");

  FAIL_CONTEXT(
    SpecialEffectsChecker(
      client,
      NSLookupRequest().
        search(fetch_string("NOTRACK/SEARCH")).
        tid(fetch_string("Tag")),
      fetch_string("CC")).check(),
    "Initial TRACK request");

  FAIL_CONTEXT(
    SpecialEffectsChecker(
      client,
      NSLookupRequest().
        referer_kw(fetch_string("NOTRACK/KWD1")).
        tid(fetch_string("Tag")),
      "0",
      SpecialEffectsChecker::SE_NO_TRACK).check(),
    "Initial NO TRACK request");

  ORM::ORMRestorer<ORM::TriggerChannel>* channel_trigger_url =
    create<ORM::TriggerChannel>(
      fetch_int("NOTRACK/ChannelTriggerUrl"));

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      channel_trigger_url->del()),
    "Can't deactivate NO TRACK trigger");

  ORM::ORMRestorer<ORM::TriggerChannel>* channel_trigger_kwd_old =
    create<ORM::TriggerChannel>(
      fetch_int("NOTRACK/ChannelTriggerKwd"));

  ORM::ORMRestorer<ORM::TriggerChannel>* channel_trigger_kwd_new =
    create<ORM::TriggerChannel>(
      fetch_int("NOTRACK/ChannelTriggerKwdNew"));

  channel_trigger_kwd_old->qa_status = "D"; 
  channel_trigger_kwd_new->qa_status = "A"; 

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      channel_trigger_kwd_old->update() &&
      channel_trigger_kwd_new->update()),
    "Can't update NO TRACK trigger");

  {
    // URL keyword trigger
    ORM::ORMRestorer<ORM::PQ::Triggers>* trigger =
      create<ORM::PQ::Triggers>();
    
    trigger->trigger_type = "K";
    trigger->normalized_trigger =
      tolower(fetch_string("NOTRACK/NewUrlWords"));
    trigger->qa_status = "A";
    trigger->channel_type = "S";
    trigger->country_code = "";
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        trigger->insert()),
      "Can't insert NO ADV trigger");
    
    ORM::ORMRestorer<ORM::TriggerChannel>* channel_trigger_kwd =
      create<ORM::TriggerChannel>();
    
    channel_trigger_kwd->trigger_type = "R";
    channel_trigger_kwd->channel_type = "S";
    channel_trigger_kwd->channel_id =  fetch_int("NOTRACK/Channel");
    channel_trigger_kwd->trigger_id = trigger->trigger_id();
    channel_trigger_kwd->original_trigger = fetch_string("NOTRACK/NewUrlWords");
    channel_trigger_kwd->qa_status = "A";

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        channel_trigger_kwd->insert()),
      "Can't insert NO ADV channel trigger");
  }

  ADD_WAIT_CHECKER(
    "Check changes loaded",
    TriggerChecker(
      this,
      fetch_string("NOTRACK/Channel"),
      AutoTest::ChannelManagerController,    
      TriggerChecker::Expected().
        channel_id(
          fetch_string("NOTRACK/Channel")).
        url(
          AutoTest::ComparableStringList(
            AutoTest::parse_list(fetch_string("NOTRACK/ExpectedUrls")),
            false)).
        url_keyword(
            AutoTest::ComparableStringList(
              AutoTest::parse_list(fetch_string("NOTRACK/ExpectedUrlWords")),
              false)).
        page_word(
          AutoTest::ComparableStringList(
            AutoTest::parse_list(fetch_string("NOTRACK/ExpectedWords")),
            false))));

  ADD_WAIT_CHECKER(
    "Check new NO TRACK",
    SpecialEffectsChecker(
      client,
      NSLookupRequest().
        referer_kw(fetch_string("NOTRACK/KWD2")).
        referer(fetch_string("NOTRACK/REF")).
        tid(fetch_string("Tag")),
      "0",
      SpecialEffectsChecker::SE_NO_TRACK));

  ADD_WAIT_CHECKER(
    "Check new NO TRACK",
    SpecialEffectsChecker(
      client,
      NSLookupRequest().
        search(fetch_string("NOTRACK/SEARCH")).
        tid(fetch_string("Tag")),
      "0",
      SpecialEffectsChecker::SE_NO_TRACK));

  ADD_WAIT_CHECKER(
    "Check new TRACK",
    SpecialEffectsChecker(
      client,
      NSLookupRequest().
        referer_kw(fetch_string("NOTRACK/KWD1")).
        tid(fetch_string("Tag")),
      fetch_string("CC")));
}


void 
DynamicTriggerListUpdate::set_up   ()
{
  add_descr_phrase("Setup.");

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      get_config().
        check_service(CTE_ALL, STE_CHANNEL_CONTROLLER)),
    " ChannelManagerController need for this test.");
}

void 
DynamicTriggerListUpdate::tear_down ()
{
  add_descr_phrase("Tear down.");
}


