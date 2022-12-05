
#include "OptOutTest.hpp"
#include <eh/Exception.hpp>
#include <climits>
#include <stdlib.h> 

REFLECT_UNIT(OptOutTest) (
  "Frontend",
  AUTO_TEST_FAST
);

namespace
{
  typedef AutoTest::RedirectChecker RedirectChecker;
  
  const int EXPIRE_LIMIT = 60; // in seconds

  struct TestCookie : public HTTP::CookieDef
  {

    TestCookie(
      const String::SubString& nam,
      const String::SubString& val,
      const String::SubString& dmn,
      const String::SubString& pth,
      const Generics::Time& exp,
      bool sec) :
      HTTP::CookieDef(
        nam, val, dmn,
        pth, exp, sec)
    { }

    TestCookie(
      const HTTP::CookieDef& cookie)
      :
      HTTP::CookieDef(cookie)
    { }
    
    bool
    operator==(
      const TestCookie& other) const
    {
      return name.equal(other.name) &&
        value.equal(other.value) &&
        domain.equal(other.domain) &&
        path.equal(other.path) &&
        abs(expires.tv_sec - other.expires.tv_sec) < EXPIRE_LIMIT &&
        secure == other.secure;
    }
  };

  std::ostream&
  operator<<(
    std::ostream& out,
    const TestCookie& c)
  {
    out << "name: " << c.name <<
      ", value: " << c.value <<
      ", domain:"  << c.domain <<
      ", path:"  << c.path <<
      ", expires:" << c.expires.
      get_gm_time().format(AutoTest::DEBUG_TIME_FORMAT) <<
      ", secure: " << (c.secure? "yes": "no");
    return out;
  }

  typedef AutoTest::NSLookupRequest  NSLookupRequest;
  
  // Request wrappers
  class OptOutRequest: public AutoTest::OptOutRequest
  {
    typedef AutoTest::OptOutRequest Base;
  public:
    OptOutRequest(
      const std::string& _op_kwd,
      const std::string& _debug_time = std::string())
      : Base(true)
    {
      op = _op_kwd;
      if (!_debug_time.empty())
      {
        debug_time = _debug_time;
      }
    }
  };

  enum CookieBehaviourEnum
  {
      CBE_Cleared,
      CBE_CookieEmpty,
      CBE_CookieNotEmpty,
      CBE_DomainCookie
  };

  enum AdserverCookie
  {
    AC_COOKIE_EMPTY = 1,
  };

  enum OptoutStatus
  {
    OS_FAIL,
    OS_SUCCESS,
    OS_ALREADY,
    OS_INVALID
  };

  struct ExpirationRequest
  {
    const char* op;
    const char* ce;
    int expire_ofs;
    OptoutStatus status;
    unsigned long flags;
  };

  /**
   * @class CookieCheck
   * @brief Check cookie.
   *
   * This checker used for test that expected cookie
   * cleared, empty or have valid value.
   */
  class CookieCheck :  public AutoTest::Checker
  {
  public:

    /**
     * @brief Constructor.
     *
     * @param client (user).
     * @param cookie name.
     * @param cookie expected behaviour
     * @param cookie expected value
     */
    CookieCheck(AdClient& client,
                const char* cookie,
                CookieBehaviourEnum behaviour,
                const std::string& value = std::string()) :
      client_(client),
      cookie_(cookie),
      behaviour_(behaviour),
      value_(value)
    {}

    /**
     * @brief Destructor.
     */
    virtual ~CookieCheck() noexcept
    {}
    
