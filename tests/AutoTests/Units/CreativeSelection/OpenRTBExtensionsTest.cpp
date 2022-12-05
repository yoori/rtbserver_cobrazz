
#include "OpenRTBExtensionsTest.hpp"

REFLECT_UNIT(OpenRTBExtensionsTest) (
  "CreativeSelection",
  AUTO_TEST_FAST | AUTO_TEST_SLOW );

namespace
{
  typedef AutoTest::UserBindRequest UserBindRequest;
  typedef AutoTest::AdClient AdClient;
  namespace ORM = AutoTest::ORM;

  const char OPENX_CASE[] = "OpenX";
  const char VAST_CASE[] = "Vast";
  const char ALLYES_CASE[] = "Allyes";

  const char MATCHING_AD1[] = "{\"abc\": \"123\", \"elem_2\": true}";
  const char MATCHING_AD2[] = "{\"null_elem\": null}";
  const char MATCHING_AD3[] = "{\"abc\": \"elem_1\"}, {\"def\": \"elem_2\"}";
  const char MATCHING_AD3_R[] = "{\"abc\": \"elem_1\"}";
  const char MATCHING_AD4[] = "{\"ar1\": [1, 2, 3], \"ar2\": [{\"a\": 1}, {\"c\": 2}]}";

  const char ALLYES_NURL[] = ".*ImprTrack\\/pt\\.gif(?=.*p=\\${AUCTION_PRICE}).*t=n";
  const int STATUS_CODE_NO_CONTENT = 204;
  const char COMPANION_AD[] = "[{\"banner\":{\"id\": \"companion_1\", \"w\": 120, \"h\": 240}}]";
};

template<>
void OpenRTBExtensionsTest::ExpectedSetter<const std::string&>::set(
  Expected& expected,
  const std::string& val)
{
  (expected.*f_)(val);
}

