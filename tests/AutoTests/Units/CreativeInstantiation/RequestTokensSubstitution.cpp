
#include "RequestTokensSubstitution.hpp"

REFLECT_UNIT(RequestTokensSubstitution) (
  "CreativeInstantiation",
  AUTO_TEST_FAST);


namespace
{
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::NSLookupRequest NSLookupRequest;

  struct Case
  {
    std::string prefix;
    RequestTokensSubstitution::CreativeEnum creative;
    bool empty_tokens;
  }; 

  // dynamic creative service request params
  const char CLICK[] = "c";
  const char RANDOM[] = "r";
  const char RESOURCES[] = "rs";
  const char FILE[] = "file";

  void get_token_value(
    Stream::Parser& stream,
    std::string& token)
  {
    if (!stream.eof())
    {
      char t[4096];
      stream.getline(t, 4096);
      token.assign(t);
    }
  };

  struct DCreativeURL
  {
    static const char AMP = '&';
    static const char EQ = '=';
    static const char QT = '?';

    std::string host_and_path_;
    std::string click_;
    std::string random_;
    std::string resources_;
    std::string file_;

    DCreativeURL():
      host_and_path_(),
      click_(),
      random_(),
      resources_(),
      file_()
    {}

    DCreativeURL(const String::SubString& url)
    {
      String::SubString::SizeType q_sep = url.find(QT);
      if (q_sep == String::SubString::NPOS)
      {
        host_and_path_ = url.str();
      }
      else
      {
        host_and_path_ = url.substr(0, q_sep).str();
        String::StringManip::SplitAmp tokenizer(url.substr(q_sep + 1));
        String::SubString token;
        while (tokenizer.get_token(token))
        {
          String::SubString::SizeType eq_sep = token.find(EQ);
          if (eq_sep != String::SubString::NPOS)
          {
            std::string param_name = token.substr(0, eq_sep).str();
            std::string param_value = token.substr(eq_sep + 1).str();
            if (param_name == CLICK)
            { click_ = param_value; }
            else if (param_name == RANDOM)
            { random_ = param_value; }
            else if (param_name == RESOURCES)
            { resources_ = param_value; }
            else if (param_name == FILE)
            { file_ = param_value; }
          }
        }
      }
    }

    DCreativeURL(
      const String::SubString& host_and_path,
      const String::SubString& click,
      const String::SubString& random,
      const String::SubString& resources,
      const String::SubString& file):
      host_and_path_(host_and_path.str()),
      random_(random.str())
    {
      String::StringManip::mime_url_encode(click, click_);
      String::StringManip::mime_url_encode(resources, resources_);
      String::StringManip::mime_url_encode(file, file_);
    }

    inline bool operator== (const DCreativeURL& a) const
    {
      return host_and_path_ == a.host_and_path_ &&
        click_ == a.click_ &&
        random_ == a.random_ &&
        resources_ == a.resources_ &&
        file_ == a.file_;
    }
  };

  std::ostream& operator<< (std::ostream& out, const DCreativeURL& url)
  {
    out << url.host_and_path_ << "?c=" << url.click_ << "&r=" << url.random_
        << "&rs=" << url.resources_ << "&file=" << url.file_;
    return out;
  };

  const Case TESTS[] =
  {
    { "SingleAdvertisier-EmptyTokens", RequestTokensSubstitution::CE_SIMPLE, true },
    { "AgencyAccount-EmptyTokens", RequestTokensSubstitution::CE_SIMPLE_AGENCY, true },
    { "SingleAdvertisier-DefaultTokens", RequestTokensSubstitution::CE_SIMPLE, false },
    { "AgencyAccount-DefaultTokens", RequestTokensSubstitution::CE_SIMPLE_AGENCY, false }
  };

  class CreativeContentChecker: public AutoTest::Checker
  {
    static const size_t MAX_TOKEN_LENGTH = 4096;

