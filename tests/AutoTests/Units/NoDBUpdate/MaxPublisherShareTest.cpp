
#include "MaxPublisherShareTest.hpp"
 
REFLECT_UNIT(MaxPublisherShareTest) (
  "NoDBUpdate",
  AUTO_TEST_SLOW
);

namespace
{
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::CampaignChecker CampaignChecker;
  typedef AutoTest::SelectedCreativeChecker SelectedCreativeChecker;
  typedef AutoTest::SelectedCreativesChecker SelectedCreativesChecker;

  const char ASPECT[] = "MaxPublisherShareTest";
  const unsigned long INFO = Logging::Logger::INFO;

  const char EXPIRE_CASE[] = "Publisher advertiser expire after request";
  const char TEXT_CASE[] = "MPS for Text campaign";
  const char THREE_SITES_CASE[] = "MPS For Three Site and Delivery Pacing: Daily Budget Cap";
  const char TEXT_DAILY_CASE[] = "Text Ad Group with non installed budget";
}

void
MaxPublisherShareTest::set_up()
{
  add_descr_phrase("Setup.");
}

void
MaxPublisherShareTest::tear_down()
{
  add_descr_phrase("Tear down.");
}

void
MaxPublisherShareTest::check() /*throw(eh::Exception)*/
{
  add_descr_phrase("Changes check#" + strof(++check_count_));
  BaseDBUnit::check();
}

bool
MaxPublisherShareTest::checker_call(
  const std::string& description,
  AutoTest::Checker* checker) /*throw(eh::Exception)*/
{
  try
  {
    return checker->check();
  }
  catch (
    const AutoTest::TimeLessChecker::TimeLessCheckFailed&)
  {
    AutoTest::Logger::thlog().stream(INFO, ASPECT) <<
      "Day switched: << '" << description << "' skipped.";
    return false;
  }
  catch(const eh::Exception& e)
  {
    Stream::Error ostr;
    ostr << description  << ":" << std::endl << "  " << e.what();
    throw AutoTest::CheckFailed(ostr);
  }                                                                 
}

bool 
MaxPublisherShareTest::run()
{

  AutoTest::Time new_day(
    (AutoTest::Time() + Generics::Time::ONE_DAY).get_gm_time().get_date());
  
  // Don't change case order
  AUTOTEST_CASE(share_expiring_(), EXPIRE_CASE);
  AUTOTEST_CASE(text_static_(), TEXT_CASE);
  AUTOTEST_CASE(three_sites_part_1_(new_day), THREE_SITES_CASE);
  AUTOTEST_CASE(text_daily_part_1_(new_day), TEXT_DAILY_CASE);

  FAIL_CONTEXT(check());
  
  if (stat_.db_active())
  {
    AUTOTEST_CASE(increase_share_(), EXPIRE_CASE);
    AUTOTEST_CASE(text_dynamic_(), TEXT_CASE);
  }

  AUTOTEST_CASE(three_sites_part_2_(new_day), THREE_SITES_CASE);
  AUTOTEST_CASE(text_daily_part_2_(new_day), TEXT_DAILY_CASE);

  FAIL_CONTEXT(check());

  if (stat_.db_active())
  {
    AUTOTEST_CASE(increase_budget_(), EXPIRE_CASE);
    AUTOTEST_CASE(text_daily_part_3_(new_day), TEXT_DAILY_CASE);
  }
  
  FAIL_CONTEXT(check());
 
  return true;
}

