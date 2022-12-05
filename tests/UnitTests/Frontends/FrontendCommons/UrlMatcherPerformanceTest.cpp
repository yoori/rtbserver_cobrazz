/// @file FrontendCommons/TestFrontend.cpp
#include <iostream>

//#include "TestFrontend.hpp"
//#include "UrlMatcherTest.hpp"

#include <Generics/Time.hpp>
#include <Generics/Rand.hpp>
#include <Generics/HashTable.hpp>
#include <Generics/HashTableAdapters.hpp>
#include <String/UTF8Case.hpp>

#include "UrlMatcherTest.hpp"

namespace
{
  const std::size_t TEST_URLS_AMOUNT = 100000;

  /// Select from PermanentFeConfig.xml regexps that
  ///  match host name exactly = RE_SET.
  const char* RE_SET[] =
  {
    "http://kr\\.search\\.yahoo"
        "(?:\\.\\w+)+/[^\\?\\&]*(?:[\\?|\\&]fr=kr-front[-_]sb.*[\\&]p=([^\\&]+)).*",
    "http://kr\\.search\\.yahoo"
    "(?:\\.\\w+)+/[^\\?\\&]*(?:[\\?|\\&]p=([^\\&]*).*[\\?|\\&]fr=kr-front[-_]sb).*",
    "http://kr\\.search\\.yahoo"
        "(?:\\.\\w+)+/[^\\?\\&]*(?:[\\?|\\&]p=([^\\&]*).*[\\?|\\&]).*",
    "http://br\\.search\\.yahoo"
        "(?:\\.\\w+)+/[^\\?\\&]*(?:[\\?|\\&](?:vc=&amp;)?p=([^\\&]+)).*",
    "http://kr\\.altavista\\.com"
    "(?:/\\w+)+/(?:results|.+)(?:[?|&].*?)q=([^&]+).*",
    "http://us\\.yhs4\\.search\\.yahoo\\.com"
    "/yhs/search(?:.*?)cs=iso88591(?:.*?)(?:\\?|&)q=([^&]+).*",
    "http://us\\.yhs4\\.search\\.yahoo\\.com"
    "/yhs/search(?:.*?)(?:\\?|&)q=([^&]+).*",
  };
  const std::size_t RE_SET_AMOUNT = sizeof(RE_SET) / sizeof(RE_SET[0]);

  // Because hostnames already matched will use supplied regulars
  const char* RE_OPT_SET[] =
  {
    "[^\\?\\&]*(?:[\\?|\\&]fr=kr-front[-_]sb.*[\\&]p=([^\\&]+)).*",
    "[^\\?\\&]*(?:[\\?|\\&]p=([^\\&]*).*[\\?|\\&]fr=kr-front[-_]sb).*",
    "[^\\?\\&]*(?:[\\?|\\&]p=([^\\&]*).*[\\?|\\&]).*",
    "[^\\?\\&]*(?:[\\?|\\&](?:vc=&amp;)?p=([^\\&]+)).*",
    "(?:\\w+)+/(?:results|.+)(?:[?|&].*?)q=([^&]+).*",
    "yhs/search(?:.*?)cs=iso88591(?:.*?)(?:\\?|&)q=([^&]+).*",
    "yhs/search(?:.*?)(?:\\?|&)q=([^&]+).*",
  };
  const std::size_t RE_OPT_SET_AMOUNT = sizeof(RE_OPT_SET) / sizeof(RE_OPT_SET[0]);

  /// Hostnames from RE_SET
  const char* RE_SET_HOSTNAMES[] =
  {
    "http://kr.search.yahoo.com",
    "http://kr.search.yahoo.com",
    "http://kr.search.yahoo.com",
    "http://br.search.yahoo.com",
    "http://kr.altavista.com",
    "http://us.yhs4.search.yahoo.com",
    "http://us.yhs4.search.yahoo.com",
  };
  const std::size_t RE_SET_HOSTS_AMOUNT =
    sizeof(RE_SET_HOSTNAMES) / sizeof(RE_SET_HOSTNAMES[0]);

  //
  // Hostname + suffix = test input data
  const char* RE_SET_MATCHABLE_SUFFIXES[] =
  {
    "/words.words/some_symbols?fr=kr-front_sb&p=TEXT0",
    "/words.words/some_symbols&p=TEXT1?fr=kr-front_sb",
    "/words.words/some_symbols&p=TEXT2?other_text",
    "/words.words/some_symbols?vc=&amp;p=TEXT3",
    "/web/results?fr=altavista&itag=ody&q=TEXT4&kgs=1&kls=0?",
    "/yhs/search;cs=iso88591?q=TEXT5",
    "/yhs/searchOtherText?q=TEXT6",
  };
  const std::size_t RE_SET_MATCHABLE_SUFFIXES_AMOUNT =
    sizeof(RE_SET_MATCHABLE_SUFFIXES) / sizeof(RE_SET_MATCHABLE_SUFFIXES[0]);

