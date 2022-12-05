#include "WalledGardenTest.hpp"

REFLECT_UNIT(WalledGardenTest) (
  "CreativeSelection",
  AUTO_TEST_FAST | AUTO_TEST_SLOW
  );

namespace
{

  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::AdClient AdClient;
  
  // Special flags

  // All creative must shown in the tag
  const short SHOWALL   = 0xFF; 

  // Walled garden creative mustn't shown
  const short NOTSHOWWG =
    WalledGardenTest::CSE_ALL |
    WalledGardenTest::CSE_OIX |
    WalledGardenTest::CSE_NON_WALLED_GARDEN;
   
  
  const WalledGardenTest::TestCase TEST_CASES[] = 
  {
    {
      "WG",
      WalledGardenTest::CSE_WALLED_GARDEN,
      WalledGardenTest::CSE_WALLED_GARDEN |
      WalledGardenTest::CSE_ALL,
      false,
      "WalledGarden marketplace",
    },
    {
      "OIX",
      WalledGardenTest::CSE_OIX,
      NOTSHOWWG,
      false,
      "OIX marketplace"
    },
    {
      "ALL",
      WalledGardenTest::CSE_ALL,
      SHOWALL,
      false,
      "WG + OIX marketplace (ALL)"
    },
    {
      "NON-WG",
      WalledGardenTest::CSE_NON_WALLED_GARDEN,
      NOTSHOWWG,
      false,
      "Non-WalledGarden marketplace"
    },
    {
      "WG-2",
      WalledGardenTest::CSE_WALLED_GARDEN,
      0,
      true,
      "WalledGarden marketplace#2"
    }
  };

  const WalledGardenTest::TestCase ADDITIONAL_TEST_CASES[] = 
  {
    {
      "ALL-2",
      WalledGardenTest::CSE_ALL,
      NOTSHOWWG,
      true,
      "WG + OIX marketplace (ALL)#2"
    },
    {
      "OIX-2",
      WalledGardenTest::CSE_OIX,
      NOTSHOWWG,
      true,
      "OIX marketplace#2"
    }
  };

  const WalledGardenTest::CCSelection CREATIVES[] =
  {
    {
      "KW-WG",
      "CC-WG",
      "CC-CPC-WG",
      WalledGardenTest::CSE_WALLED_GARDEN,
      WalledGardenTest::CT_CPC
    },
    {
      "KW-OIX",
      "CC-OIX",
      "CC-CPM-OIX",
      WalledGardenTest::CSE_OIX,
      WalledGardenTest::CT_CPM
    },
    {     
      "KW-ALL",
      "CC-ALL",
      "CC-CPM-ALL",
      WalledGardenTest::CSE_ALL,
      WalledGardenTest::CT_CPM
    },
    {     
      "KW-NON-WG",
      "CC-NON-WG",
      "CC-CPM-NON-WG",
      WalledGardenTest::CSE_NON_WALLED_GARDEN,
      WalledGardenTest::CT_CPM
    },
    {     
      "KW-NON-WG-2",
      "CC-NON-WG-2",
      "CC-CPM-NON-WG-2",
      WalledGardenTest::CSE_NON_WALLED_GARDEN,
      WalledGardenTest::CT_CPM
    }
  };

}

bool WalledGardenTest::non_wg_shown(
    const TestCase& testcase,
    const CCSelection& creative,
    const CCSelection& non_wg_creative)
{
  // NON WG creative showing condion

  double rev = fetch_float(creative.revenue) /
    (creative.ccgtype == CT_CPM? 1000: 1);

  double non_wg_rev = fetch_float(non_wg_creative.revenue) /
    (non_wg_creative.ccgtype == CT_CPM? 1000: 1);

  bool non_wg_creative_win = rev < non_wg_rev;
  
  return
    // always show instead 'Walled garden' creatives
    // on 'NON walled garden' or 'OIX' tags,
    ((testcase.tagtype == CSE_NON_WALLED_GARDEN ||
     testcase.tagtype == CSE_OIX) &&
     creative.cctype == CSE_WALLED_GARDEN) ||
    // or if 'NON Walled Garden' beat 'OIX' creative price
    // and tag isn't 'Walled garden'.
    (creative.cctype == CSE_OIX &&
     non_wg_creative_win && testcase.tagtype != CSE_WALLED_GARDEN) ||
    // or if 'NON Walled Garden' beat 'ALL' creative price
    // on 'Walled garden' or 'OIX' tags.
    (creative.cctype == CSE_ALL && non_wg_creative_win &&
     (testcase.tagtype == CSE_NON_WALLED_GARDEN ||
     testcase.tagtype == CSE_OIX));
}

