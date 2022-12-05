
#include "ColoUsers.hpp"
#include <climits>
#include <algorithm>
#include <functional>
#include <Generics/Uuid.hpp>

REFLECT_UNIT(ColoUsers) (
  "Statistics",
  AUTO_TEST_SLOW,
  AUTO_TEST_SERIALIZE_PRE);

namespace
{
  const int MINUTE = 60;
  const int HOUR = 60 * MINUTE;
  const int DAY = 24 * HOUR;
  const int YEAR = 365 * DAY;
  const AutoTest::Time ZERO_DATE = Generics::Time::ZERO;
  const AutoTest::Time UNDEF_DATE = ZERO_DATE - 1;
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::OptOutRequest OptOutRequest;
  typedef AutoTest::InventoryProfileEmptyChecker InventoryProfileChecker;

  enum TestRequestFlags
  {
    TRF_CHECK_CC = 1,
    TRF_CHECK_NO_TAG = 2,
    TRF_SAFE_TUID = 4,
    TRF_INVENTORY = 8
  };

  struct Expected
  {
    const char* colo;
    AutoTest::Time sdate;
    AutoTest::Time create_date;
    AutoTest::Time last_appearance_date;
    int unique_users;
    int network_unique_users;
    int profiling_unique_users;
    int unique_hids;
  };

  struct ColoUserExpected
  {
    const char* colo;
    AutoTest::Time sdate;
    AutoTest::Time last_appearance_date;
    int unique_users;
    int network_unique_users;
    int profiling_unique_users;
    int unique_hids;
  };

  const unsigned int MAX_LAST_APPEARANCE_DIFF = 32;

  int sum_change(
    const Generics::Time& date,
    const Generics::Time& last_appearance_date,
    int unique_counter)
  {
    // days(time) is (time / Generics::Time::ONE_DAY).tv_sec
    return (AutoTest::days(date - Generics::Time::ZERO) *
      (MAX_LAST_APPEARANCE_DIFF + 1) +
      (last_appearance_date == Generics::Time::ZERO ?
        MAX_LAST_APPEARANCE_DIFF :
       AutoTest::days(date - last_appearance_date))) * unique_counter;
  }
}

void
ColoUsers::set_up()
{
  add_descr_phrase("Setup");
}

void 
ColoUsers::tear_down()
{
  add_descr_phrase("Tear down");
}

bool 
ColoUsers::run()
{

  AUTOTEST_CASE(
    unique_users_stats_(),
    "Unique users stats");

  AUTOTEST_CASE(
    unique_hids_(),
    "Unique HIDs stats");

  AUTOTEST_CASE(
    create_and_last_appearance_dates_(),
    "Create and last appearance dates");

  // known bug ADSC-8333
  AUTOTEST_CASE(
    non_gmt_timezone_(),
    "Non-GMT timezone");

  AdClient assync_client(AdClient::create_undef_user(this));
  
  TemporaryAdClient assync_temporary(
    TemporaryAdClient::create_user(this));
  
  std::string assync_hid(AutoTest::generate_uid(false));
  
  AUTOTEST_CASE(
    basic_async_part_1_(
      assync_client,
      assync_temporary,
      assync_hid),
    "Basic asynchronous logging");

  // known bug ADSC-8351
  AUTOTEST_CASE(
    big_date_difference_(),
    "Asynchronous logging with big date difference");
  
  AUTOTEST_CASE(
    merge_on_adrequest_(),
    "Merging on adrequest");

  AUTOTEST_CASE(
    create_date_after_merge_(),
    "Create date after merging");

  AUTOTEST_CASE(
    invalid_merge_(),
    "Merging with non-existing temporary user");
  
  AUTOTEST_CASE(
    optout_(),
    "Non-opted-in users test");
  
  AUTOTEST_CASE(
    non_serialized_(),
    "Non serialized match/merge operations test");

  AUTOTEST_CASE(
    pub_inventory_(),
    "Publisher inventory mode test case");

  AUTOTEST_CASE(
    oo_service_(),
    "Logging on OO service");

  check();

  AUTOTEST_CASE(
    basic_async_part_2_(
      assync_client,
      assync_temporary,
      assync_hid),
    "Basic asynchronous logging");
  
  return true;
}

void ColoUsers::unique_users_stats_()
{
  ColoUserStats colo_user_stats;
  ColoUserDiffs colo_user_diffs;
  GlobalColoUserStats global_colo_user_stats;
  GlobalColoUserDiffs global_colo_user_diffs;
  CreatedUserStats created_user_stats;
  CreatedUserDiffs created_user_diffs;

  const Expected CASE_STATS[] =
  {
    // 1
    {
      "UNIQUE_USERS/COLO/NON_OPTOUT",
      base_time_,
      base_time_,
      ZERO_DATE,
      1,  0,  1, 0
    },
    // 2
    {
      "UNIQUE_USERS/COLO/NON_OPTOUT",
      base_time_ + DAY,
      base_time_,
      base_time_,
      1,  0,  0, 0
    },
    // 3
    {
      "UNIQUE_USERS/COLO/NON_OPTOUT",
      base_time_ + DAY,
      base_time_,
      ZERO_DATE,
      0,  1,  0, 0
    },
    // 4
    {
      "UNIQUE_USERS/COLO/NON_OPTOUT",
      base_time_ + 2*DAY,
      base_time_,
      base_time_ + DAY,
      1,  0,  0, 0
    },
    // 5
    {
      "UNIQUE_USERS/COLO/NON_OPTOUT",
      base_time_ + 2*DAY,
      base_time_,
      base_time_,
      0,  0,  1, 0
    },
    // 6
    {
      "UNIQUE_USERS/COLO/NON_OPTOUT",
      base_time_ + 3*DAY,
      base_time_,
      base_time_ + 2*DAY,
      1,  0,  1, 0
    },
    // 7
    {
      "UNIQUE_USERS/COLO/NON_OPTOUT",
      base_time_ + 3*DAY,
      base_time_,
      base_time_ + 1*DAY,
      0,  1,  0, 0
    },
    // 8
    {
      "UNIQUE_USERS/COLO/OPTIN_ONLY",
      base_time_ + 3*DAY,
      base_time_,
      ZERO_DATE,
      1,  1,  1, 0
    },
    // 9
    {
      "UNIQUE_USERS/COLO/ALL",
      base_time_,
      base_time_,
      ZERO_DATE,
      1,  0,  1, 0
    },
    // 10
    {
      "UNIQUE_USERS/COLO/ALL",
      base_time_ + DAY,
      base_time_ + DAY,
      ZERO_DATE,
      1,  1,  0, 0
    },
    // 11
    {
      "UNIQUE_USERS/COLO/ALL",
      base_time_ + 2*DAY,
      base_time_ + DAY,
      base_time_ + DAY,
      1,  0,  0, 0
    },
    // 12
    {
      "UNIQUE_USERS/COLO/ALL",
      base_time_ + 3*DAY,
      base_time_ + DAY,
      base_time_ + 2*DAY,
      1,  0,  0, 0
    }
  };

  add_stats_(
    colo_user_stats,
    colo_user_diffs,
    CASE_STATS);

  add_stats_(
    created_user_stats,
    created_user_diffs,
    CASE_STATS);
  
  add_stats_(
    global_colo_user_stats,
    global_colo_user_diffs,
    CASE_STATS);

  {
    AdClient client(AdClient::create_undef_user(this));
    TemporaryAdClient temporary1(TemporaryAdClient::create_user(this));
    TemporaryAdClient temporary2(TemporaryAdClient::create_user(this));
    TemporaryAdClient temporary3(TemporaryAdClient::create_user(this));
    
    const TestRequest REQUESTS[] =
    {
      { temporary1, -MINUTE, "UNIQUE_USERS/COLO/NON_OPTOUT", 0, 0, 0, 0, 0 },
      { temporary2, -MINUTE, "UNIQUE_USERS/COLO/NON_OPTOUT", 0, 0, 0, 0, 0 },
      { temporary3, -MINUTE, "UNIQUE_USERS/COLO/OPTIN_ONLY", 0, 0, 0, 0, 0 },
      { client, 0, "UNIQUE_USERS/COLO/NON_OPTOUT", 0, 0, 0, &temporary1, 0 },
      { client, DAY + HOUR, "UNIQUE_USERS/COLO/NON_OPTOUT", "Tags/DEFAULT", 0, 0, 0, 0 },
      { client, DAY + 2*HOUR, "UNIQUE_USERS/COLO/NON_OPTOUT", "Tags/DEFAULT", 0, 0, 0, 0 },
      { client, 2*DAY, "UNIQUE_USERS/COLO/NON_OPTOUT", 0, 0, 0, 0, 0 },
      { client, 3*DAY + HOUR, "UNIQUE_USERS/COLO/NON_OPTOUT", "Tags/DEFAULT", 0, 0, &temporary2, 0 },
      { client, 3*DAY + HOUR, "UNIQUE_USERS/COLO/OPTIN_ONLY", 0, 0, 0, 0, 0 },
      { client, 3*DAY + HOUR, "UNIQUE_USERS/COLO/OPTIN_ONLY", "Tags/DEFAULT", 0, 0, 0, 0 },
      { client, 3*DAY + 2*HOUR, "UNIQUE_USERS/COLO/OPTIN_ONLY", 0, 0, 0, &temporary3, 0 }
    };

    process_requests_(
      AutoTest::Time(
        base_time_.get_gm_time().get_date()),
      REQUESTS);
  }
  {
    // to avoid platforms match
    AdClient client(
      AdClient::create_nonoptin_user(
        this, AutoTest::UF_CENTRAL_FRONTEND));
    client.set_probe_uid();
    client.process_request(
      NSLookupRequest(false).
        colo(fetch_int("UNIQUE_USERS/COLO/ALL")).
        setuid(1).
        debug_time(base_time_));
  }

  {
    AdClient client(AdClient::create_undef_user(this));
    const TestRequest REQUESTS[] =
    {
      { client, DAY, "UNIQUE_USERS/COLO/ALL", "Tags/AD", "Campaign/KEYWORD", 0, 0, TRF_CHECK_CC },
      { client, 2*DAY, "UNIQUE_USERS/COLO/ALL", "Tags/UNEXIST", "Campaign/KEYWORD", 0, 0, TRF_CHECK_NO_TAG },
      { client, 3*DAY, "UNIQUE_USERS/COLO/ALL", "Tags/DELETED", "Campaign/KEYWORD", 0, 0, TRF_CHECK_NO_TAG }
    };

    process_requests_(
      base_time_,
      REQUESTS);
  }

  ADD_WAIT_CHECKER(
    "ColoUserStats check",
    AutoTest::stats_diff_checker(
      pq_conn_,
      colo_user_diffs,
      colo_user_stats));

  ADD_WAIT_CHECKER(
    "CreatedUserStats check",
    AutoTest::stats_diff_checker(
      pq_conn_,
      created_user_diffs,
      created_user_stats));

  ADD_WAIT_CHECKER(
    "GlobalColoUserStats check",
    AutoTest::stats_diff_checker(
      pq_conn_,
      global_colo_user_diffs,
      global_colo_user_stats));
}