void
MaxPublisherShareTest::share_expiring_()
{
  std::string description("Publisher advertiser expire after request.");
  add_descr_phrase("Start case");

  AdClient client = AdClient::create_user(this);
  
  NSLookupRequest request;
  request.referer_kw(fetch_string("EXPIRE/KEYWORD"));
  request.tid(fetch_string("EXPIRE/TID/1"));

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      CampaignChecker(
        this,
        fetch_int("EXPIRE/CCG"),
        CampaignChecker::Expected().
          status("A").
          eval_status("A").
          exclude_pub_accounts(""))).check(),
      description +
        " No excluded publishers");

  FAIL_CONTEXT(
    SelectedCreativeChecker(
      client, request, fetch_int("EXPIRE/CC")).check(),
    description + " Check ccid#1");

  add_wait_checker(
    description + " Excluded publishers",
    CampaignChecker(
      this,
      fetch_int("EXPIRE/CCG"),
      CampaignChecker::Expected().
        status("A").
        eval_status("A").
        exclude_pub_accounts(
          fetch_string("EXPIRE/PUBLISHER/1"))));

 add_checker(
   description + " Check ccid#2",
   SelectedCreativeChecker(
     client, request, 0));  
}

void
MaxPublisherShareTest::increase_share_()
{
  std::string description("Changing Maximum Publisher Share.");
  add_descr_phrase("Increase max publisher share for campaign");

  AdClient client = AdClient::create_user(this);

  NSLookupRequest request;
  request.referer_kw(fetch_string("EXPIRE/KEYWORD"));
  request.tid(fetch_string("EXPIRE/TID/1"));

  ORM::ORMRestorer<ORM::PQ::Campaign>* campaign =
    create<ORM::PQ::Campaign>(fetch_int("EXPIRE/CAMPAIGN"));
  campaign->max_pub_share = 0.2;
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      campaign->update()),
    description + " Update maximum publisher share");
  
  FAIL_CONTEXT(
    AutoTest::wait_checker(
      CampaignChecker(
        this,
        fetch_int("EXPIRE/CCG"),
        CampaignChecker::Expected().
          status("A").
          eval_status("A").
          exclude_pub_accounts(""))).check(),
      description +
        " No excluded publishers");

  FAIL_CONTEXT(
    SelectedCreativeChecker(
      client, request, fetch_int("EXPIRE/CC")).check(),
    description + " Check ccid#1");
    
  add_wait_checker(
    description + " Excluded publishers",
    CampaignChecker(
      this,
      fetch_int("EXPIRE/CCG"),
      CampaignChecker::Expected().
        status("A").
        eval_status("A").
        exclude_pub_accounts(
          fetch_string("EXPIRE/PUBLISHER/1"))));

 add_checker(
   description + " Check ccid#2",
   SelectedCreativeChecker(
     client, request, 0));  
}

void
MaxPublisherShareTest::increase_budget_()
{
  std::string description("Changing Budget Cap.");
  add_descr_phrase("Increase budget of campaign");

  AdClient client = AdClient::create_user(this);
  NSLookupRequest request;
  request.referer_kw(fetch_string("EXPIRE/KEYWORD"));
  request.tid(fetch_string("EXPIRE/TID/1"));
  
  ORM::ORMRestorer<ORM::PQ::CampaignCreativeGroup>* ccg =
    create<ORM::PQ::CampaignCreativeGroup>(
      fetch_int("EXPIRE/CCG"));
  ccg->budget = 18;
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      ccg->update()),
    description + " Update budget");

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      CampaignChecker(
        this,
        fetch_int("EXPIRE/CCG"),
        CampaignChecker::Expected().
          status("A").
          eval_status("A").
          exclude_pub_accounts(""))).check(),
      description +
        " No excluded publishers");

  client.process_request(request);

  FAIL_CONTEXT(
    SelectedCreativeChecker(
      client, request, fetch_int("EXPIRE/CC")).check(),
    description + " Check ccid#1");
    
  add_wait_checker(
    description + " Excluded publishers",
    CampaignChecker(
      this,
      fetch_int("EXPIRE/CCG"),
      CampaignChecker::Expected().
        status("A").
        eval_status("A").
        exclude_pub_accounts(
          fetch_string("EXPIRE/PUBLISHER/1"))));

 add_checker(
   description + " Check ccid#2",
   SelectedCreativeChecker(
     client, request, 0));  
}

