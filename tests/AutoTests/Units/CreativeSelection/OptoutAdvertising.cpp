
#include "OptoutAdvertising.hpp"

REFLECT_UNIT(OptoutAdvertising) (
  "CreativeSelection",
  AUTO_TEST_FAST);

namespace
{
  typedef AutoTest::SpecialEffectsChecker SpecialEffectsChecker;

  enum TestCaseBehaviour
  {
    TCB_Redirected       = 1,  // Expect redirect on advertising request.
    TCB_EmptyPassback    = 2,  // Use request with empty passback url.
    TCB_CheckNoAdv       = 4,  // Check NO ADV special channel effect.
    TCB_RepeatReq        = 8  // Repeat ad request (useful for TA Campaigns).
  };

  // Test 6.1. CPM RON ads matching
  const OptoutAdvertising::TestCase RonAds[] =
  {
    {
     "OPTINCOLO",
     "DisplayRONCPMCC",
     0,
     0,
     TCB_Redirected
    },
    {
     "ALLCOLO", 
     "DisplayRONCPMCC",
     "DisplayRONCPMCC",
     "DisplayRONCPMCC",
     0
    },
    {
     "NONOPTOUTCOLO",
     "DisplayRONCPMCC",
     "DisplayRONCPMCC",
     0,
     TCB_Redirected
    },
    {
      "NOADSCOLO",
      0,
      0,
      0,
      TCB_Redirected
    },
    {
      "DELETEDCOLO",
      0,
      0,
      0,
      TCB_Redirected
    }
  };

  // Test 6.2. Targeted ads matching on urls
  const OptoutAdvertising::TestCase TargetedAdsOnUrls [] =
  {
    {
      "OPTINCOLO",
      "DisplayCPMCC",
      0,
      0,
      TCB_Redirected
    },
    {
      "ALLCOLO", 
      "DisplayCPMCC",
      "DisplayCPMCC",
      "DisplayCPMCC",
      0
    },
    {
      "NONOPTOUTCOLO", 
      "DisplayCPMCC",
      "DisplayCPMCC",
      0,
      TCB_Redirected
    },
    {
      "NOADSCOLO",
      0,
      0,
      0,
      TCB_Redirected
    },
    {
      "DELETEDCOLO",
      0,
      0,
      0,
      TCB_Redirected
    }      
  };

  // Test 6.5. Campaigns with frequency caps
  const OptoutAdvertising::TestCase FreqCapsAds [] =
  {
    {
      "OPTINCOLO",
      "DisplayFCCC",
      0,
      0,
      TCB_Redirected
    },
    {
      "ALLCOLO", 
      "DisplayFCCC",
      0,
      0,
      TCB_Redirected
    },
    {
      "NONOPTOUTCOLO", 
      "DisplayFCCC",
      0,
      0,
      TCB_Redirected
    },
    {
      "NOADSCOLO",
      0,
      0,
      0,
      TCB_Redirected
    },
    {
      "DELETEDCOLO",
      0,
      0,
      0,
      TCB_Redirected
    }      
  };

  // Test 6.6.1. Campaigns with action tracking (RON)
  const OptoutAdvertising::TestCase RONCPAAds [] =
  {
    {
      "OPTINCOLO",
      "DisplayRONCPACC",
      0,
      0,
      TCB_Redirected
    },
    {
      "ALLCOLO",
      "DisplayRONCPACC",
      0,
      0,
      TCB_Redirected
    },
    {
      "NONOPTOUTCOLO",
      "DisplayRONCPACC",
      0,
      0,
      TCB_Redirected
    },
    {
      "NOADSCOLO",
      0,
      0,
      0,
      TCB_Redirected
    },
    {
      "DELETEDCOLO",
      0,
      0,
      0,
      TCB_Redirected
    }      
  };