void
WalledGardenTest::test_priority(
  const TestCase& testcase,
  const CCSelection& non_wg_creative)
{
  // WG campaigns (with 'ALL' or 'WG' options) have more preference, than OIX
  for (size_t i = 0; i < countof(CREATIVES) - 1; ++i)
  {
    if (CREATIVES[i].cctype != CSE_NON_WALLED_GARDEN)
    {
      NSLookupRequest request;
      request.referer_kw =
        fetch_string(CREATIVES[i].kwd) + "," +
        fetch_string(non_wg_creative.kwd);
      request.tid =
        fetch_string(std::string("TAG-") + testcase.case_name);
      request.referer     = "http://www.act.com";
      request.debug_time = today_;
      AdClient client(AdClient::create_user(this));

      std::string expected_ccid =
        non_wg_shown(testcase, CREATIVES[i], non_wg_creative)?
          fetch_string(non_wg_creative.ccid):
            testcase.flags & CREATIVES[i].cctype?
              fetch_string(CREATIVES[i].ccid) : "0";

      client.process_request(request, CREATIVES[i].ccid);

      if (expected_ccid == "0")
      {
      
        FAIL_CONTEXT(
          AutoTest::equal_checker(
            expected_ccid,
            client.debug_info.ccid).check(),
          std::string(testcase.description) +
            ". Check ccid#" + strof(i+1));
      }
      else
      {
        AutoTest::ConsequenceActionList actions;
        
        // Send click requests
        if (CREATIVES[i].ccgtype == CT_CPC)
        {
          actions.push_back(
            AutoTest::ConsequenceAction(
              AutoTest::CLICK,today_));
        }
        
        std::list<std::string> expected_ccs;

        expected_ccs.push_back(expected_ccid);
      
        FAIL_CONTEXT(
          client.do_ad_requests(
            expected_ccs, actions),
          std::string(testcase.description) +
            ". Adrequest#" + strof(i+1));
      }
    }
  }
}

void
WalledGardenTest::test_case(const TestCase& testcase)
  
{
  add_descr_phrase(testcase.description);
  NSLookupRequest request;
  request.tid = fetch_string(std::string("TAG-") + testcase.case_name);
  request.referer     = "http://www.act.com";
  request.debug_time = today_;

  unsigned long priority_check_index = 0;
  for (size_t i = 0; i < countof(CREATIVES); ++i)
  {
    AdClient client(AdClient::create_user(this));     
    request.referer_kw = fetch_string(CREATIVES[i].kwd);
    client.process_request(request);

    if (testcase.flags & CREATIVES[i].cctype)
    {
         
      // Send click requests
      if (CREATIVES[i].ccgtype == CT_CPC)
      {
        AutoTest::ConsequenceActionList actions;
        
        std::list<std::string> expected_ccs;
        
        expected_ccs.push_back(
          fetch_string(CREATIVES[i].ccid));

        actions.push_back(
            AutoTest::ConsequenceAction(
              AutoTest::CLICK, today_));

        FAIL_CONTEXT(
          client.do_ad_requests(
            expected_ccs, actions),
          std::string(testcase.description) +
            ". Adrequest#" + strof(i+1));
      }
      else
      {
        FAIL_CONTEXT(
          AutoTest::equal_checker(
            fetch_string(CREATIVES[i].ccid),
            client.debug_info.ccid).check(),
          std::string(testcase.description) +
            " .Check ccid#" + strof(i+1));
      }
     
    }
    else
    {
      
      FAIL_CONTEXT(
        AutoTest::predicate_checker(
          client.debug_info.selected_creatives.empty()),
        std::string(testcase.description) +
          " .Check ccid#" + strof(i+1));
    }

    // priority check for non walled garden creatives
    if (CREATIVES[i].cctype == CSE_NON_WALLED_GARDEN &&
        !testcase.skip_priority_check)
    {
      std::ostringstream dsc;
      dsc << testcase.description << " priority check#" <<
        ++priority_check_index;
      add_descr_phrase(dsc.str());
      test_priority(testcase, CREATIVES[i]);
    }
  }

}

