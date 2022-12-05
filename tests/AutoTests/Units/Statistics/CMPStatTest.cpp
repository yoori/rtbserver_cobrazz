
#include "CMPStatTest.hpp"
 
REFLECT_UNIT(CMPStatTest) (
  "Statistics",
  AUTO_TEST_SLOW
);

namespace
{
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::SelectedCreativesCCID SelectedCreativesCCID;
  typedef AutoTest::ChannelsCheck ChannelsCheck;
  static const size_t ITERS = 100;
  const char* GN = "GN";

  const CMPStatTest::ExpectedDiff ZERO_DIFF(0);
}

// class CMPStatTest

template<size_t Count>
void
CMPStatTest::initialize_stats(
  CMPStats& stats,
  CMPDiffs& diffs,
  const CaseStat(&case_stats) [Count])
{
  for (unsigned int i = 0; i < Count; ++i)
  {
    CMPStat::Key key;
    key.adv_account_id(fetch_int(case_stats[i].adv_account));
    key.cmp_account_id(fetch_int(case_stats[i].cmp_account));
    key.colo_id(case_stats[i].colo?
      fetch_int(case_stats[i].colo): 1);
    key.channel_id(fetch_int(case_stats[i].channel));
    key.country_code(GN);
    key.walled_garden(case_stats[i].walled_garden);

    if (case_stats[i].cc)
    {
      key.cc_id(fetch_int(case_stats[i].cc));
    }

    switch (case_stats[i].date_type)
    {
    case CDF_SDATE:
      key.sdate(case_stats[i].date);
      break;
    case CDF_ADV_SDATE:
      key.adv_sdate(case_stats[i].date);
      break;
    case CDF_CMP_SDATE:
      key.cmp_sdate(case_stats[i].date);
      break;
    }

    CMPStat stat;
    stat.key(key);
    stat.description("#" +strof(i+1));
    stat.select(pq_conn_);
   
    if (
      std::equal(
        case_stats[i].diff.begin(),
        case_stats[i].diff.end(),
        ZERO_DIFF.begin()) )
    {
      stats.push_back(stat);
      diffs.push_back(case_stats[i].diff);
    }
    else
    {
      stats.push_front(stat);
      diffs.push_front(case_stats[i].diff);
    }
  }

}

template<size_t Count>
void
CMPStatTest::send_requests(
  const AutoTest::Time& debug_time,
  const Request(&requests) [Count])
{
  for (size_t i = 0;
       i < Count; ++i)
  {
    AdClient client(AdClient::create_user(this));
    
    client.process_request(
      AutoTest::NSLookupRequest().
        tid(fetch_string(requests[i].tid)).
          referer_kw(map_objects(requests[i].referer_kws)).
            debug_time(debug_time));
    
    FAIL_CONTEXT(
      ChannelsCheck(
        this, requests[i].channels,
        client.debug_info.history_channels).check(),
      "Expected history#" + strof(i+1));

    if (requests[i].ccs)
    {
      std::list<std::string> expected_ccs;
      fetch_objects(
        std::inserter(expected_ccs, expected_ccs.begin()),
        requests[i].ccs);
    
      FAIL_CONTEXT(
        AutoTest::sequence_checker(
          expected_ccs,
          SelectedCreativesCCID(client)).check(),
        "Check CCs#" + strof(i+1));
    }
    else
    {
      FAIL_CONTEXT(
        AutoTest::predicate_checker(
          client.debug_info.selected_creatives.empty()),
        "Check CCs#" + strof(i+1));
    }
  }
}