  // Test 6.6.2 Campaigns with action tracking
  const OptoutAdvertising::TestCase CPAAds [] =
  {
    {
      "OPTINCOLO",
      "DisplayCPACC",
      0,
      0,
      TCB_Redirected
    },
    {
      "ALLCOLO", 
      "DisplayCPACC",
      0,
      0,
      TCB_Redirected
    },
    {
      "NONOPTOUTCOLO", 
      "DisplayCPACC",
      0,
      0,
      TCB_Redirected
    },
    {
      "NOADSCOLO",
      0,
      0,
      0,
      TCB_Redirected
    },
    {
      "DELETEDCOLO", 
      0,
      0,
      0,
      TCB_Redirected
    }      
  };

  // Test 6.7. Campaigns with impression/click tracking
  const OptoutAdvertising::TestCase CPCAds [] =
  {
    {
      "OPTINCOLO",
      "DisplayCPCCC",
      0,
      0,
      TCB_Redirected
    },
    {
      "ALLCOLO",
      "DisplayCPCCC",
      "DisplayCPCCC",
      "DisplayCPCCC",
      0
    },
    {
      "NONOPTOUTCOLO",
      "DisplayCPCCC",
      "DisplayCPCCC",
      0,
      TCB_Redirected,
    },
    {
      "NOADSCOLO",
      0,
      0,
      0,
      TCB_Redirected
    },
    {
      "DELETEDCOLO",
      0,
      0,
      0,
      TCB_Redirected
    }      
  };

  // Test 6.8. Text RON campaigns
  const OptoutAdvertising::TestCase TextRonAds [] =
  {
    {
      "OPTINCOLO",
      "TEXTRONCC",
      0,
      0,
      TCB_Redirected
    },
    {
      "ALLCOLO",
      "TEXTRONCC",
      "TEXTRONCC",
      "TEXTRONCC",
      0
    },
    {
      "NONOPTOUTCOLO",
      "TEXTRONCC",
      "TEXTRONCC",
      0,
      TCB_Redirected,
    },
    {
      "NOADSCOLO",
      0,
      0,
      0,
      TCB_Redirected
    },
    {
      "DELETEDCOLO",
      0,
      0,
      0,
      TCB_Redirected
    }      
  };

  //Test 6.9. Text(C) targeted campaigns
  const OptoutAdvertising::TestCase TextAds [] =
  {
    {
      "OPTINCOLO",
      "TEXTCC",
      0,
      0,
      TCB_Redirected
    },
    {
      "ALLCOLO",
      "TEXTCC",
      "TEXTCC",
      "TEXTCC",
      0
    },
    {
      "NONOPTOUTCOLO",
      "TEXTCC",
      "TEXTCC",
      0,
      TCB_Redirected,
    },
    {
      "NOADSCOLO",
      0,
      0,
      0,
      TCB_Redirected
    },
    {
      "DELETEDCOLO",
      0,
      0,
      0,
      TCB_Redirected
    }      
  };

  // Test 6.10. Text(K) targeted campaigns
  const OptoutAdvertising::TestCase TAAds [] =
  {
    {
      "OPTINCOLO",
      "TACC",
      0,
      0,
      TCB_Redirected | TCB_RepeatReq
    },
    {
      "ALLCOLO",
      "TACC",
      0,
      0,
      TCB_Redirected | TCB_RepeatReq
    },
    {
      "NONOPTOUTCOLO",
      "TACC",
      0,
      0,
      TCB_Redirected | TCB_RepeatReq,
    },
    {
      "NOADSCOLO",
      0,
      0,
      0,
      TCB_Redirected | TCB_RepeatReq
    },
    {
      "DELETEDCOLO",
      0,
      0,
      0,
      TCB_Redirected | TCB_RepeatReq
    }      
  };