void ColoUsers::unique_hids_()
{
  ColoUserStats colo_user_stats;
  ColoUserDiffs colo_user_diffs;
  GlobalColoUserStats global_colo_user_stats;
  GlobalColoUserDiffs global_colo_user_diffs;
  CreatedUserStats created_user_stats;
  CreatedUserDiffs created_user_diffs;

  {
    // ColoUserStats
    const ColoUserExpected CASE_STATS[] =
    {
      // 1
      {
        "UNIQUE_HIDS/COLO/NONE",
        base_time_ - DAY,
        ZERO_DATE,
        1,  1,  1, 0
      },
      // 2
      {
        "UNIQUE_HIDS/COLO/NONE",
        base_time_,
        base_time_ - DAY,
        1,  0,  1, 0
      },
      // 3
      {
        "UNIQUE_HIDS/COLO/NONE",
        base_time_,
        ZERO_DATE,
        0,  0,  0, 1
      },
      // 4
      {
        "UNIQUE_HIDS/COLO/NONE",
        base_time_ + DAY,
        base_time_,
        1,  0,  0, 1
      },
      // 5
      {
        "UNIQUE_HIDS/COLO/NONE",
        base_time_ + DAY,
        base_time_ - DAY,
        0,  1,  0, 0
      },
      // 6
      {
        "UNIQUE_HIDS/COLO/NONE",
        base_time_ + 2*DAY,
        base_time_ + DAY,
        1,  1,  0, 1
      },
      // 7
      {
        "UNIQUE_HIDS/COLO/NONE",
        base_time_ + 2*DAY,
        base_time_,
        0,  0,  1, 0
      },
      // 8
      {
        "UNIQUE_HIDS/COLO/NONE",
        base_time_ + 2*DAY,
        ZERO_DATE,
        0,  0,  0, 1
      },
      // 9
      {
        "UNIQUE_HIDS/COLO/NONE",
        base_time_ + 3*DAY,
        base_time_ + 2*DAY,
        1,  1,  1, 1
      },
      // 10
      {
        "UNIQUE_HIDS/COLO/NONE",
        base_time_ + 3*DAY,
        ZERO_DATE,
        1,  1,  1, 0
      },
      // 11
      {
        "UNIQUE_HIDS/COLO/ALL",
        base_time_ + 3*DAY,
        ZERO_DATE,
        1,  1,  1, 1
      }
    };

    add_stats_(
      colo_user_stats,
      colo_user_diffs,
      CASE_STATS);
  }

  {
    // CreatedUserStats & GlobalColoUserStats
    const Expected CASE_STATS[] =
    {
      // 1
      {
        "UNIQUE_HIDS/COLO/NONE",
        base_time_ - DAY,
        base_time_ - DAY,
        ZERO_DATE,
        1,  1,  1, 0
      },
      // 2
      {
        "UNIQUE_HIDS/COLO/NONE",
        base_time_,
        base_time_ - DAY,
        base_time_ - DAY,
        1,  0,  1, 0
      },
      // 3
      {
        "UNIQUE_HIDS/COLO/NONE",
        base_time_,
        base_time_,
        ZERO_DATE,
        0,  0,  0, 1
      },
      // 4
      {
        "UNIQUE_HIDS/COLO/NONE",
        base_time_ + DAY,
        base_time_ - DAY,
        base_time_,
        1,  0,  0, 0
      },
      // 5
      {
        "UNIQUE_HIDS/COLO/NONE",
        base_time_ + DAY,
        base_time_ - DAY,
        base_time_ - DAY,
        0,  1,  0, 0
      },
      // 6
      {
        "UNIQUE_HIDS/COLO/NONE",
        base_time_ + DAY,
        base_time_,
        base_time_,
        0,  0,  0, 1
      },
      // 7
      {
        "UNIQUE_HIDS/COLO/NONE",
        base_time_ + 2*DAY,
        base_time_ - DAY,
        base_time_ + DAY,
        1,  1,  0, 0
      },
      // 8
      {
        "UNIQUE_HIDS/COLO/NONE",
        base_time_ + 2*DAY,
        base_time_ - DAY,
        base_time_,
        0,  0,  1, 0
      },
      // 9
      {
        "UNIQUE_HIDS/COLO/NONE",
        base_time_ + 2*DAY,
        base_time_,
        base_time_ + DAY,
        0,  0,  0, 1
      },
      // 10
      {
        "UNIQUE_HIDS/COLO/NONE",
        base_time_ + 2*DAY,
        base_time_ + 2*DAY,
        ZERO_DATE,
        0,  0,  0, 1
      },
      // 11
      {
        "UNIQUE_HIDS/COLO/NONE",
        base_time_ + 3*DAY,
        base_time_ - DAY,
        base_time_ + 2*DAY,
        1,  1,  1, 0
      },
      // 12
      {
        "UNIQUE_HIDS/COLO/NONE",
        base_time_ + 3*DAY,
        base_time_,
        base_time_ + 2*DAY,
        0,  0,  0, 1
      },
      // 13
      {
        "UNIQUE_HIDS/COLO/NONE",
        base_time_ + 3*DAY,
        base_time_ + 3*DAY,
        ZERO_DATE,
        1,  1,  1, 0
      },
      // 14
      {
        "UNIQUE_HIDS/COLO/ALL",
        base_time_ + 3*DAY,
        base_time_ - DAY,
        ZERO_DATE,
        1,  1,  1, 0
      },
      // 15
      {
        "UNIQUE_HIDS/COLO/ALL",
        base_time_ + 3*DAY,
        base_time_,
        ZERO_DATE,
        0,  0,  0, 1
      }
    };

    add_stats_(
      created_user_stats,
      created_user_diffs,
      CASE_STATS);
    
    add_stats_(
      global_colo_user_stats,
      global_colo_user_diffs,
      CASE_STATS);
  }

  AdClient client1(AdClient::create_undef_user(this));
  AdClient client2(AdClient::create_undef_user(this));
  TemporaryAdClient temporary(TemporaryAdClient::create_user(this));
  std::string hid1(AutoTest::generate_uid(false));
  std::string hid2(AutoTest::generate_uid(false));

  const TestRequest REQUESTS[] =
  {
    { client1, -2*MINUTE, "UNIQUE_HIDS/COLO/NONE", "Tags/DEFAULT", 0, 0, &temporary, TRF_SAFE_TUID },
    { temporary, -MINUTE, "UNIQUE_HIDS/COLO/NONE", 0, 0, hid1.c_str(), 0, TRF_SAFE_TUID },
    { client1, 0, "UNIQUE_HIDS/COLO/NONE", 0, 0, hid1.c_str(), &temporary, 0 },
    { client1, DAY, "UNIQUE_HIDS/COLO/NONE", "Tags/DEFAULT", 0, hid1.c_str(), 0, 0 },
    { client1, 2*DAY, "UNIQUE_HIDS/COLO/NONE", "Tags/DEFAULT", 0, hid1.c_str(), &temporary, 0 },
    { client1, 2*DAY, "UNIQUE_HIDS/COLO/NONE", "Tags/DEFAULT", 0, hid2.c_str(), &temporary, 0 },
    { client1, 3*DAY + HOUR, "UNIQUE_HIDS/COLO/NONE", "Tags/DEFAULT", 0, hid1.c_str(), &temporary, 0 },
    { client2, 3*DAY + 2*HOUR, "UNIQUE_HIDS/COLO/NONE", "Tags/DEFAULT", 0, hid1.c_str(), &temporary, 0 },
    { client1, 3*DAY + 3*HOUR, "UNIQUE_HIDS/COLO/ALL", "Tags/DEFAULT", 0, hid1.c_str(), &temporary, 0 }
  };

  process_requests_(
    AutoTest::Time(
      base_time_.get_gm_time().get_date()),
    REQUESTS);

  ADD_WAIT_CHECKER(
    "ColoUserStats check",
    AutoTest::stats_diff_checker(
      pq_conn_,
      colo_user_diffs,
      colo_user_stats));

  ADD_WAIT_CHECKER(
    "CreatedUserStats check",
    AutoTest::stats_diff_checker(
      pq_conn_,
      created_user_diffs,
      created_user_stats));

  ADD_WAIT_CHECKER(
    "GlobalColoUserStats check",
    AutoTest::stats_diff_checker(
      pq_conn_,
      global_colo_user_diffs,
      global_colo_user_stats));
}

