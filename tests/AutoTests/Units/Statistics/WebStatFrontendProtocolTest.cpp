
#include <algorithm>
#include "WebStatFrontendProtocolTest.hpp"

REFLECT_UNIT(WebStatFrontendProtocolTest) (
  "Statistics",
  AUTO_TEST_FAST | AUTO_TEST_SLOW);

namespace
{
  typedef AutoTest::OptOutRequest OptOutRequest;
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::RedirectChecker RedirectChecker;

  enum OoOperation { OO_IN, OO_OUT, OO_STATUS };
  
  const char* OPTOUT_IN = "in";
  const char* OPTOUT_OUT = "out";
  const char* OPTOUT_STATUS = "status";
  const char* SUCCESS_URL = "http://cs.ocslab.com/opt-in/success.html";
  const char* ALREADY_URL = "http://cs.ocslab.com/opt-in/already.html";
  const char* FAIL_URL = "http://cs.ocslab.com/opt-in/fail.html";
  const char* INVALID_UID =
    "wr4q7EmtTN-tiUfV-zIGOAQEd49aaF3FCsbqM4R5Tc37Mq8yzC4faGrWXpdj2gJ3W";

  struct  OoTestCase
  {
    const char* curst;
    const char* ct;
    OoOperation operation;
    const char* result;
    const char* user_status;
    unsigned short testrequest;
    bool ct_encode;
  };

  struct CCTagCase
  {
    const char* cc;
    const char* tag;
    const char* app;
    const char* op;
    const char* src;
    const char* res;
    unsigned long flags;
  };

  const char*
  oo_to_str(
    OoOperation operation)
  {
    switch(operation)
    {
    case OO_IN:
      return OPTOUT_IN;
    case OO_OUT:
      return OPTOUT_OUT;
    default:
      return OPTOUT_STATUS;        
    }
  }

  unsigned int
  oo_state_to_status(
    OoOperation prev_operation,
    OoOperation current_operation)
  {
    return prev_operation == current_operation?
      2: 11;
  }

  const char*
  oo_state_to_redirect(
    OoOperation prev_operation,
    OoOperation current_operation)
  {
    return prev_operation == current_operation?
      ALREADY_URL: SUCCESS_URL;
  }
  
  enum CaseOption
  {
    CO_PARAM_NAME = 1,  // need get param value by name
    CO_POST = 2,        // use HTTP POST request
    CO_GEN_CURCT = 4,   // Generate ct cookie
    CO_CHECK_CORS = 8,  // Check CORS HTTP Headers
    CO_NO_UA = 16,      // Empty User-Agent
    CO_SKIP_STAT = 32,  // Don't create stat for case
    CO_NULL_CT = 64,    // Don't log ct
    CO_NULL_CURCT = 128 // Don't log curct
  };

  const char* EMPTY = "";
  const char* MISSED = "FBA2BD536387494dB930A6484AA939FB";
  const char* CT_SIZE_50 = "12345678901234567890123456789012345678901234567890";
  const char* CT_SIZE_51 = "123456789012345678901234567890123456789012345678901";
  const char* ORIGIN_HEADER = "http://cs.ocslab.com";

  const WebStatFrontendProtocolTest::TestCase BASE_CASE[] =
  {
    {"F", 0, 200, 0}
  };
}

const char* WebStatFrontendProtocolTest::BROWSER = "Firefox 19.0";
const char* WebStatFrontendProtocolTest::OS = "windows nt 6.0";
const char* WebStatFrontendProtocolTest::WebStatRequest::BASE_URL = "/sl.gif";

WebStatFrontendProtocolTest::WebStatRequest::WebStatRequest()
  : BaseRequest(
      BASE_URL,
      BaseRequest::RT_ENCODED),
    res(this, "res", "U", true),
    app(this, "app"),
    src(this, "src"),
    ct(this,  "ct", "UnitTest-CT", true),
    op(this,  "op"),
    testrequest(this, "testrequest"),
    time(this, "debug.time"),
    user_agent(this, "User-Agent", AutoTest::DEFAULT_USER_AGENT, true),
    origin(this, "Origin"),
    ccid(this,  "ccid"),
    tid(this,  "tid")
{ }