  // Test 6.11. Banned channels matching
  const OptoutAdvertising::TestCase NoAdv [] =
  {
    {
      "OPTINCOLO",
      0,
      0,
      0,
      TCB_EmptyPassback | TCB_CheckNoAdv
    },
    {
      "ALLCOLO",
      0,
      0,
      0,
      TCB_EmptyPassback | TCB_CheckNoAdv
    },
    {
      "NONOPTOUTCOLO",
      0,
      0,
      0,
      TCB_EmptyPassback | TCB_CheckNoAdv
    },
    {
      "NOADSCOLO",
      0,
      0,
      0,
      TCB_EmptyPassback
    },
    {
      "DELETEDCOLO",
      0,
      0,
      0,
      TCB_EmptyPassback
    }      
  };

  // Test 7. Opt-in status targeting

  //   7.1. optin_status_targeting = YNN (only OptIn)
  const OptoutAdvertising::TestCase OST_YNN [] =
  {
    {
      "OPTINCOLO",
      "OSTCC/YNN",
      0,
      0,
      TCB_Redirected
    },
    {
      "ALLCOLO",
      "OSTCC/YNN",
      "OSTCC/RON",
      "OSTCC/RON",
      0
    },
    {
      "NONOPTOUTCOLO",
      "OSTCC/YNN",
      "OSTCC/RON",
      0,
      TCB_Redirected
    },
    {
      "NOADSCOLO",
      0,
      0,
      0,
      TCB_Redirected
    },
    {
      "DELETEDCOLO",
      0,
      0,
      0,
      TCB_Redirected
    }      
  };

  //   7.2. optin_status_targeting = NYN (only OptOut)
  const OptoutAdvertising::TestCase OST_NYN [] =
  {
    {
      "OPTINCOLO",
      "OSTCC/RON",
      0,
      "OSTCC/NYN",
      TCB_Redirected
    },
    {
      "ALLCOLO",
      "OSTCC/RON",
      "OSTCC/RON",
      "OSTCC/NYN",
      0
    },
    {
      "NONOPTOUTCOLO",
      "OSTCC/RON",
      "OSTCC/RON",
      "OSTCC/NYN",
      0
    },
    {
      "NOADSCOLO",
      0,
      0,
      0,
      TCB_Redirected
    },
    {
      "DELETEDCOLO",
      0,
      0,
      0,
      TCB_Redirected
    }      
  };

  //   7.3. optin_status_targeting = NNY (Unknown only)
  const OptoutAdvertising::TestCase OST_NNY [] =
  {
    {
      "OPTINCOLO",
      0,
      "OSTCC/NNY",
      0,
      TCB_Redirected
    },
    {
      "ALLCOLO",
      0,
      "OSTCC/NNY",
      0,
      TCB_Redirected
    },
    {
      "NONOPTOUTCOLO",
      0,
      "OSTCC/NNY",
      0,
      TCB_Redirected
    },
    {
      "NOADSCOLO",
      0,
      0,
      0,
      TCB_Redirected
    },
    {
      "DELETEDCOLO",
      0,
      0,
      0,
      TCB_Redirected
    }      
  };

  //   7.4. optin_status_targeting = NYY (OptOut + Unknown)
  const OptoutAdvertising::TestCase OST_NYY [] =
  {
    {
      "OPTINCOLO",
      0,
      "OSTCC/NYY",
      "OSTCC/NYY",
      TCB_Redirected
    },
    {
      "ALLCOLO",
      0,
      "OSTCC/NYY",
      "OSTCC/NYY",
      TCB_Redirected
    },
    {
      "NONOPTOUTCOLO",
      0,
      "OSTCC/NYY",
      "OSTCC/NYY",
      TCB_Redirected
    },
    {
      "NOADSCOLO",
      0,
      0,
      0,
      TCB_Redirected
    },
    {
      "DELETEDCOLO",
      0,
      0,
      0,
      TCB_Redirected
    }      
  };