    /**
     * @brief Check.
     * @param throw on error flag.
     */
    bool check(bool throw_error = true) /*throw(eh::Exception)*/
    {
      Stream::Error error;
      bool result = false;
      switch (behaviour_)
      {
        case CBE_Cleared:
        {
          HTTP::CookieDefList
            cookies_keep_exp(true), cookies_all(false);
          cookies_all.load_from_headers(
            client_.get_response_headers(),
            HTTP::HTTPAddress(client_.get_stored_url()));
          cookies_keep_exp.load_from_headers(
            client_.get_response_headers(),
            HTTP::HTTPAddress(client_.get_stored_url()));
          result =
            !find_cookie(cookies_all, cookie_.c_str()) &&
              find_cookie(cookies_keep_exp, cookie_.c_str());
          error << "Cookie '" << cookie_ << "' is not cleared";
          break;
        }
        case CBE_CookieEmpty:
        {
          result =
            !(client_.has_host_cookie(cookie_.c_str()) ||
              client_.has_domain_cookie(cookie_.c_str()));
          error << "Cookie '" << cookie_ << "' is not empty";
          break;
        }
        case CBE_CookieNotEmpty:
        {
          result =
            client_.has_host_cookie(cookie_.c_str()) ||
              client_.has_domain_cookie(cookie_.c_str());
          error << "Cookie '" << cookie_ << "' is empty";
          break;
        }
        case CBE_DomainCookie:
        {
          result = client_.has_domain_cookie(cookie_.c_str());
          if (value_.empty())
          {
            error << "Domain cookie '" << cookie_ << "' is empty";
          }
          else
          {
            result = result &&
                client_.has_domain_cookie(cookie_.c_str(), value_);
            error << "Unexpected domain cookie '" << cookie_ <<
              "' value, expected='" << value_ << "'";
          }
          break;
        }
      }
      
      if (!result && throw_error)
      {
        throw AutoTest::CheckFailed(error);
      }

      return result;
    }

  private:
    AdClient& client_;          // client(user)
    std::string cookie_;        // cookie name
    CookieBehaviourEnum behaviour_; // cookie expected behaviour
    std::string value_;         // cookie expected value

    bool find_cookie(HTTP::CookieDefList& cookies, const char* cookie)
    {
      HTTP::CookieDefList::iterator it = cookies.begin();
      for (; it != cookies.end(); ++it)
      {
        if (it->name == cookie) return true;
      }
      return false;
    };
  };
}

bool 
OptOutTest::run_test()
{
  AUTOTEST_CASE(
    base_scenario(),
    "Base scenario");
  
  AUTOTEST_CASE(
    optout_status_redirect_scenario(),
    "Redirect on optout requests");
  
  AUTOTEST_CASE(
    incorrect_uid_opt_out_scenario(),
    "Redirect to 'opt_undef_url'");

  AUTOTEST_CASE(
    client_without_cookes_scenario(),
    "Client without cookies");
  
  AUTOTEST_CASE(
    cookie_expiration(),
    "Opt-out cookie expiration");
  
  return true;
}

void
OptOutTest::base_scenario ()
{
  AdClient client(AdClient::create_nonoptin_user(this));

  // Based on Test 2.1. Opt-in on the first navigation of
  // https://confluence.ocslab.com/display/QA/Opt-out+Test+Plan+%28ADSC%29
  {
    client.process_request(
      OptOutRequest("in", "01-01-2006:10-12-12"),
      "first opt-in");

    FAIL_CONTEXT(
      CookieCheck(
        client,
        "uid",
        CBE_CookieNotEmpty).check(),
      "uid isn't empty");
  }

  client.process_request(OptOutRequest("out", "02-01-2006:15-12-12"),
                         "opt-out request");


  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      !client.has_host_cookies()), 
    "Host cookies shouldn't "
    "return in optout mode");
  
  FAIL_CONTEXT(
    CookieCheck(
      client,
      "OPTED_OUT",
      CBE_DomainCookie,
      "YES").check(),
    "OPTED_OUT=YES");


  // Second opt-out with redirect
  client.process_request(
    OptOutRequest("out", "02-01-2006:15-12-12").
    success_url("test11.com").
    fail_url   ("test22.com").
    already_url("test33.com"),
    "opt-out request" );

  FAIL_CONTEXT(
    RedirectChecker(
      client,
      "http://test33.com/").check(),
    "must redirected to 'already_url'");
  
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      !client.has_host_cookies()), 
    "Host cookies shouldn't "
    "return in optout mode");
  
  FAIL_CONTEXT(
    CookieCheck(
      client,
      "OPTED_OUT",
      CBE_DomainCookie,
      "YES").check(),
    "OPTED_OUT=YES");

  client.process_request(OptOutRequest("in", "02-01-2006:15-12-12"),
                         "opt_in_request");
  
  FAIL_CONTEXT(
    CookieCheck(
      client,
      "OPTED_OUT",
      CBE_CookieEmpty).check(),
    "OPTED_OUT is empty");

  FAIL_CONTEXT(
    CookieCheck(
      client,
      "uid",
      CBE_CookieNotEmpty).check(),
    "uid isn't empty");


  // Based on Test 2.3. Opting-in second time
  // https://confluence.ocslab.com/display/QA/Opt-out+Test+Plan+%28ADSC%29
  {
    client.process_request(
      OptOutRequest("in", "10-01-2006:10-12-12").
      success_url("test11.com").
      fail_url   ("test22.com").
      already_url("test33.com"),
      "already opt-in");

    FAIL_CONTEXT(
      RedirectChecker(
        client,
        "http://test33.com/").check(),
      "must redirected to 'already_url'");

  FAIL_CONTEXT(
    CookieCheck(
      client,
      "uid",
      CBE_CookieNotEmpty).check(),
      "uid isn't empty");
  }
  
}


