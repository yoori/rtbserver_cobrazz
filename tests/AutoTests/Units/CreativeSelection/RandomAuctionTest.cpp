
#include "RandomAuctionTest.hpp"
#include <math.h>

REFLECT_UNIT(RandomAuctionTest) (
  "CreativeSelection",
  AUTO_TEST_QUIET);

namespace
{
  typedef AutoTest::SelectedCreativesCCID SelectedCreativesCCID;
  typedef AutoTest::UserBindRequest UserBindRequest;
  typedef AutoTest::OpenRTBRequest OpenRTBRequest;
  typedef AutoTest::TanxRequest TanxRequest;
  typedef AutoTest::OpenRTBResponseChecker OpenRTBResponseChecker;
  typedef AutoTest::TanxResponseChecker TanxResponseChecker;
  typedef AutoTest::Money Money;
  typedef AutoTest::CountChecker CountChecker;

  enum RTBCaseFlag
  {
    RTCF_SEARCH = 1 // Use search url
  };

  struct RTBTraits
  {
    typedef AutoTest::OpenRTBRequest Request;
    typedef AutoTest::OpenRTBResponseChecker Checker;
  };

  struct TanxTraits
  {
    typedef AutoTest::TanxRequest Request;
    typedef AutoTest::TanxResponseChecker Checker;
  };

  // Saint Lucia IP
  const char* LC_IP = "208.94.176.0";

  const size_t REQUESTS = 1000;

  struct RTBExpected
  {
    std::string ccid;
    Money price;
  };

  std::string search_url(
    const std::string& kwds)
  {
    return "http://search.live.de/results.aspx?q=" + kwds;
  }

  void
  rtb_request(
    BaseUnit* test,
    OpenRTBRequest& request,
    const RandomAuctionTest::RTBCase& rtb_case)
  {
    request.
      aid(test->fetch_int(rtb_case.aid)).
      debug_ccg(test->fetch_int(rtb_case.ccg)).
      debug_size(test->fetch_string(rtb_case.size)).
      min_cpm_price(rtb_case.bidfloor);
    
    if (rtb_case.random)
    {
      request.random = rtb_case.random;
    }

    request.referer =
      rtb_case.flags & RTCF_SEARCH?
        search_url(
          test->map_objects(rtb_case.url, " ")):
            test->fetch_string(rtb_case.url);

  }
  
  void
  rtb_request(
    BaseUnit* test,
    TanxRequest& request,
    const RandomAuctionTest::TanxCase& rtb_case)
  {
    request.
      aid(test->fetch_int(rtb_case.aid)).
      debug_ccg(test->fetch_int(rtb_case.ccg)).
      debug_size(test->fetch_string(rtb_case.size)).
      min_cpm_price(rtb_case.min_cpm_price);

    request.url =
      rtb_case.flags & RTCF_SEARCH?
        search_url(
          test->map_objects(rtb_case.url, " ")):
            test->fetch_string(rtb_case.url);
  }

  void
  rtb_expected(
    BaseUnit* test,
    OpenRTBResponseChecker::Expected& expected,
    const RTBExpected& e)
  {
    expected.
      price(e.price).
      adid(test->fetch_int(e.ccid.c_str()));
  }

  void
  rtb_expected(
    BaseUnit* test,
    TanxResponseChecker::Expected& expected,
    const RTBExpected& e)
  {
    expected.
      max_cpm_price(e.price).
      creative_id(test->fetch_string(e.ccid.c_str()));
  }