void
MaxPublisherShareTest::three_sites_part_1_(
  const AutoTest::Time& new_day)
{
  std::string description(
    "MPS For Three Site and Delivery "
    "Pacing: Daily Budget Cap, part#1.");
  add_descr_phrase("Start part#1 of the case");

  AdClient client = AdClient::create_user(this);

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      CampaignChecker(
        this,
        fetch_int("3SITES/CCG"),
        CampaignChecker::Expected().
          status("A").
          eval_status("A").
          exclude_pub_accounts(""))).check(),
      description +
        " No excluded publishers");

  FAIL_CONTEXT(
    SelectedCreativeChecker(
      client,
      NSLookupRequest().
        tid(fetch_string("3SITES/TID/1")).
        referer_kw(fetch_string("3SITES/KEYWORD")),
      fetch_int("3SITES/CC")).check(),
    description + " Check ccid");

  
  add_wait_checker(
    description + " Excluded first publisher",
    AutoTest::and_checker(
      AutoTest::throw_checker(
        AutoTest::TimeLessChecker(new_day)),
      CampaignChecker(
        this,
        fetch_int("3SITES/CCG"),
        CampaignChecker::Expected().
          status("A").
          eval_status("A").
          exclude_pub_accounts(
            fetch_string("3SITES/PUBLISHER/1")))));
 
}

void
MaxPublisherShareTest::three_sites_part_2_(
  const AutoTest::Time& new_day)
{
  std::string description(
    "MPS For Three Site and Delivery "
    "Pacing: Daily Budget Cap, part#2.");
  
  add_descr_phrase("Start part#2 of the case");

  AdClient client = AdClient::create_user(this);

  FAIL_CONTEXT(
    SelectedCreativeChecker(
      client,
      NSLookupRequest().
        tid(fetch_string("3SITES/TID/2")).
        referer_kw(fetch_string("3SITES/KEYWORD")),
      fetch_int("3SITES/CC")).check(),
    description + " Check ccid");

  add_wait_checker(
    description + " Excluded second publisher",
    AutoTest::and_checker(
      AutoTest::throw_checker(
        AutoTest::TimeLessChecker(new_day)),
      CampaignChecker(
        this,
        fetch_int("3SITES/CCG"),
        CampaignChecker::Expected().
          status("A").
          eval_status("A").
          exclude_pub_accounts(
            map_objects("3SITES/PUBLISHER/1,3SITES/PUBLISHER/2")))));
  
  add_checker(
   description + " Check ccid (site#1)",
   AutoTest::and_checker(
     AutoTest::throw_checker(
       AutoTest::TimeLessChecker(new_day)),
     SelectedCreativeChecker(
       client,
       NSLookupRequest().
         tid(fetch_string("3SITES/TID/1")).
         referer_kw(fetch_string("3SITES/KEYWORD")), 0)));

  add_checker(
   description + " Check ccid (site#2)",
   AutoTest::and_checker(
     AutoTest::throw_checker(
       AutoTest::TimeLessChecker(new_day)),
     SelectedCreativeChecker(
       client,
       NSLookupRequest().
         tid(fetch_string("3SITES/TID/2")).
         referer_kw(fetch_string("3SITES/KEYWORD")), 0)));

  add_checker(
   description + " Check ccid (site#3)",
   AutoTest::and_checker(
     AutoTest::throw_checker(
       AutoTest::TimeLessChecker(new_day)),
     SelectedCreativeChecker(
       client,
       NSLookupRequest().
         tid(fetch_string("3SITES/TID/3")).
         referer_kw(fetch_string("3SITES/KEYWORD")), 0)));  
  
}

