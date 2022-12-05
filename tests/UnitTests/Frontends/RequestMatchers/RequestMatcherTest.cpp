
#include "../../TestHelpers.hpp"

#include <iostream>

#include <Generics/Time.hpp>
#include <Frontends/FrontendCommons/RequestMatchers.hpp>

using namespace FrontendCommons;

const std::string USER_AGENTS[] = {
  "BlackBerry9000/5.0.0.93 Profile/MIDP-2.0 Configuration/CLDC-1.1 VendorID/179",
  "Mozilla/5.0 (BlackBerry; U; BlackBerry 9900; en-US) AppleWebKit/534.11+ (KHTML, like Gecko) Version/7.0.0.261 Mobile Safari/534.11+",
  "Mozilla/5.0 (PlayBook; U; RIM Tablet OS 1.0.0; en-US) AppleWebKit/534.8+ (KHTML, like Gecko) Version/0.0.1 Safari/534.8+",
  "Mozilla/5.0 (Linux; U; Android 1.6; en-us; eeepc Build/Donut) AppleWebKit/528.5+ (KHTML, like Gecko) Version/3.1.2 Mobile Safari/525.20.1",
  "Mozilla/5.0 (Linux; U; Android 2.1-update1; ru-ru; GT-I9000 Build/ECLAIR) AppleWebKit/530.17 (KHTML, like Gecko) Version/4.0 Mobile Safari/530.17",
  "Mozilla/5.0 (Linux; U; Android 2.2; ru-ru; GT-I9000 Build/FROYO) AppleWebKit/533.1 (KHTML, like Gecko) Version/4.0 Mobile Safari/533.1",
  "Mozilla/5.0 (Linux; U; Android 3.1; en-us; GT-P7510 Build/HMJ37) AppleWebKit/534.13 (KHTML, like Gecko) Version/4.0 Safari/534.13",
  "Mozilla/5.0 (Linux; U; Android 2.1-update1 (7hero-astar9.3); ru-ru; HTC Legend Build/ERE27) "
    "AppleWebKit/530.17 (KHTML, like Gecko) Version/4.0 Mobile Safari/530.17",
  "Mozilla/5.0 (X11; U; Linux i686 (x86_64); en-US; rv:1.8.1.6) Gecko/20070817 IceWeasel/2.0.0.6-g2",
  "Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; Trident/5.0)",
  "Mozilla/4.0 (compatible; MSIE 6.0; Windows CE; IEMobile m.n)",
  "Mozilla/5.0 (Windows; U; Windows CE 4.21; rv:1.8b4) Gecko/20050720 Minimo/0.007",
  "Mozilla/5.0 (Windows; I; Windows NT 5.1; ru; rv:1.9.2.13) Gecko/20100101 Firefox/4.0",
  "Opera/9.80 (Windows NT 6.1; U; ru) Presto/2.8.131 Version/11.10",
  "Opera/9.80 (Macintosh; Intel Mac OS X 10.6.7; U; ru) Presto/2.8.131 Version/11.10",
  "Opera/9.60 (J2ME/MIDP; Opera Mini/4.2.14912/812; U; ru) Presto/2.4.15",
  "Mozilla/5.0 (SymbianOS/9.1; U; en-us) AppleWebKit/413 (KHTML, like Gecko) Safari/413",
  "Mozilla/4.0 (PSP (PlayStation Portable); 2.00)"
  };

