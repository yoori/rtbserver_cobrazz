#include "UidTest.hpp"

REFLECT_UNIT(UidTest) (
  "Frontend",
  AUTO_TEST_FAST
);

namespace
{
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::NSLookupRequest NSLookupRequest;
}

void
UidTest::uid_installing()
{
  add_descr_phrase("UID installing");
  AdClient client(AdClient::create_nonoptin_user(this));
  
  client.process_request(
    NSLookupRequest().setuid(1));

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      AutoTest::PROBE_UID,
      client.get_uid(),
      AutoTest::CT_NOT_EQUAL).check(),
    "Got unexpected PROBE uid");
}

void
UidTest::probe_uid()
{
  add_descr_phrase("Probe UID case");

  {
    AdClient client(AdClient::create_nonoptin_user(this));
    
    client.set_probe_uid();
    
    client.process_request(
      NSLookupRequest().setuid(1));

    FAIL_CONTEXT(
      AutoTest::equal_checker(
        AutoTest::PROBE_UID,
        client.get_uid(),
        AutoTest::CT_NOT_EQUAL).check(),
      "Got unexpected PROBE uid");
  }

  {
    AdClient client(AdClient::create_undef_user(this));

    std::string uid = client.get_uid();
  
    client.process_request(
      NSLookupRequest());
       
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        uid,
        client.get_uid()).check(),
      "Got unexpected uid");

    client.process_request(
      NSLookupRequest().setuid(1));

    FAIL_CONTEXT(
      AutoTest::equal_checker(
        uid,
        client.get_uid()).check(),
      "Got unexpected uid");
  }
}

bool 
UidTest::run_test()
{
  NOSTOP_FAIL_CONTEXT(uid_installing());
  NOSTOP_FAIL_CONTEXT(probe_uid());
  return true;
}