  //   7.5. optin_status_targeting = YNY (OptIn + Unknown)
  const OptoutAdvertising::TestCase OST_YNY [] =
  {
    {
      "OPTINCOLO",
      "OSTCC/YNY",
      "OSTCC/YNY",
      0,
      TCB_Redirected
    },
    {
      "ALLCOLO",
      "OSTCC/YNY",
      "OSTCC/YNY",
      0,
      TCB_Redirected
    },
    {
      "NONOPTOUTCOLO",
      "OSTCC/YNY",
      "OSTCC/YNY",
      0,
      TCB_Redirected
    },
    {
      "NOADSCOLO",
      0,
      0,
      0,
      TCB_Redirected
    },
    {
      "DELETEDCOLO",
      0,
      0,
      0,
      TCB_Redirected
    }      
  };
  
  //   7.6. optin_status_targeting = YYN (OptIn + OptOut)
  const OptoutAdvertising::TestCase OST_YYN [] =
  {
    {
      "OPTINCOLO",
      "OSTCC/YYN",
      0,
      "OSTCC/YYN",
      TCB_Redirected
    },
    {
      "ALLCOLO",
      "OSTCC/YYN",
      0,
      "OSTCC/YYN",
      TCB_Redirected
    },
    {
      "NONOPTOUTCOLO",
      "OSTCC/YYN",
      0,
      "OSTCC/YYN",
      TCB_Redirected
    },
    {
      "NOADSCOLO",
      0,
      0,
      0,
      TCB_Redirected
    },
    {
      "DELETEDCOLO",
      0,
      0,
      0,
      TCB_Redirected
    }      
  };

  //   7.7. optin_status_targeting = YYY (all)
  const OptoutAdvertising::TestCase OST_YYY [] =
  {
    {
      "OPTINCOLO",
      "OSTCC/YYY",
      "OSTCC/YYY",
      "OSTCC/YYY",
      0
    },
    {
      "ALLCOLO",
      "OSTCC/YYY",
      "OSTCC/YYY",
      "OSTCC/YYY",
      0
    },
    {
      "NONOPTOUTCOLO",
      "OSTCC/YYY",
      "OSTCC/YYY",
      "OSTCC/YYY",
      0
    },
    {
      "NOADSCOLO",
      0,
      0,
      0,
      TCB_Redirected
    },
    {
      "DELETEDCOLO",
      0,
      0,
      0,
      TCB_Redirected
    }      
  };
  
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::OptOutRequest OptOutRequest;

  typedef const char* OptoutAdvertising::TestCase::* CCMember;


  enum ClientEnum
  {
    CE_Optin  = 0,  // Optin client
    CE_Undef  = 1,  // Undefined client
    CE_Optout = 2   // Optout client
  };
  
  struct ClientDictionaryItem
  {
    const char* description;
    CCMember ccid;
  };

  ClientDictionaryItem clients[] =
  {
    {
      "Optin client",
      &OptoutAdvertising::TestCase::optin_ccid
    },
    {
      "Undefined client",
      &OptoutAdvertising::TestCase::undef_ccid
    },
    {
      "Optout client",
      &OptoutAdvertising::TestCase::optout_ccid
    }
  };
  
  const char PASSBACK[] = "http://www.passback.adserver.com/";

  /**
   * @class RedirectCheck
   * @brief Check redirect to passback url.
   *
   * This checker used for test client passback
   * redirection.
   */
  class RedirectCheck :  public AutoTest::Checker
  {
  public:
       /**
     * @brief Constructor.
     *
     * @param client (user).
     * @param expected passback url.
     */
    RedirectCheck(AdClient& client,
                  const std::string& passback_url) :
      client_(client)
    {
      String::StringManip::mime_url_encode(
        String::SubString(passback_url),
        passback_url_);      
    }

    /**
     * @brief Destructor.
     */
    virtual ~RedirectCheck() noexcept
    {}


    /**
     * @brief Check.
     * @param throw on error flag.
     */
    bool check(bool throw_error = true) /*throw(eh::Exception)*/
    {

      std::string url;
      String::StringManip::mark(passback_url_.c_str(), url,
        String::AsciiStringManip::REGEX_META, '\\');
      return
        AutoTest::RedirectChecker(
          client_,
          String::RegEx(
            "^http://.*/passback\\?requestid=.*&random=0&passback=" +
              url + ".*$")).check(throw_error);
    }