void
ColoUsers::create_and_last_appearance_dates_()
{
  ColoUserStats colo_user_stats;
  ColoUserDiffs colo_user_diffs;
  GlobalColoUserStats global_colo_user_stats;
  GlobalColoUserDiffs global_colo_user_diffs;
  CreatedUserStats created_user_stats;
  CreatedUserDiffs created_user_diffs;

  {
    // ColoUserExpected
    const ColoUserExpected CASE_STATS[] =
    {
      // 1
      {
        "CREATE/COLO/ALL",
        base_time_ - 62*DAY,
        ZERO_DATE,
        1,  1,  1, 1
      },
      // 2
      {
        "CREATE/COLO/ALL",
        base_time_ - 30*DAY,
        base_time_ - 61*DAY,
        1,  1,  1, 1
      },
      // 3
      {
        "CREATE/COLO/ALL",
        base_time_,
        base_time_ - 30*DAY,
        2,  2,  2, 2
      },
      
      // 4
      {
        "CREATE/COLO/ALL",
        base_time_ - 31*DAY,
        ZERO_DATE,
        1,  1,  1, 1
      },
      // 5
      {
        "CREATE/COLO/ALL",
        base_time_,
        base_time_ - 31*DAY,
        1,  1,  1, 1
      },
      // 6
      {
        "CREATE/COLO/ALL",
        base_time_ - 30*DAY,
        ZERO_DATE,
        1,  1,  1, 1
      }
    };

    add_stats_(
      colo_user_stats,
      colo_user_diffs,
      CASE_STATS);
  };

  {
    // CreatedUserStats & GlobalColoUserStats
    const Expected CASE_STATS[] =
    {
      // 1
      {
        "CREATE/COLO/ALL",
        base_time_ - 62*DAY,
        base_time_ - 62*DAY,
        ZERO_DATE,
        1,  1,  1, 1
      },
      // 2
      {
        "CREATE/COLO/ALL",
        base_time_ - 30*DAY,
        base_time_ - 62*DAY,
        base_time_ - 61*DAY,
        1,  1,  1, 1
      },
      // 3
      {
        "CREATE/COLO/ALL",
        base_time_,
        base_time_ - 62*DAY,
        base_time_ - 30*DAY,
        1,  1,  1, 1
      },
      
      // 4
      {
        "CREATE/COLO/ALL",
        base_time_ - 31*DAY,
        base_time_ - 31*DAY,
        ZERO_DATE,
        1,  1,  1, 1
      },
      // 5
      {
        "CREATE/COLO/ALL",
        base_time_,
        base_time_ - 31*DAY,
        base_time_ - 31*DAY,
        1,  1,  1, 1
      },
      // 6
      {
        "CREATE/COLO/ALL",
        base_time_ - 30*DAY,
        base_time_ - 30*DAY,
        ZERO_DATE,
        1,  1,  1, 1
      },
      // 7
      {
        "CREATE/COLO/ALL",
        base_time_,
        base_time_ - 30*DAY,
        base_time_ - 30*DAY,
        1,  1,  1, 1
      }
    };

    add_stats_(
      created_user_stats,
      created_user_diffs,
      CASE_STATS);
    
    add_stats_(
      global_colo_user_stats,
      global_colo_user_diffs,
      CASE_STATS);
  };

  Clients clients(
    3,
    AdClient::create_nonoptin_user(this));
  
  std::generate_n(
    clients.begin(),
    3,
    std::bind(
      std::bind1st(
        std::ptr_fun(
          &AdClient::create_undef_user), this), 0));

  TemporaryAdClient temporary(
    TemporaryAdClient::create_user(this));

  Hids hids(3);
  std::generate_n(
    hids.begin(),
    3,
    std::bind(
      std::ptr_fun(
        &AutoTest::generate_uid), false));

  const TestRequest REQUESTS[] =
  {
    { clients[0], -62*DAY, "CREATE/COLO/ALL", "Tags/DEFAULT", 0, hids[0].c_str(), &temporary, 0 },
    { clients[0], -30*DAY, "CREATE/COLO/ALL", "Tags/DEFAULT", 0, hids[0].c_str(), &temporary, 0 },
    { clients[0], 0, "CREATE/COLO/ALL", "Tags/DEFAULT", 0, hids[0].c_str(), &temporary, 0 },
    { clients[1], -31*DAY, "CREATE/COLO/ALL", "Tags/DEFAULT", 0, hids[1].c_str(), &temporary, 0 },
    { clients[1], 0, "CREATE/COLO/ALL", "Tags/DEFAULT", 0, hids[1].c_str(), &temporary, 0 },
    { clients[2], -30*DAY, "CREATE/COLO/ALL", "Tags/DEFAULT", 0, hids[2].c_str(), &temporary, 0 },
    { clients[2], 0, "CREATE/COLO/ALL", "Tags/DEFAULT", 0, hids[2].c_str(), &temporary, 0 }
  };

  process_requests_(
    base_time_,
    REQUESTS);

  ADD_WAIT_CHECKER(
    "ColoUserStats check. Part#1",
    AutoTest::stats_diff_checker(
      pq_conn_,
      colo_user_diffs,
      colo_user_stats));

  ADD_WAIT_CHECKER(
    "CreatedUserStats check. Part#1",
    AutoTest::stats_diff_checker(
      pq_conn_,
      created_user_diffs,
      created_user_stats));

  ADD_WAIT_CHECKER(
    "GlobalColoUserStats check. Part#1",
    AutoTest::stats_diff_checker(
      pq_conn_,
      global_colo_user_diffs,
      global_colo_user_stats));
}