WebStatFrontendProtocolTest::WebStatRequest::WebStatRequest(
  const WebStatRequest& other)
  : BaseRequest(
      BASE_URL,
      BaseRequest::RT_ENCODED),
    res(this, other.res),
    app(this, other.app),
    src(this, other.src),
    ct(this, other.ct),
    op(this, other.op),
    testrequest(this, other.testrequest),
    time(this, other.time),
    user_agent(this, other.user_agent),
    origin(this, other.origin),
    ccid(this, other.ccid),
    tid(this, other.tid)
{ }

bool
WebStatFrontendProtocolTest::WebStatRequest::need_encode() const
{
  return false;
}

template<size_t Count>
void
WebStatFrontendProtocolTest::process_(
  AutoTest::AdClient& client,
  const TestCase(&tests)[Count],
  WebStatsList& web_stats,
  Member member,
  const char* user_status,
  const WebStatRequest& base_request,
  const std::string& os,
  const std::string& browser)
{
  for (size_t i = 0; i < Count; ++i)
  {
    WebStatRequest request(base_request);
    request.time(now_);
    if (tests[i].value)
    {
      member(request,
        tests[i].flags & CO_PARAM_NAME?
          fetch_string(tests[i].value): tests[i].value);
    }
    else
    {
      member.clear(request);
    }

    std::string curct =
      tests[i].flags & CO_GEN_CURCT?
         Generics::Uuid::create_random_based().to_string():
      !tests[i].ct_cookie? EMPTY:
         tests[i].flags & CO_PARAM_NAME?
           fetch_string(tests[i].ct_cookie): tests[i].ct_cookie;

    if ( !curct.empty() )
    {
      client.set_cookie_value("ct", curct.c_str(), false);
    }
    else
    {
      client.get_cookies().remove_cookie("ct");
    }

    if (tests[i].status == 200 && stat_.db_active() &&
      (tests[i].flags & CO_SKIP_STAT) == 0)
    {
      std::string ct(request.ct.raw_str());
      {
        String::StringManip::mime_url_decode(ct);
        std::replace( ct.begin(), ct.end(), '+', ' ');
      }

      ORM::WebStats::Key key;
      key.colo_id(1);
      key.ct(ct.length() <= 50? tests[i].flags & CO_NULL_CT? "": ct: "");
      key.curct(curct.length() <= 50? tests[i].flags & CO_NULL_CURCT? "": curct: "");
      key.app(request.app.raw_str());
      key.result(
        request.res.empty()? "U":
          toupper(request.res.raw_str()));
      key.operation(request.op.raw_str());
      key.source(request.src.raw_str());
      key.user_status(user_status);
      if (tests[i].flags & CO_NO_UA)
      {
        key.os("");
        key.browser("");
      }
      else
      {
        key.os(os);
        key.browser(browser);
      }
      key.test(
        !request.testrequest.empty() && request.testrequest.raw_str() == "1"); 
      key.country_sdate(now_);
      key.stimestamp(now_);
      
      ORM::WebStats stat(key);
      stat.description("#" + strof(i+1));
      stat.select(pq_conn_);
      web_stats.push_back(stat);
    }

    unsigned int status =
      tests[i].flags & CO_POST?
      client.process_post(request, true):
          client.process(request , true);
    
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        tests[i].status,
        status).check(),
      "Check HTTP status#" + strof(i+1));

    // Check Access-Control-Allow... HTTP headers
    if (tests[i].flags & CO_CHECK_CORS)
    {
      std::string value;
      
      FAIL_CONTEXT(
        AutoTest::predicate_checker(
          client.find_header_value("Access-Control-Allow-Origin", value)),
        "Check Access-Control-Allow-Origin#" + strof(i+1));
      
      FAIL_CONTEXT(
        AutoTest::equal_checker(
          request.origin.raw_str(),
          value).check(),
        "Check Access-Control-Allow-Origin value#" + strof(i+1));

      FAIL_CONTEXT(
        AutoTest::predicate_checker(
          client.find_header_value("Access-Control-Allow-Credentials", value)),
        "Access-Control-Allow-Credentials#" + strof(i+1));

      FAIL_CONTEXT(
        AutoTest::equal_checker(
          "true",
          value).check(),
        "Check Access-Control-Allow-Credentials value#" + strof(i+1));
    }

  }

}