  private:
    AdClient client_;   // client(user)
    std::string passback_url_;  // expected passback URL
  };
}


bool
OptoutAdvertising::run_test()
{

  add_descr_phrase("Test 6.1. RON ads matching");
  run_test_case(
    RonAds,
    NSLookupRequest().
      tid(fetch_string("DisplayRONCPMTAG")));
  
  add_descr_phrase("Test 6.2. Targeted ads matching on urls");
  run_test_case(
    TargetedAdsOnUrls,
    NSLookupRequest().
      tid(fetch_string("DisplayCPMTAG")).
      referer(fetch_string("URL")));

  add_descr_phrase("Test 6.5. Campaigns with frequency caps");
  run_test_case(
    FreqCapsAds,
    NSLookupRequest().
      tid(fetch_string("DisplayFCTAG")).
      referer(fetch_string("URL")));

  add_descr_phrase("Test 6.6.1 Campaigns with action tracking (RON)");
  run_test_case(
    RONCPAAds,
    NSLookupRequest().
      tid(fetch_string("DisplayRONCPATAG")));

  add_descr_phrase("Test 6.6.2 Campaigns with action tracking");
  run_test_case(
    CPAAds,
    NSLookupRequest().
      tid(fetch_string("DisplayCPATAG")).
      referer(fetch_string("URL")));
  
  add_descr_phrase("Test 6.7. Campaigns with "
                   "impression/click tracking");
  run_test_case(
    CPCAds,
    NSLookupRequest().
      tid(fetch_string("DisplayCPCTAG")).
      referer(fetch_string("URL")).
      format("unit-test-imp"));

  add_descr_phrase("Test 6.8. Text RON campaigns");
  run_test_case(
    TextRonAds,
    NSLookupRequest().
      tid(fetch_string("TEXTTAG")));

  add_descr_phrase("Test 6.9. Text(C) targeted campaigns");
  run_test_case(
    TextAds,
    NSLookupRequest().
      tid(fetch_string("TEXTTAG")).
      referer(fetch_string("URL")));

  add_descr_phrase("Test 6.10. Text(K) targeted campaigns");
  run_test_case(
    TAAds,
    NSLookupRequest().
      tid(fetch_string("TATAG")).
      referer_kw(fetch_string("KEYWORD")));
  
  add_descr_phrase("Test 6.11. Banned channels matching");
  run_test_case(
    NoAdv,
    NSLookupRequest().
      tid(fetch_string("DisplayRONCPMTAG")).
      referer(fetch_string("no_adv_urls")));

  add_descr_phrase("Test 7.1 Opt-in status targeting (YNN)");
  run_test_case(
    OST_YNN,
    NSLookupRequest().
      tid(fetch_string("OSTTAG1")).
      referer_kw(fetch_string("OSTKWD/YNN")));

  add_descr_phrase("Test 7.2 Opt-in status targeting (NYN)");
  run_test_case(
    OST_NYN,
    NSLookupRequest().
      tid(fetch_string("OSTTAG1")).
      referer_kw(fetch_string("OSTKWD/NYN")));

  add_descr_phrase("Test 7.3 Opt-in status targeting (NNY)");
  run_test_case(
    OST_NNY,
    NSLookupRequest().
      tid(fetch_string("OSTTAG2")).
      referer_kw(fetch_string("OSTKWD/NNY")));

  add_descr_phrase("Test 7.4 Opt-in status targeting (NYY)");
  run_test_case(
    OST_NYY,
    NSLookupRequest().
      tid(fetch_string("OSTTAG2")).
      referer_kw(fetch_string("OSTKWD/NYY")));

  add_descr_phrase("Test 7.5 Opt-in status targeting (YNY)");
  run_test_case(
    OST_YNY,
    NSLookupRequest().
      tid(fetch_string("OSTTAG2")).
      referer_kw(fetch_string("OSTKWD/YNY")));

  add_descr_phrase("Test 7.6 Opt-in status targeting (YYN)");
  run_test_case(
    OST_YYN,
    NSLookupRequest().
      tid(fetch_string("OSTTAG2")).
      referer_kw(fetch_string("OSTKWD/YYN")));

  add_descr_phrase("Test 7.7 Opt-in status targeting (YYY)");
  run_test_case(
    OST_YYY,
    NSLookupRequest().
    tid(fetch_string("OSTTAG2")).
      referer_kw(fetch_string("OSTKWD/YYY")));
  
  add_descr_phrase("Impression & clicks in optout mode");
  optout_click_and_impression();
  return true;
}

