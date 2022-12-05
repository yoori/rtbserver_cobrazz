#include "InvalidURLParamTest.hpp"
#include <String/StringManip.hpp>
#include <String/RegEx.hpp>
#include <Generics/Rand.hpp>

REFLECT_UNIT(InvalidURLParamTest) (
  "Frontend",
  AUTO_TEST_FAST
);

namespace
{
#define INVALIDURLPARAMTEST_MAX_UINT      "18446744073709551615"
#define INVALIDURLPARAMTEST_OVERFLOW_UINT "18446744073709551616"
#define INVALIDURLPARAMTEST_MAX_INT "9223372036854775807"
#define INVALIDURLPARAMTEST_OVERFLOW_INT "-9223372036854775808"

  static const char ALLOWED[] =
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "0123456789";
  
  void
  fill_random(std::string &str, size_t count)
  {
    str.reserve(str.size() + count);
    for (size_t i = 0; i < count; ++i)
    {
      // -1 to exclude tailing zero.
      size_t index = Generics::safe_rand(sizeof(ALLOWED) - 1);
      str.push_back(ALLOWED[index]);
    }
  }

#define CONCAT_STRINGS(string1,string2, string3)  \
  string1 \
  string2 \
  string3
  
  enum Service
  {
    SVC_NSLOOKUP,
    SVC_NSLOOKUP_SETUID,
    SVC_ADCLICKSERVER,
    SVC_ACTIONSERVER_CID,
    SVC_IMPRTRACK_PTGIF,
    SVC_OO,
  };

  const char *const service_url[] =
  {
    "/get?require-debug-info=header&",
    "/get?require-debug-info=header&setuid=1&",
    "/services/AdClickServer/",
    "/services/ActionServer/",
    "/track.gif?",
    "/services/OO?",
  };

  struct request_reply
  {
    Service service;
    const char*  param;
    const char*  eql;
    const char*  value;
    unsigned int status;
    const char*  header;
    const char*  regexp;
    unsigned int status2;
    bool         post;
  };