bool 
WalledGardenTest::run()
{
  for (size_t i = 0; i < countof(TEST_CASES); ++i)
  {
    NOSTOP_FAIL_CONTEXT(test_case(TEST_CASES[i]));
  }

  for (size_t i = 0; i < countof(ADDITIONAL_TEST_CASES); ++i)
  {
    NOSTOP_FAIL_CONTEXT(test_case(ADDITIONAL_TEST_CASES[i]));
  }


  {
    add_descr_phrase("'Walled garden' & 'OIX' creatives competition");
    std::string referer_kw =
      fetch_string("KW-WG") + "," +
      fetch_string("KW-OIX");
    
    std::string ccids[] = {
      fetch_string("CC-WG"),
      fetch_string("CC-OIX"),
      fetch_string("CC-WG"),
      fetch_string("CC-OIX"),
      ""};
    NOSTOP_FAIL_CONTEXT(wg_foros_competition_case(referer_kw, ccids));
  }

  {
    add_descr_phrase("'Walled garden' & 'ALL' creatives competition."
                     "'Walled garden' have winner price.");
    std::string referer_kw =
      fetch_string("KW-WG") + "," +
      fetch_string("KW-ALL");
    
    std::string ccids[] = {
      fetch_string("CC-WG"),
      fetch_string("CC-ALL"),
      fetch_string("CC-WG"),
      fetch_string("CC-ALL"),
      ""};
    NOSTOP_FAIL_CONTEXT(wg_foros_competition_case(referer_kw, ccids));
  }

  {
    add_descr_phrase("'Walled garden' & 'ALL' creatives competition"
                     "'ALL' have winner price.");
    std::string referer_kw =
      fetch_string("KW-WG") + "," +
      fetch_string("KW-ALL-2");
    
    std::string ccids[] = {
      fetch_string("CC-ALL-2"),
      fetch_string("CC-ALL-2"),
      fetch_string("CC-ALL-2"),
      fetch_string("CC-ALL-2"),
      ""};
    NOSTOP_FAIL_CONTEXT(wg_foros_competition_case(referer_kw, ccids));
  }

  {
    add_descr_phrase("'Walled garden' & 'OIX' creatives competition"
                     "'OIX' have winner price, 'WG' blocked by tag CPM.");
    std::string referer_kw =
      fetch_string("KW-WG-2") + "," +
      fetch_string("KW-NON-WG");
    
    std::string ccids[] = {
      "",
      fetch_string("CC-NON-WG"),
      fetch_string("CC-NON-WG"),
      fetch_string("CC-NON-WG"),
      ""};
    NOSTOP_FAIL_CONTEXT(wg_foros_competition_case(referer_kw, ccids));
  }


  return true;
}

template <size_t COUNT>
void
WalledGardenTest::wg_foros_competition_case (
  const std::string& referer_kw, 
  std::string (&ccids)[COUNT])
{

  NSLookupRequest request;
  request.referer_kw = referer_kw;
  request.referer     = "http://www.act.com";
  request.debug_time = today_;

  FAIL_CONTEXT(
      AutoTest::equal_checker(
        COUNT,
        countof(TEST_CASES)).check(),
    "Invalid expected ccid sequence");
  
  for (size_t i = 0; i < countof(TEST_CASES); ++i)
  {
    request.tid = fetch_string(std::string("TAG-") + TEST_CASES[i].case_name);
    AdClient client(AdClient::create_user(this));
    client.process_request(request);
    
    if (!ccids[i].empty())
    {
      AutoTest::ConsequenceActionList actions;
        
      // Send click requests
      actions.push_back(
        AutoTest::ConsequenceAction(
          AutoTest::CLICK, today_));

      std::list<std::string> expected_ccs;
      
      expected_ccs.push_back(ccids[i]);
      
      FAIL_CONTEXT(
        client.do_ad_requests(
          expected_ccs, actions),
        "must got expected ccid for #" + strof(i) + " case");

    }
    else
    {
     
      FAIL_CONTEXT(
        AutoTest::predicate_checker(
          client.debug_info.selected_creatives.empty()),
        "must got empty ccid for #"  + strof(i) + " case");
    }
  }
}