void
WebStatFrontendProtocolTest::oo_cases_()
{
  WebStatsList web_stats;
  OOStatsList oo_stats;

  const unsigned int colo = fetch_int("Colo");
  std::string CURST_1 =
    Generics::Uuid::create_random_based().to_string();
  std::string CURST_2 =
    Generics::Uuid::create_random_based().to_string();

  const OoTestCase TEST_CACES[] =
  {
    { CURST_2.c_str(), CURST_1.c_str(), OO_IN, "S", "U", 0, true },
    { "a=B", "a=B", OO_IN, "F", "I", 0, false },
    {
      fetch_string("CURCT/1").c_str(),
      fetch_string("CT/1").c_str(),
      OO_IN, "F", "I", 0, false
    },
    { CT_SIZE_50, CT_SIZE_51, OO_OUT, "S", "I", 0, true },
    { CT_SIZE_51, CT_SIZE_50, OO_OUT, "F", "O", 0, true },
    { CURST_1.c_str(), 0, OO_IN, "S", "P", 1, true }, // Known bug ADSC-8579
    { CURST_1.c_str(), 0, OO_STATUS, "S", "U", 0, true }
  };

  AdClient client(AdClient::create_nonoptin_user(this));

  OoOperation prev_op = OO_OUT;
    
  for (unsigned int i = 0; i < countof(TEST_CACES); ++i)
  {
    // Prepare user for last 2 cases
    if ( i >= 5 )
    {
      client.get_cookies().clear();
    }
    // Probe user
    if ( i == 5 )
    {
      client.set_probe_uid();
    }
    // Invalid user
    if ( i == 6 )
    {
      client.set_uid(INVALID_UID, false);
    }

    if (stat_.db_active() && i != 2)
    {
      {
        ORM::WebStats::Key key;
        key.colo_id(colo);
        key.app("adserver");
        key.source("oo");
        key.test(TEST_CACES[i].testrequest);
        key.operation(oo_to_str(TEST_CACES[i].operation));
        key.result(TEST_CACES[i].result);
        key.user_status(TEST_CACES[i].user_status);
        key.country_sdate(now_);
        if (TEST_CACES[i].operation == OO_STATUS)
        {

          key.stimestamp(
            AutoTest::Time(now_.get_gm_time().get_date()));
          key.os("");
          key.browser("");
          key.ct("");
          key.curct("");
        }
        else
        {
          key.ct(
            TEST_CACES[i].ct && strlen(TEST_CACES[i].ct) <= 50?
              i==1? "":TEST_CACES[i].ct: "");
          key.curct(
            !TEST_CACES[i].curst || strlen(TEST_CACES[i].curst) > 50?
            "":  i==1? "": TEST_CACES[i].curst);
          key.os(OS);
          key.browser(BROWSER);
          key.stimestamp(now_);
        }
    
        ORM::WebStats stat(key);
        stat.description("#" + strof(i+1));
        stat.select(pq_conn_);
        web_stats.push_back(stat);
      }

      if (TEST_CACES[i].operation != OO_STATUS )
      {
        ORM::OptOutStats::Key key;
        key.isp_sdate(now_);
        key.colo_id(colo);
        key.status(
          oo_state_to_status(
            prev_op, TEST_CACES[i].operation));
        key.operation(TEST_CACES[i].operation == OO_IN? "I": "O");
        key.test(TEST_CACES[i].testrequest? "Y": "N");
      
        ORM::OptOutStats stat(key);
        stat.description("#" + strof(i+1));
        stat.select(pq_conn_);
        oo_stats.push_back(stat);
      }
    }

    if (TEST_CACES[i].curst)
    {
      client.set_cookie_value("ct", TEST_CACES[i].curst, false);
    }
    else
    {
      client.get_cookies().remove_cookie("ct");
    }

    OptOutRequest request;
    
    request.
      op(oo_to_str(TEST_CACES[i].operation)).
      testrequest(TEST_CACES[i].testrequest).
      colo(colo).
      debug_time(now_).
      ct(TEST_CACES[i].ct).
      success_url(SUCCESS_URL).
      already_url(ALREADY_URL).
      fail_url(FAIL_URL);

    if(!TEST_CACES[i].ct_encode)
    {
      request.ct.not_encode();
    }
    
    unsigned long status = 
      client.process(request, true);

    if (TEST_CACES[i].operation == OO_STATUS)
    {
      FAIL_CONTEXT(
        AutoTest::equal_checker(
          200,
          status).check(),
        "Check HTTP status#" + strof(i+1));
    }
    else
    {
      FAIL_CONTEXT(
        RedirectChecker(
          client,
          oo_state_to_redirect(prev_op, TEST_CACES[i].operation)).check(),
        "Check redirection#"  + strof(i+1));
    }

    prev_op = TEST_CACES[i].operation;
  }

  ADD_WAIT_CHECKER(
    "WebStats check",
    AutoTest::stats_diff_checker(
      pq_conn_,
      { 1, 2, 1, 1, 1, 1 },
      web_stats));

  ADD_WAIT_CHECKER(
    "OptOutStats check",
    AutoTest::stats_diff_checker(
      pq_conn_,
      { 1, 2, 1, 1, 1 },
      oo_stats));
}

