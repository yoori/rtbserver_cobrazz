
#include "MinUidAge.hpp"

REFLECT_UNIT(MinUidAge) (
  "CreativeSelection",
  AUTO_TEST_FAST);

namespace
{
  typedef AutoTest::TemporaryAdClient TemporaryAdClient;
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::ChannelsCheck ChannelsCheck;
  typedef AutoTest::SelectedCreativeChecker SelectedCreativeChecker;
  typedef AutoTest::TagPassbackChecker TagPassbackChecker;
}

template <size_t Count>
void
MinUidAge::process_case(
  AdClient& client,
  const TestCase(&testcases)[Count],
  unsigned long colo)
{
  for (size_t i = 0; i < Count; ++i)
  {

    NSLookupRequest request;
    if (testcases[i].referer_kw)
    {
      request.referer_kw =
         map_objects(testcases[i].referer_kw);
    }

    if (colo)
    {
      request.colo = colo;
    }

    SelectedCreativeChecker checker(
      client,
      request.
        tid(fetch_string("TAG")).
        debug_time(base_time_ + testcases[i].ofset),
      testcases[i].expected_cc?
        fetch_int(testcases[i].expected_cc): 0);

    FAIL_CONTEXT(
      checker.check(),
      "CC check#" + strof(i+1));

    if (!testcases[i].expected_cc)
    {

      FAIL_CONTEXT(
        TagPassbackChecker(
          checker.client(),
          fetch_string("PASSBACK")).check(),
        "Passback check#" + strof(i+1));
    }

  }
}

void
MinUidAge::uid_age_()
{
  AdClient client(AdClient::create_user(this));

  client.process_request(
    NSLookupRequest().
      referer_kw(fetch_string("KWD/MINAGE_24")).
      debug_time(base_time_ - 12*60*60));

  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      "CHANNEL/MINAGE_24",
      client.debug_info.history_channels).check(),
    "Check history");

  const TestCase TESTCASES[] = 
  {
    {1, 0, 0},
    {12*60*60, 0, "CC/MINAGE_24"},
    {24*60*60 + 1, 0, "CC/MINAGE_24"}
  };

  process_case(client, TESTCASES);
}

void
MinUidAge::boundary_values_()
{
  {
    AdClient client(AdClient::create_user(this));

    const TestCase TESTCASES[] = 
    {
      {0, "KWD/MINAGE_0", "CC/MINAGE_0"}
    };

    process_case(client, TESTCASES);
  }

  {
    AdClient client(AdClient::create_user(this));

    const TestCase TESTCASES[] = 
    {
      {0, "KWD/MINAGE_10000,KWD/MINAGE_0", "CC/MINAGE_0"},
      {2*365*24*60*60, "KWD/MINAGE_10000,KWD/MINAGE_0", "CC/MINAGE_10000"},
    };

    process_case(
      client,
      TESTCASES,
      fetch_int("TESTCOLO"));
  }
}

void
MinUidAge::non_optin_()
{
  AdClient clients[] =
  {
    AdClient::create_nonoptin_user(this),
    AdClient::create_optout_user(this),
    AdClient::create_user(this)
  };

  clients[2].set_probe_uid();

  for (size_t i = 0; i < sizeof(clients)/sizeof(*clients); ++i)
  {
    const TestCase TESTCASES[] = 
    {
      {0, "KWD/NONOPTIN", "CC/NONOPTIN"}
    };

    process_case(
      clients[i],
      TESTCASES,
      fetch_int("ALLCOLO"));
  }
}

void
MinUidAge::temporary_()
{
  TemporaryAdClient temporary(
    TemporaryAdClient::create_user(this));

  // Use current time, to avoid temporary profile removing
  // by 'delete-old-profiles' procedure (DeleteOldUserProfiles).
  // Test environment use UIM.temp_profile_lifetime = 30 minutes.
  AutoTest::Time now;

  temporary.process_request(
    NSLookupRequest().
      referer_kw(fetch_string("KWD/TEMPORARY")).
      debug_time(now));

  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      "CHANNEL/TEMPORARY",
      temporary.debug_info.history_channels).check(),
    "Check temporary channels");

  AdClient client(AdClient::create_user(this));

  client.merge(
    temporary,
    NSLookupRequest().
      tid(fetch_int("TAG")).
      debug_time(now + 30*60));

  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      "CHANNEL/TEMPORARY",
      client.debug_info.history_channels).check(),
    "Check persistent channels");

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      client.debug_info.ccid).check(),
    "Check CC (after merging)");

  const TestCase TESTCASES[] = 
  {
    {static_cast<int>((now - base_time_).tv_sec) + 60*60, 0, 0},
    {static_cast<int>((now - base_time_).tv_sec) + 90*60, 0, "CC/TEMPORARY"}
  };    

  process_case(client, TESTCASES);
}

bool
MinUidAge::run_test()
{
  AUTOTEST_CASE(
    uid_age_(),
    "UID age test");
  
  AUTOTEST_CASE(
    boundary_values_(),
    "Boundary values");

  AUTOTEST_CASE(
    non_optin_(),
    "Not opted-in users");
  
  AUTOTEST_CASE(
    temporary_(),
    "Temporary user test");
  
  return true;
}