  typedef
    xsd::AdServer::Configuration::SearchMatchersConfigType
      UrlMatcherConfig;

  typedef std::list<std::string> TestUrlsList;
  typedef std::list<String::RegEx> RegularsList;

  typedef Generics::StringHashAdapter HostnameHash;
  typedef Generics::HashTable<HostnameHash, RegularsList>
    HostnameRegexHashMap;

  HostnameRegexHashMap hostname_regex;
  HostnameRegexHashMap hostname_opt_regex;

  /**
   * Generate data sets need for testing
   */
  void
  generate_test_data_set(TestUrlsList& urls) /*throw(eh::Exception)*/
  {
    std::size_t hostname_index = 0;
    std::string url;
    // each 10 is matched by RE_SET (10%)
    for (std::size_t i = 0; i < TEST_URLS_AMOUNT; ++i)
    {
      url.clear();
      if (i%10 == 0)
      {
        url += RE_SET_HOSTNAMES[hostname_index];
        url += RE_SET_MATCHABLE_SUFFIXES[hostname_index];
        hostname_index++;
        hostname_index %= RE_SET_HOSTS_AMOUNT;
      }
      else
      {
        // 90% unmatched urls
        url += "http://www.google.com/search?Q=NOT_MATCHED";
      }
      urls.push_back(url);
    }
    /// Fill hash table for exactly matched hostnames
    for (std::size_t i = 0; i < RE_SET_AMOUNT; ++i)
    {
      HostnameRegexHashMap::iterator it =
        hostname_regex.find(RE_SET_HOSTNAMES[i]);

      if (it != hostname_regex.end())
      {
        // key already exists
        it->second.push_back(String::RegEx(RE_SET[i]));
      }
      else
      {
        // the key does not exist in the hash table.
        hostname_regex.insert(HostnameRegexHashMap::value_type(
          RE_SET_HOSTNAMES[i], RegularsList(1, String::RegEx(RE_SET[i])) ));
      }
    }
    for (std::size_t i = 0; i < RE_OPT_SET_AMOUNT; ++i)
    {
      HostnameRegexHashMap::iterator it =
        hostname_opt_regex.find(RE_SET_HOSTNAMES[i]);

      if (it != hostname_opt_regex.end())
      {
        // key already exists
        it->second.push_back(String::RegEx(RE_OPT_SET[i]));
      }
      else
      {
        // the key does not exist in the hash table.
        hostname_opt_regex.insert(HostnameRegexHashMap::value_type(
          RE_SET_HOSTNAMES[i], RegularsList(1, String::RegEx(RE_OPT_SET[i])) ));
      }
    }
  }

  void
  load_urls(const UrlMatcherConfig& config) /*throw(eh::Exception)*/;

  //
  // Used for get RegExps from xml and put into file
  void
  load_urls(const UrlMatcherConfig& config) /*throw(eh::Exception)*/
  {
    typedef
      FrontendCommons::UrlMatcher::UrlMatcherConfig::SearchMatcher_sequence
      SearchMatcherSeq;

    const SearchMatcherSeq& regexps_seq = config.SearchMatcher();
    Stream::Error ostr;

    for (SearchMatcherSeq::const_iterator regexp_it = regexps_seq.begin();
         regexp_it != regexps_seq.end();
         ++regexp_it)
    {
      try
      {
        ostr << regexp_it->regexp() << std::endl;
      }
      catch (const String::RegEx::Exception& e)
      {
        Stream::Error ostr;
        ostr
          << "load_urls(): processing regexp '"
          << regexp_it->regexp()
          << "' String::RegEx::Exception caught: "
          << e.what();

        throw TestException(ostr);
      }
      catch (const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << "load_urls(): In match element eh::Exception caught: "
          << e.what();

        throw TestException(ostr);
      }
    }
    throw TestException(ostr);
  }