void
WebStatFrontendProtocolTest::cc_tag_cases_(
  unsigned short client_flags,
  CCTagCaseEnum ccid_log_mask)
{

  AdClient client(AdClient::create_user(this, client_flags));
  
  WebStatsList web_stats;
  
  const CCTagCase CASES[] =
  {
    // #1
    {
      fetch_string("CC/ACTIVE").c_str(),
      fetch_string("TAG/ACTIVE").c_str(),
      "App/9",
      "Operation/9",
      "Source/9",
      "U",
      CCTCE_LOG_CCID | CCTCE_LOG_TID
    },
    // #2
    {
      fetch_string("CC/ACTIVE").c_str(),
      fetch_string("TAG/INACTIVE").c_str(),
      "App/10",
      "Operation/10",
      "Source/10",
      "U",
      CCTCE_LOG_CCID
    },
    // #3
    {
      fetch_string("CC/INACTIVECC").c_str(),
      fetch_string("TAG/ACTIVE").c_str(),
      "App/11",
      "Operation/11",
      "Source/11",
      "U",
      CCTCE_LOG_TID
    },
    // #4
    {
      "",
      "",
      "App/12",
      "Operation/12",
      "Source/12",
      "U",
      0
    },
    // #5
    {
      "abc",
      "abc",
      "App/13",
      "Operation/13",
      "Source/13",
      "S",
      0
    },
    // #6
    {
      "1.1",
      "1.1",
      "App/14",
      "Operation/14",
      "Source/14",
      "F",
      0
    },
    // #7
    {
      "99999999999",
      "99999999999",
      "App/15",
      "Operation/15",
      "Source/15",
      "U",
      0
    },
    // #8
    {
      fetch_string("CC/INACTIVECCG").c_str(),
      fetch_string("TAG/ACTIVE").c_str(),
      "App/16",
      "Operation/16",
      "Source/16",
      "S",
      CCTCE_LOG_TID | CCTCE_LOG_CCID_CENTRAL
    },
    // #9
    {
      fetch_string("CC/ACTIVE").c_str(),
      fetch_string("TAG/ACTIVE").c_str(),
      "App/20",
      "Operation/20",
      "Source/20",
      "F",
      CCTCE_LOG_CCID | CCTCE_LOG_TID | CCTCE_NO_LOG_ALL
    },
    // #10
    {
      fetch_string("CC/ACTIVE").c_str(),
      fetch_string("TAG/ACTIVE").c_str(),
      "App/21",
      "Operation/21",
      "Source/21",
      "F",
      CCTCE_LOG_TID
    },
    // #11
    {
      fetch_string("CC/ACTIVE").c_str(),
      fetch_string("TAG/ACTIVE").c_str(),
      "App/22",
      "Operation/22",
      "Source/22",
      "F",
      CCTCE_LOG_CCID
    }
  };

  for (size_t i = 0; i < countof(CASES); ++i)
  {
    
    std::string curct =
      Generics::Uuid::create_random_based().to_string();

    std::string ct =
      Generics::Uuid::create_random_based().to_string();

    WebStatRequest request;
    request.time(now_);
    request.ccid(CASES[i].cc);
    request.tid(CASES[i].tag);
    request.ct = ct;
    request.app = fetch_string(CASES[i].app);
    request.src = fetch_string(CASES[i].src);
    request.op = fetch_string(CASES[i].op);
    request.res = CASES[i].res;
    
    client.set_cookie_value("ct", curct.c_str(), false);

    FAIL_CONTEXT(
      AutoTest::equal_checker(
        200,
        client.process(request)).check(),
      "Check HTTP status#" + strof(i+1));

    if (stat_.db_active())
    {
      ORM::WebStats::Key key;
      key.ct(CASES[i].flags & CCTCE_NO_LOG_CT? "":ct);
      key.curct(CASES[i].flags & CCTCE_NO_LOG_CURCT? "": curct);
      key.app(request.app.raw_str());
      key.result(
        request.res.empty()? "U":
          toupper(request.res.raw_str()));
      key.operation(request.op.raw_str());
      key.source(request.src.raw_str());
      key.user_status("I");
      key.os(CASES[i].flags & CCTCE_NO_LOG_OS? "": OS);
      key.browser(CASES[i].flags & CCTCE_NO_LOG_BROWSER? "": BROWSER);
      key.test(false);
      key.country_sdate(now_);
      key.stimestamp(CASES[i].flags & CCTCE_NO_LOG_HOUR?
        AutoTest::Time(now_.get_gm_time().get_date()): now_);
      key.cc_id(
        CASES[i].flags & ccid_log_mask? atoi(CASES[i].cc): 0);
      key.tag_id(
        CASES[i].flags & CCTCE_LOG_TID? atoi(CASES[i].tag): 0);
      
      ORM::WebStats stat(key);
      stat.description("#" + strof(i+1));
      stat.select(pq_conn_);
      
      web_stats.push_back(stat);
    }
  }

  ADD_WAIT_CHECKER(
    "WebStats check",
    AutoTest::stats_each_diff_checker(
      pq_conn_,
      1,
      web_stats));
}