void
CMPStatTest::base_scenario()
{
  CMPStats stats;
  CMPDiffs diffs;

  // Case initialization
  AutoTest::Time today = Generics::Time::get_time_of_day();
  AutoTest::Time tomorrow = today + Generics::Time::ONE_DAY;

  double cpm1 = fetch_float("Base/Channel/CPM/CPM") / 1000;
  double cpm2 = fetch_float("Base/Channel/CPM/WG") / 1000;
  double cpc = fetch_float("Base/Channel/CPC/CPC");
  double rate1 = fetch_float("RATE1");
  double rate2 = fetch_float("RATE2");

  std::string current_date = today.get_gm_time().format("%d-%m-%Y");

  const std::string debug_time[3] =
  {
    current_date + ":11-01-25",
    current_date + ":18-00-00",
    current_date + ":23-59-00"
  };

  const CaseStat BASE[] =
  {
    // CPM cases + date
    // #1
    { "Base/Account/ADV", "Base/Account/CMP",
      0, "Base/Channel/CPM", "Base/CC/CPM",
      false, CDF_SDATE, debug_time[0],
      ExpectedDiff().
        imps(25).
        clicks(9).
        adv_amount_cmp(25 * cpm1 * rate2 / rate1).
        cmp_amount(25 * cpm1).
        cmp_amount_global(25 * cpm1 / rate1)
    },
    // #2
    { "Base/Account/ADV", "Base/Account/CMP",
      0, "Base/Channel/CPM", "Base/CC/CPM",
      false, CDF_SDATE, debug_time[1],
      ExpectedDiff().
        imps(25).
        clicks(9).
        adv_amount_cmp(25 * cpm1 * rate2 / rate1).
        cmp_amount(25 * cpm1).
        cmp_amount_global(25 * cpm1 / rate1)
    },
    // #3
    { "Base/Account/ADV", "Base/Account/CMP",
      0, "Base/Channel/CPM", "Base/CC/CPM",
      false, CDF_SDATE, debug_time[2],
      ExpectedDiff().
        imps(25).
        clicks(9).
        adv_amount_cmp(25 * cpm1 * rate2 / rate1).
        cmp_amount(25 * cpm1).
        cmp_amount_global(25 * cpm1 / rate1)
    },
    // #4
    { "Base/Account/ADV", "Base/Account/CMP",
      0, "Base/Channel/CPM", "Base/CC/CPM/CC2",
      false, CDF_SDATE, debug_time[0],
      ExpectedDiff().
        imps(25).
        clicks(8).
        adv_amount_cmp(25 * cpm1 * rate2 / rate1).
        cmp_amount(25 * cpm1).
        cmp_amount_global(25 * cpm1 / rate1)
    },
    // #5
    { "Base/Account/ADV", "Base/Account/CMP",
      0, "Base/Channel/CPM", "Base/CC/CPM/CC2",
      false, CDF_SDATE, debug_time[1],
      ExpectedDiff().
        imps(25).
        clicks(8).
        adv_amount_cmp(25 * cpm1 * rate2 / rate1).
        cmp_amount(25 * cpm1).
        cmp_amount_global(25 * cpm1 / rate1)
    },
    // #6
    { "Base/Account/ADV", "Base/Account/CMP",
      0, "Base/Channel/CPM", "Base/CC/CPM/CC2",
      false, CDF_SDATE, debug_time[2],
      ExpectedDiff().
        imps(25).
        clicks(8).
        adv_amount_cmp(25 * cpm1 * rate2 / rate1).
        cmp_amount(25 * cpm1).
        cmp_amount_global(25 * cpm1 / rate1)
    },
    // #7
    { "Base/Account/ADV", "Base/Account/CMP",
      0, "Base/Channel/CPM", "Base/CC/CPM",
      false, CDF_ADV_SDATE, today,
      ExpectedDiff().
        imps(25).
        clicks(9).
        adv_amount_cmp(25 * cpm1 * rate2 / rate1).
        cmp_amount(25 * cpm1).
        cmp_amount_global(25 * cpm1 / rate1)
    },
    // #8
    { "Base/Account/ADV", "Base/Account/CMP",
      0, "Base/Channel/CPM", "Base/CC/CPM/CC2",
      false, CDF_ADV_SDATE, today,
      ExpectedDiff().
        imps(25).
        clicks(8).
        adv_amount_cmp(25 * cpm1 * rate2 / rate1).
        cmp_amount(25 * cpm1).
        cmp_amount_global(25 * cpm1 / rate1)
    },
    // #9
    { "Base/Account/ADV", "Base/Account/CMP",
      0, "Base/Channel/CPM", "Base/CC/CPM",
      false, CDF_ADV_SDATE, tomorrow,
      ExpectedDiff().
        imps(50).
        clicks(18).
        adv_amount_cmp(50 * cpm1 * rate2 / rate1).
        cmp_amount(50 * cpm1).
        cmp_amount_global(50 * cpm1 / rate1)
    },
    // #10
    { "Base/Account/ADV", "Base/Account/CMP",
      0, "Base/Channel/CPM", "Base/CC/CPM/CC2",
      false, CDF_ADV_SDATE, tomorrow,
      ExpectedDiff().
        imps(50).
        clicks(16).
        adv_amount_cmp(50 * cpm1 * rate2 / rate1).
        cmp_amount(50 * cpm1).
        cmp_amount_global(50 * cpm1 / rate1)
    },
    // #11
    { "Base/Account/ADV", "Base/Account/CMP",
      0, "Base/Channel/CPM", "Base/CC/CPM",
      false, CDF_CMP_SDATE, today,
      ExpectedDiff().
        imps(50).
        clicks(18).
        adv_amount_cmp(50 * cpm1 * rate2 / rate1).
        cmp_amount(50 * cpm1).
        cmp_amount_global(50 * cpm1 / rate1)
    },
    // #12
    { "Base/Account/ADV", "Base/Account/CMP",
      0, "Base/Channel/CPM", "Base/CC/CPM/CC2",
      false, CDF_CMP_SDATE, today,
      ExpectedDiff().
        imps(50).
        clicks(16).
        adv_amount_cmp(50 * cpm1 * rate2 / rate1).
        cmp_amount(50 * cpm1).
        cmp_amount_global(50 * cpm1 / rate1)
    },
    // #13
    { "Base/Account/ADV", "Base/Account/CMP",
      0, "Base/Channel/CPM", "Base/CC/CPM",
      false, CDF_CMP_SDATE, tomorrow,
      ExpectedDiff().
        imps(25).
        clicks(9).
        adv_amount_cmp(25 * cpm1 * rate2 / rate1).
        cmp_amount(25 * cpm1).
        cmp_amount_global(25 * cpm1 / rate1)
    },
    // #14
    { "Base/Account/ADV", "Base/Account/CMP",
      0, "Base/Channel/CPM", "Base/CC/CPM/CC2",
      false, CDF_CMP_SDATE, tomorrow,
      ExpectedDiff().
        imps(25).
        clicks(8).
        adv_amount_cmp(25 * cpm1 * rate2 / rate1).
        cmp_amount(25 * cpm1).
        cmp_amount_global(25 * cpm1 / rate1)
    },
    // CPC case
    // #15
    { "Base/Account/ADV", "Base/Account/CMP",
      0, "Base/Channel/CPC", "Base/CC/CPC",
      false, CDF_SDATE, today,
      ExpectedDiff().
        imps(50).
        clicks(17).
        adv_amount_cmp(17 * cpc * rate2 / rate1).
        cmp_amount(17 * cpc).
        cmp_amount_global(17 * cpc / rate1)
    },
    // Non default colo
    // #16
    { "Base/Account/ADV", "Base/Account/CMP",
      "COLO", "Base/Channel/CPM", "Base/CC/CPM",
      false, CDF_SDATE, debug_time[0],
      ExpectedDiff().
        imps(50).
        clicks(16).
        adv_amount_cmp(50 * cpm1 * rate2 / rate1).
        cmp_amount(50 * cpm1).
        cmp_amount_global(50 * cpm1 / rate1)
    },
    // #17
    { "Base/Account/ADV", "Base/Account/CMP",
      "COLO", "Base/Channel/CPM", "Base/CC/CPM",
      false, CDF_SDATE, debug_time[1],
      ExpectedDiff().
        imps(50).
        clicks(16).
        adv_amount_cmp(50 * cpm1 * rate2 / rate1).
        cmp_amount(50 * cpm1).
        cmp_amount_global(50 * cpm1 / rate1)
    },
    // #18
    { "Base/Account/ADV", "Base/Account/CMP",
      "COLO", "Base/Channel/CPM", "Base/CC/CPM",
      false, CDF_SDATE, debug_time[2],
      ExpectedDiff().
        imps(50).
        clicks(16).
        adv_amount_cmp(50 * cpm1 * rate2 / rate1).
        cmp_amount(50 * cpm1).
        cmp_amount_global(50 * cpm1 / rate1)
    },
    // #19
    { "Base/Account/ADV", "Base/Account/CMP",
      "COLO", "Base/Channel/CPC", "Base/CC/CPC",
      false, CDF_SDATE, today,
      ExpectedDiff().
        imps(50).
        clicks(16).
        adv_amount_cmp(16 * cpc * rate2 / rate1).
        cmp_amount(16 * cpc).
        cmp_amount_global(16 * cpc / rate1)
    },
    // Walled garden
    // #20
    { "Base/Account/WG-ADV", "Base/Account/CMP",
      0, "Base/Channel/WG", "Base/CC/WG",
      true, CDF_SDATE, today,
      ExpectedDiff().
        imps(50).
        clicks(50).
        adv_amount_cmp(50 * cpm2 * rate2 / rate1).
        cmp_amount(50 * cpm2).
        cmp_amount_global(50 * cpm2 / rate1)
    },
    // #21
    { "Base/Account/WG-ADV", "Base/Account/CMP",
      "COLO", "Base/Channel/WG", "Base/CC/WG",
      true, CDF_SDATE, today,
      ExpectedDiff().
        imps(50).
        clicks(50).
        adv_amount_cmp(50 * cpm2 * rate2 / rate1).
        cmp_amount(50 * cpm2).
        cmp_amount_global(50 * cpm2 / rate1)
    }
  };

  initialize_stats(stats, diffs, BASE);

  // Requests
  for (size_t i = 0; i < ITERS; ++i)
  {
    // CPM case + dates
    {
      AdClient client(AdClient::create_user(this));
      AutoTest::NSLookupRequest request;
      request.tid = i % 4 == 0?
        fetch_string("Base/TAG/PUBLISHER2") :
          fetch_string("Base/TAG/PUBLISHER1");
      request.referer_kw = fetch_string("Base/KWD/CPM");
      if (i % 2 == 1)
      {
        request.colo = fetch_string("COLO");
      }
      // differrent sdate checking
      for (size_t t = 0; t < 3; ++t)
      {
        request.debug_time = debug_time[t];
        client.process_request(request);
        
        FAIL_CONTEXT(
          AutoTest::equal_checker(
            i % 4 == 0?
              fetch_string("Base/CC/CPM/CC2"):
                fetch_string("Base/CC/CPM"),
            client.debug_info.ccid).check(),
          "CPM check CC#" + strof(i+1) + "-" + strof(t+1));
        
        FAIL_CONTEXT(
          AutoTest::predicate_checker(
            !client.debug_info.selected_creatives.first().click_url.empty()),
          "CPM check click#" + strof(i+1) + "-" + strof(t+1));

        // Every 3rd iteration click on ad.
        if (i % 3 == 2)
        {
          client.process_request(
            std::string(client.debug_info.selected_creatives.first().click_url));
        }
      }
    }

    // Walled garden
    {
      AdClient client(AdClient::create_user(this));
      AutoTest::NSLookupRequest request;
      request.debug_time = today;
      request.tid = fetch_string("Base/TAG/WG");
      request.referer_kw = fetch_string("Base/KWD/WG");
      if (i % 2 == 1)
      {
        request.colo = fetch_string("COLO");
      }       
      client.process_request(request);


      FAIL_CONTEXT(
        AutoTest::equal_checker(
          fetch_string("Base/CC/WG"),
          client.debug_info.ccid).check(),
        "Walled garden check CC#" + strof(i+1)
        );
      
      FAIL_CONTEXT(
        AutoTest::predicate_checker(
          !client.debug_info.selected_creatives.first().click_url.empty()),
        "Walled garden check click#" + strof(i+1));
      
      std::string click_url =
        client.debug_info.selected_creatives.first().click_url;
      
      client.process_request(click_url);
    }

    // CPC cases
    {
      AdClient client(AdClient::create_user(this));
      AutoTest::NSLookupRequest request;
      request.debug_time = today;
      request.tid = fetch_string("Base/TAG/PUBLISHER1");
      request.referer_kw = fetch_string("Base/KWD/CPC");
      if (i % 2 == 1)
      {
        request.colo = fetch_string("COLO");
      }       
      
      client.process_request(request);

      FAIL_CONTEXT(
        AutoTest::equal_checker(
          fetch_string("Base/CC/CPC"),
          client.debug_info.ccid).check(),
        "CPC check CC#" + strof(i+1)
        );
      
      FAIL_CONTEXT(
        AutoTest::predicate_checker(
          !client.debug_info.selected_creatives.first().click_url.empty()),
        "CPC check click#" + strof(i+1));

      std::string click_url = client.debug_info.selected_creatives.first().click_url;
      // Every 3rd iteration click on ad.
      if (i % 3 == 2)
      {
        client.process_request(click_url);
        client.repeat_request();
      }
    }
  }
  
  ADD_WAIT_CHECKER(
    "CMPStats check",
    AutoTest::stats_diff_checker(
      pq_conn_, diffs, stats));
}