void
WalledGardenTest::pre_condition()
{
  advstats[0].key().
    cc_id(fetch_int("CC-WG")).
    walled_garden(true).
    adv_sdate(today_);
  advstats[0].description("AdveriserStatsDaily.WalledGarden CC");
  advstats[1].key().
    cc_id(fetch_int("CC-OIX")).
    walled_garden(false).
    adv_sdate(today_);
  advstats[1].description("AdveriserStatsDaily.OIX  CC");
  advstats[2].key().
    cc_id(fetch_int("CC-ALL")).
    walled_garden(false).
    adv_sdate(today_);
  advstats[2].description("AdveriserStatsDaily.ALL  CC (WG=N)");
  advstats[3].key().
    cc_id(fetch_int("CC-ALL")).
    walled_garden(true).
    adv_sdate(today_);
  advstats[3].description("AdveriserStatsDaily.ALL  CC (WG=Y)");
  advstats[4].key().
    cc_id(fetch_int("CC-NON-WG")).
    walled_garden(false).
    adv_sdate(today_);
  advstats[4].description("AdveriserStatsDaily.Not WalledGarden CC#1");
  advstats[5].key().
    cc_id(fetch_int("CC-NON-WG-2")).
    walled_garden(false).
    adv_sdate(today_);
  advstats[5].description("AdveriserStatsDaily.Not WalledGarden CC#2");
  advstats[6].key().
    cc_id(fetch_int("CC-ALL-2")).
    walled_garden(false).
    adv_sdate(today_);
  advstats[6].description("AdveriserStatsDaily.ALL#2  CC (WG=N)");
  advstats[7].key().
    cc_id(fetch_int("CC-ALL-2")).
    walled_garden(true).
    adv_sdate(today_);
  advstats[7].description("AdveriserStatsDaily.ALL#2  CC (WG=Y)");
  advstats[8].key().
    cc_id(fetch_int("CC-WG-2"));
  advstats[8].description("AdveriserStatsDaily.WG#2  CC");
  advstats.select(pq_conn_);

  pubstats[0].key().
    tag_id(fetch_int("TAG-WG")).
    walled_garden(true).
    pub_sdate(today_);
  pubstats[0].description("PublisherStatsDaily.WalledGarden TAG (WG=Y)");
  pubstats[1].key().
    tag_id(fetch_int("TAG-WG")).
    walled_garden(false).
    pub_sdate(today_);
  pubstats[1].description("PublisherStatsDaily.WalledGarden TAG (WG=N)");
  pubstats[2].key().
    tag_id(fetch_int("TAG-OIX")).
    walled_garden(false).
    pub_sdate(today_);
  pubstats[2].description("PublisherStatsDaily.OIX TAG");
  pubstats[3].key().
    tag_id(fetch_int("TAG-ALL")).
    walled_garden(true).
    pub_sdate(today_);
  pubstats[3].description("PublisherStatsDaily.ALL TAG (WG=Y)");
  pubstats[4].key().
    tag_id(fetch_int("TAG-ALL")).
    walled_garden(false).
    pub_sdate(today_);
  pubstats[4].description("PublisherStatsDaily.ALL TAG (WG=N)");
  pubstats[5].key().
    tag_id(fetch_int("TAG-NON-WG")).
    walled_garden(false).
    pub_sdate(today_);
  pubstats[5].description("PublisherStatsDaily.Non WalledGarden TAG");
  pubstats[6].key().
    tag_id(fetch_int("TAG-WG-2")).
    walled_garden(false).
    pub_sdate(today_);
  pubstats[6].description("PublisherStatsDaily.WalledGarden TAG#2");
  pubstats[7].key().
    tag_id(fetch_int("TAG-ALL-2")).
    walled_garden(true).
    pub_sdate(today_);
  pubstats[7].description("PublisherStatsDaily.ALL TAG#2 (WG=Y)");
  pubstats[8].key().
    tag_id(fetch_int("TAG-ALL-2")).
    walled_garden(false).
    pub_sdate(today_);
  pubstats[8].description("PublisherStatsDaily.ALL TAG#2 (WG=N)");
  pubstats[9].key().
    tag_id(fetch_int("TAG-OIX-2")).
    walled_garden(false).
    pub_sdate(today_);
  pubstats[9].description("PublisherStatsDaily.OIX TAG#2");
  pubstats.select(pq_conn_);
}

