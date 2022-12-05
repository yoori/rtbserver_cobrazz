
#include "TextAdNetAndGrossTest.hpp"

REFLECT_UNIT(TextAdNetAndGrossTest) (
  "CreativeSelection",
  AUTO_TEST_FAST | AUTO_TEST_SLOW
);


namespace
{
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::SelectedCreativesCCID SelectedCreativesCCID;
}
 
bool 
TextAdNetAndGrossTest::run()
{
  NOSTOP_FAIL_CONTEXT(case_tag_with_and_without_commission());
  NOSTOP_FAIL_CONTEXT(case_competition());
  NOSTOP_FAIL_CONTEXT(case_publisher_commission());
  return true;
}


void
TextAdNetAndGrossTest::case_tag_with_and_without_commission()
{
  std::vector<std::string> exp_ccids;

  exp_ccids.push_back(fetch_string("CCID/TWAWC/2"));
  exp_ccids.push_back(fetch_string("CCID/TWAWC/1"));
  exp_ccids.push_back(fetch_string("CCID/TWAWC/3"));

  // tag without commision
  {
    AdClient client(AdClient::create_user(this));
    NSLookupRequest request;
    request.referer_kw(fetch_string("KEYWORD/TWAWC/1") + " " +
      fetch_string("KEYWORD/TWAWC/2") + " " +
      fetch_string("KEYWORD/TWAWC/3"));
    request.debug_time(target_request_time_);

    client.process_request(request);
    client.process_request(request.tid(fetch_string("TAG/TWAWC/1")));
    FAIL_CONTEXT(
      AutoTest::sequence_checker(
        exp_ccids,
        SelectedCreativesCCID(client)).check(),
      "cc_ids tag without commision");
  }
  // tag with commision
  {
    AdClient client(AdClient::create_user(this));
    NSLookupRequest request;
    request.referer_kw(fetch_string("KEYWORD/TWAWC/1") + " " +
      fetch_string("KEYWORD/TWAWC/2") + " " +
      fetch_string("KEYWORD/TWAWC/3"));
    request.debug_time(target_request_time_);

    client.process_request(request);
    client.process_request(request.tid(fetch_string("TAG/TWAWC/2")));
    FAIL_CONTEXT(
      AutoTest::sequence_checker(
        exp_ccids,
        SelectedCreativesCCID(client)).check(),
      "cc_ids with commision");
  }
}