  public:

    CreativeContentChecker(
      const std::string& got_content,
      const std::string& frontend_address,
      const std::string& resources_path):
      got_content_(got_content),
      adimage_path_(frontend_address + "/creatives" + resources_path),
      dcradvtmp_file_(), // empty
      dcradvsz_file_()   // empty
    {}

    CreativeContentChecker(
      const std::string got_content,
      const std::string& frontend_address,
      const std::string& resources_path,
      const std::string& click,
      const std::string& random,
      const std::string& dcradvtmp_file,
      const std::string& dcradvsz_file):
      got_content_(got_content),
      adimage_path_(frontend_address + "/creatives" + resources_path),
      dcradvtmp_file_(frontend_address + "/services/dcreative",
                      click,
                      random,
                      resources_path,
                      dcradvtmp_file),
      dcradvsz_file_(frontend_address + "/services/dcreative",
                      click,
                      random,
                      resources_path,
                      dcradvsz_file)
    {}

    ~CreativeContentChecker() noexcept
    {}

    bool check(bool = true) /*throw(AutoTest::CheckFailed, eh::Exception)*/
    {
      if (got_content_.empty())
      {
        Stream::Error error;
        error << "Got empty creative content";
        throw AutoTest::CheckFailed(error);
      }

      Stream::Parser body(got_content_);
      std::string adimage_path, dcradvtmp_file, dcradvsz_file;
      get_token_value(body, adimage_path);
      get_token_value(body, dcradvtmp_file);
      get_token_value(body, dcradvsz_file);

      return AutoTest::and_checker(
        AutoTest::equal_checker(
          adimage_path_,
          adimage_path),
        AutoTest::equal_checker(
          dcradvtmp_file_,
          DCreativeURL(dcradvtmp_file))).and_if(
        AutoTest::equal_checker(
          dcradvsz_file_,
          DCreativeURL(dcradvsz_file))).check();
    }

  private:
    std::string got_content_;
    std::string adimage_path_;
    DCreativeURL dcradvtmp_file_;
    DCreativeURL dcradvsz_file_;
  };
}

bool
RequestTokensSubstitution::run_test()
{
  GlobalConfig::Service frontend =
    get_config().check_service(CTE_ALL_REMOTE, STE_FRONTEND)?
      get_config().get_service(CTE_ALL_REMOTE, STE_FRONTEND):
        get_config().get_service(CTE_ALL, STE_FRONTEND);

  AdClient client(AdClient::create_user(this));

  for (size_t i = 0; i < countof(TESTS); ++i)
  {
    add_descr_phrase(TESTS[i].prefix);
    NSLookupRequest request;
    request.tid = fetch_string(TESTS[i].prefix + "/TAG");
    request.referer_kw = fetch_string(TESTS[i].prefix + "/KWD");
    client.process_request(request);
        
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        fetch_string(TESTS[i].prefix + "/CC"),
        client.debug_info.ccid).check(),
      TESTS[i].prefix + 
        " Unexpected ccid.");

    std::string adv_resources_path("/");
    if ( TESTS[i].creative == CE_SIMPLE_AGENCY )
      adv_resources_path += fetch_string("Agency") + "/" + fetch_string("AgencyAdvertiser") + "/";
    else
      adv_resources_path += fetch_string("SingleAdvertiser") + "/";

    CreativeContentChecker crbody_checker = TESTS[i].empty_tokens
      ? CreativeContentChecker(client.req_response_data(),
          frontend.address,
          adv_resources_path)
      : CreativeContentChecker(client.req_response_data(),
          frontend.address,
          adv_resources_path,
          client.debug_info.click_url.value(),
          "0",
          fetch_string("DynamicFile1"),
          fetch_string("DynamicFile2"));

    FAIL_CONTEXT(crbody_checker.check(),
      TESTS[i].prefix + " Unexpected creative response body.")
  }

  return true;
}