void
CMPStatTest::adv_exp_ch_scenario()
{
  CMPStats stats;
  CMPDiffs diffs;

  // Case initialization
  AutoTest::Time today = Generics::Time::get_time_of_day();
  double cpm = fetch_float("AdvExpr/Channel/CPM/CPM") / 1000;
  double cpc = fetch_float("Base/Channel/CPC/CPC");

  const CaseStat ADV_EXPR[] =
  {
    // #1
    {
      "AdvExpr/Account/ADV", "AdvExpr/Account/CMP",
      0, "AdvExpr/Channel/CPM", "AdvExpr/CC/CCG",
      false, CDF_SDATE, today,
      ExpectedDiff().
        imps(ITERS).
        clicks(ITERS / 3).
        adv_amount_cmp(ITERS * cpm).
        cmp_amount(ITERS * cpm).
        cmp_amount_global(ITERS * cpm)
    },
    // #2
    {
      "AdvExpr/Account/ADV", "AdvExpr/Account/CMP",
      0, "AdvExpr/Channel/CPC", "AdvExpr/CC/CCG",
      false, CDF_SDATE, today,
      ExpectedDiff().
        imps(2 * ITERS).
        clicks(ITERS / 3 + ITERS / 5).
        adv_amount_cmp((ITERS / 3 + ITERS / 5) * cpc).
        cmp_amount((ITERS / 3 + ITERS / 5) * cpc).
        cmp_amount_global((ITERS / 3 + ITERS / 5) * cpc)
    },
    // #3
    {
      "AdvExpr/Account/ADV", "AdvExpr/Account/CMP",
      0, "AdvExpr/Channel/PRI", 0,
      false, CDF_SDATE, today,
      ZERO_DIFF
    },
    // #4
    {
      "AdvExpr/Account/ADV", "AdvExpr/Account/CMP",
      0, "AdvExpr/Expr/EXPR", 0,
      false, CDF_SDATE, today,
      ZERO_DIFF
    },
    // #5
    {
      "AdvExpr/Account/ADV", "AdvExpr/Account/ADV",
      0, "AdvExpr/Channel/PRI", 0,
      false, CDF_SDATE, today,
      ZERO_DIFF
    },
    // #6
    {
      "AdvExpr/Account/ADV", "AdvExpr/Account/ADV",
      0, "AdvExpr/Expr/EXPR", 0,
      false, CDF_SDATE, today,
      ZERO_DIFF
    }
  };

  initialize_stats(stats, diffs, ADV_EXPR);

  // Requests

  for (size_t i = 0; i < ITERS; ++i)
  {
    AutoTest::NSLookupRequest request;
    {
      AdClient client(AdClient::create_user(this));
      request.tid = fetch_string("AdvExpr/TAG/PUBLISHER");
      request.referer_kw = fetch_string("AdvExpr/KWD/CPM");
      request.debug_time(today);
      
      client.process_request(request);
      
      FAIL_CONTEXT(
        AutoTest::equal_checker(
          fetch_string("AdvExpr/CC/CCG"),
          client.debug_info.ccid).check(),
        "Check CC#" + strof(i+1) + "-1");
      
      
      FAIL_CONTEXT(
        AutoTest::predicate_checker(
          !client.debug_info.selected_creatives.first().click_url.empty()),
        "Check click#" + strof(i+1) + "-1");
      
      // Every 3rd iteration click on ad.
      if (i % 3 == 2)
      {
        client.process_request(
          std::string(client.debug_info.selected_creatives.first().click_url));
      }
    }

    {
      AdClient client(AdClient::create_user(this));

      request.referer_kw = fetch_string("AdvExpr/KWD/PRI");
      
      client.process_request(request);
      
      FAIL_CONTEXT(
        AutoTest::equal_checker(
          fetch_string("AdvExpr/CC/CCG"),
          client.debug_info.ccid).check(),
        "Check CC#" + strof(i+1) + "-2");

      FAIL_CONTEXT(
        AutoTest::predicate_checker(
          !client.debug_info.selected_creatives.first().click_url.empty()),
        "Check click#" + strof(i+1) + "-2");

      // Every 5th iteration click on ad.
      if (i % 5 == 2)
      {
        client.process_request(
          std::string(client.debug_info.selected_creatives.first().click_url));
      }
    }

    {
      AdClient client(AdClient::create_user(this));

      request.referer_kw = fetch_string("AdvExpr/KWD/CPC");
     
      client.process_request(request);

      FAIL_CONTEXT(
        AutoTest::predicate_checker(
          client.debug_info.selected_creatives.empty()),
        "Check CCG#" + strof(i+1) + "-3");
    }
  }

  ADD_WAIT_CHECKER(
    "CMPStats check",
    AutoTest::stats_diff_checker(
      pq_conn_, diffs, stats));
}