void
WebStatFrontendProtocolTest::pre_condition()
{  }

void
WebStatFrontendProtocolTest::invalid_uid()
{
  WebStatsList web_stats;

  AdClient client(AdClient::create_user(this));
  client.set_uid(INVALID_UID, false);

  const TestCase CASE[] =
  {
    {"F", 0, 200, 0 },
    {"F", 0, 200, CO_SKIP_STAT}
  };

  process_(
    client,
    CASE,
    web_stats,
    &WebStatRequest::res,
    "I",
    WebStatRequest().
      app(fetch_string("App/1")).
      src(fetch_string("Source/1")).
      op(fetch_string("Operation/1")));

  ADD_WAIT_CHECKER(
    "WebStats check",
    AutoTest::stats_each_diff_checker(
      pq_conn_,
      2,
      web_stats));
}

void
WebStatFrontendProtocolTest::probe_uid()
{
  AdClient client(AdClient::create_user(this));
  client.set_probe_uid();

  WebStatsList web_stats;
  
  process_(
    client,
    BASE_CASE,
    web_stats,
    &WebStatRequest::res,
    "P",
    WebStatRequest().
      app(fetch_string("App/2")).
      src(fetch_string("Source/2")).
      op(fetch_string("Operation/2")));

  ADD_WAIT_CHECKER(
    "WebStats check",
    AutoTest::stats_each_diff_checker(
      pq_conn_,
      1,
      web_stats));
}

