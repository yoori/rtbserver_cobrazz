#include <string>
#include <vector>
#include <iostream>
#include <String/RegEx.hpp>
#include <Generics/Time.hpp>
#include <Frontends/FrontendCommons/RequestMatchers.hpp>

#include "../../TestHelpers.hpp"

using namespace FrontendCommons;

struct MarkedRegExp
{
  MarkedRegExp(const char* marker_val, const char* re_val)
    : marker(marker_val),
      re(String::SubString(re_val))
  {}

  std::string marker;
  String::RegEx re;
};

TEST(RegExPerformanceTest)
{
  const int ITERS = 10000;
  std::string str("Lynx/2.8.6rel.4 libwww-FM/2.14 SSL-MM/1.4.1 OpenSSL/0.9.8g");
//std::string str("Mozilla/5.0 (iPhone; U; CPU iPhone OS 4_3_5 like Mac OS X; ru-ru) AppleWebKit/533.17.9 (KHTML, like Gecko) Version/5.0.2 Mobile/8L1 Safari/6533.18.5");

  {
    std::list<String::RegEx> regexps;
    regexps.push_back(String::RegEx(String::SubString("\\sChrome[/ ](\\d+.\\d+(?:.\\d+)*)")));
    regexps.push_back(String::RegEx(String::SubString("Opera/.*\\sVersion/(\\d+.\\d+(?:.\\d+)*)")));
    regexps.push_back(String::RegEx(String::SubString("\\sMSIE\\s+(\\d+.\\d+(?:.\\d+)*)")));
    regexps.push_back(String::RegEx(String::SubString("\\sFirefox/(\\d+.\\d+(?:.\\d+)*)")));
    regexps.push_back(String::RegEx(String::SubString("\\sNetscape(?:6)?/(\\d+\\.\\d+(?:\\.\\d+)*)")));
    regexps.push_back(String::RegEx(String::SubString("\\sVersion/(\\d+[.\\d]*)\\s+Safari/")));
    regexps.push_back(String::RegEx(String::SubString("\\sCS(?: )?2000 (\\d+\\.\\d+(?:\\.\\d+)*)")));
    regexps.push_back(String::RegEx(String::SubString("\\sAOL/(\\d+\\.\\d+(?:\\.\\d+)*)")));
    regexps.push_back(String::RegEx(String::SubString("\\sFirebird/(\\d+\\.\\d+(?:\\.\\d+)*)")));
    regexps.push_back(String::RegEx(String::SubString("\\sCamino/(\\d+\\.\\d+(?:\\.\\d+)*)")));
    regexps.push_back(String::RegEx(String::SubString("\\sVersion/(\\d+[.\\d]*)\\s+Safari/")));
    regexps.push_back(String::RegEx(String::SubString("Mozilla/(\\d+\\.\\d+(?:\\.\\d+)*).*Gecko/\\d+$")));

    Generics::CPUTimer timer;
    timer.start();

    for(int i = 0; i < ITERS; ++i)
    {
      for(std::list<String::RegEx>::const_iterator it = regexps.begin();
          it != regexps.end(); ++it)
      {
        String::RegEx::Result res;
        if (it->search(res, str))
        {
          break;
        }
      }
    }

    timer.stop();
    std::cout << "1: " << timer.elapsed_time() << std::endl;
  }

  {
    std::list<MarkedRegExp> regexps;
    regexps.push_back(MarkedRegExp("Chrome", "\\sChrome[/ ](\\d+.\\d+(?:.\\d+)*)"));
    regexps.push_back(MarkedRegExp("Opera", "Opera/.*\\sVersion/(\\d+.\\d+(?:.\\d+)*)"));
    regexps.push_back(MarkedRegExp("MSIE", "\\sMSIE\\s+(\\d+.\\d+(?:.\\d+)*)"));
    regexps.push_back(MarkedRegExp("Firefox", "\\sFirefox/(\\d+.\\d+(?:.\\d+)*)"));
    regexps.push_back(MarkedRegExp("Netscape", "\\sNetscape(?:6)?/(\\d+\\.\\d+(?:\\.\\d+)*)"));
    regexps.push_back(MarkedRegExp("CS", "\\sCS(?: )?2000 (\\d+\\.\\d+(?:\\.\\d+)*)"));
    regexps.push_back(MarkedRegExp("AOL", "\\sAOL/(\\d+\\.\\d+(?:\\.\\d+)*)"));
    regexps.push_back(MarkedRegExp("Firebird", "\\sFirebird/(\\d+\\.\\d+(?:\\.\\d+)*)"));
    regexps.push_back(MarkedRegExp("Camino", "\\sCamino/(\\d+\\.\\d+(?:\\.\\d+)*)"));
    regexps.push_back(MarkedRegExp("Safari", "\\sVersion/(\\d+[.\\d]*)\\s+Safari/"));
    regexps.push_back(MarkedRegExp("Mozilla", "Mozilla/(\\d+\\.\\d+(?:\\.\\d+)*).*Gecko/\\d+$"));

    Generics::CPUTimer timer;
    timer.start();

    for(int i = 0; i < ITERS; ++i)
    {
      for(std::list<MarkedRegExp>::const_iterator it = regexps.begin();
          it != regexps.end(); ++it)
      {
        if(str.find(it->marker) != std::string::npos)
        {
          String::RegEx::Result res;
          if(it->re.search(res, str))
          {
            break;
          }
        }
      }
    }

    timer.stop();
    std::cout << "2: " << timer.elapsed_time() << std::endl;
  }
}


TEST(MatchTest)
{
  UrlMatcher_var url_matcher(new UrlMatcher());

  url_matcher->add_rule(
    10U,
    "test.com",
    "q=(.*)",
    0U,
    0,
    0);

  url_matcher->add_rule(
    20U,
    "test2.*",
    "q=(.*)",
    0U,
    0,
    0);

  url_matcher->add_rule(
    30U,
    "test2.*.*",
    "q2=(.*)",
    0U,
    0,
    0);

  url_matcher->add_rule(
    40U,
    "ask.com",
    ".*[?&]q=([^&]+).*",
    1U,
    0,
    0);

  url_matcher->add_rule(
    38U,
    "live.*.*",
    ".*[?&](?:q=|searchText=|siteSearchQuery=)([^&]+).*",
    1U,
    0,
    0);

  std::string res;
  unsigned long search_engine_id = 0;

  ASSERT_TRUE(
    url_matcher->match(
      search_engine_id,
      res,
      String::SubString("test.com/q=test"),
      0));

  ASSERT_TRUE(
    url_matcher->match(
      search_engine_id,
      res,
      String::SubString("test2.com/q=test"),
      0));

  ASSERT_TRUE(
    url_matcher->match(
      search_engine_id,
      res,
      String::SubString("test2.co.uk/q2=test"),
      0));

  ASSERT_TRUE(
    url_matcher->match(
      search_engine_id,
      res,
      String::SubString("http://br.ask.com/?l=dis&o=14595&qsrc=119&"
      "q=(tfr)%20paulo%20aplicou%20dois%20capitais%20a%20juros%20simples%20comerciais:%20o%20primeiro%20a%20taxa%20de%2072%%20aa,%20durante%20seis%20meses,%20e%20o%20segundo"),
      0));

  ASSERT_FALSE(
    url_matcher->match(
      search_engine_id,
      res,
      String::SubString("http://search.live.de/results.aspx?q=ADSCOpenRTBCPM1"),
      0));
}

RUN_TESTS