void
WalledGardenTest::post_condition()
{
  add_descr_phrase("Post condition check");
  const double cc_cpc_wg = fetch_float("CC-CPC-WG");
  const double cc_cpm_foros = fetch_float("CC-CPM-OIX");
  const double cc_cpm_all = fetch_float("CC-CPM-ALL");
  const double cc_cpm_non_wg = fetch_float("CC-CPM-NON-WG");
  const double cc_cpm_non_wg_2 = fetch_float("CC-CPM-NON-WG-2");
  const double cc_cpc_all_2 = fetch_float("CC-CPC-ALL-2");
  
  const AdvDiff advdiff[] =
  {
    AdvDiff().
      imps(10).
      adv_amount(
        ORM::stats_diff_type(
          cc_cpc_wg * 10,
          0.001)),
    AdvDiff().
      imps(10).
      adv_amount(
        ORM::stats_diff_type(
          cc_cpm_foros * 10 / 1000,
          0.001)),
    AdvDiff().
      imps(8).
      adv_amount(
        ORM::stats_diff_type(
          cc_cpm_all * 8 / 1000,
          0.001)),
    AdvDiff().
      imps(6).
      adv_amount(
        ORM::stats_diff_type(
          cc_cpm_all * 6 / 1000,
          0.001)),
    AdvDiff().
      imps(15).
      adv_amount(
        ORM::stats_diff_type(
          cc_cpm_non_wg * 15 / 1000,
          0.001)),
    AdvDiff().
      imps(7).
      adv_amount(
        ORM::stats_diff_type(
          cc_cpm_non_wg_2 * 7 / 1000,
          0.001)),
    AdvDiff().
      imps(2).
      adv_amount(
        ORM::stats_diff_type(
          cc_cpc_all_2 * 2,
          0.001)),
    AdvDiff().
      imps(2).
      adv_amount(
        ORM::stats_diff_type(
          cc_cpc_all_2 * 2,
          0.001)),
    AdvDiff(0)
  };

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        pq_conn_, advdiff, advstats)).check(),
    "AdvertiserStatsDaily check");

  const double tag_cpm_wg = fetch_float("TAG-CPM-WG");
  const double tag_cpm_foros = fetch_float("TAG-CPM-OIX");
  const double tag_cpm_all = fetch_float("TAG-CPM-ALL");
  const double tag_cpm_non_wg = fetch_float("TAG-CPM-NON-WG");

  
  const PubDiff pubdiff[] =
  {
    PubDiff().
      imps(9).
      pub_amount(
        ORM::stats_diff_type(
        (tag_cpm_wg * 9) / 1000,
        0.001)),
    PubDiff(0),
    PubDiff().
      imps(14).
      pub_amount(
        ORM::stats_diff_type(
          (tag_cpm_foros * 14) / 1000,
          0.001)),
    PubDiff().
      imps(9).
      pub_amount(
        ORM::stats_diff_type(
          (tag_cpm_all * 9) / 1000,
          0.001)),
    PubDiff().
      imps(6).
      pub_amount(
        ORM::stats_diff_type(
          (tag_cpm_all * 6) / 1000, 
          0.001)),
    PubDiff().
      imps(14).
      pub_amount(
        ORM::stats_diff_type(
          (tag_cpm_non_wg * 14) / 1000,
          0.001)),
    PubDiff(0),
    PubDiff(0),
    PubDiff().
      imps(4).
      pub_amount(0),
    PubDiff().
      imps(4).
      pub_amount(0)
  };

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        pq_conn_, pubdiff, pubstats)).check(),
    "PublisherStatsDaily check");
}

void
WalledGardenTest::tear_down()
{
}