void
WebStatFrontendProtocolTest::res_param(
  AdClient& client)
{

  WebStatsList web_stats;

  const TestCase RES_CASES[] =
  {
    { 0, 0, 200, CO_GEN_CURCT },
    { EMPTY, 0, 400, CO_GEN_CURCT },
    { "FAIL", 0, 400, CO_GEN_CURCT },
    { "A", 0, 400, CO_GEN_CURCT },
    {"u", 0, 200, CO_GEN_CURCT}
  };
  
  process_(
    client,
    RES_CASES,
    web_stats,
    &WebStatRequest::res,
    "U",
    WebStatRequest().
      app(fetch_string("App/3")).
      src(fetch_string("Source/3")).
      op(fetch_string("Operation/3")));

  ADD_WAIT_CHECKER(
    "WebStats check",
    AutoTest::stats_each_diff_checker(
      pq_conn_,
      1,
      web_stats));
}

void
WebStatFrontendProtocolTest::testrequest_param(
  AdClient& client)
{
  WebStatsList web_stats;

  const TestCase TESTREQUEST_CASES[] =
  {
    { EMPTY, 0, 200, CO_GEN_CURCT },
    { "0", 0, 200, CO_GEN_CURCT },
    { "1", 0, 200, CO_GEN_CURCT }
  };

  process_(
    client,
    TESTREQUEST_CASES,
    web_stats,
    &WebStatRequest::testrequest,
    "U",
    WebStatRequest().
    res("S").
      app(fetch_string("App/4")).
      src(fetch_string("Source/4")).
      op(fetch_string("Operation/4")));

  ADD_WAIT_CHECKER(
    "WebStats check",
    AutoTest::stats_each_diff_checker(
      pq_conn_,
      1,
      web_stats));
}

void
WebStatFrontendProtocolTest::post_cors(
  AdClient& client)
{
  WebStatsList web_stats;

  const TestCase POST_CORS_CASES[] =
  {
    { "fake", 0, 400, CO_GEN_CURCT},
    { "fake", 0, 400, CO_GEN_CURCT},
    { "App/1", 0, 200, CO_GEN_CURCT | CO_CHECK_CORS | CO_PARAM_NAME},
    { "App/1", 0, 200, CO_GEN_CURCT | CO_CHECK_CORS | CO_PARAM_NAME}
  };

  
  process_(
    client,
    POST_CORS_CASES,
    web_stats,
    &WebStatRequest::app,
    "U",
    WebStatRequest().
      op(fetch_string("Operation/1")).
      src(fetch_string("Source/1")).
      origin(ORIGIN_HEADER));

  ADD_WAIT_CHECKER(
    "WebStats check",
    AutoTest::stats_each_diff_checker(
      pq_conn_,
      1,
      web_stats));
}

void
WebStatFrontendProtocolTest::app_param(
  AdClient& client)
{
  WebStatsList web_stats;

  const TestCase APP_CASES[] =
  {
    { MISSED, 0, 400, CO_GEN_CURCT },
    { 0, 0, 400, CO_GEN_CURCT },
    { EMPTY, 0, 400, CO_GEN_CURCT }
  };

  process_(
    client,
    APP_CASES,
    web_stats,
    &WebStatRequest::app,
    "O",
    WebStatRequest().
      op(fetch_string("Operation/5")).
      src(fetch_string("Source/5")));

  ADD_WAIT_CHECKER(
    "WebStats check",
    AutoTest::stats_each_diff_checker(
      pq_conn_,
      1,
      web_stats));
}