TEST(PlatformMatcherPerformanceTest)
{
  PlatformMatcher_var platform_matcher(new PlatformMatcher());

  platform_matcher->add_rule(20010061, String::SubString("blackberry"), "", "", "BlackBerry", "", "", 7);
  platform_matcher->add_rule(20010061, String::SubString("blackberry"), "", "", "PlayBook", "", "", 5);
  platform_matcher->add_rule(20010062, String::SubString("iphone"), "", "", "iPhone", "", "\\s+OS\\s+([_.\\d]*)", 7);
  platform_matcher->add_rule(20010063, String::SubString("ipad"), "", "", "iPad", "", "\\s+OS\\s+([_.\\d]*)", 7);
  platform_matcher->add_rule(20010064, String::SubString("android"), "", "", "Android", "", "Android\\s+([_.\\d]*)", 7);
  platform_matcher->add_rule(20010064, String::SubString("android"), "", "", "HTC_Sensation_", "", "", 5);
  platform_matcher->add_rule(20010064, String::SubString("android"), "", "", "Silk", "", "", 6);
  platform_matcher->add_rule(20010065, String::SubString("windows phone os"), "", "", "Windows Phone OS", "", "Windows Phone OS\\s+([_.\\d]*)", 7);
  platform_matcher->add_rule(20010066, String::SubString("palmos"), "", "", "PalmOS", "", "", 7);
  platform_matcher->add_rule(20010066, String::SubString("palmos"), "", "", "PalmSource", "", "", 7);
  platform_matcher->add_rule(20010067, String::SubString("symbos"), "", "", "SymbOS", "", "", 7);
  platform_matcher->add_rule(20010067, String::SubString("symbos"), "", "", "Symbian", "", "", 7);
  platform_matcher->add_rule(20010068, String::SubString("windows mobile"), "", "", "HTC-S", "", "", 5);
  platform_matcher->add_rule(20010068, String::SubString("windows mobile"), "", "", "HTC_HD2_", "", "", 5);
  platform_matcher->add_rule(20010068, String::SubString("windows mobile"), "", "", "HTC_Touch_", "", "", 5);
  platform_matcher->add_rule(20010068, String::SubString("windows mobile"), "", "", "Windows CE", "", "", 7);
  platform_matcher->add_rule(20010068, String::SubString("windows mobile"), "", "", "Windows Mobile", "", "", 7);
  platform_matcher->add_rule(20010068, String::SubString("windows mobile"), "", "", "XV6850", "", "", 5);
  platform_matcher->add_rule(20010069, String::SubString("mac os x"), "", "", "Mac OS X", "", "Mac OS X\\s+([\\d._]+)", 1);
  platform_matcher->add_rule(20010070, String::SubString("linux"), "", "", "Linux", "", "Linux\\s+([\\d._]+)", 1);
  platform_matcher->add_rule(20010071, String::SubString("windows"), "", "", "Win", "", "Win([A-Z]*)([.0-9]*)", 1);
  platform_matcher->add_rule(20010071, String::SubString("windows"), "", "", "Windows", "", "Windows\\s+([.0-9A-Z ]+)", 2);
  platform_matcher->add_rule(20010072, String::SubString("windows me"), "", "", "Win 9x 4.90", "", "", 2);
  platform_matcher->add_rule(20010073, String::SubString("openbsd"), "", "", "OpenBSD", "", "OpenBSD\\s+([\\d._]+)", 1);
  platform_matcher->add_rule(20010074, String::SubString("freebsd"), "", "", "FreeBSD", "", "FreeBSD\\s+([\\d._]+)", 1);
  platform_matcher->add_rule(20010075, String::SubString("netbsd"), "", "", "NetBSD", "", "NetBSD\\s+([\\d._]+)", 1);
  platform_matcher->add_rule(20010076, String::SubString("sunos"), "", "", "SunOS", "", "SunOS\\s+([\\d._]+)", 1);
  platform_matcher->add_rule(20015122, String::SubString("maemo"), "", "", "Maemo", "", "", 7);
  platform_matcher->add_rule(20015123, String::SubString("e-ink"), "", "", "Kindle", "", "", 5);
  platform_matcher->add_rule(20015124, String::SubString("mobile os"), "", "", "Fennec", "", "", 4);
  platform_matcher->add_rule(20015124, String::SubString("mobile os"), "", "", "Minimo", "", "", 4);
  platform_matcher->add_rule(20015124, String::SubString("mobile os"), "", "", "Mobile", "", "", 4);
  platform_matcher->add_rule(20015124, String::SubString("mobile os"), "", "", "midori", "", "", 4);
  platform_matcher->add_rule(20015124, String::SubString("mobile os"), "", "", "tear", "", "", 4);
  platform_matcher->add_rule(20015141, String::SubString("tablet"), "DEVICE-TABLET", "", "A101IT", "", "", 1);
  platform_matcher->add_rule(20015141, String::SubString("tablet"), "DEVICE-TABLET", "", "A70BHT", "", "", 1);
  platform_matcher->add_rule(20015141, String::SubString("tablet"), "DEVICE-TABLET", "", "Advent Vega", "", "", 1);
  platform_matcher->add_rule(20015141, String::SubString("tablet"), "DEVICE-TABLET", "", "Dell Streak 7", "", "", 1);
  platform_matcher->add_rule(20015141, String::SubString("tablet"), "DEVICE-TABLET", "", "GT-P10", "", "", 1);
  platform_matcher->add_rule(20015141, String::SubString("tablet"), "DEVICE-TABLET", "", "HTC_Flyer", "", "", 1);
  platform_matcher->add_rule(20015141, String::SubString("tablet"), "DEVICE-TABLET", "", "Ideos S7", "", "", 1);
  platform_matcher->add_rule(20015141, String::SubString("tablet"), "DEVICE-TABLET", "", "MID7015", "", "", 1);
  platform_matcher->add_rule(20015141, String::SubString("tablet"), "DEVICE-TABLET", "", "SC-01C", "", "", 1);
  platform_matcher->add_rule(20015141, String::SubString("tablet"), "DEVICE-TABLET", "", "SCH-I800", "", "", 1);
  platform_matcher->add_rule(20015141, String::SubString("tablet"), "DEVICE-TABLET", "", "SGH-I987", "", "", 1);
  platform_matcher->add_rule(20015141, String::SubString("tablet"), "DEVICE-TABLET", "", "SGH-T849", "", "", 1);
  platform_matcher->add_rule(20015141, String::SubString("tablet"), "DEVICE-TABLET", "", "SHW-M180L", "", "", 1);
  platform_matcher->add_rule(20015141, String::SubString("tablet"), "DEVICE-TABLET", "", "SHW-M180S", "", "", 1);
  platform_matcher->add_rule(20015141, String::SubString("tablet"), "DEVICE-TABLET", "", "SPH-P100", "", "", 1);
  platform_matcher->add_rule(20015141, String::SubString("tablet"), "DEVICE-TABLET", "", "Sprint ATP51", "", "", 1);
  platform_matcher->add_rule(20015141, String::SubString("tablet"), "DEVICE-TABLET", "", "Tablet", "", "", 1);
  platform_matcher->add_rule(20015141, String::SubString("tablet"), "DEVICE-TABLET", "", "ViewPad7", "", "", 1);
  platform_matcher->add_rule(20015141, String::SubString("tablet"), "DEVICE-TABLET", "", "iPad", "", "", 1);
  platform_matcher->add_rule(20015141, String::SubString("tablet"), "DEVICE-TABLET", "", "pandigitalnova", "", "", 1);
  platform_matcher->add_rule(20015141, String::SubString("tablet"), "DEVICE-TABLET", "", "pandigitalsprnova", "", "", 1);
  platform_matcher->add_rule(20015141, String::SubString("tablet"), "DEVICE-TABLET", "", "zt180", "", "", 1);
  platform_matcher->add_rule(20015142, String::SubString("smartphone"), "DEVICE-SMARTPHONE", "", "HTC Magic", "", "", 1);
  platform_matcher->add_rule(20015142, String::SubString("smartphone"), "DEVICE-SMARTPHONE", "", "HTCX06HT", "", "", 1);
  platform_matcher->add_rule(20015142, String::SubString("smartphone"), "DEVICE-SMARTPHONE", "", "Mobile", "", "", 1);
  platform_matcher->add_rule(20015142, String::SubString("smartphone"), "DEVICE-SMARTPHONE", "", "Nexus One", "", "", 1);
  platform_matcher->add_rule(20015142, String::SubString("smartphone"), "DEVICE-SMARTPHONE", "", "SC-02B", "", "", 1);
  platform_matcher->add_rule(20015142, String::SubString("smartphone"), "DEVICE-SMARTPHONE", "", "fone 945", "", "", 1);
  platform_matcher->add_rule(20028061, String::SubString("webos"), "", "", "webOS", "", "\\s+webOS/([_.\\d]*)", 7);
  platform_matcher->add_rule(20029402, String::SubString("meego"), "", "", "MeeGo", "", "", 7);

  const size_t examples_size = sizeof(USER_AGENTS) / sizeof(USER_AGENTS[0]);
  const size_t repeats_count = 100000 * examples_size;
  size_t matched = 0;
  long checksum = 0;

  Generics::CPUTimer timer;
  timer.start();

  for (size_t i = 0; i < repeats_count; ++i)
  {
    const size_t inx = i % examples_size;
    std::string short_os;
    std::string os;
    std::set<unsigned long> platform_id_set;
    
    if (platform_matcher->match(&platform_id_set, short_os, os, USER_AGENTS[inx]))
    {
      checksum += (os[2] + short_os[1] + platform_id_set.size());
      ++matched;
    }
  }

  timer.stop();

  const long expected_checksum = 340400000;
  std::cout << "elapsed time: " << timer.elapsed_time() << std::endl;
  ASSERT_EQUALS(checksum, expected_checksum);
  ASSERT_EQUALS(matched, 1600000U);
}

