
#include "BaiduTest.hpp"

REFLECT_UNIT(BaiduTest) (
  "CreativeSelection",
  AUTO_TEST_FAST);

namespace
{
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::BaiduRequest BaiduRequest;
  typedef AutoTest::UserBindRequest UserBindRequest;
  typedef AutoTest::BaiduResponseChecker BaiduResponseChecker;
}


void
BaiduTest::set_up()
{ }

void
BaiduTest::tear_down()
{ }

bool
BaiduTest::run()
{
  AUTOTEST_CASE(
    base_(),
    "Base case");

  return true;
}

void
BaiduTest::base_()
{
  AdClient rtb(AdClient::create_user(this));

  std::string tid(rtb.get_uid());

  rtb.process_request(
    UserBindRequest().tid(tid));

  // to avoid min_RTB_user_age check
  rtb.process_post(
    BaiduRequest().
      id(tid).
      debug_time(now_ - 24*60*60));
 
  rtb.process_post(
    BaiduRequest().
      aid(fetch_int("BASE/ACCOUNT")).
      debug_ccg(fetch_int("BASE/CCG")).
      id(tid).
      url(fetch_string("BASE/URL")).
      debug_size(fetch_string("SIZE")).
      debug_time(now_).
      minimum_cpm(0));

  FAIL_CONTEXT(
    BaiduResponseChecker(rtb,
      BaiduResponseChecker::Expected().
        advertiser_id(fetch_int("BASE/ADV")).
        creative_id(fetch_int("BASE/CREATIVE"))).check(),
    "Baidu response creative");
}


