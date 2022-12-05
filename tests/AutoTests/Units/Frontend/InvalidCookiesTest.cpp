#include "InvalidCookiesTest.hpp"

REFLECT_UNIT(InvalidCookiesTest) (
  "Frontend",
  AUTO_TEST_FAST
);

namespace
{
  const char CREATIVE_FORMAT[] = "unit-test-imp"; // format field of nslookup request
  typedef AutoTest::ClickResponseChecker ClickResponseChecker;
}

bool 
InvalidCookiesTest::run_test()
{
 
  set_up();

  //ATTENTION! Do not break tests sequence

  // this tests using request with Keyword/01
  invalid_base64_uid_test_case();
  invalid_uid_test_case();

  invalid_last_colo_id_test_case();

  // this tests need previously sending opt-out request
  opt_out_test_case();
  invalid_opt_out_status_test_case();
  opt_out_after_crack_test_case();
  
  return true;
}                      

std::string InvalidCookiesTest::crack_cookie(
  const char* cookie_name,
  unsigned short pos,
  const char* extStr,
  bool replacing)
{
  std::string new_cookie_value;
  unsigned short ext_str_len = strlen(extStr);
  std::string cookie_value;
  client.get_cookie_value(cookie_name, cookie_value);
  if (pos > cookie_value.length())
  {
    Stream::Error ostr;
    ostr << "Cann't set cookie '" << cookie_name << "', out of range (try to place '" <<
      extStr << "' into the '" << cookie_value << "' (len=" <<
      cookie_value.length() << ") in place pos)";
    throw AutoTest::CheckFailed(ostr);
  }

  new_cookie_value = cookie_value;
  if (replacing)
  {
    new_cookie_value.replace(pos, ext_str_len, extStr, ext_str_len);
  }
  else
  {
    new_cookie_value.insert( pos, extStr );
  }

  client.set_cookie_value(cookie_name, new_cookie_value.c_str());
  return new_cookie_value;
}

void
InvalidCookiesTest::
crack_cookie_and_check_serv_behaviour(const char* cookie_name,
                                      unsigned short pos,
                                      const char* extStr,
                                      bool replacing,
                                      bool check_host_cookie_presents)
{
  crack_cookie( cookie_name, pos, extStr, replacing );
  //NSLookup request
  client.process_request(ns_request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      client.debug_info.ccid,
      fetch_string("CCID/01")).check(),
    "unexpected ccid");

  if (check_host_cookie_presents)
  {
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        client.has_cookie(cookie_name)),
      std::string("must has") + cookie_name +
      "host cookie");
  }
      
  FAIL_CONTEXT(
    AutoTest::predicate_checker(!client.debug_info.track_pixel_url.empty()),
    "must have debug_info.track_pixel_url");
  
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      !client.debug_info.click_url.empty()),
    "must have debug_info.click_url");
  
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      !client.debug_info.selected_creatives.first().action_adv_url.empty()),
    "must have debug_info.action_adv_url");

  //save URLs
  std::string click_url = client.debug_info.click_url;
  std::string action_adv_url = client.debug_info.selected_creatives.first().action_adv_url;
  std::string track_pixel_url = client.debug_info.track_pixel_url;

  crack_cookie( cookie_name, pos, extStr, replacing );
  //track.gif? request
  client.process_request(track_pixel_url);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      200,
      client.req_status()).check(),
    "must got expected request status");
  std::string header_value;
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      client.find_header_value("Content-Type", header_value)),
    "Content-Type not found");
  
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "image/gif",
      header_value).check(),
    "Content-Type must be 'image/gif'");
  header_value.clear();

  //AdClickServer request
  client.process_request(click_url);

  FAIL_CONTEXT(
    ClickResponseChecker(
      client,
      "",
      "http://www.autotest.com/").check(),
    "Check click response");

  //ActionServer request
  client.process_request(action_adv_url);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      200,
      client.req_status()).check(),
    "must got expected request status");
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      client.find_header_value("Content-Type", header_value)),
    "Content-Type not found");
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "image/gif",
      header_value).check(),
    "Content-Type must be 'image/gif'");

  if (check_host_cookie_presents)
  {
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        client.has_host_cookie(cookie_name)),
      std::string("must has") + cookie_name +
      "host cookie");
  }
}

void InvalidCookiesTest::set_up()
{
  add_descr_phrase("'InvalidCookies' set_up");
  ns_request.format = CREATIVE_FORMAT;
  ns_request.tid = fetch_string("Tag/01");
  ns_request.referer_kw = fetch_string("Keyword/01");
}