void
CMPStatTest::cmp_exp_ch_scenario()
{
  CMPStats stats;
  CMPDiffs diffs;

  // Case initialization
  AutoTest::Time today = Generics::Time::get_time_of_day();
  double cpm = fetch_float("CmpExpr/Expr/CPM/EXPR") / 1000;

  const CaseStat CMP_EXPR[] =
  {
    // #1
    {
      "CmpExpr/Account/ADV", "CmpExpr/Account/CMP",
      0, "CmpExpr/Channel/CPM", 0,
      false, CDF_SDATE, today,
      ZERO_DIFF
    },
    // #2
    {
      "CmpExpr/Account/ADV", "CmpExpr/Account/CMP",
      0, "CmpExpr/Expr/EXPR", "CmpExpr/CC/CCG",
      false, CDF_SDATE, today,
      ExpectedDiff().
        imps(2 * ITERS).
        clicks(2 * ITERS / 3).
        adv_amount_cmp(2 * ITERS * cpm).
        cmp_amount(2 * ITERS * cpm).
        cmp_amount_global(2 * ITERS * cpm)
    },
    // #3
    {
      "CmpExpr/Account/ADV", "CmpExpr/Account/CMP",
      0, "CmpExpr/Channel/PRI", 0,
      false, CDF_SDATE, today,
      ZERO_DIFF
    },
  };

   initialize_stats(stats, diffs, CMP_EXPR);

  // Requests
  
  const std::string keyword[2] =
  {
    fetch_string("CmpExpr/KWD/CPM"),
    fetch_string("CmpExpr/KWD/PRI")
  };
  
  for (size_t i = 0; i < ITERS; ++i)
  {
    for (size_t k = 0; k < 2; ++k)
    {
      AdClient client(AdClient::create_user(this));
      AutoTest::NSLookupRequest request;
      request.debug_time(today);
      request.referer_kw = keyword[k];
      request.tid = fetch_string("CmpExpr/TAG/PUBLISHER");

      client.process_request(request);

      FAIL_CONTEXT(
        AutoTest::equal_checker(
          fetch_string("CmpExpr/CC/CCG"),
          client.debug_info.ccid).check(),
        "Check CC#" + strof(i+1) + "-" + strof(k+1));

      FAIL_CONTEXT(
        AutoTest::predicate_checker(
          !client.debug_info.selected_creatives.first().click_url.empty()),
        "Check click#" + strof(i+1) + "-" + strof(k+1));

      
      // Every 3rd iteration click on ad.
      if (i % 3 == 2)
      {
        client.process_request(
          std::string(client.debug_info.selected_creatives.first().click_url));
       }
     }
  }

  ADD_WAIT_CHECKER(
    "CMPStats check",
    AutoTest::stats_diff_checker(
      pq_conn_, diffs, stats));
}