  const request_reply tests[] =
  {
    // nslookup
    // tid
    { SVC_NSLOOKUP, "tid", "=", "",
      204, "Debug-Info", "\\btid = 0;", 204, false },
    { SVC_NSLOOKUP, "tid", "=", "0",
      204, "Debug-Info", "\\btid = 0;", 204, false},
    { SVC_NSLOOKUP, "tid", "=", "-1",
      204, "Debug-Info", "\\btid = 0;", 204, false },
    { SVC_NSLOOKUP, "tid", "=", INVALIDURLPARAMTEST_OVERFLOW_INT,
      204, "Debug-Info", "\\btid = 0;", 204, false },
    { SVC_NSLOOKUP, "tid", "=", INVALIDURLPARAMTEST_MAX_INT,
      204, "Debug-Info", CONCAT_STRINGS("\\btid = ", INVALIDURLPARAMTEST_MAX_INT, ";"), 204, false },
    { SVC_NSLOOKUP, "tid", "=", INVALIDURLPARAMTEST_MAX_UINT,
      204, "Debug-Info", CONCAT_STRINGS("\\btid = ", INVALIDURLPARAMTEST_MAX_UINT, ";"), 204, false },
    { SVC_NSLOOKUP, "tid", "=", INVALIDURLPARAMTEST_OVERFLOW_UINT,
      204, "Debug-Info", "\\btid = 0;", 204, false },
    { SVC_NSLOOKUP, "tid", "=", "1s",
      204, "Debug-Info", "\\btid = 0;",204, false },
    { SVC_NSLOOKUP, "tid", "=", "s1",
      204, "Debug-Info", "\\btid = 0;",204, false },
    { SVC_NSLOOKUP, "tid", "=", ",1,2",
     204, "Debug-Info", "\\btid = 0;",204, false },
    { SVC_NSLOOKUP, "tid", "=", "1,,2",
      204, "Debug-Info", "\\btid = 0;",204, false },
    { SVC_NSLOOKUP, "tid", "=", "1,2,",
      204, "Debug-Info", "\\btid = 0;",204, false },
    { SVC_NSLOOKUP, "tid", "=", "1+2",
      204, "Debug-Info", "\\btid = 0;",204, false },

    // country
    { SVC_NSLOOKUP, "country", "=", "abc",
      204, "Debug-Info", "\\blocation = ;",204, false },
    { SVC_NSLOOKUP, "country", "=", "a",
      204, "Debug-Info", "\\blocation = ;",204, false },

    // muid & tuid
    { SVC_NSLOOKUP, "tuid", "=", "",
      400, 0, 0, 400, false },
    { SVC_NSLOOKUP, "tuid", "=", AutoTest::PROBE_UID,
      204, "X-Merge-Failed", "^source profile - probe user$", 204, false },
    { SVC_NSLOOKUP, "muid", "=", AutoTest::PROBE_UID,
      204, "X-Merge-Failed", "^source profile - probe user$", 204, false },
    { SVC_NSLOOKUP, "tuid", "=", "invalid_base64..",
      400, 0, 0, 400, false },
    { SVC_NSLOOKUP, "muid", "=", "invalid_base64..",
      400, 0, 0, 400, false },

    // uid
    // On invalid uid server assigns probe uid.
    { SVC_NSLOOKUP_SETUID, "uid", "=", "",
      204, 0, 0, 204, false },
    { SVC_NSLOOKUP_SETUID, "uid", "=", "invalid_base64..",
      204, 0, 0, 204, false },
    // On probe uid server assigns a valid uid.
    { SVC_NSLOOKUP, "uid", "=", AutoTest::PROBE_UID,
      204, "Debug-Info", "\\bsigned_uid = (?!P{21}A.*)[A-Za-z0-9_-]{65};", 204, false },

    // referer
    { SVC_NSLOOKUP, "referer", "=", "",
      204, "Debug-Info", "\\breferer = ;", 204, false },
    { SVC_NSLOOKUP, "referer", "=", "http://?",
      204, "Debug-Info", "\\breferer = ;", 204, false },

    // AdClickServer
    // ccid
    { SVC_ADCLICKSERVER, "ccid", "*eql*", "",
      200, 0, 0, 200, false },
    { SVC_ADCLICKSERVER, "ccid", "*eql*", "0",
      200, 0, 0, 200, false },
    { SVC_ADCLICKSERVER, "ccid", "*eql*", "-1",
      200, 0, 0, 200, false },
    { SVC_ADCLICKSERVER, "ccid", "*eql*",  INVALIDURLPARAMTEST_MAX_UINT,
      200, 0, 0, 200, false },
    { SVC_ADCLICKSERVER, "ccid", "*eql*", INVALIDURLPARAMTEST_OVERFLOW_UINT,
      200, 0, 0, 200, false },

    // requestid
    { SVC_ADCLICKSERVER, "requestid", "*eql*", "",
      200, 0, 0, 200, false },
    { SVC_ADCLICKSERVER, "requestid", "*eql*", "dfgdsgewg",
      200, 0, 0, 200, false },

    // ch
    { SVC_ADCLICKSERVER, "ch", "*eql*", ",1,-3,0,,5,",
      200, 0, 0, 200, false },

    // ActionServer cid
    { SVC_ACTIONSERVER_CID, "cid", "*eql*", "-1",
      400, 0, 0, 400, false },

    // track.gif
    // requestid
    { SVC_IMPRTRACK_PTGIF, "requestid", "=", "random_text",
      200, 0, 0, 200, false },

    // OO (OptOut)
    // op
    { SVC_OO, "op", "=", "garbage",
      400, 0, 0, 400, false },
    { SVC_OO, "op", "=", "",
      400, 0, 0, 400, false },

    // fail_url
    { SVC_OO, "op", "=", "in&fail_url=http://?",
      400, 0, 0, 400, false },

    // REQ-1734
    { SVC_NSLOOKUP, "referer-kw", "=", "?RANDOM540",
      204, 0, 0, 204, false }, //76 get

    { SVC_NSLOOKUP, "referer-kw", "=", "?RANDOM540",
      204, 0, 0, 204, true }, //77 post

    { SVC_NSLOOKUP, "referer-kw", "=", "?RANDOM541",//first bad for any other param ?
      204, 0, 0, 204, false }, //78 get

    { SVC_NSLOOKUP, "referer-kw", "=", "?RANDOM541",
      204, 0, 0, 204, true }, //79 post
    ////// referer
    { SVC_NSLOOKUP, "referer", "=", "?RANDOM4096",
      204, 0, 0, 204, false }, //80 get

    { SVC_NSLOOKUP, "referer", "=", "?RANDOM4096",
      204, 0, 0, 204, true }, //81 post

    { SVC_NSLOOKUP, "referer", "=", "?RANDOM4097",//first bad for referer
      400, 0, 0, 400, false }, //82 get

    { SVC_NSLOOKUP, "referer", "=", "?RANDOM4097",
      204, 0, 0, 400, true }, //83 post
    ////// referer-kw
    { SVC_NSLOOKUP, "referer-kw", "=", "?RANDOM6144",
      204, 0, 0, 204, false }, //84 get

    { SVC_NSLOOKUP, "referer-kw", "=", "?RANDOM6144",
      204, 0, 0, 204, true }, //85 post

    { SVC_NSLOOKUP, "referer-kw", "=", "?RANDOM6145",//first bad get for referer-kw
      400, 0, 0, 204, false }, //86 get

    { SVC_NSLOOKUP, "referer-kw", "=", "?RANDOM6145",
      204, 0, 0, 204, true }, //87 post
    //////
    { SVC_NSLOOKUP, "referer-kw", "=", "?RANDOM8180", //first bad post for referer-kw
      204, 0, 0, 400, true }, //88 post

    { SVC_NSLOOKUP, "referer-kw", "=", "?RANDOM10000",
      204, 0, 0, 400, true }, //89 post

    { SVC_NSLOOKUP, "referer-kw", "=", "?RANDOM10204", 
      204, 0, 0, 400, true }, //90 post

     { SVC_NSLOOKUP, "referer-kw", "=", "?RANDOM30500", 
       204, 0, 0, 400, true }, //91 post
    
    { SVC_NSLOOKUP, "referer-kw", "=", "?RANDOM30684", 
      413, 0, 0, 400, true }, //92 post
  };
}