  class ProbabilityChecker :
     public AutoTest::Checker,
     public AutoTest::OrChecker::ICounter
  {

    struct ProbabilityCounter
    {
      template <typename Probability>
      ProbabilityCounter(
        const Probability& p) :
        count(0),
        probability(p.probability)
      { }

      size_t count;
      double probability;
    };
    
  public:
    template <typename Probability, size_t COUNT>
    ProbabilityChecker(
      const Probability(& test_case)[COUNT],
      size_t sample_size) :
      counts_(test_case, test_case + COUNT),
      sample_size_(sample_size)
    { }

    virtual ~ProbabilityChecker() noexcept
    { }

    virtual
    void incr(size_t index)
    {
      if (index >= counts_.size())
      {
        return;
      }
      counts_[index].count++;
    }

    virtual
    bool
    check(bool throw_error = true)
      /*throw(AutoTest::CheckFailed, eh::Exception)*/
    {

      bool result = true;
      Stream::Error error;
      error << "failed conditions:";

      size_t i = 0;
      
      for (auto it = counts_.begin(); it != counts_.end(); it++, ++i)
      {
        // confidence intervals for probability:
        //   c.i. = p +/- Z * sqrt (p * (1-p) / N),
        //     N - sample size;
        //     p - probability for each case in the sample;
        //     Z - standard deviation for level 99,9% = 3.291
        //         (Student quantile table -
        //          http://en.wikipedia.org/wiki/Student%27s_t-distribution)
        double p = it->probability;
        double sd_part = 3.291 * sqrt((1 - p) * p / sample_size_);
        unsigned long low_limit = (p - sd_part) * sample_size_;
        unsigned long high_limit = (p + sd_part) * sample_size_;

                
        AutoTest::Logger::thlog().stream(Logging::Logger::TRACE) <<
          "Condition#" << i+1 << ": "  << low_limit << " < " <<
          it->count << " < " << high_limit;
        
        if ( it->count < low_limit || it->count > high_limit )
        {
          result = false;

          error << std::endl << "  The condition#"  << i+1 << " is not satisfied: " <<
            it->count << " not in [ " << low_limit << ", " <<  high_limit << " ]";
        }
      }

      if (throw_error && !result)
      {
        throw AutoTest::CheckFailed(error);
      }
       
      return result;
    }

  private:
    typedef std::vector<ProbabilityCounter> CounterList;
    CounterList counts_;
    size_t sample_size_;
  };
}

template <size_t COUNT>
RandomAuctionTest::OrChecker
RandomAuctionTest::prepare_checker(
  AdClient& client,
  OrChecker::ICounter* counter,
  const ExpectedSequence(&test_case)[COUNT])
{

  SelectedCreativesCCID got_ccs(client);
  
  OrChecker checker(counter);
  for (size_t i = 0; i < COUNT; ++i)
  {
    std::list<std::string> expected_ccs;
    fetch_objects(
      std::inserter(expected_ccs, expected_ccs.begin()),
      test_case[i].ccs);

    checker.or_if(
      AutoTest::sequence_checker(
        expected_ccs,
        got_ccs));
  }
  return checker;
}

template <size_t COUNT>
RandomAuctionTest::OrChecker
RandomAuctionTest::prepare_checker(
  AdClient& client,
  OrChecker::ICounter* counter,
  const ExpectedEntry(&test_case)[COUNT])
{
  OrChecker checker(counter);
  for (size_t i = 0; i < COUNT; ++i)
  {
    checker.or_if(
      AutoTest::entry_checker(
        fetch_string(test_case[i].cc),
        SelectedCreativesCCID(client)));
  }
  return checker;
}


template <typename Expected, size_t COUNT>
void
RandomAuctionTest::test_case(
  const Expected(&expected)[COUNT],
  size_t sample_size,
  const NSLookupRequest& request)
{
  ProbabilityChecker counter(expected, sample_size);

  for (size_t i = 0; i < sample_size; ++i)
  {
     AdClient client(AdClient::create_user(this));

     client.process_request(request);

     FAIL_CONTEXT(
       prepare_checker(
         client,
         &counter,
         expected).check(),
       "Check CC#" + strof(i+1));
  }

  FAIL_CONTEXT(
    counter.check(),
    "Probability checker");
}


template <typename Traits, typename CaseType, size_t COUNT>
void
RandomAuctionTest::rtb_test_case(
  AdClient& client,
  typename Traits::Request& base_request,
  const CaseType(&rtb_cases)[COUNT])
{
  for (size_t i = 0; i < COUNT; ++i)
  {
    std::list<RTBExpected> expected;
    if (rtb_cases[i].expected)
    {
      String::StringManip::CharSplitter
        tokenizer(
          String::SubString(rtb_cases[i].expected),
          String::AsciiStringManip::CharCategory(","));
      
      String::SubString token;
      while(tokenizer.get_token(token))
      {
        String::RegEx re(
          String::SubString("([^:]+):([^:]+)"));

        String::RegEx::Result result;
        re.search(result, token);
          
        FAIL_CONTEXT(
          AutoTest::equal_checker(
            3,
            result.size()).check(),
          "Invalid expected format");
        
        RTBExpected e;
        e.ccid = result[1].str();
        e.price = result[2].str();
        
        expected.push_back(e);
      }
    }

    CountChecker counter(expected.size(), rtb_cases[i].count);

    for (size_t j = 0; j < rtb_cases[i].count; ++j)
    {
      typename Traits::Request request(base_request);

      rtb_request(
        this,
        request,
        rtb_cases[i]);
      
      client.process_post(request);

      if (expected.empty())
      {
        FAIL_CONTEXT(
          AutoTest::or_checker(
            AutoTest::and_checker(
              AutoTest::equal_checker(
                200,
                client.req_status()),
              typename Traits::Checker(
                client,
                typename Traits::Checker::Expected())),
            AutoTest::equal_checker(
              204,
              client.req_status())).check(),
            "No content check#" + strof(i+1));

        
      }
      else
      {
        OrChecker checker(
          static_cast<OrChecker::ICounter*>(&counter));
        for (auto it = expected.begin(); it != expected.end(); ++it)
        {
          typename Traits::Checker::Expected expected;

          rtb_expected(this, expected, *it);
          
          checker.or_if(
            typename Traits::Checker(
              client,
              expected));
        }

        FAIL_CONTEXT(
          checker.check(),
          "Check CC#" + strof(i+1));
      }
    }

    FAIL_CONTEXT(
      counter.check(),
      "Events count checker#" + strof(i+1));
  }
}