const OpenRTBExtensionsTest::CaseRequest OpenRTBExtensionsTest::OPENX_REQUESTS[] = {
  // matching_ad_id filling
  { "OpenX/ACCOUNT", "body-openx", "728x90", "OpenX/URL#3",
    RequestParams({{&RTBRequest::matching_ad_id, MATCHING_AD1}}),
    "OpenX/CREATIVE#3", ExpectedValues({{&Expected::matching_ad_id, MATCHING_AD1, 0}}) },

  { "OpenX/ACCOUNT", "body-openx", "728x90", "OpenX/URL#3",
    RequestParams({{&RTBRequest::matching_ad_id, MATCHING_AD2}}),
    "OpenX/CREATIVE#3", ExpectedValues({{&Expected::matching_ad_id, MATCHING_AD2, 0}}) },

  { "OpenX/ACCOUNT", "body-openx", "728x90", "OpenX/URL#3",
    RequestParams({{&RTBRequest::matching_ad_id, MATCHING_AD3}}),
    "OpenX/CREATIVE#3", ExpectedValues({{&Expected::matching_ad_id, MATCHING_AD3_R, 0}}) },

  { "OpenX/ACCOUNT", "body-openx", "728x90", "OpenX/URL#3",
    RequestParams({{&RTBRequest::matching_ad_id, MATCHING_AD4}}),
    "OpenX/CREATIVE#3", ExpectedValues({{&Expected::matching_ad_id, MATCHING_AD4, 0}}) },

  // No matching_ad_id in response for none openX RTB
  { "OpenRTB/ACCOUNT", "body", "728x90", "OpenX/URL#3",
    RequestParams({{&RTBRequest::matching_ad_id, MATCHING_AD1}}),
    "OpenX/CREATIVE#3", ExpectedValues({{&Expected::matching_ad_id, "", 0}}) },

  { "OpenX/ACCOUNT", "body-openx", "728x90", "OpenX/URL#3",
    RequestParams({{&RTBRequest::matching_ad_id, MATCHING_AD1}}),
    "OpenX/CREATIVE#3", ExpectedValues({{&Expected::adm,
    ".*\\bTRACKPIXEL = [^\\s]+oxp={winning_price}.*\\bCLICK = {clickurl}.*\\bPRECLICK = {clickurl}.*", 0}}) },

  { "OpenX/ACCOUNT", "iframe-url-openx", "728x90", "OpenX/URL#3",
    RequestParams({{&RTBRequest::matching_ad_id, MATCHING_AD1}}),
    "OpenX/CREATIVE#3", ExpectedValues({{&Expected::adm,
    "<iframe src=\\\".+\\/inst?[^\\s]*(oxp={winning_price}[^\\s]+"
    "preclick={clickurl_enc}|preclick={clickurl_enc}[^\\s]+oxp={winning_price}"
    ")[^\\s]*\\\".*</iframe>", 0}}) },

  // ad_ox_cats filling
  { "OpenX/ACCOUNT", "body-openx", "728x90", "OpenX/URL#3", RequestParams(),//nullptr, 0,
    "OpenX/CREATIVE#3", ExpectedValues({{&Expected::ad_ox_cats, "CATEGORIES/content/openx_key", 1}}) },

  // site.ext.ssl_enabled filling
  { "OpenX/ACCOUNT", "body", "728x90", "OpenX/HTTP_URL#3",
    RequestParams({{&RTBRequest::ssl_enabled, "true"}}),
    "OpenX/CREATIVE#3", ExpectedValues({{&Expected::adm, ".*\\bTRACKPIXEL = https://.*", 0}}) },

  { "OpenX/ACCOUNT", "body-openx", "728x90", "OpenX/HTTP_URL#3",
    RequestParams({{&RTBRequest::ssl_enabled, "true"}}), 
    "OpenX/CREATIVE#3", ExpectedValues({{&Expected::adm, ".*\\bTRACKPIXEL = https://.*", 0}}) },

  { "OpenX/ACCOUNT", "body", "728x90", "OpenX/HTTP_URL#3",
    RequestParams({{&RTBRequest::ssl_enabled, "false"}}), 
    "OpenX/CREATIVE#3", ExpectedValues({{&Expected::adm, ".*\\bTRACKPIXEL = http://.*", 0}}) },

  { "OpenX/ACCOUNT", "body", "728x90", "OpenX/HTTP_URL#3",
    RequestParams({{&RTBRequest::ssl_enabled, "1"}}), 
    "OpenX/CREATIVE#3", ExpectedValues({{&Expected::adm, ".*\\bTRACKPIXEL = https://.*", 0}}) },

  { "OpenX/ACCOUNT", "body", "728x90", "OpenX/HTTP_URL#3",
    RequestParams({{&RTBRequest::ssl_enabled, "0"}}), 
    "OpenX/CREATIVE#3", ExpectedValues({{&Expected::adm, ".*\\bTRACKPIXEL = http://.*", 0}}) },

  { "OpenX/ACCOUNT", "body", "728x90", "OpenX/HTTPS_URL#3",
    RequestParams({{&RTBRequest::ssl_enabled, "false"}}), 
    "OpenX/CREATIVE#3", ExpectedValues({{&Expected::adm, ".*\\bTRACKPIXEL = https://.*", 0}}) },

  { "OpenX/ACCOUNT", "body", "728x90", "OpenX/HTTP_URL#3",
    RequestParams({
      {&RTBRequest::ssl_enabled, "false"},
      {&RTBRequest::secure, "1"}}), 
    "OpenX/CREATIVE#3", ExpectedValues({{&Expected::adm, ".*\\bTRACKPIXEL = https://.*", 0}}) }
};

const OpenRTBExtensionsTest::CaseRequest OpenRTBExtensionsTest::OPENX_REQUESTS_SLOW[] = {
  // is_test checking
  { "OpenX/ACCOUNT", "body-openx", "728x90", "OpenX/URL#1",
    RequestParams({{&RTBRequest::is_test, "true"}}),
    "OpenX/CREATIVE#1", ExpectedValues() },
  { "OpenX/ACCOUNT", "body-openx", "728x90", "OpenX/URL#2",
    RequestParams({{&RTBRequest::is_test, "false"}}),
    "OpenX/CREATIVE#2", ExpectedValues() },
  { "OpenX/ACCOUNT", "body-openx", "728x90", "OpenX/URL#2",
    RequestParams({{&RTBRequest::is_test_string, "true"}}),
    "OpenX/CREATIVE#2", ExpectedValues() }
};

