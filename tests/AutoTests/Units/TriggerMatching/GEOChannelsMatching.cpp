
#include "GEOChannelsMatching.hpp"

REFLECT_UNIT(GEOChannelsMatching) (
  "TriggerMatching",
  AUTO_TEST_FAST);

namespace {
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::ChannelsCheck ChannelsCheck;

  const GEOChannelsMatching::LocTestRequest LOCREQUESTS[] =
  {
    {
      "Country location.",
      "gb", "GB/CntryCH", "State1CH,State2CH,City1CH",
      &NSLookupRequest::loc_name
    },
    {
      "State location.",
      "gb/State1", "GB/CntryCH,State1CH", "State2CH,City1CH",
      &NSLookupRequest::loc_name
    },
    {
      "Full location.",
      "gb/State2/City1", "GB/CntryCH,State2CH,City1CH", "State1CH",
      &NSLookupRequest::loc_name
    },
    {
      "Full location with country.",
      "gb/State2/City1", "GB/CntryCH,State2CH,City1CH", "State1CH",
      &NSLookupRequest::loc_name
    },

    // Coordinates request isn't implement yet.
    //{
    //  "Coordinate location.",
    // "51.30/0.07", "State2CH,City1CH", "State1CH",
    //  &NSLookupRequest::loc_coord, 0
    //},

    // Empty geo_channels expected
    {
      "Unknown location.",
      "gb/utah/moscow", "GB/CntryCH", 0,
      &NSLookupRequest::loc_name
    }, 
    {
      "Invalid location.",
      "1/2/3", 0, 0,
      &NSLookupRequest::loc_name
    },
    {
      "Empty location.",
      "", 0, 0,
      &NSLookupRequest::loc_name
    }
  };

 const GEOChannelsMatching::IPTestRequest IPREQUESTS[] =
 {
   //{
   //  "Utf-8 symbols in loc name.",
   //  "78.123.82.0",
   //  "Location1", 0
   //},
   {
     "Empty region#1.",
     "203.198.96.0",
     "Location2", "HK/CntryCH,GlobalCity2CH"
   },
   //{
   //  "Empty region#2.",
   //  "46.36.200.101",
   //  "Location2", 0
   //},
   {
     "Region code equal to other country code #1.",
     "199.195.32.0",
     "Location3", "US/CntryCH,GlobalState2CH,GlobalCity4CH"
   },
   {
     "Region code equal to other country code #2.",
     "4.224.144.0",
     "Location4", "US/CntryCH,GlobalState3CH,GlobalCity5CH"
   }
 };
}


bool
GEOChannelsMatching::run_test()
{
  for (unsigned long i = 0; i < countof(LOCREQUESTS); ++i)
  {
    AUTOTEST_CASE(
      location_name_case(LOCREQUESTS[i]),
      LOCREQUESTS[i].description);
  }

  if (get_config().check_service(CTE_CENTRAL, STE_FRONTEND))
  {
    for (unsigned long i = 0; i < countof(IPREQUESTS); ++i)
    {
      AUTOTEST_CASE(
        ip_case(IPREQUESTS[i]),
        IPREQUESTS[i].description);
    }
  }
  else
  {
    AutoTest::Logger::thlog().log(
      "'debug.ip' part wasn't run, due to the fact that "
      "central frontend config is absent.",
      Logging::Logger::WARNING);
  }
  return true;
}

void GEOChannelsMatching::location_name_case(
  const LocTestRequest& testcase)
{
    AdClient client(AdClient::create_user(this));
    
    NSLookupRequest request;

    testcase.param(
      request,
      map_objects(testcase.location, "/"));

    client.process_request(request);

    if (!testcase.unexpected)
    {
      if (!testcase.expected)
      {
        FAIL_CONTEXT(
          AutoTest::equal_checker(
            0,
            client.debug_info.geo_channels.size()).check(),
          "Expect empty geo_channels");
      }
      else
      {
        FAIL_CONTEXT(
          ChannelsCheck(
            this, testcase.expected,
            client.debug_info.geo_channels,
            AutoTest::SCE_COMPARE).check(),
          "Expected geo_channels");
      }
    }
    else
    {
      FAIL_CONTEXT(
        ChannelsCheck(
          this, testcase.expected,
          client.debug_info.geo_channels).check(),
        "Expected geo_channels");

      FAIL_CONTEXT(
        ChannelsCheck(
          this, testcase.unexpected,
          client.debug_info.geo_channels,
          AutoTest::SCE_NOT_ENTRY).check(),
        "Unexpected geo_channels");
    }
}

void GEOChannelsMatching::ip_case(
  const IPTestRequest& testcase)
{

  AdClient client(
    AdClient::create_user(
        this, AutoTest::UF_CENTRAL_FRONTEND));
    
  client.process_request(
    NSLookupRequest().
      debug_ip(testcase.ip).
      loc_name(0));

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string(testcase.expected_location),
      client.debug_info.location).check(),
    "Expected location");

  if (testcase.expected_channels)
  {
    FAIL_CONTEXT(
      ChannelsCheck(
        this,
        testcase.expected_channels,
        client.debug_info.geo_channels).check(),
      "Expected geo_channels");
  }

}