void
RandomAuctionTest::random_text_1()
{
  const ExpectedSequence EXPECTED[] =
  {
    { "RANDOMTEXT1/DISP/CC/1", 0.25 },
    { "RANDOMTEXT1/DISP/CC/2", 0.25 },
    { "RANDOMTEXT1/DISP/CC/3", 0.25 },
    { "RANDOMTEXT1/CH/CC/1", 0.25 }
  };

  test_case(
    EXPECTED, REQUESTS,
    NSLookupRequest().
      loc_name("lc").
      referer_kw(
        map_objects(
          "RANDOMTEXT1/DISP/KWD/1,"
          "RANDOMTEXT1/DISP/KWD/2,"
          "RANDOMTEXT1/DISP/KWD/3,"
          "RANDOMTEXT1/CH/KWD/1")).
      tid(fetch_int("RANDOMTEXT1/TAG")));
}

void
RandomAuctionTest::random_text_2()
{
  const ExpectedSequence EXPECTED[] =
  {
    { "RANDOMTEXT2/CH/CC/1,RANDOMTEXT2/CH/CC/2", 0.25 },
    { "RANDOMTEXT2/CH/CC/1,RANDOMTEXT2/CH/CC/4", 0.25 },
    { "RANDOMTEXT2/CH/CC/2,RANDOMTEXT2/CH/CC/1", 0.25 },
    { "RANDOMTEXT2/CH/CC/4,RANDOMTEXT2/CH/CC/1", 0.25 }
  };

  test_case(
    EXPECTED, REQUESTS,
    NSLookupRequest().
      loc_name("lc").
      referer_kw(
        map_objects(
          "RANDOMTEXT2/CH/KWD/1,"
          "RANDOMTEXT2/CH/KWD/2,"
          "RANDOMTEXT2/CH/KWD/3,"
          "RANDOMTEXT2/CH/KWD/4")).
      tid(fetch_int("RANDOMTEXT2/TAG")));
}

void
RandomAuctionTest::creative_size_1()
{
  const ExpectedEntry EXPECTED[] =
  {
    { "CREATIVESIZE/CH/CC/1", 0.67 },
    { "CREATIVESIZE/CH/CC/2", 0.67 },
    { "CREATIVESIZE/CH/CC/3", 0.67 },
    { "CREATIVESIZE/CH/CC/4", 0.13 },
    { "CREATIVESIZE/CH/CC/5", 0.13 },
    { "CREATIVESIZE/CH/CC/6", 0.13 },
    { "CREATIVESIZE/CH/CC/7", 0.13 },
    { "CREATIVESIZE/CH/CC/8", 0.13 }
  };

  test_case(
    EXPECTED, REQUESTS,
    NSLookupRequest().
      loc_name("lc").
      referer_kw(
        map_objects(
          "CREATIVESIZE/CH/KWD/1,"
          "CREATIVESIZE/CH/KWD/2,"
          "CREATIVESIZE/CH/KWD/3,"
          "CREATIVESIZE/CH/KWD/4,"
          "CREATIVESIZE/CH/KWD/5,"
          "CREATIVESIZE/CH/KWD/6,"
          "CREATIVESIZE/CH/KWD/7,"
          "CREATIVESIZE/CH/KWD/8")).
      tid(fetch_int("CREATIVESIZE/TAG")));
}