void
WebStatFrontendProtocolTest::app_webapp_deleted(
  AdClient& client)
{
  WebStatsList web_stats;

  const TestCase APP_DELETED_CASES[] =
  {
    { "App/17", 0, 400, CO_PARAM_NAME | CO_GEN_CURCT}
  };


  process_(
    client,
    APP_DELETED_CASES,
    web_stats,
    &WebStatRequest::app,
    "O",
    WebStatRequest().
      op(fetch_string("Operation/17")).
      src(fetch_string("Source/17")));

  ADD_WAIT_CHECKER(
    "WebStats check",
    AutoTest::stats_each_diff_checker(
      pq_conn_,
      1,
      web_stats));
}

void
WebStatFrontendProtocolTest::op_param(
  AdClient& client)
{
  WebStatsList web_stats;

  const TestCase OP_CASES[] =
  {
    { MISSED, 0, 400, CO_GEN_CURCT },
    { 0, 0, 400, CO_GEN_CURCT },
    { EMPTY, 0, 400, CO_GEN_CURCT },
  };

  process_(
    client,
    OP_CASES,
    web_stats,
    &WebStatRequest::op,
    "O",
    WebStatRequest().
      app(fetch_string("App/6")).
      src(fetch_string("Source/6")));

  ADD_WAIT_CHECKER(
    "WebStats check",
    AutoTest::stats_each_diff_checker(
      pq_conn_,
      1,
      web_stats));
}

void
WebStatFrontendProtocolTest::src_missed(
  AdClient& client)
{

  WebStatsList web_stats;

  const TestCase SRC_MISSED_CASES[] =
  {
    { MISSED, 0, 400, CO_GEN_CURCT }
  };

  process_(
    client,
    SRC_MISSED_CASES,
    web_stats,
    &WebStatRequest::src,
    "O",
    WebStatRequest().
      app(fetch_string("App/7")).
      op(fetch_string("Operation/7")));

  ADD_WAIT_CHECKER(
    "WebStats check",
    AutoTest::stats_each_diff_checker(
      pq_conn_,
      1,
      web_stats));
}

void
WebStatFrontendProtocolTest::src_param(
  AdClient& client)
{

  WebStatsList web_stats;

  const TestCase SRC_CASES[] =
  {
    { 0, 0, 400, CO_GEN_CURCT },
    { EMPTY, 0, 400, CO_GEN_CURCT },
  };

  process_(
    client,
    SRC_CASES,
    web_stats,
    &WebStatRequest::src,
    "O",
    WebStatRequest().
      app(fetch_string("App/8")).
      op(fetch_string("Operation/8")));

  ADD_WAIT_CHECKER(
    "WebStats check",
    AutoTest::stats_each_diff_checker(
      pq_conn_,
      1,
      web_stats));
}

void
WebStatFrontendProtocolTest::ct_param()
{
  AdClient client(AdClient::create_user(this));

  WebStatsList web_stats;

  const TestCase CT_CASES[] =
  {
    { 0, 0, 200, CO_GEN_CURCT },
    { EMPTY, 0, 200, CO_GEN_CURCT },
    { CT_SIZE_51, CT_SIZE_50, 200, 0},
    { CT_SIZE_50, CT_SIZE_51, 200, 0},
    { "a=b", "a=b", 200, CO_NULL_CT | CO_NULL_CURCT },
    { "CT/1", "CURCT/1", 200, CO_PARAM_NAME | CO_SKIP_STAT}
  };

  process_(
    client,
    CT_CASES,
    web_stats,
    &WebStatRequest::ct,
    "I",
    WebStatRequest().
      app(fetch_string("App/9")).
      src(fetch_string("Source/9")).
      op(fetch_string("Operation/9")));

  ADD_WAIT_CHECKER(
    "WebStats check",
    AutoTest::stats_diff_checker(
      pq_conn_,
      { 1, 1, 1, 1, 2 },
      web_stats));
}