void
CMPStatTest::pub_exp_ch_scenario()
{
  CMPStats stats;
  CMPDiffs diffs;

  // Case initialization
  AutoTest::Time today = Generics::Time::get_time_of_day();

  const CaseStat PUB_EXPR[] =
  {
    // #1
    {
      "PubExpr/Account/ADV", "PubExpr/Account/CMP",
      0, "PubExpr/Channel/CPM", 0,
      false, CDF_SDATE, today,
      ZERO_DIFF
    },
    // #2
    {
      "PubExpr/Account/ADV", "PubExpr/Account/CMP",
      0, "PubExpr/Expr/EXPR", 0,
      false, CDF_SDATE, today,
      ZERO_DIFF
    }
  };

  initialize_stats(stats, diffs, PUB_EXPR);

  // Requests
  
  for (size_t i = 0; i < ITERS; ++i)
  {
    AdClient client(AdClient::create_user(this));
    AutoTest::NSLookupRequest request;
    request.tid = fetch_string("PubExpr/TAG/PUBLISHER");
    request.referer_kw =  fetch_string("PubExpr/KWD/CPM");
    request.debug_time(today);
    
    client.process_request(request);

    FAIL_CONTEXT(
      AutoTest::equal_checker(
        fetch_string("PubExpr/CC/CCG"),
        client.debug_info.ccid).check(),
      "Check CCG#" + strof(i+1));

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        !client.debug_info.selected_creatives.first().click_url.empty()),
      "Check click#" + strof(i+1));
      
    client.process_request(
      std::string(client.debug_info.selected_creatives.first().click_url));
   }

  ADD_WAIT_CHECKER(
    "CMPStats check",
    AutoTest::stats_diff_checker(
      pq_conn_, diffs, stats));
}