void
RandomAuctionTest::proportional_1()
{
  const ExpectedSequence EXPECTED[] =
  {
    { "PROPORTIONAL1/DISP/CC/1", 0.17 },
    { "PROPORTIONAL1/DISP/CC/2", 0.17 },
    { "PROPORTIONAL1/DISP/CC/3", 0.17 },
    { "PROPORTIONAL1/CH/CC/1", 0.5 }
  };
  
  test_case(
    EXPECTED, REQUESTS,
    NSLookupRequest().
      loc_name("lc").
      referer_kw(
        map_objects(
          "PROPORTIONAL1/DISP/KWD/1,"
          "PROPORTIONAL1/DISP/KWD/2,"
          "PROPORTIONAL1/DISP/KWD/3,"
          "PROPORTIONAL1/CH/KWD/1")).
      tid(fetch_int("PROPORTIONAL1/TAG")));
}

void
RandomAuctionTest::proportional_2()
{
  const ExpectedSequence EXPECTED[] =
  {
    { "PROPORTIONAL2/CH/CC/1,PROPORTIONAL2/CH/CC/2", 0.25 },
    { "PROPORTIONAL2/CH/CC/1,PROPORTIONAL2/CH/CC/4", 0.25 },
    { "PROPORTIONAL2/CH/CC/2,PROPORTIONAL2/CH/CC/1", 0.25 },
    { "PROPORTIONAL2/CH/CC/4,PROPORTIONAL2/CH/CC/1", 0.25 }
  };
  
  test_case(
    EXPECTED, REQUESTS,
    NSLookupRequest().
      loc_name("lc").
      referer_kw(
        map_objects(
          "PROPORTIONAL2/CH/KWD/1,"
          "PROPORTIONAL2/CH/KWD/2,"
          "PROPORTIONAL2/CH/KWD/3,"
          "PROPORTIONAL2/CH/KWD/4")).
      tid(fetch_int("PROPORTIONAL2/TAG")));
}

void
RandomAuctionTest::open_rtb_random(
  AdClient& client)
{
  const RTBCase CASES[] =
  {
    {
      "RTB/ACCOUNT/1", "RTB/CCG/3", "SIZE/1",
      "RTB/URL/3", 0, 0,
      "RTB/CREATIVEID/3/1:530", 1, 0
    },
    {
      "RTB/ACCOUNT/1", "RTB/CCG/4", "SIZE/1",
      "RTB/URL/4", 1, 0,
      "RTB/CREATIVEID/4/1:530", 1, 0
    },
    {
      "RTB/ACCOUNT/1", "RTB/CCG/3", "SIZE/1",
      "RTB/URL/3", 530, 0,
      "RTB/CREATIVEID/3/1:530", 1, 0
    },
    {
      "RTB/ACCOUNT/1", "RTB/CCG/3", "SIZE/1",
      "RTB/URL/3", 530.1, 0,
      0, 1, 0
    },
    {
      "RTB/ACCOUNT/1", "RTB/CCG/6", "SIZE/4",
      "RTB/KWD/7", 10, 0,
      "RTB/CREATIVEID/6/1:530,RTB/CREATIVEID/7/1:530", 100,
      RTCF_SEARCH
    },
    {
      "RTB/ACCOUNT/2", "RTB/CCG/3", "SIZE/1",
      "RTB/KWD/3", 529, 0,
      "RTB/CREATIVEID/3/1:529.97", 1,
      RTCF_SEARCH
    },
    {
      "RTB/ACCOUNT/2", "RTB/CCG/3", "SIZE/1",
      "RTB/KWD/3", 530, 0,
      0, 1, RTCF_SEARCH
    }
  };
  
  rtb_test_case<RTBTraits>(
    client,
    RTBTraits::Request().
      ip(LC_IP),
    CASES);
}

void
RandomAuctionTest::open_rtb_secondary(
  AdClient& client)  
{
  // Secondary auctions
  const RTBCase CASES[] =
  {
    {
      "RTB/ACCOUNT/1", "RTB/CCG/3", "SIZE/0",
      "RTB/KWD/2 RTB/KWD/3", 530, 0,
      "RTB/CREATIVEID/3/3:530", 10, RTCF_SEARCH
    },
    {
      "RTB/ACCOUNT/1", "RTB/CCG/2", "SIZE/0",
      "RTB/KWD/2 RTB/KWD/3", 531, 0,
      "RTB/CREATIVEID/2/2:2100", 10, RTCF_SEARCH
    },
    {
      "RTB/ACCOUNT/1", "RTB/CCG/2", "SIZE/2",
      "RTB/KWD/2 RTB/KWD/3", 531, 10000,
      "RTB/CREATIVEID/2/1:2100", 100, RTCF_SEARCH
    },
    {
      "RTB/ACCOUNT/1", "RTB/CCG/2", "SIZE/3",
      "RTB/KWD/2 RTB/KWD/3", 531, 5100000,
      "RTB/CREATIVEID/1/3:1999.99,RTB/CREATIVEID/2/3:2100", 100, RTCF_SEARCH
    }
  };
  
  rtb_test_case<RTBTraits>(
    client,
    RTBTraits::Request().
      ip(LC_IP),
    CASES);
}

