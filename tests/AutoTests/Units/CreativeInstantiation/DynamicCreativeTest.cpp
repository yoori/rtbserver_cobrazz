
#include "DynamicCreativeTest.hpp"

REFLECT_UNIT(DynamicCreativeTest) (
  "CreativeInstantiation",
  AUTO_TEST_FAST);

namespace
{
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::NSLookupRequest NSLookupRequest;

  struct FrontendTestCase
  {
    std::string description;
    const char* params;
    unsigned long expected_status;
    bool check_body;
  };

  const char* SERVICE = "/services/dcreative?";

  const FrontendTestCase FRONTEND_TESTS[] = 
  {
    {
      "Unexisting creative HTML file.",
      "file=%2FDynamicCreative%2Funexisting.html&c=http%3A%2F%2Fya.ru&r=123",
      404, false
    },
    {
      "Incorrect click URL.",
      "file=%2FDynamicCreative%2Fsimple.html&c=ttp%3A%2F%2Fya.ru&r=123",
      400, false
    },
    {
      "Incorrect Random value.",
      "file=%2FDynamicCreative%2Fsimple.html&c=http%3A%2F%2Fya.ru&r=rnd",
      400, false
    },
    {
      "Missing 'random' parameter.",
      "file=%2FDynamicCreative%2Fsimple.html&c=http%3A%2F%2Fya.ru",
      200, true
    },
    {
      "Missing 'click' parameter.",
      "file=%2FDynamicCreative%2Fsimple.html&rnd=123",
      400, false
    },
    {
      "Missing 'file' parameter.",
      "c=http%3A%2F%2Fya.ru&r=123",
      404, false
    },
    {
      "Security test.",
      "file=%2F%2E%2E%2F%2E%2E%2Flog%2Fapache%2Fsecure_error_log&c=http%3A%2F%2Fya.ru",
      403, false
    } 
  };

  /**
   * @class Creative
   * @brief Dynamic creative content.
   */
  class Creative
  {
  public:
    /**
     * @brief Constructor.
     *
     * @param frontend
     * @param click URL
     * @param account ID
     * @param agency account ID
     */
    Creative(
      const std::string& frontend,
      const std::string& click,
      unsigned long account = 0,
      unsigned long agency = 0,
      const char* random = 0) :
      frontend_(frontend),
      click_(click),
      adimage_path_(
        frontend + "/creatives" +
        (agency? "/" + strof(agency): "") +
        (account? "/" + strof(account) + "/": "")),
      random_(random? random: "0")
    { }

    /**
     * @brief get OIXCLICK.
     *
     * @return content for OIXCLICK token
     */
    const std::string& click() const
    {
      return click_;
    }

    /**
     * @brief get ADIMAGE-PATH.
     *
     * @return content for ADIMAGE-PATH token
     */
    const std::string& adimage_path() const
    {
      return adimage_path_;
    }

    /**
     * @brief get ADIMAGE-SERVER.
     *
     * @return content for ADIMAGE-SERVER token
     */
    const std::string& adimage_server() const
    {
      return frontend_;
    }

    /**
     * @brief get RANDOM.
     *
     * @return content for RANDOM token
     */
    const std::string& random() const
    {
      return random_;
    }
    
  private:
    std::string frontend_;
    std::string click_;
    std::string adimage_path_;
    std::string random_;
  };

  /**
   * @class CheckCreative
   * @brief Check dynamic creative content.
   */
  class CheckCreative : public AutoTest::Checker
  {

    static const size_t MAX_TOKEN_SIZE = 4096;
    static const size_t TOKEN_COUNT = 5;
    
    typedef const std::string& (Creative::*CreativeMember)() const;

    struct Token
    {
      std::string token;
      CreativeMember content;
      const char* suffix;
    };

    static const Token TOKENS[TOKEN_COUNT];
    
  public:
    /**
     * @brief Constructor.
     *
     * @param creative body
     * @param expected creative
     */
    CheckCreative(
      const std::string& body,
      const Creative& expected) :
      body_(body),
      expected_(expected)
    { }
     

    /**
     * @brief Destructor.
     */
    ~CheckCreative() noexcept
    { }

    /**
     * @brief Check.
     * @param throw on error flag.
     */
    bool check(bool = true) /*throw(eh::Exception)*/
    {
      unsigned long index = 0;
      Stream::Parser in(body_);
      char line[MAX_TOKEN_SIZE];
      while(!in.eof())
      {
        in.getline(line, MAX_TOKEN_SIZE);
        std::string got_token(line);

        if (index >= TOKEN_COUNT)
        {
          Stream::Error ostr;
          ostr << "Unexpected token#" << index <<
            " ('" << got_token << "')";;
          throw AutoTest::CheckFailed(ostr);
        }
        
        std::string expected_token = TOKENS[index].token +
          " = " + (TOKENS[index].content?
            (expected_.*(TOKENS[index].content))(): "") +
          (TOKENS[index].suffix? TOKENS[index].suffix: "");

        if (!AutoTest::equal(expected_token, got_token))
        {
          Stream::Error ostr;
          ostr << "Unexpected token#" << index <<
             " ('" << expected_token << "' != '" <<
            got_token << "')";
          throw AutoTest::CheckFailed(ostr);
        }
        index++;
      }
      return true;
    }

  private:
    std::string body_;
    const Creative& expected_;
  };