void
CMPStatTest::currency_scenario()
{
  CMPStats stats;
  CMPDiffs diffs;

  // Case initialization
  AutoTest::Time today = Generics::Time::get_time_of_day();

  double cpm1 = fetch_float("Currency/Expr/CPM/EXPR2") / 1000;
  double cpm2 = fetch_float("Currency/Channel/CPM/CPM2") / 1000;
  double cpm3 = fetch_float("Currency/Channel/CPM/CPM3") / 1000;
  double rate = fetch_float("RATE");

  const CaseStat CURRENCY[] =
  {
    // #1
    {
      "Currency/Account/ADV1", "Currency/Account/CMP2",
      0, "Currency/Channel/CPM2", "Currency/CC/CCG1",
      false, CDF_SDATE, today,
      ExpectedDiff().
        imps(1).
        clicks(0).
        adv_amount_cmp(cpm2 / rate).
        cmp_amount(cpm2).
        cmp_amount_global(cpm2 / rate)
    },
    // #2
    {
      "Currency/Account/ADV2", "Currency/Account/CMP1",
      0, "Currency/Expr/EXPR2", "Currency/CC/CCG2",
      false, CDF_SDATE, today,
      ExpectedDiff().
        imps(1).
        clicks(0).
        adv_amount_cmp(cpm1 * rate).
        cmp_amount(cpm1).
        cmp_amount_global(cpm1)
    },
    // #3
    {
      "Currency/Account/ADV2", "Currency/Account/CMP2",
      0, "Currency/Channel/CPM3", "Currency/CC/CCG3",
      false, CDF_SDATE, today,
      ExpectedDiff().
        imps(1).
        clicks(0).
        adv_amount_cmp(cpm3).
        cmp_amount(cpm3).
        cmp_amount_global(cpm3 / rate)
    },
    // #4
    {
      "Currency/Account/ADV2", "Currency/Account/CMP1",
      0, "Currency/Expr/EXPR1", 0,
      false, CDF_SDATE, today,
      ZERO_DIFF
    },
    // #5
    {
      "Currency/Account/ADV2", "Currency/Account/CMP1",
      0, "Currency/Channel/CPM1", 0,
      false, CDF_SDATE, today,
      ZERO_DIFF
    }
  };

  initialize_stats(stats, diffs, CURRENCY);


  // Requests
  
  const Request requests[] =
  {
    {
      "Currency/KWD/CPM2",
      "Currency/TAG/PUBLISHER1",
      "Currency/Channel/CPM2",
      "Currency/CC/CCG1"
    },
    {
      "Currency/KWD/CPM1",
      "Currency/TAG/PUBLISHER1",
      "Currency/Channel/CPM1",
      "Currency/CC/CCG2"
    },
    {
      "Currency/KWD/CPM1",
      "Currency/TAG/PUBLISHER2",
      "Currency/Channel/CPM1",
      0
    },
    {
      "Currency/KWD/CPM3",
      "Currency/TAG/PUBLISHER1",
      "Currency/Channel/CPM3",
      "Currency/CC/CCG3"
    },
  };

  send_requests(today, requests);

  ADD_WAIT_CHECKER(
    "CMPStats check",
    AutoTest::stats_diff_checker(
      pq_conn_, diffs, stats));

}


