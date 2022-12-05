#include "DisplayAdNetAndGrossTest.hpp"
 
REFLECT_UNIT(DisplayAdNetAndGrossTest) (
  "CreativeSelection",
  AUTO_TEST_FAST | AUTO_TEST_SLOW
);

typedef AutoTest::NSLookupRequest  NSLookupRequest;
typedef AutoTest::AdClient AdClient;

bool 
DisplayAdNetAndGrossTest::run()
{
  NOSTOP_FAIL_CONTEXT(case_net_campaign_win());
  NOSTOP_FAIL_CONTEXT(case_gross_campaign_win());
  NOSTOP_FAIL_CONTEXT(case_net_campaign_win_with_commission());
  NOSTOP_FAIL_CONTEXT(case_publisher_commission());
  return true;
}

void
DisplayAdNetAndGrossTest::case_net_campaign_win()
{
  add_descr_phrase("case 1. GROSS Campaign eCPM < NET Campaign eCPM");// ecpm = 400
  // ( 10 / 2) * (1-0.3) = ecpm = 350
  // 8 / 2 =  ecpm = 400
  AdClient user(AdClient::create_user(this));

  std::string tag_id = fetch_string("Tag Id/1");
  std::string cc_id = fetch_string("CC Id/1/2");

  NSLookupRequest request;
  request.referer_kw = fetch_string("Keyword/1");
  request.tid = tag_id;
  request.debug_time(target_request_time_);

  user.process_request(request);

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      tag_id,
      user.debug_info.tag_id).check(), 
    "must got tag_id = Tag Id/1 in response");
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      cc_id,
      user.debug_info.ccid).check(),
    "must got ccid = CC Id/1/2 in response");
}

void
DisplayAdNetAndGrossTest::case_gross_campaign_win()
{
  add_descr_phrase("case 2. eCPM Campaign GROSS > eCPM Campaign NET");// ecpm = 420;
  // ( 12 / 2) * (1 - 0.3) = ecpm = 420
  // 8 / 2 = ecp = 400
  AdClient user(AdClient::create_user(this));

  std::string tag_id = fetch_string("Tag Id/2");
  std::string cc_id = fetch_string("CC Id/2/1");

  NSLookupRequest request;
  request.referer_kw = fetch_string("Keyword/2");
  request.tid = tag_id;
  request.debug_time(target_request_time_);

  user.process_request(request);

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      tag_id,
      user.debug_info.tag_id).check(), 
    "must got tag_id = Tag Id/2 in response");
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      cc_id,
      user.debug_info.ccid).check(),
    "must got ccid = CC Id/2/1 in response");
}

void
DisplayAdNetAndGrossTest::case_net_campaign_win_with_commission()
{
  add_descr_phrase("case 3. eCPM Campaign GROSS < eCPM Campaign NET(Commission on)");//ecpm = 400
  // ( 10 / 2) * (1 - 0.3) = ecpm = 350
  // (8 / 2 )  = ecp = 400 
  AdClient user(AdClient::create_user(this));

  std::string tag_id = fetch_string("Tag Id/3");
  std::string cc_id = fetch_string("CC Id/3/2");

  NSLookupRequest request;
  request.referer_kw = fetch_string("Keyword/3");
  request.tid = tag_id;
  request.debug_time(target_request_time_);

  user.process_request(request);

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      tag_id,
      user.debug_info.tag_id).check(), 
    "must got tag_id = Tag Id/3 in response");
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      cc_id,
      user.debug_info.ccid).check(),
    "must got ccid = CC Id/3/2 in response");
}

void
DisplayAdNetAndGrossTest::case_publisher_commission()
{
  add_descr_phrase("case 4. Check Publisher Commission");
  // ( 2.8 / 2) * (1 - 0.3) = ecpm = 98
  // ( 3 / 2) * (1 - 0.4) = ecpm = 90
  // ( 3.4 / 2) * (1 - 0.4) = ecpm = 102
  AdClient user(AdClient::create_user(this));

  std::string tag_id1 = fetch_string("Tag Id/4/1");
  std::string tag_id2 = fetch_string("Tag Id/4/2");
  std::string cc_id = fetch_string("CC Id/4");

  NSLookupRequest request;
  request.referer_kw = fetch_string("Keyword/4");
  request.debug_time(target_request_time_);

  request.tid = tag_id1;
  user.process_request(request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      tag_id1,
      user.debug_info.tag_id).check(), 
    "must got tag_id = Tag Id/4/1 in response");
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      cc_id,
      user.debug_info.ccid).check(),
    "must got ccid = CC Id/4 in response");
  request.tid = tag_id2;
  user.process_request(request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      tag_id2,
      user.debug_info.tag_id).check(), 
    "must got tag_id = Tag Id/4/2 in response");
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      user.debug_info.ccid).check(),
    "must got ccid = 0 in response");
}

void
DisplayAdNetAndGrossTest::pre_condition()
{
  target_request_time_ = Generics::Time::get_time_of_day();
  target_request_time_ = target_request_time_.get_gm_time().get_date() +
    Generics::Time::ONE_HOUR * target_request_time_.get_gm_time().tm_hour;
  Generics::Time target_sdate = target_request_time_;
  // set stats keys
  for(int i = 0; i < 5; ++i)
  {
    stats_[i].key().stimestamp(target_sdate);
  }

  stats_[0].key().
    pub_account_id(fetch_int("PubAcc Id/1")).
    tag_id(fetch_int("Tag Id/1"));
  stats_[1].key().
    pub_account_id(fetch_int("PubAcc Id/2")).
    tag_id(fetch_int("Tag Id/2"));
  stats_[2].key().
    pub_account_id(fetch_int("PubAcc Id/3")).
    tag_id(fetch_int("Tag Id/3"));
  stats_[3].key().
    pub_account_id(fetch_int("PubAcc Id/4")).
    tag_id(fetch_int("Tag Id/4/1"));
  stats_[4].key().
    pub_account_id(fetch_int("PubAcc Id/4")).
    tag_id(fetch_int("Tag Id/4/2"));
  stats_.select(pq_conn_);
}

void
DisplayAdNetAndGrossTest::post_condition()
{

  typedef ORM::HourlyStats::Diffs Diffs;

  const Diffs diffs[] =
  {
    Diffs().
      imps(1).
      requests(1).
      clicks(0).
      actions(0).
      adv_amount(0.008).
      adv_comm_amount(0).
      pub_amount(0.003).
      pub_comm_amount(0),
    Diffs().
      imps(1).
      requests(1).
      clicks(0).
      actions(0).
      adv_amount(0.0084).
      adv_comm_amount(0.0036).
      pub_amount(0.003).
      pub_comm_amount(0),
    Diffs().
      imps(1).
      requests(1).
      clicks(0).
      actions(0).
      adv_amount(0.008).
      adv_comm_amount(0.0034286).
      pub_amount(0.003).
      pub_comm_amount(0),
    Diffs().
      imps(1).
      requests(1).
      clicks(0).
      actions(0).
      adv_amount(0.00196).
      adv_comm_amount(0.00084).
      pub_amount(0.0018).
      pub_comm_amount(0.0012),
    Diffs().
      imps(0).
      requests(1).
      clicks(0).
      actions(0).
      adv_amount(0).
      adv_comm_amount(0).
      pub_amount(0).
      pub_comm_amount(0)
  };

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        pq_conn_, diffs, stats_)).check(),
  "RequestStatsHourly check");
}

void
DisplayAdNetAndGrossTest::tear_down()
{
}