const OpenRTBExtensionsTest::CaseRequest OpenRTBExtensionsTest::VAST_REQUESTS[] = {
  //Test 1.1
  { "Vast/ACCOUNT", "body", nullptr, "Vast/URL#1",
    RequestParams( { {&RTBRequest::mimes, "\"video/mp4\", \"video/x-flv\""} } ),
    "Vast/CREATIVE#1", ExpectedValues() },
  //Test 1.2
  { "Vast/ACCOUNT", "body", nullptr, "Vast/URL#1",
    RequestParams( { {&RTBRequest::mimes, "\"video/x-flv\""} } ),
    nullptr, ExpectedValues() },
  //Test 1.3
  { "Vast/ACCOUNT", "body", nullptr, "Vast/URL#1",
    RequestParams( { {&RTBRequest::minduration, "15"} } ),
  "Vast/CREATIVE#1", ExpectedValues() },
  //Test 1.4
  { "Vast/ACCOUNT", "body", nullptr, "Vast/URL#1",
    RequestParams( { {&RTBRequest::minduration, "16"} } ),
    "Vast/CREATIVE#2", ExpectedValues() },
  //Test 1.5
  { "Vast/ACCOUNT", "body", nullptr, "Vast/URL#1",
    RequestParams( { {&RTBRequest::maxduration, "15"} } ),
    "Vast/CREATIVE#1", ExpectedValues() },
  //Test 1.6
  { "Vast/ACCOUNT", "body", nullptr, "Vast/URL#1",
    RequestParams( { {&RTBRequest::maxduration, "14"} } ),
    nullptr, ExpectedValues() },
  //Test 1.7
  { "Vast/ACCOUNT", "body", nullptr, "Vast/URL#1",
    RequestParams( { {&RTBRequest::minduration, "16"},
                     {&RTBRequest::maxduration, "29"} } ),
    nullptr, ExpectedValues() },
  //Test 1.8
  { "Vast/ACCOUNT", "body", nullptr, "Vast/URL#1",
    RequestParams( { {&RTBRequest::protocol, "2"} } ),
  "Vast/CREATIVE#1", ExpectedValues() },
  //Test 1.9
  { "Vast/ACCOUNT", "body", nullptr, "Vast/URL#1",
    RequestParams( { {&RTBRequest::protocol, "1, 3"} } ),
    "Vast/CREATIVE#1", ExpectedValues() },
  //Test 1.10
  { "Vast/ACCOUNT", "body", nullptr, "Vast/URL#1",
    RequestParams( { {&RTBRequest::protocol,"23, 4"} } ),
    nullptr, ExpectedValues() },
  //Test 1.11
  { "Vast/ACCOUNT", "body", nullptr, "Vast/URL#1",
    RequestParams( { {&RTBRequest::playbackmethod, ""} } ),
    "Vast/CREATIVE#1", ExpectedValues() },
  //Test 1.12
  { "Vast/ACCOUNT", "body", nullptr, "Vast/URL#1",
    RequestParams( { {&RTBRequest::playbackmethod, "1, 2" } } ),
    "Vast/CREATIVE#1", ExpectedValues() },
  //Test 1.13
  { "Vast/ACCOUNT", "body", nullptr, "Vast/URL#1",
    RequestParams( { {&RTBRequest::playbackmethod, "2, 3, 3"} } ),
    "Vast/CREATIVE#1", ExpectedValues() },
  //Test 1.14
  { "Vast/ACCOUNT", "body", nullptr, "Vast/URL#1",
    RequestParams( { {&RTBRequest::playbackmethod, "2"} } ),
    nullptr, ExpectedValues() },
  //Test 1.15
  { "Vast/ACCOUNT", "body", nullptr, "Vast/URL#1",
    RequestParams( { {&RTBRequest::startdelay, "0" } } ),
    "Vast/CREATIVE#1", ExpectedValues() },
  //Test 1.16
  { "Vast/ACCOUNT", "body", nullptr, "Vast/URL#1",
    RequestParams( { {&RTBRequest::startdelay, "-1"} } ),
  nullptr, ExpectedValues() },
  //Test 1.17
  { "Vast/ACCOUNT", "body", nullptr, "Vast/URL#1",
    RequestParams( { {&RTBRequest::linearity, "1"} } ),
    "Vast/CREATIVE#1", ExpectedValues() },
  //Test 1.18
  { "Vast/ACCOUNT", "body", nullptr, "Vast/URL#1",
    RequestParams( { {&RTBRequest::linearity, "2"} } ),
    nullptr, ExpectedValues() },
  //Test 1.19
  { "Vast/ACCOUNT", "body", nullptr, "Vast/URL#1",
    RequestParams({{&RTBRequest::ext_adtype, "0"} }),
    "Vast/CREATIVE#1", ExpectedValues() },
  //Test 1.20
  { "Vast/ACCOUNT", "body", nullptr, "Vast/URL#1",
    RequestParams({{&RTBRequest::ext_adtype, "3"} }),
    "Vast/CREATIVE#1", ExpectedValues() },
  //Test 1.21
  { "Vast/ACCOUNT", "body", nullptr, "Vast/URL#1",
    RequestParams( { {&RTBRequest::ext_adtype, "1"} } ),
    nullptr, ExpectedValues() },
  //Test 1.22
  { "Vast/ACCOUNT", "body", nullptr, "Vast/URL#1",
    RequestParams( { {&RTBRequest::ext_adtype, "2"} }),
    nullptr, ExpectedValues() },
  //Test 1.23
  { "Vast/ACCOUNT", "body", nullptr, "Vast/URL#1",
    RequestParams( { {&RTBRequest::companionad, COMPANION_AD} } ),
    "Vast/CREATIVE#1", ExpectedValues() },
  //Test 1.24
  { "Vast/ACCOUNT", "body", nullptr, "Vast/URL#1",
    RequestParams( { {&RTBRequest::video_pos, "1"} } ),
    "Vast/CREATIVE#1", ExpectedValues() },
  //Test 1.25
  { "Vast/ACCOUNT", "body", nullptr, "Vast/URL#1",
    RequestParams( { {&RTBRequest::video_pos, "1"} } ),
    "Vast/CREATIVE#1", ExpectedValues() },
  //Test 1.26
  { "Vast/ACCOUNT", "body", nullptr, "Vast/URL#1",
    RequestParams( { {&RTBRequest::video_battr, "CATEGORIES/visual/iab_key", 1} } ),
    "Vast/CREATIVE#2", ExpectedValues() }
};