void 
OptOutTest::optout_status_redirect_scenario()
{
  AdClient client(AdClient::create_nonoptin_user(this));
  // "Empty" user send request for optout status

  client.process_request(OptOutRequest("status").
                         opted_in_url ("test1.com").
                         opted_out_url("test2.com").
                         opt_undef_url("test3.com"));

  FAIL_CONTEXT(
    RedirectChecker(
      client,
      "http://test3.com/").check(),
    "must be redirected to opt_undef_url on undef");

  // "Forged" user send request for optout status
  client.set_uid("qwerty|", false);
  client.process_request(OptOutRequest("status").
                         opted_in_url ("test1.com").
                         opted_out_url("test2.com").
                         opt_undef_url("test3.com"));

  FAIL_CONTEXT(
    RedirectChecker(
      client,
      "http://test3.com/").check(),
    "must be redirected to opt_undef_url on undef");

  // "Known" user with probe uid request for optout status
  client.set_probe_uid();
  
  client.process_request(OptOutRequest("status").
                         opted_in_url ("testX.com").
                         opted_out_url("testY.com").
                         opt_undef_url("testZ.com"));

  FAIL_CONTEXT(
    RedirectChecker(
      client,
      "http://testz.com/").check(),
    "must be redirected to opted_in_url on in");

  // User with persistent uid request for optout status
  client.process_request(OptOutRequest("in"));
 
  client.process_request(OptOutRequest("status").
                         opted_in_url ("testA.com").
                         opted_out_url("testB.com").
                         opt_undef_url("testC.com"));

  FAIL_CONTEXT(
    RedirectChecker(
      client,
      "http://testa.com/").check(),
    "must be redirected to opted_in_url on in" );
 
  // Optout request
  client.process_request(OptOutRequest("out").
                         success_url("test11.com").
                         fail_url   ("test22.com").
                         already_url("test33.com"));

  FAIL_CONTEXT(
    RedirectChecker(
      client,
      "http://test11.com/").check(),
    "must be redirected to success_url on success");
  
  client.process_request(OptOutRequest("status").
                         opted_in_url ("testA.com").
                         opted_out_url("testB.com").
                         opt_undef_url("testC.com"));

  FAIL_CONTEXT(
    RedirectChecker(
      client,
      "http://testb.com/").check(),
    "must be redirected to opted_out_url on out");

}

void 
OptOutTest::incorrect_uid_opt_out_scenario ()
{
  AdClient client(AdClient::create_user(this));
  
  client.set_uid(">:-." + client.get_uid() + ":-("); 

  client.process_request(OptOutRequest("status")
                         .opted_in_url ("test_invalid_uid_in.com")
                         .opted_out_url("test_invalid_uid_out.com")
                         .opt_undef_url("test_invalid_uid_undef.com"));

  FAIL_CONTEXT(
    RedirectChecker(
      client,
      "http://test_invalid_uid_undef.com/").check(),
    "opt_undef_url redirection check");
  
}