TEST(PlatformMatcherEmptyMarker)
{
  PlatformMatcher_var platform_matcher(new PlatformMatcher());
  platform_matcher->add_rule(20010061, String::SubString("blackberry"), "DEVICE", "", "", "", "", 7);
  platform_matcher->add_rule(20010070, String::SubString("linux"), "", "", "Linux", "", "Linux\\s+([\\d._]+)", 1);

  {
    std::string short_os;
    std::string os;
    std::set<unsigned long> platform_id_set;

    ASSERT_TRUE (platform_matcher->match(
      &platform_id_set, short_os, os, USER_AGENTS[1]))
    ASSERT_EQUALS (platform_id_set.size(), 1U);
    ASSERT_EQUALS (*platform_id_set.begin(), 20010061U);
  }

  {
    std::string short_os;
    std::string os;
    std::set<unsigned long> platform_id_set;

    ASSERT_TRUE (platform_matcher->match(
      &platform_id_set, short_os, os, USER_AGENTS[4]))
    ASSERT_EQUALS (platform_id_set.size(), 2U);
  }
}

TEST(WebBrowserMatcherEmptyMarker)
{
  WebBrowserMatcher_var matcher(new WebBrowserMatcher());
  matcher->add_rule(String::SubString("IE"), "IE", "\\sMSIE\\s+([.\\d]*)", true, 2);
  matcher->add_rule(String::SubString("Firefox"), "", "\\sFirefox/([.\\d]*)", true, 3);

  {
    std::string browser;
    ASSERT_TRUE (matcher->match(browser, USER_AGENTS[10]))
    ASSERT_EQUALS (browser, "IE 6.0");
  }

  {
    std::string browser;
    ASSERT_TRUE (matcher->match(browser, USER_AGENTS[12]))
    ASSERT_EQUALS (browser, "Firefox 4.0");
  }
}