void ColoUsers::non_gmt_timezone_() // see also ADSC-5331
{
  ColoUserStats colo_user_stats;
  ColoUserDiffs colo_user_diffs;
  GlobalColoUserStats global_colo_user_stats;
  GlobalColoUserDiffs global_colo_user_diffs;
  CreatedUserStats created_user_stats;
  CreatedUserDiffs created_user_diffs;

  {
    // ColoUserStats & CreatedUserStats
    const Expected CASE_STATS[] =
    {
      // 1
      {
        
        "NON_GMT/COLO/ALL",
        base_time_ - DAY,
        base_time_ - DAY,
        ZERO_DATE,
        1,  1,  1, 1
      },
      // 1
      {
        
        "NON_GMT/COLO/ALL",
        base_time_,
        base_time_ - DAY,
        base_time_ - DAY,
        1,  0,  1, 1
      },
    };

    add_stats_(
      colo_user_stats,
      colo_user_diffs,
      CASE_STATS);

    add_stats_(
      created_user_stats,
      created_user_diffs,
      CASE_STATS);
  }

  {
    // GlobalColoUserStats
    const Expected CASE_STATS[] =
    {
      // 1
      {
        "NON_GMT/COLO/ALL",
        base_time_,
        base_time_,
        ZERO_DATE,
        1,  1,  1, 1
      }
    };

    add_stats_(
      global_colo_user_stats,
      global_colo_user_diffs,
      CASE_STATS);
  }

  AdClient client(AdClient::create_undef_user(this));
  TemporaryAdClient temporary(TemporaryAdClient::create_user(this));
  std::string hid(AutoTest::generate_uid(false));
  
  const TestRequest REQUESTS[] =
  {
    { client, 6*HOUR, "NON_GMT/COLO/ALL", "Tags/AD", "Campaign/KEYWORD", hid.c_str(), &temporary, TRF_CHECK_CC },
    { client, 9*HOUR, "NON_GMT/COLO/ALL", 0, 0, hid.c_str(), 0, 0 }
  };

  process_requests_(
    AutoTest::Time(
      base_time_.get_gm_time().get_date()),
    REQUESTS);

  ADD_WAIT_CHECKER(
    "ColoUserStats check",
    AutoTest::stats_diff_checker(
      pq_conn_,
      colo_user_diffs,
      colo_user_stats));

  ADD_WAIT_CHECKER(
    "CreatedUserStats check",
    AutoTest::stats_diff_checker(
      pq_conn_,
      created_user_diffs,
      created_user_stats));

  ADD_WAIT_CHECKER(
    "GlobalColoUserStats check",
    AutoTest::stats_diff_checker(
      pq_conn_,
      global_colo_user_diffs,
      global_colo_user_stats));
}

void
ColoUsers::basic_async_part_1_(
  AdClient& client,
  TemporaryAdClient& temporary,
  const std::string& hid)
{
  ColoUserStats colo_user_stats;
  ColoUserDiffs colo_user_diffs;
  GlobalColoUserStats global_colo_user_stats;
  GlobalColoUserDiffs global_colo_user_diffs;
  CreatedUserStats created_user_stats;
  CreatedUserDiffs created_user_diffs;

  const Expected CASE_STATS[] =
  {
    // 1
    {
      "BASE_ASSYNC/COLO/ALL",
      base_time_,
      base_time_,
      ZERO_DATE,
      1,  1,  1, 1
    }
  };
  
  add_stats_(
    colo_user_stats,
      colo_user_diffs,
      CASE_STATS);

  add_stats_(
    created_user_stats,
    created_user_diffs,
    CASE_STATS);

  add_stats_(
    global_colo_user_stats,
    global_colo_user_diffs,
    CASE_STATS);

  const TestRequest REQUESTS[] =
  {
    { client, HOUR, "BASE_ASSYNC/COLO/ALL", "Tags/DEFAULT", 0, hid.c_str(), &temporary, 0 }
  };

  process_requests_(
    AutoTest::Time(
      base_time_.get_gm_time().get_date()),
    REQUESTS);

  ADD_WAIT_CHECKER(
    "ColoUserStats check",
    AutoTest::stats_diff_checker(
      pq_conn_,
      colo_user_diffs,
      colo_user_stats));

  ADD_WAIT_CHECKER(
    "CreatedUserStats check",
    AutoTest::stats_diff_checker(
      pq_conn_,
      created_user_diffs,
      created_user_stats));

  ADD_WAIT_CHECKER(
    "GlobalColoUserStats check",
    AutoTest::stats_diff_checker(
      pq_conn_,
      global_colo_user_diffs,
      global_colo_user_stats));
}

void
ColoUsers::basic_async_part_2_(
  AdClient& client,
  TemporaryAdClient& temporary,
  const std::string& hid)
{
  ColoUserStats colo_user_stats;
  ColoUserDiffs colo_user_diffs;
  GlobalColoUserStats global_colo_user_stats;
  GlobalColoUserDiffs global_colo_user_diffs;
  CreatedUserStats created_user_stats;
  CreatedUserDiffs created_user_diffs;

  const Expected CASE_STATS[] =
  {
    // 1
    {
      "BASE_ASSYNC/COLO/ALL",
      base_time_,
      base_time_,
      ZERO_DATE,
      -1,  -1,  -1, -1
    },
    // 2
    {
      "BASE_ASSYNC/COLO/ALL",
      base_time_ - DAY,
      base_time_ - DAY,
      ZERO_DATE,
      1,  1,  1, 1
    },
    // 3
    {
      "BASE_ASSYNC/COLO/ALL",
      base_time_,
      base_time_ - DAY,
      base_time_ - DAY,
      1,  1,  1, 1
    }
  };
  
  add_stats_(
    colo_user_stats,
      colo_user_diffs,
      CASE_STATS);

  add_stats_(
    created_user_stats,
    created_user_diffs,
    CASE_STATS);

  add_stats_(
    global_colo_user_stats,
    global_colo_user_diffs,
    CASE_STATS);

  
  const TestRequest REQUESTS[] =
  {
    { client, -30, "BASE_ASSYNC/COLO/ALL", "Tags/DEFAULT", 0, hid.c_str(), &temporary, 0 },
    { client, 30, "BASE_ASSYNC/COLO/ALL",  "Tags/DEFAULT", 0, hid.c_str(), &temporary, 0 }
  };

  process_requests_(
    AutoTest::Time(
      base_time_.get_gm_time().get_date()),
    REQUESTS);

  ADD_WAIT_CHECKER(
    "ColoUserStats check",
    AutoTest::stats_diff_checker(
      pq_conn_,
      colo_user_diffs,
      colo_user_stats));

  ADD_WAIT_CHECKER(
    "CreatedUserStats check",
    AutoTest::stats_diff_checker(
      pq_conn_,
      created_user_diffs,
      created_user_stats));

  ADD_WAIT_CHECKER(
    "GlobalColoUserStats check",
    AutoTest::stats_diff_checker(
      pq_conn_,
      global_colo_user_diffs,
      global_colo_user_stats));
}