void
OptOutTest::client_without_cookes_scenario()
{
  // Based on Test 1.3 of
  // https://confluence.ocslab.com/display/QA/Opt-out+Test+Plan+%28ADSC%29
  AdClient client(AdClient::create_nonoptin_user(this));

  client.process_request(OptOutRequest("out").
                         success_url("test11.com").
                         fail_url   ("test22.com").
                         already_url("test33.com"), "opt-out request" );

  FAIL_CONTEXT(
    RedirectChecker(
      client,
      "http://test11.com/").check(),
    "must be redirected to success_url on success");
  
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      !client.has_host_cookies()), 
    "Host cookies shouldn't "
    "return in optout mode");
  
  FAIL_CONTEXT(
    CookieCheck(
      client,
      "OPTED_OUT",
      CBE_DomainCookie,
      "YES").check(),
    "OPTED_OUT=YES");

}

void
OptOutTest::cookie_expiration()
{
  const ExpirationRequest REQUESTS[] =
  {
    { "out", "-1", 0, OS_INVALID, 0 },
    { "out", "99999999999999999999", 0, OS_FAIL, 0 },
    { "out", "1day", 0, OS_FAIL, 0 },
    { "out", "1s", 0, OS_FAIL, 0 },
    { "out", "0", 2*365*24*60*60, OS_SUCCESS, 0 },
    { "in", "1", 365*24*60*60, OS_SUCCESS, 0 },
    { "out", "1", 24*60*60, OS_SUCCESS, AC_COOKIE_EMPTY },
    { "out", "10d", 10*24*60*60, OS_ALREADY, 0 },
    { "out", "60m", 60*60, OS_ALREADY, 0 },
    { "out", "731d", 731*24*60*60, OS_ALREADY, 0 }
  };

  AdClient client(AdClient::create_user(this));
  
  for (size_t i = 0; i < countof(REQUESTS); ++i)
  {
    unsigned int status =
      client.process(
        AutoTest::OptOutRequest().
          success_url("http://success.com").
          fail_url("http://fail.com").
          already_url("http://already.com").
          op(REQUESTS[i].op).
          ce(REQUESTS[i].ce), true);

    FAIL_CONTEXT(
      AutoTest::equal_checker(
        REQUESTS[i].status == OS_INVALID? 400: 200,
        status).check(),
      "Unexpected response status");
    
    if (REQUESTS[i].status == OS_INVALID)
    {
      break;
    }
    
    FAIL_CONTEXT(
      RedirectChecker(
        client,
        REQUESTS[i].status == OS_FAIL? "http://fail.com/":
        REQUESTS[i].status == OS_ALREADY?
        "http://already.com/": "http://success.com/").check(),
      "Optout redirect check");

    HTTP::HTTPAddress address(client.get_stored_url());
    HTTP::CookieDefList client_cookies;

    client_cookies.load_from_headers(
      client.get_response_headers(),
      address);

    std::string domain(client.get_domain());
    
    if (REQUESTS[i].status != OS_FAIL)
    {
      
      std::list<TestCookie> got_cookies(
        client_cookies.begin(),
        client_cookies.end());
      std::list<TestCookie> exp_cookies;
      if (strcmp(REQUESTS[i].op, "out") == 0)
      {
        exp_cookies.push_back(
           TestCookie(
             String::SubString("OPTED_OUT"),
             String::SubString("YES"),
             String::SubString(domain),
             String::SubString("/"),
             AutoTest::Time() + REQUESTS[i].expire_ofs,
             address.secure()));
      }
      else
      {
        std::string uid(client.get_uid());

        exp_cookies.push_back(
          TestCookie(
            String::SubString("uid"),
            String::SubString(uid),
            String::SubString(domain),
            String::SubString("/"),
            AutoTest::Time() + REQUESTS[i].expire_ofs,
            address.secure()));
      }

      FAIL_CONTEXT(
        AutoTest::sequence_checker(
          exp_cookies,
          got_cookies).check(),
        "Check cookies#" + strof(i+1));
    }
    else
    {
      FAIL_CONTEXT(
        AutoTest::predicate_checker(
          client_cookies.empty()),
        "Check empty cookies#" + strof(i+1));
    }

    if (REQUESTS[i].flags & AC_COOKIE_EMPTY)
    {
      client.process_request(NSLookupRequest());

      HTTP::CookieDefList cookies(false);
      
      cookies.load_from_headers(
        client.get_response_headers(),
        address);
      
      FAIL_CONTEXT(
        AutoTest::predicate_checker(
          cookies.empty()),
       "Check cookies#" + strof(i+1));
    }
  }
}