  const CheckCreative::Token CheckCreative::TOKENS[CheckCreative::TOKEN_COUNT] =
  {
    { "OIXCLICK", &Creative::click, 0 },
    { "OIXPRECLICK", &Creative::click, "*amp*relocate*eql*" },
    { "ADIMAGE-SERVER", &Creative::adimage_server, 0 },
    { "ADIMAGE-PATH", &Creative::adimage_path, 0 },
    { "RANDOM", &Creative::random, 0 }
  };

  struct TokenTestCase
  {
    std::string description;
    const char* keyword;
    const char* tid;
    const char* ccid;
    const char* random;
    unsigned long status;
    const char* body;
    const char* account;
    const char* agency;
  };

  const TokenTestCase TOKEN_TESTS[] =
  {
    {
      "Unexisting token.",
      "Unexisting/KWD",
      "Unexisting/TAG",
      "Unexisting/CC",
      0,
      500, 0, 0, 0
    },
    {
      "Incomplete token.",
      "Incomplete/KWD",
      "Incomplete/TAG",
      "Incomplete/CC",
      0,
      404, 0, 0, 0
    },
    {
      "No tokens.",
      "NoTokens/KWD",
      "NoTokens/TAG",
      "NoTokens/CC",
      0,
      200, "No tokens: #RANDOM#", 0, 0
    },
    {
      "Single advertisier.",
      "SingleAdvertisier/KWD",
      "SingleAdvertisier/TAG",
      "SingleAdvertisier/CC",
      "126",
      200, 0, "SingleAdvertiser", 0
    },
    {
      "Agency advertiser.",
      "AgencyAccount/KWD",
      "AgencyAccount/TAG",
      "AgencyAccount/CC",
      "9999999",
      200, 0, "AgencyAdvertiser", "Agency"
    },
  };
}

bool
DynamicCreativeTest::run_test()
{

  frontend =
    get_config().check_service(CTE_ALL_REMOTE, STE_FRONTEND)?
      get_config().get_service(CTE_ALL_REMOTE, STE_FRONTEND).address:
        get_config().get_service(CTE_ALL, STE_FRONTEND).address;

  NOSTOP_FAIL_CONTEXT(dcreatives_frontend());
  NOSTOP_FAIL_CONTEXT(token_substitution());
  
  return true;
}

void DynamicCreativeTest::dcreatives_frontend()
{
  AdClient client(AdClient::create_user(this));
  for (size_t i = 0; i < countof(FRONTEND_TESTS); ++i)
  {
    add_descr_phrase(FRONTEND_TESTS[i].description);
    
    std::string url(SERVICE);
    url+=FRONTEND_TESTS[i].params;

    FAIL_CONTEXT(
      AutoTest::equal_checker(
        FRONTEND_TESTS[i].expected_status,
        client.process(url, true)).check(),
      FRONTEND_TESTS[i].description + 
        " Unexpected response status.");
  
    if (FRONTEND_TESTS[i].check_body)
    {
      Creative creative(frontend, "http://ya.ru");

      FAIL_CONTEXT(
        CheckCreative(
          client.req_response_data(),
          creative).check());

    }
  }
}

void DynamicCreativeTest::token_substitution()
{
  for (size_t i = 0; i < countof(TOKEN_TESTS); ++i)
  {
    add_descr_phrase(TOKEN_TESTS[i].description);
    AdClient client(AdClient::create_user(this));
    NSLookupRequest request;
    request.tid = fetch_string(TOKEN_TESTS[i].tid);
    request.referer_kw = fetch_string(TOKEN_TESTS[i].keyword);
    if (TOKEN_TESTS[i].random)
      request.random = TOKEN_TESTS[i].random;
    client.process_request(request);

    FAIL_CONTEXT(
      AutoTest::equal_checker(
        fetch_string(TOKEN_TESTS[i].ccid),
        client.debug_info.ccid).check(),
      TOKEN_TESTS[i].description + 
        " Unexpected ccid.");

    std::string html_url(
      client.req_response_data());

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        !html_url.empty()),
      TOKEN_TESTS[i].description + 
      " Unexpected htm_url.");

    std::string click(client.debug_info.click_url);
    
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        !click.empty()),
      TOKEN_TESTS[i].description + 
      " Unexpected click_url.");

    unsigned long got_status =
      client.process(html_url, true);

    FAIL_CONTEXT(
      AutoTest::equal_checker(
        TOKEN_TESTS[i].status,
        got_status).check(),
      TOKEN_TESTS[i].description + 
        " Unexpected response status.");

    // Check creative
    if (got_status == 200)
    {
      if (TOKEN_TESTS[i].body)
      {
        FAIL_CONTEXT(
          AutoTest::equal_checker(
            TOKEN_TESTS[i].body,
            client.req_response_data()).check(),
          TOKEN_TESTS[i].description + 
            " Unexpected dcreative response body.");
      }
      else
      {
        Creative creative(
          frontend, click,
          TOKEN_TESTS[i].account?
            fetch_int(TOKEN_TESTS[i].account): 0,
          TOKEN_TESTS[i].agency?
            fetch_int(TOKEN_TESTS[i].agency): 0,
          TOKEN_TESTS[i].random
          );
        
        FAIL_CONTEXT(
          CheckCreative(
            client.req_response_data(),
            creative).check(),
          TOKEN_TESTS[i].description +
          " Unexpected creative content.");
      }
      
    }
  }
  
}