const OpenRTBExtensionsTest::MultislotCaseRequest OpenRTBExtensionsTest::VAST_MULTISLOT_REQUESTS[] = {
  //Test 2.1
  { "Vast/ACCOUNT", "body", "Vast/URL#1",
    ImpParams({{"imp1", "\"video/mp4\"", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
               {"imp2", "\"video/mp4\"", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr} } ),
    ExpectedValuesList( { ExpectedValues( { { &Expected::adid, "Vast/CREATIVE#1", 1 } } ),
                          ExpectedValues( { { &Expected::adid, "Vast/CREATIVE#1", 1 } } )
                        } )
  },
  //Test 2.2
  { "Vast/ACCOUNT", "body", "Vast/URL#1",
    ImpParams({{"imp1", "\"video/x-flv\"", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
               {"imp2", "\"video/mp4\"",   nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr} } ),
    ExpectedValuesList( { ExpectedValues( { { &Expected::adid, "Vast/CREATIVE#1", 1 }, { &Expected::impid, "imp2", 0 } } ),
                        } )
  },
  //Test 2.3
  { "Vast/ACCOUNT", "body", "Vast/URL#1",
    ImpParams({{"imp1", "\"video/mp4\"", "16",    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
               {"imp2", "\"video/mp4\"", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr} } ),
    ExpectedValuesList( { ExpectedValues( { { &Expected::adid, "Vast/CREATIVE#2", 1 } } ),
                          ExpectedValues( { { &Expected::adid, "Vast/CREATIVE#1", 1 } } )
                        } )
  },
  //Test 2.4
  { "Vast/ACCOUNT", "body", "Vast/URL#1",
    ImpParams({{"imp1", "\"video/mp4\"", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
               {"imp2", "\"video/mp4\"", nullptr, "14",    nullptr, nullptr, nullptr, nullptr, nullptr} } ),
    ExpectedValuesList( { ExpectedValues( { { &Expected::adid, "Vast/CREATIVE#1", 1 }, { &Expected::impid, "imp1", 0 } } ),
                        } )
  },
  //Test 2.5
  { "Vast/ACCOUNT", "body", "Vast/URL#1",
    ImpParams({{"imp1", "\"video/mp4\"", nullptr, nullptr, "5",     nullptr, nullptr, nullptr, nullptr},
               {"imp2", "\"video/mp4\"", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr} } ),
    ExpectedValuesList( { ExpectedValues( { { &Expected::adid, "Vast/CREATIVE#1", 1 }, { &Expected::impid, "imp2", 0 } } ),
                        } )
  },
  //Test 2.6
  { "Vast/ACCOUNT", "body", "Vast/URL#1",
    ImpParams({{"imp1", "\"video/mp4\"", nullptr, nullptr, nullptr, "4",     nullptr, nullptr, nullptr},
               {"imp2", "\"video/mp4\"", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr} } ),
    ExpectedValuesList( { ExpectedValues( { { &Expected::adid, "Vast/CREATIVE#1", 1 }, { &Expected::impid, "imp2", 0 } } ),
                        } )
  },
  //Test 2.7
  { "Vast/ACCOUNT", "body", "Vast/URL#1",
    ImpParams({{"imp1", "\"video/mp4\"", nullptr, nullptr, nullptr, nullptr, "-2",    nullptr, nullptr},
               {"imp2", "\"video/mp4\"", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr} } ),
    ExpectedValuesList( { ExpectedValues( { { &Expected::adid, "Vast/CREATIVE#1", 1 }, { &Expected::impid, "imp2", 0 } } ),
                        } )
  },
  //Test 2.8
  { "Vast/ACCOUNT", "body", "Vast/URL#1",
    ImpParams({{"imp1", "\"video/mp4\"", nullptr, nullptr, nullptr, nullptr, nullptr, "3",     nullptr},
               {"imp2", "\"video/mp4\"", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr} } ),
    ExpectedValuesList( { ExpectedValues( { { &Expected::adid, "Vast/CREATIVE#1", 1 }, { &Expected::impid, "imp2", 0 } } ),
                        } )
  },
  //Test 2.9
  { "Vast/ACCOUNT", "body", "Vast/URL#1",
    ImpParams({{"imp1", "\"video/mp4\"", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, "2"    },
               {"imp2", "\"video/mp4\"", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr} } ),
    ExpectedValuesList( { ExpectedValues( { { &Expected::adid, "Vast/CREATIVE#1", 1 }, { &Expected::impid, "imp2", 0 } } ),
                        } )
  }
};

const OpenRTBExtensionsTest::CaseRequest OpenRTBExtensionsTest::VAST_NEGATIVE_REQUESTS[] = {
  //Test 3.1
  { "Vast/ACCOUNT", "body", nullptr, "Vast/URL#1",
    RequestParams( { {&RTBRequest::minduration, "\"16\""} } ),
    "Vast/CREATIVE#1", ExpectedValues() },
  //Test 3.2
  { "Vast/ACCOUNT", "body", nullptr, "Vast/URL#1",
    RequestParams( { {&RTBRequest::maxduration, "\"14\""} } ),
    "Vast/CREATIVE#1", ExpectedValues() },
  //Test 3.3
  { "Vast/ACCOUNT", "body", nullptr, "Vast/URL#1",
    RequestParams( { {&RTBRequest::protocol, "\"14\""} } ),
    "Vast/CREATIVE#1", ExpectedValues() },
  //Test 3.4
  { "Vast/ACCOUNT", "body", nullptr, "Vast/URL#1",
    RequestParams( { {&RTBRequest::playbackmethod, "\"2\""} } ),
    "Vast/CREATIVE#1", ExpectedValues() },
  //Test 3.5
  { "Vast/ACCOUNT", "body", nullptr, "Vast/URL#1",
    RequestParams( { {&RTBRequest::startdelay, "\"-A\""} } ),
    "Vast/CREATIVE#1", ExpectedValues() },
  //Test 3.6
  { "Vast/ACCOUNT", "body", nullptr, "Vast/URL#1",
    RequestParams( { {&RTBRequest::linearity, "\"2\""} } ),
    "Vast/CREATIVE#1", ExpectedValues() },
  //Test 3.7
  { "Vast/ACCOUNT", "body", nullptr, "Vast/URL#1",
    RequestParams( { {&RTBRequest::ext_adtype, "\"1\""} } ),
    "Vast/CREATIVE#1", ExpectedValues() }
};

const OpenRTBExtensionsTest::CaseRequest OpenRTBExtensionsTest::ALLYES_REQUESTS[] = {
  { "Allyes/ACCOUNT", "body-allyes", "728x90", "Allyes/URL", RequestParams(),
    "Allyes/CREATIVEIDS/default",
    ExpectedValues({
      {&Expected::crid, "Allyes/CREATIVES/default", 1},
      {&Expected::adm, ".*\\bTRACKPIXEL = [^\\s]*ImprTrack\\/pt\\.gif\\?"
                       "(?!.*t=n)(?!.*p=\\${AUCTION_PRICE})"
                       ".*\\bCLICK = {!ssp_click_url}http%3A.*", 0},
      {&Expected::nurl, ALLYES_NURL, 0}}) },

  { "Allyes/ACCOUNT", "iframe-url-allyes", "728x90", "Allyes/URL", RequestParams(),
    "Allyes/CREATIVEIDS/default",
    ExpectedValues({
      {&Expected::crid, "Allyes/CREATIVES/default", 1},
      {&Expected::adm, "<iframe src=\"(?!.*p=\\${AUCTION_PRICE})"
                       "(?=.*preclick={!ssp_click_url_esc}).*"
                       "<\\/iframe>", 0},
      {&Expected::nurl, ALLYES_NURL, 0}}) },

  { "Allyes/ACCOUNT", "body-allyes", "468x60", 0, RequestParams(), 
    "Allyes/CREATIVEIDS/text1",
    ExpectedValues({
      {&Expected::crid, "Allyes/CREATIVES/text1", 1},
      {&Expected::adm, ".*\\bTRACKPIXEL = .*ImprTrack\\/pt\\.gif(?!.*t=n)(?!.*p=\\${AUCTION_PRICE})"
                       ".*\\bCREATIVES_JSON = \\[{.*\"CCID\":\"\\d+\".*},{.*\"CCID\":\"\\d+\".*}\\].*", 0},
      {&Expected::nurl, ALLYES_NURL, 0}}) },

  { "Allyes/ACCOUNT", "iframe-url-allyes", "468x60", 0, RequestParams(), 
    "Allyes/CREATIVEIDS/text1",
    ExpectedValues({
      {&Expected::crid, "Allyes/CREATIVES/text1", 1},
      {&Expected::adm, ".*\\/inst\\?"
                       "(?!.*t=n)(?!.*p=\\${AUCTION_PRICE})"
                       "(?=.*preclick={!ssp_click_url_esc})"
                       ".*ad=\\d+,\\d+.*", 0},
      {&Expected::nurl, ALLYES_NURL, 0}}) },

  { "Allyes/ACCOUNT", "body-allyes", "160x600", "Allyes/URL", RequestParams(), 
    "Allyes/CREATIVEIDS/no_imp_track",
    ExpectedValues({
      {&Expected::crid, "Allyes/CREATIVES/no_imp_track", 1},
      {&Expected::adm, ".*\\bTRACKPIXEL = \\n.*", 0},
      {&Expected::nurl, ALLYES_NURL, 0}}) },

  { "Allyes/ACCOUNT", "iframe-url-allyes", "160x600", "Allyes/URL", RequestParams(),
    "Allyes/CREATIVEIDS/no_imp_track",
    ExpectedValues({
      {&Expected::crid, "Allyes/CREATIVES/no_imp_track", 1},
      {&Expected::adm, ".*\\/inst\\?(?!.*p=\\${AUCTION_PRICE}).*", 0},
      {&Expected::nurl, ALLYES_NURL, 0}}) },


  { "Allyes/ACCOUNT", "body-allyes", "240x400", "Allyes/URL", RequestParams(), 
    "Allyes/CREATIVEIDS/cat_fill",
    ExpectedValues({
      {&Expected::fmt, "CATEGORIES/visual/allyes_key", 1},
      {&Expected::cat, "CATEGORIES/content/allyes_key", 1}}) },

  { "Allyes/ACCOUNT", "body-openrtb-notice", "728x90", "Allyes/URL", RequestParams(),
    "Allyes/CREATIVEIDS/default",
    ExpectedValues({
      {&Expected::nurl, ALLYES_NURL, 0}}) },

  { "Allyes/ACCOUNT", "iframe-url-adriver", "728x90", "Allyes/URL", RequestParams(),
    "Allyes/CREATIVEIDS/default",
    ExpectedValues({
      {&Expected::nurl, ALLYES_NURL, 0}}) }
};

template<size_t COUNT>
void
OpenRTBExtensionsTest::process_requests_(
  const OpenRTBExtensionsTest::CaseRequest (&requests)[COUNT], bool send_banner /*= true*/)
{
  for (size_t i = 0; i < COUNT; ++i)
  {
    AdClient client(AdClient::create_user(this));

    client.process_request(UserBindRequest().
      ssp_user_id(client.get_uid()).
      src(requests[i].src));

    RTBRequest request(AutoTest::OpenRtb::RF_SET_DEFS | (send_banner ? AutoTest::OpenRtb::RF_SEND_BANNER : 0));
    request.
      debug_ccg("21674705").
      external_user_id(client.get_uid()).
      aid(fetch_int(requests[i].aid)).
      src(requests[i].src);

    if (requests[i].referer)
    {
      request.referer(fetch_string(requests[i].referer));
    }

    if (requests[i].size)
    {
      request.debug_size(fetch_string(std::string("Sizes/") + requests[i].size));
    }

    for (auto it = requests[i].request_params.begin(); it != requests[i].request_params.end(); ++it)
    {
      it->member(request, it->need_fetch ? fetch_string(it->value) : it->value);
    }

    client.process_post(request);

    if (!requests[i].adid)
    {
      NOSTOP_FAIL_CONTEXT(AutoTest::equal_checker(STATUS_CODE_NO_CONTENT, client.req_status()).check(), "Check OpenRTB response for expected values");
    }
    else
    {
      Expected expected;
      expected.adid(fetch_int(requests[i].adid));

      for (auto it = requests[i].expected_values.begin(); it != requests[i].expected_values.end(); ++it)
      {
        it->setter(expected,
          it->need_fetch ? fetch_string(it->value) : std::string(it->value));
      }

      NOSTOP_FAIL_CONTEXT(
        OpenRTBResponseChecker(client, expected).check(),
        "Check OpenRTB response for expected values");
    }
  }
}

#define copy_attr(attr) \
  if (requests[i].request_params[j].attr) \
    request.imp[j].attr(requests[i].request_params[j].attr);

template<size_t COUNT>
void OpenRTBExtensionsTest::process_multislotcase_requests_(const OpenRTBExtensionsTest::MultislotCaseRequest (&requests)[COUNT])
{
  for (size_t i = 0; i < COUNT; ++i)
  {
    AdClient client(AdClient::create_user(this));

    client.process_request(UserBindRequest().
        ssp_user_id(client.get_uid()).
        src(requests[i].src));

    RTBRequest request(AutoTest::OpenRtb::RF_SET_DEFS);
    request.
        //debug_ccg(fetch_int("Vast/CCG#1")).
        external_user_id(client.get_uid()).
        aid(fetch_int(requests[i].aid)).
        src(requests[i].src);

    if (requests[i].referer)
    {
      request.referer(fetch_string(requests[i].referer));
    }


    for (size_t j = 0; j < requests[i].request_params.size(); ++j)
    {
      copy_attr(id            )
      copy_attr(mimes         )
      copy_attr(minduration   )
      copy_attr(maxduration   )
      copy_attr(protocol      )
      copy_attr(playbackmethod)
      copy_attr(startdelay    )
      copy_attr(linearity     )
      copy_attr(ext_adtype    )
    }

    client.process_post(request);

    ExpectedList expected_list;
    for (auto it1 = requests[i].expected_values_list.begin(); it1 != requests[i].expected_values_list.end(); ++it1)
    {
      Expected expected;
      for (auto it2 = it1->begin(); it2 != it1->end(); ++it2)
      {
        it2->setter(expected, it2->need_fetch ? fetch_string(it2->value) : std::string(it2->value));
      }
      expected_list.push_back(expected);
    }

    NOSTOP_FAIL_CONTEXT(OpenRTBResponseChecker(client, expected_list).check(), "Check OpenRTB response for expected values");
  }
}

void
OpenRTBExtensionsTest::pre_condition()
{

}

void
OpenRTBExtensionsTest::openx_slow_()
{
  std::string prefix("OpenX");

  const unsigned long cc1 = fetch_int(prefix + "/CC#1");
  const unsigned long cc2 = fetch_int(prefix + "/CC#2");

  ORM::StatsArray<ORM::HourlyStats, 4> stats;
  stats[0].table(ORM::HourlyStats::RequestStatsHourlyTest).key().cc_id(cc1);
  stats[1].key().cc_id(cc1);
  stats[2].table(ORM::HourlyStats::RequestStatsHourlyTest).key().cc_id(cc2);
  stats[3].key().cc_id(cc2);
  stats.select(pq_conn_);

  process_requests_(OPENX_REQUESTS_SLOW);

  ORM::HourlyStats::Diffs diffs[4]= {
    // There's only test stats for 1st creative
    ORM::HourlyStats::Diffs().requests(1),
    ORM::HourlyStats::Diffs(0),
    // There's only non test stats for 2nd creative
    ORM::HourlyStats::Diffs(0),
    ORM::HourlyStats::Diffs().requests(2)
  };

  add_wait_checker("Check stats for OpenX RTB requests",
    AutoTest::stats_diff_checker(pq_conn_, diffs, stats));
}

bool
OpenRTBExtensionsTest::run()
{
  AUTOTEST_CASE(process_requests_              (OPENX_REQUESTS),          OPENX_CASE );
  AUTOTEST_CASE(process_requests_              (VAST_REQUESTS, false),    VAST_CASE  );
  AUTOTEST_CASE(process_multislotcase_requests_(VAST_MULTISLOT_REQUESTS), VAST_CASE  );
  AUTOTEST_CASE(process_requests_              (VAST_NEGATIVE_REQUESTS),  VAST_CASE  );
  AUTOTEST_CASE(process_requests_              (ALLYES_REQUESTS),         ALLYES_CASE);

  return true;
}

void
OpenRTBExtensionsTest::post_condition()
{
  AUTOTEST_CASE(openx_slow_(), OPENX_CASE);
}

void
OpenRTBExtensionsTest::tear_down()
{}