  bool
  special_matching(const std::string& url, std::string& match_result)
    /*throw(eh::Exception)*/
  {
    const char FUN[] = "special_matching()";
    // Take hostname from URL
    std::string::size_type pos = url.find('/', 7);
    if (pos == std::string::npos)
    {
      return false;
    }
    HostnameRegexHashMap::iterator it = hostname_regex.find(url.substr(0, pos));
    if (it == hostname_regex.end())
    {
      return false;
    }
    try
    {
      for (RegularsList::const_iterator cit = (*it).second.begin();
        cit != (*it).second.end(); ++cit)
      {
        String::RegEx::Result sub_strs;
        if ((*cit).search(sub_strs, url) && (sub_strs.size() > 1))
        {
          try
          {
            std::string tmp;
            if (!String::StringManip::flatten(tmp,
              String::StringManip::trim_ret(sub_strs[1])))
            {
              continue;
            }
            if (!String::case_change<String::Uniform>(tmp, match_result))
            {
              continue;
            }
            return true;
          }
          catch (...)
          {
            std::cout << "Unknown exception" << std::endl;
          }
        }
      }
    }
    catch(const String::RegEx::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught String::RegEx::Exception has been caught: "
        << e.what();
      throw TestException(ostr);
    }

    return false;
  }

  bool
  very_special_matching(const std::string& url, std::string& match_result)
    /*throw(eh::Exception)*/
  {
    const char FUN[] = "very_special_matching()";
    // Take hostname from URL
    std::string::size_type pos = url.find('/', 7);
    if (pos == std::string::npos)
    {
      return false;
    }
    HostnameRegexHashMap::iterator it =
      hostname_opt_regex.find(url.substr(0, pos));
    if (it == hostname_opt_regex.end())
    {
      return false;
    }
    const char* url_rest = url.c_str() + pos + 1;
    try
    {
      for (RegularsList::const_iterator cit = (*it).second.begin();
        cit != (*it).second.end(); ++cit)
      {
        String::RegEx::Result sub_strs;
        if ((*cit).search(sub_strs, url_rest) && (sub_strs.size() > 1))
        {
          try
          {
            std::string tmp;
            if (!String::StringManip::flatten(tmp,
              String::StringManip::trim_ret(sub_strs[1])))
            {
              continue;
            }
            if (!String::case_change<String::Uniform>(tmp, match_result))
            {
              continue;
            }
            return true;
          }
          catch (...)
          {
            std::cout << "Unknown exception" << std::endl;
          }
        }
      }
    }
    catch(const String::RegEx::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught String::RegEx::Exception has been caught: "
        << e.what();
      throw TestException(ostr);
    }

    return false;
  }
}

void
performance_test(FrontendCommons::UrlMatcher& matcher) /*throw(eh::Exception)*/
{
  std::string match_result;

  TestUrlsList urls;
  generate_test_data_set(urls);

  Generics::CPUTimer timer;
  timer.start();
  for (TestUrlsList::const_iterator cit = urls.begin();
    cit != urls.end(); ++cit)
  {
//        bool result =
    matcher.match((*cit).c_str(), match_result, 0);

//      std::cout << *cit << " | "
//        << (result? "MATCH" : "false") << " | " << match_result << std::endl;
//      match_result.clear();
  }
  timer.stop();
  std::cout << "Time need to match 10% RE_SET and 90% unmatchable url,"
    " data set size="
    <<  TEST_URLS_AMOUNT << " elements:\n" << timer.elapsed_time()
    << std::endl;

  timer.start();
  for (TestUrlsList::const_iterator cit = urls.begin();
    cit != urls.end(); ++cit)
  {
//        bool result =
    special_matching((*cit), match_result);

//      std::cout << *cit << " | "
//        << (result? "MATCH" : "false") << " | " << match_result << std::endl;
//      match_result.clear();
  }
  timer.stop();
  std::cout << "Time need to match 10% RE_SET and 90% unmatchable url,"
    " data set size="
    <<  TEST_URLS_AMOUNT << " elements, SPECIAL algo:\n"
    << timer.elapsed_time() << std::endl;

  timer.start();
  for (TestUrlsList::const_iterator cit = urls.begin();
    cit != urls.end(); ++cit)
  {
//        bool result =
    very_special_matching((*cit), match_result);

//      std::cout << *cit << " | "
//        << (result? "MATCH" : "false") << " | " << match_result << std::endl;
//      match_result.clear();
  }
  timer.stop();
  std::cout << "Time need to match 10% RE_SET and 90% unmatchable url,"
    " data set size="
    <<  TEST_URLS_AMOUNT << " elements, VERY SPECIAL algo:\n"
    << timer.elapsed_time() << std::endl;

}

int
main() noexcept
{
  try
  {
    FrontendCommons::UrlMatcher_var matcher(
      new FrontendCommons::UrlMatcher());
    load_url_matcher(*matcher, "PermanentFeConfig.xml");
    performance_test(*matcher);
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << ex.what() << std::endl;
  }
  catch (...)
  {
    return 1;
  }

  return 0;
}