void
CMPStatTest::ta_scenario()
{
  CMPStats stats;
  CMPDiffs diffs;

  // Case initialization
  AutoTest::Time today = Generics::Time::get_time_of_day();

  double cpc = fetch_float("TA/Channel/CPC/TEXT");

  const CaseStat TA[] =
  {
    // #1
    {
      "TA/Account/ADV1", "TA/Account/CMP",
      0, "TA/Channel/TEXT", "TA/CC/CCG1",
      false, CDF_SDATE, today,
      ExpectedDiff().
        imps(1).
        clicks(1).
        adv_amount_cmp(cpc).
        cmp_amount(cpc).
        cmp_amount_global(cpc)
    },
    // #2
    {
      "TA/Account/ADV2", "TA/Account/CMP",
      0, "TA/Channel/TEXT", "TA/CC/CCG2",
      false, CDF_SDATE, today,
      ExpectedDiff().
        imps(1).
        clicks(1).
        adv_amount_cmp(cpc).
        cmp_amount(cpc).
        cmp_amount_global(cpc)
    }
  };

  initialize_stats(stats, diffs, TA);

  AdClient client(AdClient::create_user(this));
    
  client.process_request(
    AutoTest::NSLookupRequest().
    tid(fetch_string("TA/TAG/PUBLISHER")).
    referer_kw(fetch_string("TA/KWD/TEXT")).
    debug_time(today));

  AutoTest::ConsequenceActionList actions;

  actions.push_back(
    AutoTest::ConsequenceAction(
      AutoTest::CLICK, today));

  std::list<std::string> expected_ccs;
  fetch_objects(
    std::inserter(expected_ccs, expected_ccs.begin()),
    "TA/CC/CCG1,TA/CC/CCG2");

  FAIL_CONTEXT(
    client.do_ad_requests(
      expected_ccs, actions));

  ADD_WAIT_CHECKER(
    "CMPStats check",
    AutoTest::stats_diff_checker(
      pq_conn_, diffs, stats));

}