template <size_t Count>
void OptoutAdvertising::run_test_case(
  const TestCase(&testcases)[Count],
  const NSLookupRequest& base_request)
{
 
  for (unsigned int i=0; i < Count; ++i)
  {
   
    for (unsigned int j=0; j < countof(clients); ++j)
    {

      AdClient client(
        j == CE_Optin? AdClient::create_user(this):
          AdClient::create_nonoptin_user(this));

      if (j == CE_Optout)
      {
        client.process_request(OptOutRequest().op("out"));
      }
     
      // Set request parameters & send request
      
      NSLookupRequest request(base_request);
      request.colo = fetch_string(testcases[i].colo);

      if ((testcases[i].flags & TCB_EmptyPassback) == 0)
      {
        request.passback = PASSBACK;
        request.pt = "redir";
      }


      if (testcases[i].flags & TCB_RepeatReq)
      {
        client.process_request(request);
      }                

      // Check results
      
      SpecialEffectsChecker checker(
        client,
        request,
        testcases[i].*(clients[j].ccid)?
          fetch_string(testcases[i].*(clients[j].ccid)): "0",
        SpecialEffectsChecker::SE_TRACK |
        (testcases[i].flags & TCB_CheckNoAdv?
          SpecialEffectsChecker::SE_NO_ADV: SpecialEffectsChecker::SE_ADV));

      FAIL_CONTEXT(
        checker.check(),
        std::string(clients[j].description) +
        ". " +  testcases[i].colo +  " Ccid check#" + strof(i));


      if (testcases[i].*(clients[j].ccid))
      {

        FAIL_CONTEXT(
          AutoTest::predicate_checker(
            checker.client().req_status() == 200 ||
            checker.client().req_status() == 204),
          std::string(clients[j].description) +
            ". " +  testcases[i].colo +
            ". Invalid HTTP status#" + strof(i));

      }
      else
      {
        
        if (testcases[i].flags & TCB_Redirected)
        {
          FAIL_CONTEXT(
            RedirectCheck(
              checker.client(),
              PASSBACK).check(),
            std::string(clients[j].description) +
              ". " +  testcases[i].colo +
              ". Passback check#"  + strof(i));
        }
      }
    }
  }
}

void OptoutAdvertising::optout_click_and_impression()
{
  AdClient client(AdClient::create_user(this));

  client.process_request(
    NSLookupRequest().
    referer(fetch_string("URL")).
    format("unit-test-imp").
    tid(fetch_string("DisplayCPCTAG")));

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      !client.debug_info.click_url.empty()),
    "click_url empty check");

  std::string click_url(client.debug_info.click_url);

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      !client.debug_info.track_pixel_url.empty()), 
    "track_pixel_url empty check");
  
  std::string track_pixel_url(client.debug_info.track_pixel_url);

  client.process_request(OptOutRequest().op("out"));

  client.process_request(click_url.c_str());

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      !client.has_host_cookies()), 
    "Host cookies shouldn't return in optout mode");

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      client.has_domain_cookie("OPTED_OUT", "YES")),
    "OPTED_OUT=YES");

  client.process_request(track_pixel_url.c_str());

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      !client.has_host_cookies()), 
    "Host cookies shouldn't return in optout mode");

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      client.has_domain_cookie("OPTED_OUT", "YES")),
    "OPTED_OUT=YES");
}