void
MaxPublisherShareTest::text_static_()
{
  std::string description("MPS for Text campaign (static).");
  add_descr_phrase("Start case");

  AdClient client = AdClient::create_user(this);

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      CampaignChecker(
        this,
        fetch_int("TEXT/CCG/1"),
        CampaignChecker::Expected().
          status("A").
          eval_status("A").
          exclude_pub_accounts(""))).check(),
      description +
        " Initial (text#1)");

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      CampaignChecker(
        this,
        fetch_int("TEXT/CCG/2"),
        CampaignChecker::Expected().
          status("A").
          eval_status("A").
          exclude_pub_accounts(""))).check(),
      description +
        " Initial (text#2)");

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      CampaignChecker(
        this,
        fetch_int("TEXT/CCG/3"),
        CampaignChecker::Expected().
          status("A").
          eval_status("A").
          exclude_pub_accounts(""))).check(),
      description +
        " Initial (text#3)");

  std::list<std::string> expected_ccids;
  fetch_objects(
    std::inserter(
      expected_ccids,
      expected_ccids.begin()),
    "TEXT/CC/1,TEXT/CC/2,TEXT/CC/3");

  AutoTest::ConsequenceActionList actions;
  actions.push_back(AutoTest::CLICK);

  FAIL_CONTEXT(
    client.do_ad_requests(
      NSLookupRequest().
        tid(fetch_string("TEXT/TID/1")).
        referer_kw(map_objects("TEXT/KEYWORD/1,TEXT/KEYWORD/2,TEXT/KEYWORD/3")),
      expected_ccids,
      actions),
    description + " Ad request");
    
 
  add_wait_checker(
    description + " Excluded publisher (text#3)",
    CampaignChecker(
      this,
      fetch_int("TEXT/CCG/3"),
      CampaignChecker::Expected().
        status("A").
        eval_status("A").
         exclude_pub_accounts(
           fetch_string("TEXT/PUBLISHER/1"))));

  add_wait_checker(
    description + " Excluded publisher (text#2)",
    CampaignChecker(
      this,
      fetch_int("TEXT/CCG/2"),
      CampaignChecker::Expected().
        status("A").
        eval_status("A").
         exclude_pub_accounts(
           fetch_string("TEXT/PUBLISHER/1"))));

  add_wait_checker(
    description + " No excluded publisher (text#1)",
    CampaignChecker(
      this,
      fetch_int("TEXT/CCG/1"),
      CampaignChecker::Expected().
        status("A").
        eval_status("A").
         exclude_pub_accounts("")));
}

void
MaxPublisherShareTest::text_dynamic_()
{
  std::string description("MPS for Text campaign (dynamic).");
  add_descr_phrase("Start dynamic part of the case");

  AdClient client = AdClient::create_user(this);

  ORM::ORMRestorer<ORM::PQ::CampaignCreativeGroup>* ccg =
    create<ORM::PQ::CampaignCreativeGroup>(
      fetch_int("TEXT/CCG/1"));
  ccg->budget = 0.1;
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      ccg->update()),
    description + " Update budget");

  ORM::ORMRestorer<ORM::PQ::Campaign>* campaign =
    create<ORM::PQ::Campaign>(
      fetch_int("TEXT/CAMPAIGN/1"));
  campaign->max_pub_share = 0.4;
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      campaign->update()),
    description + " Update maximum publisher share");

  add_wait_checker(
    description + " Excluded publisher (text#1)",
    CampaignChecker(
      this,
      fetch_int("TEXT/CCG/1"),
      CampaignChecker::Expected().
        status("A").
        eval_status("A").
         exclude_pub_accounts(
           fetch_string("TEXT/PUBLISHER/1"))));

  add_wait_checker(
    description + " No Excluded publisher (text#2)",
    CampaignChecker(
      this,
      fetch_int("TEXT/CCG/2"),
      CampaignChecker::Expected().
        status("A").
        eval_status("A").
         exclude_pub_accounts("")));

  add_checker(
   description + " Check ccid",
   SelectedCreativesChecker(
     this,
     client,
     NSLookupRequest().
       tid(fetch_string("TEXT/TID/1")).
       referer_kw(
         map_objects("TEXT/KEYWORD/1,TEXT/KEYWORD/2,TEXT/KEYWORD/3")),
     "TEXT/CC/2"));  
}