void
CMPStatTest::expression_scenario()
{
  CMPStats stats;
  CMPDiffs diffs;

  // Case initialization
  AutoTest::Time today = Generics::Time::get_time_of_day();

  double cpm1 = fetch_float("Expression/Channel/CPM/CH1") / 1000;
  double cpm3 = fetch_float("Expression/Channel/CPM/CH3") / 1000;
  double cpm4 = fetch_float("Expression/Channel/CPM/CH4") / 1000;
  double cpm5 = fetch_float("Expression/Channel/CPM/CH5") / 1000;
  double cpm6 = fetch_float("Expression/Channel/CPM/CH6") / 1000;
  double cpm7 = fetch_float("Expression/Channel/CPM/CH7") / 1000;
  double cpm8 = fetch_float("Expression/Channel/CPM/CH8") / 1000;
  double cpm10 = fetch_float("Expression/Channel/CPM/CH10") / 1000;
  double cpm11 = fetch_float("Expression/Channel/CPM/CH11") / 1000;
  double cpm_e1 = fetch_float("Expression/Expr/CPM/EXPR1") / 1000;

  const CaseStat EXPRESSION[] =
  {
    // #1
    {
      "Expression/Account/ADV", "Expression/Account/CMP",
      0, "Expression/Channel/CH1", "Expression/CC/CCG1",
      false, CDF_SDATE, today,
      ExpectedDiff().
        imps(1).
        clicks(0).
        adv_amount_cmp(cpm1).
        cmp_amount(cpm1).
        cmp_amount_global(cpm1)
    },
    // #2
    {
      "Expression/Account/ADV", "Expression/Account/CMP",
      0, "Expression/Channel/CH2", "Expression/CC/CCG1",
      false, CDF_SDATE, today,
      ZERO_DIFF
    },
    // #3
    {
      "Expression/Account/ADV", "Expression/Account/CMP",
      0, "Expression/Channel/CH3", "Expression/CC/CCG1",
      false, CDF_SDATE, today,
      ExpectedDiff().
        imps(1).
        clicks(0).
        adv_amount_cmp(cpm3).
        cmp_amount(cpm3).
        cmp_amount_global(cpm3)
    },
    // #4
    {
      "Expression/Account/ADV", "Expression/Account/CMP",
      0, "Expression/Channel/CH4", "Expression/CC/CCG1",
      false, CDF_SDATE, today,
      ExpectedDiff().
        imps(1).
        clicks(0).
        adv_amount_cmp(cpm4).
        cmp_amount(cpm4).
        cmp_amount_global(cpm4)
    },
    // #5
    {
      "Expression/Account/ADV", "Expression/Account/CMP",
      0, "Expression/Expr/EXPR1", "Expression/CC/CCG1",
      false, CDF_SDATE, today,
      ExpectedDiff().
        imps(1).
        clicks(0).
        adv_amount_cmp(cpm_e1).
        cmp_amount(cpm_e1).
        cmp_amount_global(cpm_e1)
    },
    // #6
    {
      "Expression/Account/ADV", "Expression/Account/CMP",
      0, "Expression/Channel/CH5", "Expression/CC/CCG2",
      false, CDF_SDATE, today,
      ExpectedDiff().
        imps(1).
        clicks(0).
        adv_amount_cmp(cpm5).
        cmp_amount(cpm5).
        cmp_amount_global(cpm5)
    },
    // #7
    {
      "Expression/Account/ADV", "Expression/Account/CMP",
      0, "Expression/Channel/CH6", "Expression/CC/CCG2",
      false, CDF_SDATE, today,
      ExpectedDiff().
        imps(1).
        clicks(0).
        adv_amount_cmp(cpm6).
        cmp_amount(cpm6).
        cmp_amount_global(cpm6)
    },
    // #8
    {
      "Expression/Account/ADV", "Expression/Account/CMP",
      0, "Expression/Channel/CH7", "Expression/CC/CCG2",
      false, CDF_SDATE, today,
      ExpectedDiff().
        imps(1).
        clicks(0).
        adv_amount_cmp(cpm7).
        cmp_amount(cpm7).
        cmp_amount_global(cpm7)
    },
    // #9
    {
      "Expression/Account/ADV", "Expression/Account/CMP",
      0, "Expression/Channel/CH8", "Expression/CC/CCG2",
      false, CDF_SDATE, today,
      ExpectedDiff().
        imps(1).
        clicks(0).
        adv_amount_cmp(cpm8).
        cmp_amount(cpm8).
        cmp_amount_global(cpm8)
    },
    // #10
    {
      "Expression/Account/ADV", "Expression/Account/CMP",
      0, "Expression/Channel/CH9", "Expression/CC/CCG3",
      false, CDF_SDATE, today,
      ZERO_DIFF
    },
    // #10
    {
      "Expression/Account/ADV", "Expression/Account/CMP",
      0, "Expression/Channel/CH10", "Expression/CC/CCG3",
      false, CDF_SDATE, today,
      ExpectedDiff().
        imps(1).
        clicks(0).
        adv_amount_cmp(cpm10).
        cmp_amount(cpm10).
        cmp_amount_global(cpm10)
    },
    // #11
    {
      "Expression/Account/ADV", "Expression/Account/CMP",
      0, "Expression/Channel/CH11", "Expression/CC/CCG3",
      false, CDF_SDATE, today,
      ExpectedDiff().
        imps(1).
        clicks(0).
        adv_amount_cmp(cpm11).
        cmp_amount(cpm11).
        cmp_amount_global(cpm11)
    },
    // #12
    {
      "Expression/Account/ADV", "Expression/Account/CMP",
      0, "Expression/Channel/UNLINK", 0,
      false, CDF_SDATE, today,
      ZERO_DIFF
    }

  };

  initialize_stats(stats, diffs, EXPRESSION);

  // Requests

  const Request requests[] =
  {
    {
      // referer_kws
      "Expression/KWD/CH1,Expression/KWD/CH2,"
      "Expression/KWD/CH3,Expression/KWD/CH4,"
      "Expression/KWD/UNLINK",
      // tid
      "Expression/TAG/PUBLISHER1",
      // channels
      "Expression/Channel/CH1,Expression/Channel/CH2,"
      "Expression/Channel/CH3,Expression/Channel/CH4,"
      "Expression/Channel/UNLINK",
      // ccs
      "Expression/CC/CCG1"
    },
    {
      "Expression/KWD/CH5",
      "Expression/TAG/PUBLISHER2",
      "Expression/Channel/CH5",
      "Expression/CC/CCG2"
    },
    {
      "Expression/KWD/CH11",
      "Expression/TAG/PUBLISHER3",
      "Expression/Channel/CH11",
      "Expression/CC/CCG3"
    }
  };

  send_requests(today, requests);
  
  ADD_WAIT_CHECKER(
    "CMPStats check",
    AutoTest::stats_diff_checker(
      pq_conn_, diffs, stats));
}

void
CMPStatTest::set_up()
{ }

bool 
CMPStatTest::run()
{
  AUTOTEST_CASE(
    pub_exp_ch_scenario(),
    "PUB expression");

  AUTOTEST_CASE(
    base_scenario(),
    "Base  cases");

  AUTOTEST_CASE(
    adv_exp_ch_scenario(),
    "Advertiser expression");
    
  AUTOTEST_CASE(
    cmp_exp_ch_scenario(),
    "CMP expression");

  AUTOTEST_CASE(
    currency_scenario(),
    "Currency cases");
  
  AUTOTEST_CASE(
    expression_scenario(),
    "Expression cases");

  AUTOTEST_CASE(
    ta_scenario(),
    "Text advertising");

  return true;
}

void
CMPStatTest::tear_down()
{ }