void ColoUsers::big_date_difference_()
{
  ColoUserStats colo_user_stats;
  ColoUserDiffs colo_user_diffs;
  GlobalColoUserStats global_colo_user_stats;
  GlobalColoUserDiffs global_colo_user_diffs;
  CreatedUserStats created_user_stats;
  CreatedUserDiffs created_user_diffs;

  const Expected CASE_STATS[] =
  {
    // 1
    {
      "BIG_DATE/COLO/ALL",
      base_time_ - 60*DAY,
      base_time_ - 60*DAY,
      ZERO_DATE,
      1, 0, 1, 1
    },
    // 2
    {
      "BIG_DATE/COLO/ALL",
      base_time_ - 40*DAY,
      base_time_ - 60*DAY,
      ZERO_DATE,
      0, 1, 0, 0
    },
    // 3
    {
      "BIG_DATE/COLO/ALL",
      base_time_ - 40*DAY,
      base_time_ - 60*DAY,
      base_time_ - 60*DAY,
      1, 0, 0, 1
    },
    // 4
    {
      "BIG_DATE/COLO/ALL",
      base_time_ - 9*DAY,
      base_time_ - 60*DAY,
      base_time_ - 40*DAY,
      1, 1, 0, 1
    },
    // 5
    {
      "BIG_DATE/COLO/ALL",
      base_time_,
      base_time_ - 60*DAY,
      base_time_ - 31*DAY,
      0, 0, 1, 0
    },
    // 6
    {
      "BIG_DATE/COLO/ALL",
      base_time_,
      base_time_ - 60*DAY,
      base_time_ - 9*DAY,
      1, 0, 0, 1
    }
  };

  add_stats_(
    colo_user_stats,
      colo_user_diffs,
      CASE_STATS);

  add_stats_(
    created_user_stats,
    created_user_diffs,
    CASE_STATS);

  add_stats_(
    global_colo_user_stats,
    global_colo_user_diffs,
    CASE_STATS);

  
  AdClient client(AdClient::create_undef_user(this));
  std::string hid(AutoTest::generate_uid(false));

  {
    const TestRequest REQUESTS[] =
    {
      { client, -60*DAY, "BIG_DATE/COLO/ALL", 0, 0, hid.c_str(), 0, 0 },
      { client, -40*DAY, "BIG_DATE/COLO/ALL", "Tags/DEFAULT", 0, hid.c_str(), 0, 0 },
      { client, 0, "BIG_DATE/COLO/ALL", 0, 0, hid.c_str(), 0, 0 }
    };

    process_requests_(base_time_, REQUESTS);
  }

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::InventoryProfileChecker(
        this,
        "\\" + client.debug_info.uid.value(),
        AutoTest::InventoryProfileChecker::Expected().
          last_request_time(
            base_time_.get_gm_time().
              format("%Y-%m-%d 00:00:00")))).check(),
    "Check user inventory profile");

  {
    const TestRequest REQUESTS[] =
    {
      { client, -9*DAY, "BIG_DATE/COLO/ALL", "Tags/DEFAULT", 0, hid.c_str(), 0, 0 }
    };

    process_requests_(base_time_, REQUESTS);
  }

  ADD_WAIT_CHECKER(
    "ColoUserStats check",
    AutoTest::stats_diff_checker(
      pq_conn_,
      colo_user_diffs,
      colo_user_stats));

  ADD_WAIT_CHECKER(
    "CreatedUserStats check",
    AutoTest::stats_diff_checker(
      pq_conn_,
      created_user_diffs,
      created_user_stats));

  ADD_WAIT_CHECKER(
    "GlobalColoUserStats check",
    AutoTest::stats_diff_checker(
      pq_conn_,
      global_colo_user_diffs,
      global_colo_user_stats));
}

void
ColoUsers::merge_on_adrequest_()
{
  ColoUserStats colo_user_stats;
  ColoUserDiffs colo_user_diffs;
  GlobalColoUserStats global_colo_user_stats;
  GlobalColoUserDiffs global_colo_user_diffs;
  CreatedUserStats created_user_stats;
  CreatedUserDiffs created_user_diffs;

  const Expected CASE_STATS[] =
  {
    // 1
    {
      "AD_MERGE/COLO/ALL",
      base_time_,
      base_time_,
      ZERO_DATE,
      1, 1, 1, 1
    },
    // 2
    {
      "AD_MERGE/COLO/ALL",
      base_time_ + DAY,
      base_time_,
      base_time_,
      1, 1, 1, 1
    }
  };

  add_stats_(
    colo_user_stats,
      colo_user_diffs,
      CASE_STATS);

  add_stats_(
    created_user_stats,
    created_user_diffs,
    CASE_STATS);

  add_stats_(
    global_colo_user_stats,
    global_colo_user_diffs,
    CASE_STATS);

  AdClient client(AdClient::create_undef_user(this));
  TemporaryAdClient temporary1(TemporaryAdClient::create_user(this));
  TemporaryAdClient temporary2(TemporaryAdClient::create_user(this));
  std::string hid(AutoTest::generate_uid(false));
  
  const TestRequest REQUESTS[] =
  {
    { temporary1, 0, "AD_MERGE/COLO/ALL", 0, "Keyword", hid.c_str(), 0, 0 },
    { temporary2, DAY, "AD_MERGE/COLO/ALL", 0, 0, hid.c_str(), 0, 0 },
    { client, 0, "AD_MERGE/COLO/ALL", "Tags/DEFAULT", 0, hid.c_str(), &temporary1, 0 },
    { client, 0, "AD_MERGE/COLO/ALL", "Tags/DEFAULT", 0, hid.c_str(), 0, 0 },
    { client, DAY, "AD_MERGE/COLO/ALL", "Tags/DEFAULT", 0, hid.c_str(), &temporary2, 0 }
  };

  process_requests_(base_time_, REQUESTS);

  ADD_WAIT_CHECKER(
    "ColoUserStats check",
    AutoTest::stats_diff_checker(
      pq_conn_,
      colo_user_diffs,
      colo_user_stats));

  ADD_WAIT_CHECKER(
    "CreatedUserStats check",
    AutoTest::stats_diff_checker(
      pq_conn_,
      created_user_diffs,
      created_user_stats));

  ADD_WAIT_CHECKER(
    "GlobalColoUserStats check",
    AutoTest::stats_diff_checker(
      pq_conn_,
      global_colo_user_diffs,
      global_colo_user_stats));
}

void
ColoUsers::create_date_after_merge_()
{
  ColoUserStats colo_user_stats;
  ColoUserDiffs colo_user_diffs;
  GlobalColoUserStats global_colo_user_stats;
  GlobalColoUserDiffs global_colo_user_diffs;
  CreatedUserStats created_user_stats;
  CreatedUserDiffs created_user_diffs;

  const Expected CASE_STATS[] =
  {
    // 1
    {
      "CREATE_DATE/COLO/ALL1",
      base_time_,
      base_time_,
      ZERO_DATE,
      1, 1, 1, 1
    },
    // 2
    {
      "CREATE_DATE/COLO/ALL2",
      base_time_,
      base_time_,
      ZERO_DATE,
      1, 1, 1, 1
    }
  };

  add_stats_(
    colo_user_stats,
      colo_user_diffs,
      CASE_STATS);

  add_stats_(
    created_user_stats,
    created_user_diffs,
    CASE_STATS);

  add_stats_(
    global_colo_user_stats,
    global_colo_user_diffs,
    CASE_STATS);

  // Empty persistent profile on merging
  {
    AdClient client(AdClient::create_undef_user(this));
    TemporaryAdClient temporary(TemporaryAdClient::create_user(this));
    std::string hid(AutoTest::generate_uid(false));
  
    const TestRequest REQUESTS[] =
    {
      { temporary, -DAY, "CREATE_DATE/COLO/ALL1", 0, 0, hid.c_str(), 0, 0 },
      { client, 0, "CREATE_DATE/COLO/ALL1", "Tags/DEFAULT", 0, hid.c_str(), &temporary, 0 }
    };
    
    process_requests_(base_time_, REQUESTS);
  }

  // Non-empty persistent profile on merging
  {
    AdClient client(AdClient::create_undef_user(this));
    TemporaryAdClient temporary(TemporaryAdClient::create_user(this));
    std::string hid(AutoTest::generate_uid(false));
  
    const TestRequest REQUESTS[] =
    {
      { temporary, -30, "CREATE_DATE/COLO/ALL2", 0, 0, hid.c_str(), 0, 0 },
      { client, 30, "CREATE_DATE/COLO/ALL2", "Tags/DEFAULT", 0, hid.c_str(), 0, 0 },
      { client, 60, "CREATE_DATE/COLO/ALL2", "Tags/DEFAULT", 0, hid.c_str(), &temporary, 0 }
    };
    
    process_requests_(
      AutoTest::Time(
        base_time_.get_gm_time().get_date()),
      REQUESTS);
  }

  ADD_WAIT_CHECKER(
    "ColoUserStats check",
    AutoTest::stats_diff_checker(
      pq_conn_,
      colo_user_diffs,
      colo_user_stats));

  ADD_WAIT_CHECKER(
    "CreatedUserStats check",
    AutoTest::stats_diff_checker(
      pq_conn_,
      created_user_diffs,
      created_user_stats));

  ADD_WAIT_CHECKER(
    "GlobalColoUserStats check",
    AutoTest::stats_diff_checker(
      pq_conn_,
      global_colo_user_diffs,
      global_colo_user_stats));
}