void
scenario(AutoTest::AdClient &client,
         const char *url, const request_reply *test, unsigned int status)
{
  try
  {
    if (test->post)
    {
      client.process_post_request(url);
    }
    else
    {
      client.process_request(url);
    }
  }
  catch (...)
  {
    // "400 Bad Request" throws an exception.  Do nothing.
  }

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      status,
      client.req_status()).check(),
    "must got expected request status");

  if (test->header)
  {
    std::string value;
    
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        client.find_header_value(test->header, value)),
      std::string(test->header) + " not found");
 
    if (test->regexp)
    {
      String::RegEx regexp(String::SubString(test->regexp));
      FAIL_CONTEXT(
        AutoTest::predicate_checker(
          regexp.match(value)),
        std::string("Header '") + test->header
        + ":' doesn't match '" + test->regexp
        + "'");
    }
  }
}


bool
InvalidURLParamTest::run_test()
{
  std::string description = "run tests count: " + strof(countof(tests));
  add_descr_phrase(description.c_str());
  for (size_t i = 0; i < countof(tests); ++i)
  {

    // Do not send probe uid request if uid parameter testing, because
    // client uid sending in cookie have more priority than uid in url 
    AutoTest::AdClient
      client(strcmp(tests[i].param, "uid") != 0?
         AutoTest::AdClient::create_user(this) :
           AutoTest::AdClient::create_undef_user(this));
  
    const char *service = service_url[tests[i].service];
    std::string url = service;
    std::string value;
    const char* raw = tests[i].value;
    if (raw[0] == '?')
    {
      if (!strncmp(raw + 1, "RANDOM", 6))
      {
        int len = atoi(raw + 7);
        if(len)
        {
          fill_random(value, len);
        }
        else
        {
          throw AutoTest::Exception("bad random command for parameter");
        }
      }
      else
      {
        throw AutoTest::Exception("unknown command for parameter");
      }
      url = url + tests[i].param + tests[i].eql + value;
    }
    else
    {
      String::StringManip::mime_url_encode(String::SubString(raw), value);
      url = url + tests[i].param + tests[i].eql + value;
      value = raw;
    }

    std::string description = strof(i) + "@" + "URL params: " + url;
    add_descr_phrase(description.c_str());
    NOSTOP_FAIL_CONTEXT(
      {
        scenario(client, url.c_str(), &tests[i], tests[i].status);
      },
      description);

    switch (tests[i].service)
    {
      default:
        continue;

      case SVC_NSLOOKUP:
        break;
    }

    description = strof(i) + "@" + std::string("Header params: ") + service + " : "
      + tests[i].param + ": " + value;
    add_descr_phrase(description);
    client.add_http_header(tests[i].param, value);
    NOSTOP_FAIL_CONTEXT(
      {
        scenario(client, service, &tests[i], tests[i].status2);
      },
      description);
  }

  return true;
}
