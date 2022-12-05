#include "XsltCreativeTemplateValidation.hpp"

REFLECT_UNIT(XsltCreativeTemplateValidation) (
  "CreativeInstantiation",
  AUTO_TEST_FAST
);

namespace
{
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::SelectedCreativeChecker SelectedCreativeChecker;
  
  struct TestData
  {
    const char* keyword;
    const char* tid;
    const char* ccid;
    std::string description;
    unsigned int status;
  };

  const TestData TEST_CASES[] =
  {
    {
      "Keyword/01", "Tag/01", 0, "Empty template", 204
    },
    {
      "Keyword/02", "Tag/02", 0, "Absent template file", 204
    },
    {
      "Keyword/03", "Tag/03", 0, "Invalid foros:escape-js call", 204
    },
    {
      "Keyword/04", "Tag/04", "Cid/04", "Valid foros calls", 200
    },
    {
      "Keyword/05", "Tag/05", 0, "Invalid template XML", 204
    }, 
    {
      "Keyword/06", "Tag/06", 0, "Absent namespace", 204
    },
    {
      "Keyword/07", "Tag/07", 0, "Unknow function call", 204 
    },
    {
      "Keyword/08", "Tag/08", "Cid/08", "Include", 200
    },
    {
      "Keyword/09", "Tag/09", 0, "Invalid include", 204
    }
  };
}

bool 
XsltCreativeTemplateValidation::run_test()
{
  AdClient client(AdClient::create_user(this));
  
  for(size_t i = 0; i < countof(TEST_CASES); ++i)
  {

    SelectedCreativeChecker checker(
      client,
      NSLookupRequest().
        tid(fetch_int(TEST_CASES[i].tid)).
      referer_kw(fetch_string(TEST_CASES[i].keyword)),
      TEST_CASES[i].ccid? fetch_int(TEST_CASES[i].ccid): 0);

    FAIL_CONTEXT(
      checker.check(),
      TEST_CASES[i].description +
        ". Check ccid");

    FAIL_CONTEXT(
      AutoTest::equal_checker(
        TEST_CASES[i].status,
        checker.client().req_status()).check(),
      TEST_CASES[i].description +
        ". Check status");
  }

  return true;
}
 
