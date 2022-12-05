
#include "GeoDataMergingTest.hpp"

REFLECT_UNIT(GeoDataMergingTest) (
  "UserProfiling",
  AUTO_TEST_FAST);

namespace
{
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::ChannelsCheck ChannelsCheck;
}

bool
GeoDataMergingTest::run_test()
{
  AUTOTEST_CASE(
    empty_muid_(),
    "Merging empty muid with non-empty profile");

  AUTOTEST_CASE(
    empty_profile_(),
    "Merging non-empty muid with empty profile");

  AUTOTEST_CASE(
    merge_with_cleanup_(),
    "Merging two non-empty profiles with cleanup");

  AUTOTEST_CASE(
    same_geodata_(),
    "Merging two non-emty profiles with the same geo_data");
  
  return true;
}

void
GeoDataMergingTest::empty_muid_()
{
  AdClient client1(AdClient::create_user(this));
  AdClient client2(AdClient::create_user(this));
  client2.process_request(
    NSLookupRequest().loc_coord(fetch_string("Location/L1")));
  client2.process_request(
    NSLookupRequest().muid(client1.get_uid()));
  
  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      "Address/L1",
      client2.debug_info.geo_channels).check(),
    "Geo channels check");  
}

void GeoDataMergingTest::empty_profile_()
{
  AdClient client1(AdClient::create_user(this));
  AdClient client2(AdClient::create_user(this));
  client1.process_request(
    NSLookupRequest().loc_coord(fetch_string("Location/L2")));
  client2.process_request(
    NSLookupRequest().muid(client1.get_uid()));
  
  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      "Address/L2",
      client2.debug_info.geo_channels).check(),
    "Geo channels check");  
}

void GeoDataMergingTest::merge_with_cleanup_()
{
  AdClient client1(AdClient::create_undef_user(this));
  AdClient client2(AdClient::create_undef_user(this));

  AutoTest::Time time(
      (AutoTest::Time().get_gm_time().format("%d-%m-%Y") +
        ":" + "00-00-01").c_str());

  client1.process_request(
    NSLookupRequest().
      loc_coord(fetch_string("Location/L3")).
      debug_time(time - 24*60*60));

  std::string exp_channels_1;

  for (int i = 1; i <= 50; i++)
  {
    exp_channels_1+="Address/" + strof(i) + (i == 50? "": ",");
    
    client1.process_request(
      NSLookupRequest().
        loc_coord(fetch_string("Location/" + strof(i))).
        debug_time(time));
  }

  client2.process_request(
    NSLookupRequest().
      loc_coord(fetch_string("Location/L4")).
      debug_time(time - 24*60*60));

  std::string exp_channels_2;

  for (int i = 51; i <= 100; i++)
  {
    exp_channels_2+= "Address/" + strof(i) + (i == 100? "": ",");
    client1.process_request(
      NSLookupRequest().
        loc_coord(fetch_string("Location/" + strof(i))).
        debug_time(time));
  }

  client2.process_request(
    NSLookupRequest().
      muid(client1.get_uid()).
      debug_time(time));

  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      (exp_channels_1 + "," + exp_channels_2).c_str(),
      client2.debug_info.geo_channels).check(),
    "Geo channels check#1");

  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      "Address/L3,Address/L4",
      client2.debug_info.geo_channels,
      AutoTest::SCE_NOT_ENTRY).check(),
    "Geo channels check#2");  
}

void GeoDataMergingTest::same_geodata_()
{
  AdClient client1(AdClient::create_user(this));
  AdClient client2(AdClient::create_user(this));
 
  AutoTest::Time time(
      (AutoTest::Time().get_gm_time().format("%d-%m-%Y") +
        ":" + "00-00-01").c_str());

  client1.process_request(
    NSLookupRequest().
      loc_coord(fetch_string("Location/L3")).
      debug_time(time - 24*60*60));

  client1.process_request(
    NSLookupRequest().
      loc_coord(fetch_string("Location/L4")).
      debug_time(time - 24*60*60));

  std::string exp_channels = "Address/L3,Address/L4";

  for (int i = 1; i <= 98; i++)
  {
    exp_channels+=",Address/" + strof(i);
    
    client1.process_request(
      NSLookupRequest().
        loc_coord(fetch_string("Location/" + strof(i))).
        debug_time(time));
  }

  client2.process_request(
    NSLookupRequest().
      loc_coord(fetch_string("Location/L3")).
      debug_time(time - 24*60*60));

  client2.process_request(
    NSLookupRequest().
      loc_coord(fetch_string("Location/L4")).
      debug_time(time - 24*60*60));

  client2.process_request(
    NSLookupRequest().
      muid(client1.get_uid()).
      debug_time(time));

  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      exp_channels.c_str(),
      client2.debug_info.geo_channels).check(),
    "Geo channels check");
}
