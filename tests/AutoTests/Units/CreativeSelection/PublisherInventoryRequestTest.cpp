#include "PublisherInventoryRequestTest.hpp"

REFLECT_UNIT(PublisherInventoryRequestTest) (
  "CreativeSelection",
  AUTO_TEST_FAST
);

bool 
PublisherInventoryRequestTest::run_test()
{
  std::string tag_inv = fetch_string("TAG_INV");
  std::string tag_no_inv = fetch_string("TAG_NO_INV");
  AutoTest::NSLookupRequest request;
  request.referer_kw = fetch_string("KEYWORD");
  std::string cc = fetch_string("CC");

  AdClient client(AdClient::create_user(this));
  // tag in inventory estimation mode and no tag.inv => show passback
  {
    request.tid = tag_inv;
    client.process_request(request,
      "tag in inventory estimation mode and tag.inv=0");
  }
  // tag in advertising mode and no tag.inv => show ad
  {
    request.tid = tag_no_inv;
    client.process_request(request,
      "tag in advertising mode and no tag.inv");
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        cc,
        client.debug_info.ccid).check(),
      "must got expected ccid");
  }
  // tag in advertising mode and tag.inv=1 => response no content
  {
    request.tag_inv = 1;
    client.process_request(request,
      "tag in advertising mode and tag.inv=1");
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        client.debug_info.selected_creatives.empty()),
      "must got empty ccid");
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        204,
        client.req_status()).check(),
      "Server must return 'no content' status");
  }
  // tag in inventory estimation mode and tag.inv=1 => log inventory (no content)
  {
    request.tid = tag_inv;
    client.process_request(request,
      "tag in inventory mode and tag.inv=1");
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        204,
        client.req_status()).check(),
      "Server must return 'no content' status");
  }

  return true;
}