void
ColoUsers::invalid_merge_()
{
  ColoUserStats colo_user_stats;
  ColoUserDiffs colo_user_diffs;
  GlobalColoUserStats global_colo_user_stats;
  GlobalColoUserDiffs global_colo_user_diffs;
  CreatedUserStats created_user_stats;
  CreatedUserDiffs created_user_diffs;

  const Expected CASE_STATS[] =
  {
    // 1
    {
      "INVALID_MERGE/COLO/ALL",
      base_time_,
      base_time_,
      ZERO_DATE,
      1, 1, 1, 1
    },
    // 2
    {
      "INVALID_MERGE/COLO/ALL",
      base_time_ + DAY,
      base_time_,
      base_time_,
      1, 1, 1, 1
    }
  };

  add_stats_(
    colo_user_stats,
      colo_user_diffs,
      CASE_STATS);

  add_stats_(
    created_user_stats,
    created_user_diffs,
    CASE_STATS);

  add_stats_(
    global_colo_user_stats,
    global_colo_user_diffs,
    CASE_STATS);

  AdClient client(AdClient::create_undef_user(this));
  TemporaryAdClient temporary(TemporaryAdClient::create_user(this));
  std::string hid(AutoTest::generate_uid(false));
  
  const TestRequest REQUESTS[] =
  {
    { temporary, 0, "INVALID_MERGE/COLO/ALL", 0, 0, hid.c_str(), 0, 0 },
    { client, 0, "INVALID_MERGE/COLO/ALL", "Tags/DEFAULT", 0, hid.c_str(), 0, 0 },
    { client, 0, "INVALID_MERGE/COLO/ALL", "Tags/DEFAULT", 0, hid.c_str(), &temporary, 0 },
    { client, 0, "INVALID_MERGE/COLO/ALL", "Tags/DEFAULT", 0, hid.c_str(), &temporary, 0 },
    { client, DAY, "INVALID_MERGE/COLO/ALL", "Tags/DEFAULT", 0, hid.c_str(), &temporary, 0 }
  };
    
  process_requests_(
    AutoTest::Time(
      base_time_.get_gm_time().get_date()),
    REQUESTS);

  ADD_WAIT_CHECKER(
    "ColoUserStats check",
    AutoTest::stats_diff_checker(
      pq_conn_,
      colo_user_diffs,
      colo_user_stats));

  ADD_WAIT_CHECKER(
    "CreatedUserStats check",
    AutoTest::stats_diff_checker(
      pq_conn_,
      created_user_diffs,
      created_user_stats));

  ADD_WAIT_CHECKER(
    "GlobalColoUserStats check",
    AutoTest::stats_diff_checker(
      pq_conn_,
      global_colo_user_diffs,
      global_colo_user_stats));
}

void ColoUsers::optout_()
{
  ColoUserStats colo_user_stats;
  ColoUserDiffs colo_user_diffs;
  GlobalColoUserStats global_colo_user_stats;
  GlobalColoUserDiffs global_colo_user_diffs;
  CreatedUserStats created_user_stats;
  CreatedUserDiffs created_user_diffs;
  
  const Expected CASE_STATS[] =
  {
    // 1
    {
      "OPTOUT/COLO/ALL",
      base_time_ + DAY,
      base_time_ + DAY,
      ZERO_DATE,
      1, 1, 1, 1
    },
    // 2
    {
      "OPTOUT/COLO/ALL",
      base_time_,
      base_time_,
      ZERO_DATE,
      0, 0, 0, 0
    } 
  };
  
  add_stats_(
    colo_user_stats,
      colo_user_diffs,
      CASE_STATS);

  add_stats_(
    created_user_stats,
    created_user_diffs,
    CASE_STATS);

  add_stats_(
    global_colo_user_stats,
    global_colo_user_diffs,
    CASE_STATS);

  AdClient optin(AdClient::create_undef_user(this));
  AdClient optout(AdClient::create_optout_user(this));
  AdClient undef(AdClient::create_nonoptin_user(this));
  AdClient invalid(AdClient::create_undef_user(this));
  AdClient probe(AdClient::create_nonoptin_user(this));
  probe.set_probe_uid();
  TemporaryAdClient temporary(TemporaryAdClient::create_user(this));
  std::string hid(AutoTest::generate_uid(false));

  std::string invalid_uid(invalid.get_uid());
  char& last_uid_symbol = invalid_uid[invalid_uid.length() - 1];
  last_uid_symbol = last_uid_symbol == 'A'? 'B': 'A';

  invalid.set_uid(invalid_uid);

  const TestRequest REQUESTS[] =
  {
    { undef, 0, "OPTOUT/COLO/ALL", "Tags/DEFAULT", 0, hid.c_str(), &temporary, 0 },
    { probe, 0, "OPTOUT/COLO/ALL", "Tags/DEFAULT", 0, hid.c_str(), &temporary, 0 },
    { optout, 0, "OPTOUT/COLO/ALL", "Tags/DEFAULT", 0, hid.c_str(), &temporary, 0 },
    { invalid, 0, "OPTOUT/COLO/ALL", "Tags/DEFAULT", 0, hid.c_str(), &temporary, 0 },
    { optin, DAY, "OPTOUT/COLO/ALL", "Tags/DEFAULT", 0, hid.c_str(), &temporary, 0 },
  };
    
  process_requests_(
    AutoTest::Time(
      base_time_.get_gm_time().get_date()),
    REQUESTS);

  ADD_WAIT_CHECKER(
    "ColoUserStats check",
    AutoTest::stats_diff_checker(
      pq_conn_,
      colo_user_diffs,
      colo_user_stats));

  ADD_WAIT_CHECKER(
    "CreatedUserStats check",
    AutoTest::stats_diff_checker(
      pq_conn_,
      created_user_diffs,
      created_user_stats));

  ADD_WAIT_CHECKER(
    "GlobalColoUserStats check",
    AutoTest::stats_diff_checker(
      pq_conn_,
      global_colo_user_diffs,
      global_colo_user_stats));
}