void
TextAdNetAndGrossTest::case_competition()
{
  //competition 1 < 2, ordered
  {
    std::vector<std::string> exp_ccids;      
    exp_ccids.push_back(fetch_string("CCID/COMPETITION/2"));
    exp_ccids.push_back(fetch_string("CCID/COMPETITION/1"));
    
    AdClient client(AdClient::create_user(this));
    NSLookupRequest request;
    request.referer_kw(fetch_string("KEYWORD/COMPETITION/1") + " " +
      fetch_string("KEYWORD/COMPETITION/2"));
    request.debug_time(target_request_time_);

    client.process_request(request);
    client.process_request(request.tid(fetch_string("TAG/COMPETITION/1")));
    FAIL_CONTEXT(
      AutoTest::sequence_checker(
        exp_ccids,
        SelectedCreativesCCID(client)).check(),
      "cc_ids 2,1 for tag 1");
  }
  //competition 1 = 3, not ordered
  {
    std::vector<std::string> exp_ccids;
    exp_ccids.push_back(fetch_string("CCID/COMPETITION/3"));
    exp_ccids.push_back(fetch_string("CCID/COMPETITION/1"));
    
    AdClient client(AdClient::create_user(this));
    NSLookupRequest request;
    request.referer_kw(fetch_string("KEYWORD/COMPETITION/1") + " " +
      fetch_string("KEYWORD/COMPETITION/3"));
    request.debug_time(target_request_time_);

    client.process_request(request);
    client.process_request(request.tid(fetch_string("TAG/COMPETITION/1")));
    // may be in different orders
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        2,
        SelectedCreativesCCID(client).size()).check(),
      "selected_creatives size");

    FAIL_CONTEXT(
      AutoTest::sequence_checker(
        exp_ccids,
        SelectedCreativesCCID(client),
        AutoTest::SCE_ENTRY).check(),
      "cc_ids 1,3 for tag 1");
  }
  //competition 1 < 2, ordered
  {
    std::vector<std::string> exp_ccids;      
    exp_ccids.push_back(fetch_string("CCID/COMPETITION/2"));
    exp_ccids.push_back(fetch_string("CCID/COMPETITION/1"));
    
    AdClient client(AdClient::create_user(this));
    NSLookupRequest request;
    request.referer_kw(fetch_string("KEYWORD/COMPETITION/1") + " " +
      fetch_string("KEYWORD/COMPETITION/2"));
    request.debug_time(target_request_time_);

    client.process_request(request);
    client.process_request(request.tid(fetch_string("TAG/COMPETITION/2")));
    FAIL_CONTEXT(
      AutoTest::sequence_checker(
        exp_ccids,
        SelectedCreativesCCID(client)).check(),
      "cc_ids 2,1 for tag 2");
  }
  //competition 1 = 3, not ordered
  {
    std::vector<std::string> exp_ccids;
    exp_ccids.push_back(fetch_string("CCID/COMPETITION/3"));
    exp_ccids.push_back(fetch_string("CCID/COMPETITION/1"));
    
    AdClient client(AdClient::create_user(this));
    NSLookupRequest request;
    request.referer_kw(fetch_string("KEYWORD/COMPETITION/1") + " " +
      fetch_string("KEYWORD/COMPETITION/3"));
    request.debug_time(target_request_time_);

    client.process_request(request);
    client.process_request(request.tid(fetch_string("TAG/COMPETITION/2")));
    // may be in different orders
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      2, SelectedCreativesCCID(client).size()).check(),
    "selected_creatives size");

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      exp_ccids,
      SelectedCreativesCCID(client),
      AutoTest::SCE_ENTRY).check(),
    "cc_ids 1,3 for tag 2");
  }
}

void
TextAdNetAndGrossTest::case_publisher_commission()
{
  // sub case 1 with commision - show in tag1, not show in tag2

  std::string ccid1 = fetch_string("CCID/PUBLISHER-COMMISSION/1");

  {
    AdClient client(AdClient::create_user(this));
    NSLookupRequest request;
    request.referer_kw(fetch_string("KEYWORD/PUBLISHER-COMMISSION/1"));
    request.debug_time(target_request_time_);

    client.process_request(request);
    request.tid(fetch_string("TAG/PUBLISHER-COMMISSION/1"));
    client.process_request(request);
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        1,
        SelectedCreativesCCID(client).size()).check(),
      "selected_creatives size");
    
    FAIL_CONTEXT(
      AutoTest::entry_checker(
        ccid1,
        SelectedCreativesCCID(client)).check(),
      "cc_ids 1 for tag 1");
  }
  {
    AdClient client(AdClient::create_user(this));
    NSLookupRequest request;
    request.referer_kw(fetch_string("KEYWORD/PUBLISHER-COMMISSION/1"));
    request.debug_time(target_request_time_);

    client.process_request(request);
    request.tid(fetch_string("TAG/PUBLISHER-COMMISSION/2"));
    client.process_request(request);
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        0,
        SelectedCreativesCCID(client).size()).check(),
      "selected_creatives size");
  }
  // sub case 2 without commision - show in tag1 and show in tag2

  std::string ccid2 = fetch_string("CCID/PUBLISHER-COMMISSION/2");
  
  {
    AdClient client(AdClient::create_user(this));
    NSLookupRequest request;
    request.referer_kw(fetch_string("KEYWORD/PUBLISHER-COMMISSION/2"));
    request.debug_time(target_request_time_);

    client.process_request(request);
    request.tid(fetch_string("TAG/PUBLISHER-COMMISSION/1"));
    client.process_request(request);

    FAIL_CONTEXT(
      AutoTest::equal_checker(
        1,
        SelectedCreativesCCID(client).size()).check(),
      "selected_creatives size");
    
    FAIL_CONTEXT(
      AutoTest::entry_checker(
        ccid2,
        SelectedCreativesCCID(client)).check(),
      "cc_ids 2 for tag 1");
  }
  {
    AdClient client(AdClient::create_user(this));
    NSLookupRequest request;
    request.referer_kw(fetch_string("KEYWORD/PUBLISHER-COMMISSION/2"));
    request.debug_time(target_request_time_);

    client.process_request(request);
    request.tid(fetch_string("TAG/PUBLISHER-COMMISSION/2"));
    client.process_request(request);
    
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        1,
        SelectedCreativesCCID(client).size()).check(),
      "selected_creatives size");
    
    FAIL_CONTEXT(
      AutoTest::entry_checker(
        ccid2,
        SelectedCreativesCCID(client)).check(),
      "cc_ids 2 for tag 2");
  }
}