TEST(WebBrowserMatcherPerformanceTest)
{
  WebBrowserMatcher_var matcher(new WebBrowserMatcher());
  matcher->add_rule(String::SubString("AOL"), "AOL", "\\sAOL/([.\\d]*)", true, 3);

  matcher->add_rule(String::SubString("AOL"), "America Online Browser", "\\sAmerica Online Browser\\s+([.\\d]*)", true, 3);
  matcher->add_rule(String::SubString("CS"), "CS", "\\sCS(?: )?2000 ([.\\d]*)", true, 3);
  matcher->add_rule(String::SubString("Camino"), "Camino", "\\sCamino/([.\\d]*)", true, 3);
  matcher->add_rule(String::SubString("Chrome"), "Chrome", "\\sChrome[/ ]([.\\d]*)", true, 3);
  matcher->add_rule(String::SubString("Firebird"), "Firebird", "\\sFirebird/([.\\d]*)", true, 3);
  matcher->add_rule(String::SubString("Firefox"), "Firefox", "\\sFirefox/([.\\d]*)", true, 3);
  matcher->add_rule(String::SubString("IE"), "IE", "\\sMSIE\\s+([.\\d]*)", true, 2);
  matcher->add_rule(String::SubString("IEMobile"), "IEMobile", "\\sIEMobile/([.\\d]*)", true, 3);
  matcher->add_rule(String::SubString("Mozilla"), "Mozilla", "Mozilla/(\\d+\\.\\d+(?:\\.\\d+)*).*Gecko/\\d+$", true, 1);
  matcher->add_rule(String::SubString("Netscape"), "Netscape", "\\sNetscape(?:6)?/([.\\d]*)", true, 3);
  matcher->add_rule(String::SubString("Opera"), "Opera", "Opera/.*\\sVersion/([.\\d]*)", true, 3);
  matcher->add_rule(String::SubString("Opera Mini"), "Opera Mini", "Opera Mini/([.\\d]*)", true, 3);
  matcher->add_rule(String::SubString("Safari"), "Safari", "\\sVersion/(\\d+[.\\d]*)(?:\\s+.*)?\\s+Safari/", true, 3);

  const size_t examples_size = sizeof(USER_AGENTS) / sizeof(USER_AGENTS[0]);
  const size_t repeats_count = 100000 * examples_size;
  size_t matched = 0;
  long checksum = 0;

  Generics::CPUTimer timer;
  timer.start();

  for (size_t i = 0; i < repeats_count; ++i)
  {
    const size_t inx = i % examples_size;
    std::string browser;
    
    if (matcher->match(browser, USER_AGENTS[inx]))
    {
      checksum += (browser[1] + browser.size());
      ++matched;
    }
  }

  timer.stop();

  const long expected_checksum = 140300000;
  std::cout << "elapsed time: " << timer.elapsed_time() << std::endl;
  ASSERT_EQUALS(checksum, expected_checksum);
  ASSERT_EQUALS(matched, 1300000U);
}