void InvalidCookiesTest::invalid_base64_uid_test_case()
{
  add_descr_phrase("'InvalidCookies' send invalid uid (invalid base64)");
  std::string old_uid_value, got_uid_value;
  client.process_request(ns_request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      client.debug_info.ccid,
      fetch_string("CCID/01")).check(),
    "unexpected ccid");
  
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      client.has_cookie("uid")),
    "must has uid cookie ");
  //crack cookie
  client.get_cookie_value("uid", old_uid_value);
  std::string new_uid_value = crack_cookie( "uid", 0, "*");  // bad string for base64 decoder

  //sending request with cracked uid cookie
  ns_request.tid = fetch_string("PassbackTag");
  client.process_request(ns_request);
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      client.has_cookie("uid")),
    "must has uid cookie");

  client.get_cookie_value("uid", got_uid_value);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      new_uid_value,
      got_uid_value).check(),
    "unexpected uid");

  // CPA creatives doesn't show for unknow user
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      client.debug_info.ccid).check(),
    "Check ccid");

  // restore uid  
  client.set_cookie_value("uid", old_uid_value.c_str());

  ns_request.tid = fetch_string("Tag/01");
}

void InvalidCookiesTest::invalid_uid_test_case()
{
  add_descr_phrase("'InvalidCookies' send invalid uid (changed)");
  std::string old_uid_value, got_uid_value;
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      client.has_cookie("uid")),
    "must has uid cookie");
  client.get_cookie_value("uid", old_uid_value);
  std::string new_uid_value = crack_cookie( "uid", 0, "TTTT");  //it is crete correct other uid
  client.process_request(ns_request);
  client.get_cookie_value("uid", got_uid_value);

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      new_uid_value,
      got_uid_value).check(),
    std::string("must has new uid!='") + new_uid_value +
      "' cookie");

  // CPA creatives doesn't show for unknow user
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      client.debug_info.ccid).check(),
    "Check ccid");

  // ADSC-3949
  ns_request.tid = fetch_string("PassbackTag");
  client.process_request(ns_request);
  client.get_cookie_value("uid", got_uid_value);

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      new_uid_value,
      got_uid_value).check(),
    std::string("must has new uid!='") + new_uid_value +
      "' cookie");

  // CPA creatives doesn't show for unknow user
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      client.debug_info.ccid).check(),
    "Check ccid");

  // restore uid
  client.set_cookie_value("uid", old_uid_value.c_str());

  client.process_request(ns_request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("CCID/01"),
      client.debug_info.ccid).check(),
    "Check ccid");

  ns_request.tid = fetch_string("Tag/01");

  client.process_request(ns_request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("CCID/01"),
      client.debug_info.ccid).check(),
    "Check ccid");
}


void InvalidCookiesTest::invalid_last_colo_id_test_case()
{
  add_descr_phrase("'InvalidCookies' send invalid last_colo_id (invalid content)");
  client.set_cookie_value("lc", "1", false);
  client.process_request(ns_request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      client.debug_info.ccid,
      fetch_string("CCID/01")).check(),
    "unexpected ccid");
  crack_cookie_and_check_serv_behaviour("lc", 0, "A", true, false);
  crack_cookie_and_check_serv_behaviour("lc", 0, "-1", true, false);
}

void InvalidCookiesTest::opt_out_test_case()
{
 add_descr_phrase("'InvalidCookies' send opt-out");
 client.process_request(optout_request.op("out"));
 // check clearing cookie for adFrontend domain
 FAIL_CONTEXT(
   AutoTest::predicate_checker(
     !client.has_host_cookies()), 
   "must not has host cookies");
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      client.has_domain_cookie("OPTED_OUT", "YES")), 
    "must has OPTED_OUT=YES domain cookie");
}

void InvalidCookiesTest::invalid_opt_out_status_test_case()
{
  add_descr_phrase("'InvalidCookies' send invalid OPTED_OUT");
  client.process_request(ns_request);
  FAIL_CONTEXT(
    AutoTest::predicate_checker(!client.has_cookie("uid")), 
    "must not has uid cookie in opted-out mode");
  client.set_cookie_value("OPTED_OUT", "UNKNOWN");
  client.process_request(AutoTest::OptOutRequest().op("in"));
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      client.has_cookie("uid")), 
    "must has uid cookie in opted-out mode");

  std::string uid_value;
  
  client.get_cookie_value("uid", uid_value);
  
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      AutoTest::PROBE_UID,
      uid_value,
      AutoTest::CT_NOT_EQUAL).check(),
    "unexpected uid after");
}

void InvalidCookiesTest::opt_out_after_crack_test_case()
{
  add_descr_phrase("'InvalidCookies' send optout after crack OPTED_OUT cookie");

  client.process_request(optout_request);
  FAIL_CONTEXT(
    AutoTest::predicate_checker(!client.has_host_cookies()), 
    "must not has host cookies");
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      client.has_domain_cookie("OPTED_OUT", "YES")), 
    "must has OPTED_OUT=YES domain cookie");
  client.process_request(ns_request);
  FAIL_CONTEXT(
    AutoTest::predicate_checker(!client.has_host_cookies()), 
    "must not has host cookies");
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      client.has_domain_cookie("OPTED_OUT", "YES")), 
    "must has OPTED_OUT=YES domain cookie");
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      !client.has_cookie("uid")), 
    "must not has uid");
}

 