void
TextAdNetAndGrossTest::pre_condition()
{
  target_request_time_ = Generics::Time::get_time_of_day();
  target_request_time_ = target_request_time_.get_gm_time().get_date() +
    Generics::Time::ONE_HOUR * target_request_time_.get_gm_time().tm_hour;

  Generics::Time target_sdate = target_request_time_;

  // case tag with and without commission
  stats_[0].key().
    pub_account_id(fetch_int("PUB/TWAWC/1")).
    tag_id(fetch_int("TAG/TWAWC/1")).
    stimestamp(target_sdate);
  stats_[1].key().
    pub_account_id(fetch_int("PUB/TWAWC/2")).
    tag_id(fetch_int("TAG/TWAWC/2")).
    stimestamp(target_sdate);
  // case competition
  stats_[2].key().
    pub_account_id(fetch_int("PUB/COMPETITION/1")).
    tag_id(fetch_int("TAG/COMPETITION/1")).
    stimestamp(target_sdate);
  stats_[3].key().
    pub_account_id(fetch_int("PUB/COMPETITION/2")).
    tag_id(fetch_int("TAG/COMPETITION/2")).
    stimestamp(target_sdate);
  // case publisher commission
  stats_[4].key().
    pub_account_id(fetch_int("PUB/PUBLISHER-COMMISSION")).
    tag_id(fetch_int("TAG/PUBLISHER-COMMISSION/1")).
    stimestamp(target_sdate);
  stats_[5].key().
    pub_account_id(fetch_int("PUB/PUBLISHER-COMMISSION")).
    tag_id(fetch_int("TAG/PUBLISHER-COMMISSION/2")).
    stimestamp(target_sdate);

  stats_.select(pq_conn_);
}

void
TextAdNetAndGrossTest::post_condition()
{
  typedef ORM::HourlyStats::Diffs Diffs;
  
  const Diffs diffs[6] =
  {
    // case tag with and without commission
    Diffs().
      imps(3).
      requests(3).
      clicks(0).
      actions(0).
      adv_amount(0).
      adv_comm_amount(0).
      pub_amount(0.003).
      pub_comm_amount(0),
    Diffs().
      imps(3).
      requests(3).
      clicks(0).
      actions(0).
      adv_amount(0).
      adv_comm_amount(0).
      pub_amount(0.00015).
      pub_comm_amount(0.00285),
    Diffs().
      imps(4).
      requests(4).
      clicks(0).
      actions(0).
      adv_amount(0).
      adv_comm_amount(0).
      pub_amount(0.01).
      pub_comm_amount(0),
    Diffs().
      imps(4).
      requests(4).
      clicks(0).
      actions(0).
      adv_amount(0).
      adv_comm_amount(0).
      pub_amount(0.007).
      pub_comm_amount(0.003),
    Diffs().
      imps(2).
      requests(2).
      clicks(0).
      actions(0).
      adv_amount(0).
      adv_comm_amount(0).
      pub_amount(0.007).
      pub_comm_amount(0.0046666),
    Diffs().
      imps(1).
      requests(2).
      clicks(0).
      actions(0).
      adv_amount(0).
      adv_comm_amount(0).
      pub_amount(0.053).
      pub_comm_amount(0.0353333)
  };
  
  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        pq_conn_, diffs, stats_)).check(),
  "RequestStatsHourly check");
}

void
TextAdNetAndGrossTest::tear_down()
{
}