void
WebStatFrontendProtocolTest::ua_case(
  AdClient& client)
{
  WebStatsList web_stats;
  
  std::string timestamp(
    now_.get_gm_time().format("%Y%m%d%H%M%S"));
  
  std::string UA(
    "Mozilla/5.0 (Windows NT " + timestamp +
      "; rv:12.0) Gecko/20100101 Firefox/" + timestamp);

  const TestCase UA_CASES[] =
  {
    { UA.c_str(), 0, 200, 0 },
    { 0, 0, 200, CO_NO_UA }
  };

  process_(
    client,
    UA_CASES,
    web_stats,
    &WebStatRequest::user_agent,
    "I",
    WebStatRequest().
      app(fetch_string("App/1")).
      src(fetch_string("Source/1")).
      op(fetch_string("Operation/1")),
    "windows nt " + timestamp,
    "Firefox " + timestamp);

  ADD_WAIT_CHECKER(
    "WebStats check",
    AutoTest::stats_each_diff_checker(
      pq_conn_,
      1,
      web_stats));
}

void
WebStatFrontendProtocolTest::not_exists_triplet(
  AdClient& client)
{
  WebStatsList web_stats;

  const TestCase NON_EXISTS_CASE[] =
  {
    { "S", 0, 400, CO_GEN_CURCT }
  };

  process_(
    client,
    NON_EXISTS_CASE,
    web_stats,
    &WebStatRequest::res,
    "I",
    WebStatRequest().
      app(fetch_string("App/1")).
      src(fetch_string("Source/2")).
      op(fetch_string("Operation/3")));

  ADD_WAIT_CHECKER(
    "WebStats check",
    AutoTest::stats_each_diff_checker(
      pq_conn_,
      1,
      web_stats));
}


bool
WebStatFrontendProtocolTest::run()
{

  AUTOTEST_CASE(
    invalid_uid(),
    "Invalid uid");

  AUTOTEST_CASE(
    probe_uid(),
    "Probe uid");

  {
    AdClient client(AdClient::create_nonoptin_user(this));

    AUTOTEST_CASE(
      res_param(client),
      "'res' parameter tests");

    AUTOTEST_CASE(
      testrequest_param(client),
      "'testrequest' parameter tests");

    AUTOTEST_CASE(
      post_cors(client),
      "POST/CORS requests");
  }

  {
    AdClient client(AdClient::create_optout_user(this));

    AUTOTEST_CASE(
      app_param(client),
      "'app' parameter tests");

    AUTOTEST_CASE(
      app_webapp_deleted(client),
      "'app' WebOperation deleted");

    AUTOTEST_CASE(
      op_param(client),
      "'op' parameter tests");

    AUTOTEST_CASE(
      src_missed(client),
      "'src' DB missed");

     AUTOTEST_CASE(
       src_param(client),
       "'src' parameter tests");
  }
  
  AUTOTEST_CASE(
    ct_param(),
    "'ct' parameter tests");

  {
    AdClient client(AdClient::create_user(this));

    AUTOTEST_CASE(
      ua_case(client),
      "'User-Agent' tests");

    AUTOTEST_CASE(
      not_exists_triplet(client),
      "Non-existent combination of parameter 'app', 'src', 'op' test");
  }

  AUTOTEST_CASE(
    oo_cases_(),
    "Optout case");

  AUTOTEST_CASE(
    cc_tag_cases_(
      0,
      get_config().check_service(CTE_ALL_REMOTE, STE_FRONTEND)?
      CCTCE_LOG_CCID_REMOTE: CCTCE_LOG_CCID_CENTRAL),
    "Tag and CC case");

  // Repeat on central for remote runs
  if (
    get_config().check_service(CTE_CENTRAL, STE_FRONTEND) &&
    get_config().check_service(CTE_ALL_REMOTE, STE_FRONTEND))
  {
    check();
    
    AUTOTEST_CASE(
      cc_tag_cases_(
        AutoTest::UF_CENTRAL_FRONTEND),
      "Tag and CC case (central)");
    
  }
    
  return true;
}

void
WebStatFrontendProtocolTest::post_condition()
{ }

void
WebStatFrontendProtocolTest::tear_down()
{ }


