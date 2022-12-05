#include "ChannelTargetingTest.hpp"

REFLECT_UNIT(ChannelTargetingTest) (
  "CreativeSelection",
  AUTO_TEST_FAST
);

namespace
{
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::SelectedCreativesCCID SelectedCreativesCCID;
  typedef AutoTest::SelectedCreativesActualCPC SelectedCreativesActualCPC;
}
 
bool 
ChannelTargetingTest::run_test()
{
  NOSTOP_FAIL_CONTEXT(part1());
  NOSTOP_FAIL_CONTEXT(part2());
  return true;
}
 
void
ChannelTargetingTest::part1()
{
  std::string keyword   = fetch_string("Key1");
  std::string tid       = fetch_string("Tag1");
  std::string exp_ccid[] ={
    fetch_string("CC1"),
    fetch_string("CC2")
  };
  add_descr_phrase("Prepare server context part1");
  AdClient client(AdClient::create_user(this));
  AutoTest::NSLookupRequest request;
  request.tid = tid;
  request.referer_kw = keyword;
  client.process_request(request);
  client.process_request(request);

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      exp_ccid,
      SelectedCreativesCCID(client)).check(),
    "selected_creatives cc_ids");
}

void
ChannelTargetingTest::part2()
{
  std::string keyword   = fetch_string("Key2");
  std::string tid       = fetch_string("Tag2");
  std::string exp_ccid[] ={
    fetch_string("CC3"),
    fetch_string("CC4")
  };
  add_descr_phrase("Prepare server context part2");
  AdClient client(AdClient::create_user(this));

  AutoTest::NSLookupRequest request;
  request.tid = tid;
  request.referer_kw = keyword;
  client.process_request(request);
  client.process_request(request);
  
  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      exp_ccid,
      SelectedCreativesCCID(client)).check(),
    "selected_creatives cc_ids");
}