void
MaxPublisherShareTest::text_daily_part_1_(
  const AutoTest::Time& new_day)
{
  std::string description(
    "Text Ad Group with non "
    "installed budget (=0), part#1.");
  add_descr_phrase("Start part#1 of the case");

  AdClient client = AdClient::create_user(this);

  FAIL_CONTEXT(
    SelectedCreativesChecker(
      this,
      client,
      NSLookupRequest().
        tid(fetch_string("TEXTDAILY/TID/1")).
        referer_kw(fetch_string("TEXTDAILY/KEYWORD/1")),
      "TEXTDAILY/CC/1").check(),
    description + " Check ccid");

  add_wait_checker(
    description + " Excluded publisher",
    AutoTest::and_checker(
      AutoTest::throw_checker(
        AutoTest::TimeLessChecker(new_day)),
      CampaignChecker(
        this,
        fetch_int("TEXTDAILY/CCG/1"),
        CampaignChecker::Expected().
          status("A").
          eval_status("A").
          exclude_pub_accounts(
            fetch_string("TEXTDAILY/PUBLISHER/1")))));
  
}

void
MaxPublisherShareTest::text_daily_part_2_(
  const AutoTest::Time& new_day)
{
  std::string description(
    "Text Ad Group with non "
    "installed budget (=0), part#2.");
  add_descr_phrase("Start part#2 of the case");

  AdClient client = AdClient::create_user(this);

  FAIL_CONTEXT(
    SelectedCreativesChecker(
      this,
      client,
      NSLookupRequest().
        tid(fetch_string("TEXTDAILY/TID/2")).
        referer_kw(fetch_string("TEXTDAILY/KEYWORD/1")),
      "TEXTDAILY/CC/1").check(),
    description + " Check ccid");
  
  add_wait_checker(
    description + " Excluded publishers",
    AutoTest::and_checker(
     AutoTest::throw_checker(
       AutoTest::TimeLessChecker(new_day)),
     CampaignChecker(
       this,
       fetch_int("TEXTDAILY/CCG/1"),
       CampaignChecker::Expected().
         status("A").
         eval_status("A").
         exclude_pub_accounts(
           map_objects("TEXTDAILY/PUBLISHER/1,TEXTDAILY/PUBLISHER/2")))));

  add_checker(
   description + " Check ccid",
   AutoTest::and_checker(
     AutoTest::throw_checker(
       AutoTest::TimeLessChecker(new_day)),
     SelectedCreativeChecker(
       client,
       NSLookupRequest().
         tid(fetch_string("TEXTDAILY/TID/1")).
         referer_kw(fetch_string("TEXTDAILY/KEYWORD/1")), 0))); 
}

void
MaxPublisherShareTest::text_daily_part_3_(
  const AutoTest::Time& new_day)
{
  std::string description(
    "Text Ad Group with non "
    "installed budget (=0), part#3.");
  add_descr_phrase("Start part#3 of the case");

  ORM::ORMRestorer<ORM::PQ::Campaign>* campaign =
    create<ORM::PQ::Campaign>(
      fetch_int("TEXTDAILY/CAMPAIGN/1"));
  campaign->max_pub_share = 1.0;
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      campaign->update()),
    description + " Update maximum publisher share");

  add_wait_checker(
    description + " No excluded publishers",
    AutoTest::and_checker(
     AutoTest::throw_checker(
       AutoTest::TimeLessChecker(new_day)),
     CampaignChecker(
       this,
       fetch_int("TEXTDAILY/CCG/1"),
       CampaignChecker::Expected().
         status("A").
         eval_status("A").
         exclude_pub_accounts("")))); 
}