void ColoUsers::non_serialized_()
{
  ColoUserStats colo_user_stats;
  ColoUserDiffs colo_user_diffs;
  GlobalColoUserStats global_colo_user_stats;
  GlobalColoUserDiffs global_colo_user_diffs;
  CreatedUserStats created_user_stats;
  CreatedUserDiffs created_user_diffs;

  const Expected CASE_STATS[] =
  {
    // 1
    {
      "NOT_SERIALIZED/COLO/ALL",
      base_time_,
      base_time_,
      ZERO_DATE,
      1, 1, 1, 1
    },
    // 2
    {
      "NOT_SERIALIZED/COLO/ALL",
      base_time_ + DAY,
      base_time_,
      base_time_,
      1, 1, 0, 1
    } 
  };

  add_stats_(
    colo_user_stats,
      colo_user_diffs,
      CASE_STATS);

  add_stats_(
    created_user_stats,
    created_user_diffs,
    CASE_STATS);

  add_stats_(
    global_colo_user_stats,
    global_colo_user_diffs,
    CASE_STATS);

  AdClient client(AdClient::create_undef_user(this));
  TemporaryAdClient temporary(TemporaryAdClient::create_user(this));
  std::string hid(AutoTest::generate_uid(false));

  const TestRequest REQUESTS[] =
  {
    { temporary, DAY + 1, "NOT_SERIALIZED/COLO/ALL", 0, 0, hid.c_str(), 0, 0 },
    { client, DAY - 1, "NOT_SERIALIZED/COLO/ALL", "Tags/DEFAULT", 0, hid.c_str(), 0, 0 },
    { client, DAY - 1, "NOT_SERIALIZED/COLO/ALL", 0, 0, hid.c_str(), &temporary, 0 },
    { client, DAY + 1, "NOT_SERIALIZED/COLO/ALL", "Tags/DEFAULT", 0, hid.c_str(), 0, 0 }
  };

  process_requests_(
    AutoTest::Time(
      base_time_.get_gm_time().get_date()),
    REQUESTS);

  ADD_WAIT_CHECKER(
    "ColoUserStats check",
    AutoTest::stats_diff_checker(
      pq_conn_,
      colo_user_diffs,
      colo_user_stats));

  ADD_WAIT_CHECKER(
    "CreatedUserStats check",
    AutoTest::stats_diff_checker(
      pq_conn_,
      created_user_diffs,
      created_user_stats));

  ADD_WAIT_CHECKER(
    "GlobalColoUserStats check",
    AutoTest::stats_diff_checker(
      pq_conn_,
      global_colo_user_diffs,
      global_colo_user_stats));
}

void ColoUsers::pub_inventory_()
{
  ColoUserStats colo_user_stats;
  ColoUserDiffs colo_user_diffs;
  GlobalColoUserStats global_colo_user_stats;
  GlobalColoUserDiffs global_colo_user_diffs;
  CreatedUserStats created_user_stats;
  CreatedUserDiffs created_user_diffs;

  const Expected CASE_STATS[] =
  {
    // 1
    {
      "PUB_INV/COLO/ALL",
      base_time_,
      base_time_,
      ZERO_DATE,
      1, 1, 0, 1
    },
    // 2
    {
      "PUB_INV/COLO/NON_OPTOUT",
      base_time_,
      base_time_,
      ZERO_DATE,
      1, 0, 0, 1
    },
    // 3
    {
      "PUB_INV/COLO/ALL",
      base_time_ + DAY,
      UNDEF_DATE,
      UNDEF_DATE,
      0, 0, 0, 0
    },
    // 4
    {
      "PUB_INV/COLO/ALL",
      base_time_ + 2*DAY,
      base_time_ + 2*DAY,
      ZERO_DATE,
      1, 1, 0, 1
    }
  };

  add_stats_(
    colo_user_stats,
      colo_user_diffs,
      CASE_STATS);

  add_stats_(
    created_user_stats,
    created_user_diffs,
    CASE_STATS);

  add_stats_(
    global_colo_user_stats,
    global_colo_user_diffs,
    CASE_STATS);

  AdClient client1(AdClient::create_undef_user(this));
  AdClient client2(AdClient::create_undef_user(this));
  AdClient client3(AdClient::create_undef_user(this));
  std::string hid1(AutoTest::generate_uid(false));
  std::string hid2(AutoTest::generate_uid(false));
  std::string hid3(AutoTest::generate_uid(false));

  const TestRequest REQUESTS[] =
  {
    { client1, 0, "PUB_INV/COLO/ALL", "Tags/Inventory", 0, hid1.c_str(), 0, 0 },
    { client1, 0, "PUB_INV/COLO/ALL", "Tags/Inventory", 0, hid1.c_str(), 0, TRF_INVENTORY },
    { client2, 0, "PUB_INV/COLO/NON_OPTOUT", "Tags/Inventory", 0, hid2.c_str(), 0, TRF_INVENTORY },
    { client3, DAY, "PUB_INV/COLO/ALL", "Tags/DEFAULT", 0, hid3.c_str(), 0, TRF_INVENTORY },
    { client3, 2*DAY, "PUB_INV/COLO/ALL", "Tags/DEFAULT", 0, hid3.c_str(), 0, 0 },
  };

  process_requests_(
    AutoTest::Time(
      base_time_.get_gm_time().get_date()),
    REQUESTS);

  ADD_WAIT_CHECKER(
    "ColoUserStats check",
    AutoTest::stats_diff_checker(
      pq_conn_,
      colo_user_diffs,
      colo_user_stats));

  ADD_WAIT_CHECKER(
    "CreatedUserStats check",
    AutoTest::stats_diff_checker(
      pq_conn_,
      created_user_diffs,
      created_user_stats));

  ADD_WAIT_CHECKER(
    "GlobalColoUserStats check",
    AutoTest::stats_diff_checker(
      pq_conn_,
      global_colo_user_diffs,
      global_colo_user_stats));
}

void ColoUsers::oo_service_()
{
  ColoUserStats colo_user_stats;
  ColoUserDiffs colo_user_diffs;
  GlobalColoUserStats global_colo_user_stats;
  GlobalColoUserDiffs global_colo_user_diffs;
  CreatedUserStats created_user_stats;
  CreatedUserDiffs created_user_diffs;

  const Expected CASE_STATS[] =
  {
    // 1
    {
      "OO/COLO/ALL1",
      base_time_,
      base_time_,
      ZERO_DATE,
      1, 0, 0, 0
    },
    // 2
    {
      "OO/COLO/ALL1",
      base_time_ + DAY,
      base_time_,
      base_time_,
      1, 0, 0, 0
    },
    // 3
    {
      "OO/COLO/ALL1",
      base_time_ + DAY,
      base_time_,
      ZERO_DATE,
      0, 1, 1, 0
    },
    // 4
    {
      "OO/COLO/ALL2",
      base_time_,
      base_time_,
      ZERO_DATE,
      1, 0, 0, 0
    },
    // 5
    {
      "OO/COLO/ALL3",
      base_time_,
      base_time_,
      ZERO_DATE,
      1, 0, 0, 0
    },
  };

  add_stats_(
    colo_user_stats,
      colo_user_diffs,
      CASE_STATS);

  add_stats_(
    created_user_stats,
    created_user_diffs,
    CASE_STATS);

  add_stats_(
    global_colo_user_stats,
    global_colo_user_diffs,
    CASE_STATS);

  // Opt-in request
  {
    AdClient client(AdClient::create_nonoptin_user(this));
    client.process_request(
      OptOutRequest(false).
        colo(fetch_int("OO/COLO/ALL1")).
        op("in").
        debug_time(base_time_));

    TemporaryAdClient temporary(TemporaryAdClient::create_user(this));
    
    const TestRequest REQUESTS[] =
    {
      { client, DAY, "OO/COLO/ALL1", "Tags/DEFAULT", 0, 0, &temporary, 0 }
    };

    process_requests_(
      base_time_,
      REQUESTS);
  }

  // Opt-out request
  {
    {
      AdClient client(AdClient::create_nonoptin_user(this));
      client.process_request(
        OptOutRequest(false).
          colo(fetch_int("OO/COLO/ALL2")).
          op("in").
          debug_time(base_time_));
      client.process_request(
        OptOutRequest(false).
          colo(fetch_int("OO/COLO/ALL2")).
          op("out").
          debug_time(base_time_));
    }
    {
      AdClient client(AdClient::create_nonoptin_user(this));
      client.process_request(
        OptOutRequest(false).
          colo(fetch_int("OO/COLO/ALL2")).
          op("out").
          debug_time(base_time_));
    }
  }

  // Opt-out status
  {
    AdClient client(AdClient::create_nonoptin_user(this));
    client.process_request(
      OptOutRequest(false).
        colo(fetch_int("OO/COLO/ALL3")).
        op("in").
        debug_time(base_time_));
    client.process_request(
      OptOutRequest(false).
        colo(fetch_int("OO/COLO/ALL3")).
        op("status").
        debug_time(base_time_));
    }

  ADD_WAIT_CHECKER(
    "ColoUserStats check",
    AutoTest::stats_diff_checker(
      pq_conn_,
      colo_user_diffs,
      colo_user_stats));

  ADD_WAIT_CHECKER(
    "CreatedUserStats check",
    AutoTest::stats_diff_checker(
      pq_conn_,
      created_user_diffs,
      created_user_stats));

  ADD_WAIT_CHECKER(
    "GlobalColoUserStats check",
    AutoTest::stats_diff_checker(
      pq_conn_,
      global_colo_user_diffs,
      global_colo_user_stats));
  
}