void
RandomAuctionTest::tanx()  
{

  AdClient client(AdClient::create_nonoptin_user(this));

  // Precondition

  client.process_request(
    NSLookupRequest().
      referer(fetch_string("TANX/URL/3")).
      tid(fetch_int("TANX/Tag/2/3")).
      colo(fetch_int("OpenRTBColo")).
      loc_name("lc").
      format("html"));

  NOSTOP_FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string("TANX/CC/3/2"),
      client.debug_info.ccid).check(),
    "Precondition check");
    
  const TanxCase CASES[] =
  {
    {
      "TANX/ACCOUNT/1", "TANX/CCG/2", "SIZE/1",
      "TANX/URL/2", 0, "TANX/CREATIVE/2/1:10600", 1, 0
    },
    {
      "TANX/ACCOUNT/1", "TANX/CCG/2", "SIZE/1",
      "TANX/URL/2", 10600,
      "TANX/CREATIVE/2/1:10600", 1, 0
    },
    {
      "TANX/ACCOUNT/1", "TANX/CCG/2", "SIZE/1",
      "TANX/URL/2", 10601, 0, 1, 0
    },
    {
      "TANX/ACCOUNT/1", "TANX/CCG/5", "SIZE/4",
      "TANX/KWD/6", 10000,
      "TANX/CREATIVE/5/1:10600,TANX/CREATIVE/6/1:10600", 100,
      RTCF_SEARCH
    },
    {
      "TANX/ACCOUNT/2", "TANX/CCG/2", "SIZE/1",
      "TANX/KWD/2", 10598,
      "TANX/CREATIVE/2/1:10599", 1,
      RTCF_SEARCH
    },
    {
      "TANX/ACCOUNT/2", "TANX/CCG/2", "SIZE/1",
      "TANX/KWD/2", 10600,
      0, 1, RTCF_SEARCH
    },
    {
      "TANX/ACCOUNT/1", "TANX/CCG/2", "SIZE/0",
      "TANX/KWD/2", 10599, "TANX/CREATIVE/2/2:10600", 1,
      RTCF_SEARCH
    },
    {
      "TANX/ACCOUNT/1", "TANX/CCG/1", "SIZE/0",
      "TANX/KWD/2", 10601,
      "TANX/CREATIVE/1/2:19999", 1,
      RTCF_SEARCH
    },
    {
      "TANX/ACCOUNT/1", "TANX/CCG/3", "SIZE/5",
      "TANX/URL/3", 0,
      0, 1, 0
    },
    {
      "TANX/ACCOUNT/1", "TANX/CCG/4", "SIZE/5",
      "TANX/URL/4", 0,
      "TANX/CREATIVE/4/2:1", 1, 0
    }
  };

  rtb_test_case<TanxTraits>(
    client,
    TanxTraits::Request().
      ip(LC_IP),
    CASES);
}

void
RandomAuctionTest::set_up()
{
  AutoTest::ORM::clear_stats(
    pq_conn_,
    "country_code",
    fetch_string("COUNTRYCODE"));

  AutoTest::ORM::calc_ctr(pq_conn_);
}

bool
RandomAuctionTest::run()
{
  AUTOTEST_CASE(
    random_text_1(),
    "Random auction. Multiple sizes (display)");

  AUTOTEST_CASE(
    random_text_2(),
    "Random auction. Multiple sizes (text)");

  AUTOTEST_CASE(
    creative_size_1(),
    "Random auction. Creative size probability (text)");

  AUTOTEST_CASE(
    proportional_1(),
    "Proporion probability. Creative size probability (display)");

  AUTOTEST_CASE(
    proportional_2(),
    "Proporion probability. Creative size probability (text)");

  {
    AdClient client(AdClient::create_nonoptin_user(this));
    
    AUTOTEST_CASE(
      open_rtb_random(client),
      "Open RTB. Random auction");

    AUTOTEST_CASE(
      open_rtb_secondary(client),
      "Open RTB. Secondary auctions");
  }
  
  AUTOTEST_CASE(
    tanx(),
    "TanX cases");

  return true;
}

void
RandomAuctionTest::tear_down()
{
  AutoTest::ORM::clear_stats(
    pq_conn_,
    "country_code",
    fetch_string("COUNTRYCODE"));

  AutoTest::ORM::calc_ctr(pq_conn_);
}
