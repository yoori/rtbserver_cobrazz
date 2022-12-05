#include "RefererUrlParsingTest.hpp"

REFLECT_UNIT(RefererUrlParsingTest) (
  "TriggerMatching",
  AUTO_TEST_FAST
);

bool
RefererUrlParsingTest::run_test()
{
  std::string required_channel = fetch_string("RefererUrlParsingTest/C/01");
  unsigned int tid = fetch_int("Tags/Default");

  AutoTest::AdClient client(AutoTest::AdClient::create_user(this));
  
  AutoTest::NSLookupRequest request;
  request.tid(tid).referer_kw("Secret%20trigger");

  request.referer("http://auction1.taobao.com/auction/today_top5.htm"
                  "?asker=wangwang"
                  "&nick=%B0%AE%DC%E7%DC%E71"
                  "&auto="
                  "&title=%B0%A2%C0%EF%CD%FA%CD%FA%C3%BF%C8%D5%BD%B9%B5%E3"
                  "&width=546"
                  "&height=422"
                  "&reject="
                  "&uid=cntaobao%B0%AE%DC%E7%DC%E71");

  client.process_request(request);

  FAIL_CONTEXT(
    AutoTest::entry_checker(
      required_channel,
      client.debug_info.trigger_channels +
        client.debug_info.trigger_channels).check(),
    "must have channel for valid url 1");

  request.referer("http://www.ukaka.com/website/y/165.php"
                  "?a=2100"
                  "&b=2917"
                  "&c=1"
                  "&d=702"
                  "&e=165"
                  "&g="
                  "&k=null"
                  "&exs_1=www.aiting.com"
                  "&x=%BD%F1%CC%EC%C4%E3%D2%AA%BC%07%B8%F8%CE%D2"
                  "&a=2100"
                  "&c=1"
                  "&d=702"
                  "&e=165"
                  "&domain=http%3A%2F%2Fmg.ukaka.com"
                  "&g="
                  "&pid=378d4c38a432f45d274afcdb496cfef4"
                  "&x=%BD%F1%CC%EC%C4%E3%D2%AA%BC%07%B8%F8%CE%D2"
                  "&n2=");
                                        
  client.process_request(request);

  FAIL_CONTEXT(
    AutoTest::entry_checker(
      required_channel,
      client.debug_info.trigger_channels +
        client.debug_info.trigger_channels).check(),
    "must have channel for valid url 1");

  return true;
}
