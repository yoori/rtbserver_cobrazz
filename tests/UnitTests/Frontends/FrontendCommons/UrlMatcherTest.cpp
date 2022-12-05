/// @file FrontendCommons/UrlMatcherTest.cpp

#include <iostream>

#include "UrlMatcherTest.hpp"

struct PositiveSample
{
  /// This URL's should be matched
  const char* url;
  /// Standard  result of matching
  const char* standard;
};

const PositiveSample MATCH_URLS[] =
{
  {
    "http://kr.search.yahoo.com"
      "/words.words/some_symbols?fr=kr-front_sb&p=TEXT0",
    "text0",
  },
  {
    "http://kr.search.yahoo.com"
      "/words.words/some_symbols&p=TEXT1?fr=kr-front_sb",
    "text1",
  },
  {
    "http://kr.search.yahoo.com"
      "/words.words/some_symbols&p=TEXT2?other_text",
    "text2",
  },
  {
    "http://br.search.yahoo.com"
      "/words.words/some_symbols?vc=&amp;p=TEXT3",
    "text3",
  },
  {
    "http://kr.altavista.com"
      "/web/results?fr=altavista&itag=ody&q=TEXT4&kgs=1&kls=0?",
    "text4",
  },
  {
    "http://us.yhs4.search.yahoo.com"
      "/yhs/search;cs=iso88591?q=TEXT5",
    "text5",
  },
  {
    "http://us.yhs4.search.yahoo.com"
      "/yhs/searchOtherText?q=TEXT6",
    "text6",
  },
/// Matched through brute force hostname check
  {
    "http://some.text.websearch.rakuten.co.jp"
      "/WebXlebbb_?qt=TEXT7",
    "text7",
  },
  {
    "http://amazon.co.jp"
      "/?field-?keywords=tEXt8&rest",
    "text8"
  },
  {
    "http://we-we-we.google.co.kr"
      "/searchsomething&q=text9&",
    "text9"
  },
  {
    "http://google.ru/&&&q=text10",
    "text10"
  },
  {
    "http://dogpile.com/  /Web/TEXT11",
    "text11"
  },
};
const std::size_t MATCH_URLS_AMOUNT =
  sizeof(MATCH_URLS) / sizeof(MATCH_URLS[0]);

/// Code examples that should be not matched
const char* NOT_MATCH_URLS[] =
{
  "http://some.text.websearch.rakuten.co.jp/WebXlebbb_?&;)qt=TEXT7",
  "http://dogpile.com/Web",
  "http://kr.altavista.com",
  "http://kr.altavista.com/",
  "http://kr.altavista.com//",
  "http://kr.altavista.com/&?",
  "http://us.yhs4.search.yahoo.com/?q=TEXT6",
  "",
//  0,
  "http://t.com/?q=TEXT6",
  "http://kr.altavista.com"
    "/web/results?fr=altavista&itag=ody&q=",
};
const std::size_t NOT_MATCH_URLS_AMOUNT =
  sizeof(NOT_MATCH_URLS) / sizeof(NOT_MATCH_URLS[0]);

void
url_matcher_test(FrontendCommons::UrlMatcher& matcher) /*throw(eh::Exception)*/
{
  std::string match_result;
  for (std::size_t i = 0; i < MATCH_URLS_AMOUNT; ++i)
  {
    bool result =
    matcher.match(MATCH_URLS[i].url, match_result, 0);

    if (!result && match_result != MATCH_URLS[i].standard)
    {
      std::cout << "ERROR: ";
      std::cout << MATCH_URLS[i].url << " | "
        << (result? "MATCH" : "false") << " | " << match_result << std::endl
        << "Standard=" << MATCH_URLS[i].standard << std::endl;
    }
    match_result.clear();
  }
  std::cout << "Checked " << MATCH_URLS_AMOUNT << " positive samples" << std::endl;
  for (std::size_t i = 0; i < NOT_MATCH_URLS_AMOUNT; ++i)
  {
    bool result =
    matcher.match(NOT_MATCH_URLS[i], match_result, 0);
    if (result)
    {
      std::cout << "ERROR: matched " << NOT_MATCH_URLS[i]
        << ", but must be NOT matched. Result="
        << match_result << std::endl;
      match_result.clear();
    }
  }
  std::cout << "Checked " << NOT_MATCH_URLS_AMOUNT << " negative samples"
    << std::endl;
}

int
main() noexcept
{
  try
  {
    FrontendCommons::UrlMatcher_var matcher(
      new FrontendCommons::UrlMatcher());
    load_url_matcher(*matcher, "UrlMatcherTestConfig.xml");
    url_matcher_test(*matcher);
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << ex.what() << std::endl;
    return 1;
  }
  return 0;
}
