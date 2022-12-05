#include "DynamicCreativeContentTest.hpp"
 
REFLECT_UNIT(DynamicCreativeContentTest) (
  "CreativeInstantiation",
  AUTO_TEST_QUIET
);


namespace
{

  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::DebugInfo::SimpleValue SimpleValue;

  struct CreativeFile
  {
    std::string file_name;
    const char* expected_body;
    bool no_ads;
  };

  const CreativeFile CREATIVE_FILES[] =
  {
    {
      "DynamicCreativeContentTemplateFile1",
      "Some creative",
      false
    },
    {
      "DynamicCreativeContentTemplateFile2",
      "",
      true
    },
    {
      "DynamicCreativeContentTemplateFile3",
      "##OIXCLICK##\n"
      "##ADIMAGE##\n"
      "##WIDTH##\n"
      "##HEIGHT##\n"
      "##ALTTEXT##\n"
      "##ACTIONPIXEL##\n"
      "##TRACKPIXEL##\n"
      "##CRADVTRACKPIXEL##\n",
      false
    },
  };


  /**
   * @class BodyChecker
   * @brief Check ads response body.
   */
  class BodyChecker : public AutoTest::Checker
  {
    static const size_t MAX_TOKEN_SIZE = 4096;
    static const size_t TOKEN_COUNT = 9;
    typedef std::string (BodyChecker::*TokenValueGetter)() const;

    struct Token
    {
      std::string token;
      TokenValueGetter value;
    };

    static const Token TOKENS[TOKEN_COUNT];
    
  public:
    /**
     * @brief Constructor.
     *
     * @param client
     * @param request
     * @param expected body
     */
    BodyChecker(
      AdClient& client,
      const NSLookupRequest& request,
      const char* expected_body) :
      client_(client),
      request_(request),
      expected_body_(expected_body)
    { }

    /**
     * @brief Destructor.
     */
    ~BodyChecker() noexcept
    { }

    /**
     * @brief Check.
     * @param throw on error flag.
     */
    bool check(bool throw_on_error = true)  /*throw(eh::Exception)*/
    {
      client_.process_request(request_);
      std::string got_body = client_.req_response_data();
      Stream::Parser exp_in(expected_body_);
      Stream::Parser got_in(got_body);
      char exp_line_raw[MAX_TOKEN_SIZE];
      char got_line[MAX_TOKEN_SIZE];
      unsigned long index = 0;
      while( !(exp_in.eof() || got_in.eof()) )
      {
        exp_in.getline(exp_line_raw, MAX_TOKEN_SIZE);
        got_in.getline(got_line, MAX_TOKEN_SIZE);
        std::string exp_line(exp_line_raw);
        if (!compare_lines(exp_line, got_line))
        {
          if (throw_on_error)
          {
            Stream::Error ostr;
            ostr << "Unexpected line#" << index <<
              " ('" << exp_line << "' != '" <<
              got_line << "')";
            throw AutoTest::CheckFailed(ostr);
          }
          return false;
        }
      }
      if ( !(exp_in.eof() && got_in.eof()) )
      {
        if (throw_on_error)
        {
          throw AutoTest::CheckFailed(
            "Expected and got body is unequal");
        }
        return false;
      }
      return true;
    }

  private:

    bool compare_lines(
      std::string& exp_line,
      const std::string& got_line)
    {
      for ( unsigned long i = 0; i < TOKEN_COUNT; ++i )
      {
        if (exp_line == TOKENS[i].token)
        {
          if ( !TOKENS[i].value)
          {
            // No need to check
            return true;
          }
          exp_line = (this->*(TOKENS[i].value))();
          break;
        }
      }
      return AutoTest::equal(exp_line, got_line);
    }

    std::string foros_click() const
    {
      return client_.debug_info.click_url;
    }


  private:
    AdClient& client_;
    NSLookupRequest request_;
    std::string expected_body_;
  };
  
  const BodyChecker::Token BodyChecker::TOKENS[BodyChecker::TOKEN_COUNT] =
  {
    { "##OIXCLICK##", &BodyChecker::foros_click },
    { "##ADIMAGE##", 0 },
    { "##WIDTH##", 0 },
    { "##HEIGHT##", 0 },
    { "##ALTTEXT##", 0 },
    { "##ACTIONPIXEL##", 0 },
    { "##TRACKPIXEL##", 0 },
    { "##CRADVTRACKPIXEL##", 0 }
  };

}


bool 
DynamicCreativeContentTest::run_test()
{
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      get_global_params().TemplatesIn()),
    "Template destination path must defined in the config");
  
  std::string templatesin = get_global_params().TemplatesIn()->path();
  std::string keyword = fetch_string("Keyword");
  std::string tid = fetch_string("Tag");
  std::string ccid = fetch_string("CC");
  std::string dst = templatesin + fetch_string("TemplateFile");

  NSLookupRequest request;
  request.tid         = tid;
  request.referer_kw = keyword;
  request.debug_nofraud = 1;
  

  // Restore init
  add_descr_phrase("Initial.");
  try
  {
    AutoTest::MoveCmd(dst, dst + "~").exec();
  }
  catch (...)
  {
    // Ignore move errors
  }

  {
    AdClient client(AdClient::create_user(this));
    client.process_request(request);

    FAIL_CONTEXT(
      AutoTest::wait_checker(
        AutoTest::EqualChecker<std::string, std::string>(
          "0",client.debug_info.ccid)).check(),
      "Check ccid (initial)");
  }

  for (unsigned long i = 0; i < countof(CREATIVE_FILES); ++i)
  {

    add_descr_phrase(CREATIVE_FILES[i].file_name + " file check.");
    AdClient client(AdClient::create_user(this));
    
    std::string file = fetch_string(CREATIVE_FILES[i].file_name);
    AutoTest::CopyCmd(file, dst).exec();

    FAIL_CONTEXT(
      AutoTest::wait_checker(
        AutoTest::and_checker(
          BodyChecker(
            client,
            request,
            CREATIVE_FILES[i].expected_body),
        AutoTest::EqualChecker<std::string, SimpleValue&>(
          CREATIVE_FILES[i].no_ads? "0": ccid,
          client.debug_info.ccid))).check(),
        "Check ccid & body#" + strof(i));
   
  }
  
  return true;
}