/**
 * New browser family items;
 *  see ADSC-7041
 *  see ADSC-7409
 */
TEST(WebBrowserMatcherDetectionTest)
{
  WebBrowserMatcher_var matcher(new WebBrowserMatcher());
  matcher->add_rule(String::SubString("AOL"), "AOL", "\\sAOL/([.\\d]*)", true, 3);

  matcher->add_rule(String::SubString("AOL"), "America Online Browser", "\\sAmerica Online Browser\\s+([.\\d]*)", true, 3);
  matcher->add_rule(String::SubString("CS"), "CS", "\\sCS(?: )?2000 ([.\\d]*)", true, 3);
  matcher->add_rule(String::SubString("Camino"), "Camino", "\\sCamino/([.\\d]*)", true, 3);
  matcher->add_rule(String::SubString("Chrome"), "Chrome", "\\sChrome[/ ]([.\\d]*)", true, 3);
  matcher->add_rule(String::SubString("Firebird"), "Firebird", "\\sFirebird/([.\\d]*)", true, 3);
  matcher->add_rule(String::SubString("Firefox"), "Firefox", "\\sFirefox/([.\\d]*)", true, 3);
  matcher->add_rule(String::SubString("IE"), "IE", "\\sMSIE\\s+([.\\d]*)", true, 2);
  matcher->add_rule(String::SubString("IEMobile"), "IEMobile", "\\sIEMobile/([.\\d]*)", true, 3);
  matcher->add_rule(String::SubString("Mozilla"), "Mozilla", "Mozilla/(\\d+\\.\\d+(?:\\.\\d+)*).*Gecko/\\d+$", true, 1);
  matcher->add_rule(String::SubString("Netscape"), "Netscape", "\\sNetscape(?:6)?/([.\\d]*)", true, 3);
  matcher->add_rule(String::SubString("Opera"), "Opera", "Opera/.*\\sVersion/([.\\d]*)", true, 3);
  matcher->add_rule(String::SubString("Safari"), "Safari", "\\sVersion/(\\d+[.\\d]*)(?:\\s+.*)?\\s+Safari/", true, 3);

  // ADSC-7041
  matcher->add_rule(String::SubString("Opera Mobile"), "Opera Mobi", "Opera Mobi/.*\\sVersion/([.\\d]*)", true, 5);
  matcher->add_rule(String::SubString("Safari Mobile"), "Safari", "\\sVersion/([.\\d]*)\\sMobile.*Safari", true, 5);
  matcher->add_rule(String::SubString("Opera Mini"), "Opera Mini", "Opera Mini/([.\\d]*)", true, 5);
  matcher->add_rule(String::SubString("Chrome Mobile"), "Chrome", "\\sChrome[/ ]([.\\d]*).*Mobile", true, 5);
  matcher->add_rule(String::SubString("Firefox Mobile"), "Fennec", "\\sFirefox/([.\\d]*)", false, 5);
  matcher->add_rule(String::SubString("Firefox Mobile"), "Firefox", "Mobile.*\\sFirefox/([.\\d]*)", true, 5);
  matcher->add_rule(String::SubString("UC Browser"), "UCBrowser", "UCBrowser/([.\\d]*)", false, 7);
  matcher->add_rule(String::SubString("Chrome Mobile"), "CriOS", "CriOS/([.\\d]*).*Mobile", false, 5);
  matcher->add_rule(String::SubString("Opera Mobile"), "Opera Tablet", "Opera Tablet/.*Version/([.\\d]*)", true, 5);

  // ADSC-7409
  matcher->add_rule(String::SubString("IE"), "Trident", "^Mozilla.+Trident/7\\.0.+rv:([.\\d]*)", true, 3);

  // ADSC-7515
  matcher->add_rule(String::SubString("Nokia Browser"), "NokiaBrowser", "NokiaBrowser/([.\\d]*)", false, 5);

  // ADSC-7828
  matcher->add_rule(String::SubString("Yandex Browser"), "YaBrowser", "YaBrowser/([.\\d]*)", false, 5);

  // ADSC-7902
  matcher->add_rule(String::SubString("Android Browser"), "Android", "Android.*AppleWebKit.*Version/([.\\d]*)", true, 6);


  {
    std::string browser;
    const std::string user_agent("Mozilla/5.0 (iPhone; U; CPU iPhone OS 4_3_5 like Mac OS X; ru-ru) AppleWebKit/533.17.9 (KHTML, like Gecko) Version/5.0.2 Mobile/8L1 Safari/6533.18.5");
    ASSERT_TRUE( matcher->match(browser, user_agent) );
    ASSERT_EQUALS( browser, "Safari Mobile 5.0.2" );
  }

  {
    std::string browser;
    const std::string user_agent("Mozilla/5.0 (Linux U; Android 2.2.1; en-us; lepad_001b Build/PQXU100.4.0097) AppleWebKit/533.1 (KHTML, like Gecko) Version/4.0 Mobile Safari 533.1");
    ASSERT_TRUE( matcher->match(browser, user_agent) );
    ASSERT_EQUALS( browser, "Android Browser 4.0" );
  }

  {
    std::string browser;
    const std::string user_agent("Opera/9.80 (iPhone; Opera mini/6.1.15738/27.1251; U; ru) Presto/2.9.119 Version/11.10");
    ASSERT_TRUE( matcher->match(browser, user_agent) );
    ASSERT_EQUALS( browser, "Opera Mini 6.1.15738" );
  }

  {
    std::string browser;
    const std::string user_agent("Opera/9.80 (Android 2.3.4; Linux; Opera Mobi/ADR-1202231246; U; en) Presto/2.10.254 Version/12.00");
    ASSERT_TRUE( matcher->match(browser, user_agent) );
    ASSERT_EQUALS( browser, "Opera Mobile 12.00" );
  }

  {
    std::string browser;
    const std::string user_agent("Mozilla/5.0 (iPhone; CPU iPhone OS 6_0 like Mac OS X; ru-ru) AppleWebKit/534.46.0 (KHTML, like Gecko) CriOS/21.0.1180.82 Mobile/10A403 Safari/7534.48.3");
    ASSERT_TRUE( matcher->match(browser, user_agent) );
    ASSERT_EQUALS( browser, "Chrome Mobile 21.0.1180.82" );
  }

  {
    std::string browser;
    const std::string user_agent("Mozilla/5.0 (Linux; Android 4.1.1; Galaxy Nexus Build/JRO03C) AppleWebKit/535.19 (KHTML, like Gecko) Chrome/18.0.1025.166 Mobile Safari/535.19");
    ASSERT_TRUE( matcher->match(browser, user_agent) );
    ASSERT_EQUALS( browser, "Chrome Mobile 18.0.1025.166" );
  }

  {
    std::string browser;
    const std::string user_agent("Mozilla/5.0 (Android; Linux armv7l; rv:10.0.2) Gecko/20120215 Firefox/10.0.2 Fennec/10.0.2");
    ASSERT_TRUE( matcher->match(browser, user_agent) );
    ASSERT_EQUALS( browser, "Firefox Mobile 10.0.2" );
  }

  {
    std::string browser;
    const std::string user_agent("Mozilla/5.0 (Android; Mobile; rv:18.0) Gecko/18.0 Firefox/18.0");
    ASSERT_TRUE( matcher->match(browser, user_agent) );
    ASSERT_EQUALS( browser, "Firefox Mobile 18.0" );
  }

  {
    std::string browser;
    const std::string user_agent("Mozilla/5.0 (Linux; U; Android 2.3.6; en-us ; GT-S5300 Build/GINGERBREAD) AppleWebKit/533.1 (KHTML, like Gecko) Version/4.0 Mobile Safari/533.1/UCBrowser/8.6.1.262/145/355");
    ASSERT_TRUE( matcher->match(browser, user_agent) );
    ASSERT_EQUALS( browser, "UC Browser 8.6.1.262" );
  }

  {
    std::string browser;
    const std::string user_agent("Opera/9.80 (iPhone; Opera mini/6.1.15738/27.1251; U; ru) Presto/2.9.119 Version/11.10");
    ASSERT_TRUE( matcher->match(browser, user_agent) );
    ASSERT_EQUALS( browser, "Opera Mini 6.1.15738" );
  }

  {
    std::string browser;
    const std::string user_agent("Opera/9.80 (BlackBerry; Opera mini/6.1.15738/27.1251; U; ru) Presto/2.9.119 Version/11.10");
    ASSERT_TRUE( matcher->match(browser, user_agent) );
    ASSERT_EQUALS( browser, "Opera Mini 6.1.15738" );
  }

  {
    std::string browser;
    const std::string user_agent("Opera/9.80 (Android 3.1; Linux; Opera Tablet/ADR-1111101157; U; ru) Presto/2.9.201 Version/11.50");
    ASSERT_TRUE( matcher->match(browser, user_agent) );
    ASSERT_EQUALS( browser, "Opera Mobile 11.50" );
  }

  {
    std::string browser;
    const std::string user_agent("Mozilla/5.0 (Windows NT 6.1; WOW64; Trident/7.0; rv:11.0) like Gecko");
    ASSERT_TRUE( matcher->match(browser, user_agent) );
    ASSERT_EQUALS( browser, "IE 11.0" );
  }
  
  {
    std::string browser;
    const std::string user_agent("Mozilla/5.0 (MeeGo; NokiaN9) AppleWebKit/534.13 (KHTML, like Gecko) NokiaBrowser/8.5.0 Mobile Safari/534.13");
    ASSERT_TRUE( matcher->match(browser, user_agent) );
    ASSERT_EQUALS( browser, "Nokia Browser 8.5.0" );
  }

  {
    std::string browser;
    const std::string user_agent("Mozilla/5.0 (Windows NT 6.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/28.0.1500.95 YaBrowser/13.10.1500.9323 Safari/537.36");
    ASSERT_TRUE( matcher->match(browser, user_agent) );
    ASSERT_EQUALS( browser, "Yandex Browser 13.10.1500.9323" );
  }

  {
    std::string browser;
    const std::string user_agent("Mozilla/5.0 (Linux; U; Android 4.2; en-us; Nexus 10 Build/JVP15I) AppleWebKit/534.30 (KHTML, like Gecko) Version/4.0 Safari/534.30");
    ASSERT_TRUE( matcher->match(browser, user_agent) );
    ASSERT_EQUALS( browser, "Android Browser 4.0" );
  }

  {
    std::string browser;
    const std::string user_agent("Mozilla/5.0 (Linux; U; Android 4.2.1; ru-ru; Galaxy Nexus Build/JOP40D) AppleWebKit/534.30 (KHTML, like Gecko) Version/4.0 Mobile Safari/534.30");
    ASSERT_TRUE( matcher->match(browser, user_agent) );
    ASSERT_EQUALS( browser, "Android Browser 4.0" );
  }

  {
    std::string browser;
    const std::string user_agent("Mozilla/5.0 (Linux; U; Android 2.2.2; ru-ru; YP-G70 Build/FROYO) AppleWebKit/533.1 (KHTML, like Gecko) Version/4.0 Mobile Safari/533.1");
    ASSERT_TRUE( matcher->match(browser, user_agent) );
    ASSERT_EQUALS( browser, "Android Browser 4.0" );
  }
}

RUN_TESTS