template<class Stat, class Expected, size_t Count>
void
ColoUsers::add_stats_(
  ORM::StatsList<Stat>& stats,
  std::list<typename Stat::Diffs>& diffs,
  const Expected (&expected)[Count])
{
  typedef typename Stat::Diffs Diff;
  typedef std::map<unsigned long, Diff> SumDiffsMap;
  SumDiffsMap sum_diffs;
  for (size_t i = 0; i < Count; ++i)
  {
    unsigned long colo = fetch_int(expected[i].colo);
    Stat stat;
    init_stat_(
      stat,
      expected[i]);
    stat.description("#" + strof(i+1));
    stat.select(pq_conn_);
    stats.push_back(stat);
    add_diff_(diffs, expected[i]);

    Diff sum_diff;

    init_sum_diff_(
      sum_diff,
      expected[i]);

    if (sum_diffs.find(colo) == sum_diffs.end())
    {
      sum_diffs[colo] = sum_diff;
    }
    else
    {
      sum_diffs[colo] += sum_diff;
    }
  }

  typename SumDiffsMap::const_iterator it = sum_diffs.begin();

  for (size_t i = 0; it != sum_diffs.end(); ++it, ++i)
  {
    Stat stat;
    stat.key().
      colo_id(it->first);
    stat.description("#" + strof(i+Count+1));
    stat.select(pq_conn_);
    stats.push_back(stat);
    diffs.push_back(it->second);    
  }
}

template<class Expected>
void
ColoUsers::init_stat_(
 ColoUserStat& stat,
 const Expected& expected)
{
  stat.key().
    colo_id(fetch_int(expected.colo)).
    isp_sdate(expected.sdate);
  
  if (expected.last_appearance_date != UNDEF_DATE)
  {
    stat.key().last_appearance_date(expected.last_appearance_date);
  }
}

template<class Expected>
void
ColoUsers::init_stat_(
  GlobalColoUserStat& stat,
  const Expected& expected)
{
  stat.key().
    colo_id(fetch_int(expected.colo)).
    global_sdate(expected.sdate);

  if (expected.last_appearance_date != UNDEF_DATE)
  {
    stat.key().last_appearance_date(expected.last_appearance_date);
  }
  if (expected.create_date != UNDEF_DATE)
  {
    stat.key().create_date(expected.create_date);
  }

}

template<class Expected>
void
ColoUsers::init_stat_(
  CreatedUserStat& stat,
  const Expected& expected)
{
  stat.key().
    colo_id(fetch_int(expected.colo)).
    isp_sdate(expected.sdate);
  if (expected.last_appearance_date != UNDEF_DATE)
  {
    stat.key().last_appearance_date(expected.last_appearance_date);
  }
  if (expected.create_date != UNDEF_DATE)
  {
    stat.key().create_date(expected.create_date);
  }

}

template<class Diffs, class Expected>
void
ColoUsers::add_diff_(
  std::list<Diffs>& diffs,
  const Expected& expected)
{
  diffs.push_back(
    Diffs().
      unique_users(expected.unique_users).
      network_unique_users(expected.network_unique_users).
      profiling_unique_users(expected.profiling_unique_users).
      unique_hids(expected.unique_hids));
}

template<class Diffs, class Expected>
void
ColoUsers::init_sum_diff_(
  Diffs& diff,
  const Expected& expected)
{
  diff.
    unique_users(expected.unique_users).
    network_unique_users(expected.network_unique_users).
    profiling_unique_users(expected.profiling_unique_users).
    unique_hids(expected.unique_hids).
    neg_control_unique_users(0).
    neg_control_network_unique_users(0).
    neg_control_profiling_unique_users(0).
    neg_control_unique_hids(0).
    control_unique_users_1(
      sum_change(
        expected.sdate,
        expected.last_appearance_date,
        expected.unique_users)).
    control_unique_users_2(
      sum_change(
        expected.create_date,
        expected.last_appearance_date,
        expected.unique_users)).
    control_network_unique_users_1(
      sum_change(
        expected.sdate,
        expected.last_appearance_date,
        expected.network_unique_users)).
    control_network_unique_users_2(
      sum_change(
        expected.create_date,
        expected.last_appearance_date,
        expected.network_unique_users)).
    control_profiling_unique_users_1(
      sum_change(
        expected.sdate,
        expected.last_appearance_date,
        expected.profiling_unique_users)).
    control_profiling_unique_users_2(
      sum_change(
        expected.create_date,
        expected.last_appearance_date,
        expected.profiling_unique_users)).
    control_unique_hids_1(
      sum_change(
        expected.sdate,
        expected.last_appearance_date,
        expected.unique_hids)).
    control_unique_hids_2(
      sum_change(
        expected.create_date,
        expected.last_appearance_date,
        expected.unique_hids));
}

template<class Expected>
void
ColoUsers::init_sum_diff_(
  ColoUserDiff& diff,
  const Expected& expected)
{
  diff.
    unique_users(expected.unique_users).
    network_unique_users(expected.network_unique_users).
    profiling_unique_users(expected.profiling_unique_users).
    unique_hids(expected.unique_hids).
    neg_control_unique_users(0).
    neg_control_network_unique_users(0).
    neg_control_profiling_unique_users(0).
    neg_control_unique_hids(0).
    control_unique_users(
      sum_change(
        expected.sdate,
        expected.last_appearance_date,
        expected.unique_users)).
    control_network_unique_users(
            sum_change  (
              expected.sdate,
              expected.last_appearance_date,
              expected.network_unique_users)).
    control_profiling_unique_users(
      sum_change  (
        expected.sdate,
        expected.last_appearance_date,
        expected.profiling_unique_users)).
    control_unique_hids(
      sum_change(
        expected.sdate,
        expected.last_appearance_date,
        expected.unique_hids));
}

template<size_t Count>
void
ColoUsers::process_requests_(
  const AutoTest::Time& base_time,
  const TestRequest(&requests)[Count])
{
  for (size_t i = 0; i < Count; ++i)
  {
    // Initialize request
    NSLookupRequest request;
    request.colo = fetch_int(requests[i].colo);
    request.debug_time = base_time + requests[i].time_offset;
    if (requests[i].tid)
    {
      request.tid = fetch_int(requests[i].tid);
    }
    if (requests[i].referer_kw)
    {
      request.referer_kw = fetch_string(requests[i].referer_kw);
    }
    if (requests[i].hid)
    {
      request.hid = requests[i].hid;
    }
    if (requests[i].flags & TRF_SAFE_TUID)
    {
      request.rm_tuid = 0;
    }
    if (requests[i].flags & TRF_INVENTORY)
    {
      request.tag_inv = 1;
    }

    // Send request
    if (requests[i].temporary)
    {
      requests[i].client.merge(*requests[i].temporary, request);
    }
    else
    {
      requests[i].client.process_request(request);
    }

    // Check response
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        requests[i].flags & TRF_CHECK_CC?
          fetch_string("Campaign/CC_ID").c_str(): "0",
        requests[i].client.debug_info.ccid).check(),
      "Expected creative#" + strof(i+1));

    if (requests[i].flags & TRF_CHECK_NO_TAG)
    {
      FAIL_CONTEXT(
        AutoTest::equal_checker(
          "0",
          requests[i].client.debug_info.tag_id).check(),
        "Expected tag#" + strof(i+1));
    }
  }
}
