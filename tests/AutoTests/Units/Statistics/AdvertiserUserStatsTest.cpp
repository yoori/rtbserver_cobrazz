
#include <algorithm> 
#include "AdvertiserUserStatsTest.hpp"

REFLECT_UNIT(AdvertiserUserStatsTest) (
  "Statistics",
  AUTO_TEST_SLOW);

namespace
{
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::ChannelsCheck ChannelsCheck;
  typedef AutoTest::TemporaryAdClient TemporaryAdClient;

  const Generics::Time ZERO_TIME = Generics::Time::ZERO;

  const int DAY = 24 * 60 * 60;

  const unsigned int MAX_LAST_APPEARANCE_DIFF = 32;

  long sum_change(
    const Generics::Time& adv_sdate,
    const Generics::Time& last_appearance_date,
    long unique_users)
  {
    // days(time) is (time / Generics::Time::ONE_DAY).tv_sec
    return (AutoTest::days(adv_sdate - Generics::Time::ZERO) *
      (MAX_LAST_APPEARANCE_DIFF + 1) +
      (last_appearance_date == Generics::Time::ZERO ?
        MAX_LAST_APPEARANCE_DIFF :
       AutoTest::days(adv_sdate - last_appearance_date))) * unique_users;
  }
  
}

template <class Stats>
void
AdvertiserUserStatsTest::initialize_stat_(
  Stats& stat,
  typename Stats::Diffs& diff,
  const Expected& expected,
  unsigned long entity,
  unsigned long colo)
{
  stat.key(
    entity,
    colo? colo: fetch_int("DEFAULT_COLO"),
    expected.last_appearance,
    expected.sdate);
  diff.
    unique_users(
      expected.unique_users?
        expected.unique_users:
          expected.text_unique_users?
             expected.text_unique_users:expected.display_unique_users);
}

void
AdvertiserUserStatsTest::initialize_stat_(
  AdvStats& stat,
  AdvDiff& diff,
  const Expected& expected,
  unsigned long entity,
  unsigned long)
{
  stat.key(
    entity,
    expected.last_appearance,
    expected.sdate);
  diff.
    unique_users(expected.unique_users).
    display_unique_users(expected.display_unique_users).
    text_unique_users(expected.text_unique_users);
}

template <class Stats, size_t Count>
void
AdvertiserUserStatsTest::initialize_stats_(
  const std::string& description,
  ORM::StatsList<Stats>& stats,
  std::list<typename Stats::Diffs>& diffs,
  const Expected (&expected) [Count],
  Entity entity,
  unsigned long colo)
{
  unsigned long stat_index = 0;
  for (size_t i = 0; i < Count; i++)
  {
    if (expected[i].*(entity))
    {
      std::list<unsigned long> entities;
      fetch_objects(
        std::inserter(entities, entities.begin()),
        expected[i].*(entity));

      for (
        std::list<unsigned long>::iterator it = entities.begin();
        it !=  entities.end(); ++it)
      {
        Stats stat;
        typename Stats::Diffs diff;
        initialize_stat_(stat, diff, expected[i], *it, colo);
        stat.description(description + "#" + strof(++stat_index));
        stat.select(conn_);
        stats.push_back(stat);
        diffs.push_back(diff);
      }
    }
  }
}

template <size_t Count>
void
AdvertiserUserStatsTest::initialize_all_stats_(
  const std::string& description,
  const Expected (&expected) [Count],
  unsigned long colo)
{
  initialize_stats_(
    description,
    adv_stats_,
    adv_diffs_, expected,
    &Expected::advertisers,
    colo);
  
  initialize_stats_(
    description,
    cmp_stats_,
    cmp_diffs_,
    expected,
    &Expected::campaigns,
    colo);
  
  initialize_stats_(
    description,
    ccg_stats_,
    ccg_diffs_,
    expected,
    &Expected::ccgs,
    colo);
  
  initialize_stats_(
    description,
    cc_stats_,
    cc_diffs_,
    expected,
    &Expected::ccs,
    colo);
}


template <class Stats, typename Key, size_t Count>
void
AdvertiserUserStatsTest::initialize_sum_stats_(
  const std::string& description,
  ORM::StatsList<Stats>& stats,
  std::list<typename Stats::Diffs>& diffs,
  const std::string& entity_name,
  Key& (Key::*Func)(const int&),
  const SumExpected (&expected) [Count])
{
  typedef typename Stats::Diffs Diff;
  for (size_t i = 0; i < Count; i++)
  {
    Key key;
    ((key).*(Func))(
      fetch_int(entity_name + strof(i+1)));
    Stats stat(key);
    stat.description(description + "#" + strof(i+1));
    stat.select(conn_);
    stats.push_back(stat);
    diffs.push_back(
      Diff().
        unique_users(expected[i].users).
        control_sum(static_cast<int>(expected[i].control_sum)));
  }
}

void
AdvertiserUserStatsTest::log_request_(
  std::string request_id)
{
  AutoTest::AdminsArray<AutoTest::RequestProfileAdmin> admins;
  
  admins.initialize(
    this,
    CTE_ALL,
    STE_REQUEST_INFO_MANAGER,
    "\\" + request_id);
  
  admins.log(AutoTest::Logger::thlog());
}

template<size_t ReqCount>
void
AdvertiserUserStatsTest::process_requests_(
  AdClient& client,
  const std::string& description,
  const Request(&requests)[ReqCount],
  const AutoTest::Time& base_time,
  unsigned long colo)
{
  for (size_t i = 0; i < ReqCount; ++i)
  {

    NSLookupRequest request;
    request.tid = fetch_int(requests[i].tid);
    if (requests[i].kwd)
    {
      request.referer_kw = map_objects(requests[i].kwd);
    }
    request.format = requests[i].format;
    request.debug_time = base_time + requests[i].time_ofset;
    if (colo)
    {
      request.colo = colo;
    }
    
    client.process_request(request);

    if (requests[i].expected_ccs)
    {
      std::list<std::string> expected_ccs;
      
      fetch_objects(
        std::inserter(expected_ccs, expected_ccs.begin()),
        requests[i].expected_ccs);

      FAIL_CONTEXT(
        AutoTest::sequence_checker(
          expected_ccs,
          AutoTest::SelectedCreativesCCID(client)).check(),
        description + 
          " Check creatives#" + strof(i+1));

      requests_.push_back(
        client.debug_info.creative_request_id.value());
    }
    else
    {
      FAIL_CONTEXT(
        AutoTest::equal_checker(
          "0",
          client.debug_info.ccid).check(),
        description + 
          " Check creatives#" + strof(i+1));
    }
    if (strcmp(requests[i].format, "unit-test-imp") == 0)
    {
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        !client.debug_info.track_pixel_url.empty()),
      description + 
        " Check traclk pixel url# " + strof(i+1));
    }
  }
}

void
AdvertiserUserStatsTest::check_stats_()
{
  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        conn_,
        adv_diffs_,
        adv_stats_)).check(),
    "AdvertiserUserStats test");
  
  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        conn_,
        cmp_diffs_,
        cmp_stats_)).check(),
    "CampaignUserStats test");
  
  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        conn_,
        ccg_diffs_,
        ccg_stats_)).check(),
    "CCGUserStats test");
  
  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        conn_,
        cc_diffs_,
        cc_stats_)).check(),
    "CCUserStats test");

  adv_stats_.clear();
  cmp_stats_.clear();
  ccg_stats_.clear();
  cc_stats_.clear();
  adv_diffs_.clear();
  cmp_diffs_.clear();
  ccg_diffs_.clear();
  cc_diffs_.clear();
    
  // Check agregated stats
  
  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        conn_,
        adv_sum_diffs_,
        adv_sum_stats_)).check(),
    "AdvertiserUserStats aggregation test");
  
  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        conn_,
        cmp_sum_diffs_,
        cmp_sum_stats_)).check(),
    "CampaignUserStats aggregation test");
  
  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        conn_,
        ccg_sum_diffs_,
        ccg_sum_stats_)).check(),
    "CCGUserStats aggregation test");
  
  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        conn_,
        cc_sum_diffs_,
        cc_sum_stats_)).check(),
    "CCUserStats aggregation test");

  adv_sum_stats_.clear();
  cmp_sum_stats_.clear();
  ccg_sum_stats_.clear();
  cc_sum_stats_.clear();
  adv_sum_diffs_.clear();
  cmp_sum_diffs_.clear();
  ccg_sum_diffs_.clear();
  cc_sum_diffs_.clear();
}

bool
AdvertiserUserStatsTest::run_test()
{
  try
  {
    optout_users_();
    base_();
    unique_users_();
    last_usage_();
    timezones_();
    AdClient client1(async_part_1_());
    AdClient client2(big_date_diff_part_1_());
    colo_();
    temporary_user_();
    
    add_descr_phrase("Check stats#1");
    check_stats_();

    async_part_2_(client1);
    big_date_diff_part_2_(client2);

    add_descr_phrase("Check stats#2");
    check_stats_();

    async_part_3_(client1);
    
    add_descr_phrase("Check stats#3");
    check_stats_();
    
    return true;
  }
  catch (const eh::Exception&)
  {
    add_descr_phrase("Log requests by RIA.");    
    std::for_each(
      requests_.begin(),
      requests_.end(),
      std::bind1st(
        std::mem_fun(
          &AdvertiserUserStatsTest::log_request_), this));
    throw;

  }
}

void
AdvertiserUserStatsTest::base_()
{
  std::string description("Ordered and unordered sequence.");
  add_descr_phrase(description);
  
  const Expected EXPECTED[] =
  {
    {
      "BASE/Account/Adv1",
      "BASE/Campaign/1",
      "BASE/CCG/1",
      "BASE/CC/1",
      ZERO_TIME,
      today_,
      1, 1, 0
    },
    {
      "BASE/Account/Adv1",
      "BASE/Campaign/1",
      "BASE/CCG/1",
      "BASE/CC/1",
      today_,
      today_ + DAY,
      1, 1, 0
    },
    {
      "BASE/Account/Adv1",
      "BASE/Campaign/1",
      "BASE/CCG/1",
      "BASE/CC/1",
      today_ + DAY,
      today_ + 10 * DAY,
      1, 1, 0
    },
    {
      "BASE/Account/Adv1",
      "BASE/Campaign/1",
      "BASE/CCG/1",
      "BASE/CC/1",
      today_ + 19 * DAY,
      today_ + 50 * DAY,
      1, 1, 0
    },
    {
      "BASE/Account/Adv2",
      "BASE/Campaign/2",
      "BASE/CCG/2",
      "BASE/CC/2",
      today_ + 4 * DAY,
      today_ + 5 * DAY,
      1, 1, 0
    },
    {
      "BASE/Account/Adv2",
      "BASE/Campaign/2",
      "BASE/CCG/2",
      "BASE/CC/2",
      today_,
      today_ + 4 * DAY,
      1, 1, 0
    },
    {
      "BASE/Account/Adv2",
      "BASE/Campaign/2",
      "BASE/CCG/2",
      "BASE/CC/2",
      ZERO_TIME,
      today_,
      1, 1, 0
    }
  };
  
  initialize_all_stats_(description, EXPECTED);
  
  const SumExpected SUM_EXPECTED[] =
  {
    {
      4,
      static_cast<int>(sum_change(today_, Generics::Time::ZERO, 1) + 
        sum_change(today_ + 1 * DAY, today_, 1) +
        sum_change(today_ + 10 * DAY, today_ + 1 * DAY, 1) +
        sum_change(today_ + 50 * DAY, today_ + 19 * DAY, 1))
    },
    {
      3,
      static_cast<int>(sum_change(today_ + 5 * DAY, today_ + 4 * DAY, 1) +
        sum_change(today_ + 4 * DAY, today_, 1) +
        sum_change(today_, Generics::Time::ZERO, 1))
    }
  };
  
  initialize_sum_stats_(
    description,
    adv_sum_stats_,
    adv_sum_diffs_,
    "BASE/Account/Adv",
    &ORM::AdvertiserUserStats::Key::adv_account_id,
    SUM_EXPECTED);
  initialize_sum_stats_(
    description,
    cmp_sum_stats_,
    cmp_sum_diffs_,
    "BASE/Campaign/",
    &ORM::CampaignUserStats::Key::campaign_id,
    SUM_EXPECTED);
  initialize_sum_stats_(
    description,
    ccg_sum_stats_,
    ccg_sum_diffs_,
    "BASE/CCG/",
    &ORM::CCGUserStats::Key::ccg_id,
    SUM_EXPECTED);
  initialize_sum_stats_(
    description,
    cc_sum_stats_,
    cc_sum_diffs_,
    "BASE/CC/",
    &ORM::CCUserStats::Key::cc_id,
    SUM_EXPECTED);
  
  const Request BASE_REQUESTS[] =
  {
    // Ordered sequence
    {"BASE/KWD/1", "BASE/Tag", 0, "unit-test", "BASE/CC/1"},
    {"BASE/KWD/1", "BASE/Tag", 0, "unit-test", "BASE/CC/1"},
    {"BASE/KWD/1", "BASE/Tag", DAY, "unit-test", "BASE/CC/1"},
    {"BASE/KWD/1", "BASE/Tag", DAY, "unit-test", "BASE/CC/1"},
    {"BASE/KWD/1", "BASE/Tag", 10 * DAY, "unit-test", "BASE/CC/1"},
    {"BASE/KWD/1", "BASE/Tag", 10 * DAY, "unit-test", "BASE/CC/1"},
    {"BASE/KWD/1", "BASE/Tag", 50 * DAY, "unit-test", "BASE/CC/1"},
    // Unordered sequence
    {"BASE/KWD/2", "BASE/Tag", 5 * DAY, "unit-test", "BASE/CC/2"},
    {"BASE/KWD/2", "BASE/Tag", 4 * DAY, "unit-test", "BASE/CC/2"},
    {"BASE/KWD/2", "BASE/Tag", 0, "unit-test", "BASE/CC/2"},
  };

  AdClient client(AdClient::create_user(this));
  process_requests_(client, description, BASE_REQUESTS, today_);
  
}

void
AdvertiserUserStatsTest::unique_users_()
{
  std::string description("Unique users.");
  add_descr_phrase(description);

  AutoTest::Time base_time(
    today_.get_gm_time().format("%d-%m-%Y:%H-00-00"));

  const Expected EXPECTED[] =
  {

    // Request 1 & 2
    {
      "UNIQUE/Account/Adv2,UNIQUE/Account/Adv3",
      "UNIQUE/Campaign/Display2,UNIQUE/Campaign/Text3",
      "UNIQUE/CCG/Display2,UNIQUE/CCG/Text3",
      "UNIQUE/CC/Display2,UNIQUE/CC/Text3",
      ZERO_TIME,
      today_,
      0, 0, 0
    },

    // Request 3
    {
      "UNIQUE/Account/Adv1",
      "UNIQUE/Campaign/Display1",
      "UNIQUE/CCG/Display1",
      "UNIQUE/CC/Display1",
      ZERO_TIME,
      today_,
      1, 1, 0
    },

    // Request 4, 5
    {
      "UNIQUE/Account/Adv4",
      "UNIQUE/Campaign/Display4,UNIQUE/Campaign/Keyword4",
      "UNIQUE/CCG/Display4,UNIQUE/CCG/Keyword4",
      "UNIQUE/CC/Display4,UNIQUE/CC/Keyword4",
      ZERO_TIME,
      today_ + DAY,
      1, 1, 1
    },

    // Request 6 & 7
    {
      "UNIQUE/Account/Adv2",
      "UNIQUE/Campaign/Display2",
      "UNIQUE/CCG/Display2",
      "UNIQUE/CC/Display2",
      ZERO_TIME,
      today_ + DAY,
      1, 1, 0
    },
    {
      "UNIQUE/Account/Adv2,UNIQUE/Account/Adv4",
      "UNIQUE/Campaign/Display2",
      "UNIQUE/CCG/Display2",
      "UNIQUE/CC/Display2",
      today_ + DAY,
      today_ + DAY,
      0, 0, 0
    },

    // Request 8
    {
      "UNIQUE/Account/Adv3",
      "UNIQUE/Campaign/Text3",
      "UNIQUE/CCG/Text3",
      "UNIQUE/CC/Text3",
      ZERO_TIME,
      today_ + DAY,
      1, 0, 1
    },
    
    // Request 9
    {
      "UNIQUE/Account/Adv1",
      0,
      0,
      0,
      ZERO_TIME,
      today_ + 2*DAY,
      0, 0, 1
    },
    {
      "UNIQUE/Account/Adv1",
      0,
      0,
      0,
      today_,
      today_ + 2*DAY,
      1, 0, 0
    },
    {
      0,
      "UNIQUE/Campaign/Keyword1",
      "UNIQUE/CCG/Keyword1,UNIQUE/CCG/Text1",
      "UNIQUE/CC/Keyword1,UNIQUE/CC/Text1",
      ZERO_TIME,
      today_ + 2*DAY,
      1, 0, 1
    },
    
    // Request 10 & 11
    {
      "UNIQUE/Account/Adv1",
      "UNIQUE/Campaign/Keyword1",
      "UNIQUE/CCG/Keyword1,UNIQUE/CCG/Text1",
      "UNIQUE/CC/Keyword1,UNIQUE/CC/Text1",
      today_ + 2*DAY,
      today_ + 3*DAY,
      1, 0, 1
    },

    {
      "UNIQUE/Account/Adv1",
      0,
      "UNIQUE/CCG/Keyword1",
      0,
      today_ + 3*DAY,
      today_ + 3*DAY,
      0, 0, 0
    },
    
    // Request 12
    {
      "UNIQUE/Account/Adv1",
      "UNIQUE/Campaign/Keyword1",
      "UNIQUE/CCG/Text1",
      "UNIQUE/CC/Text1",
      today_ + 3*DAY,
      today_ + 4*DAY,
      1, 0, 1
    },

    {
      0,
      0,
      "UNIQUE/CCG/Text1",
      0,
      today_ + 4*DAY,
      today_ + 4*DAY,
      0, 0, 0
    },
    
    //  Request 13
    {
      0, 0, 0,
      "UNIQUE/CC/Text1A",
      ZERO_TIME,
      today_ + 4*DAY,
      1, 0, 1
    },
    
    // Request 14
    {
      0,
      "UNIQUE/Campaign/Display1",
      "UNIQUE/CCG/Display1",
      "UNIQUE/CC/Display1",
      today_,
      today_ + 4*DAY,
      1, 1, 0
    },
    {
      "UNIQUE/Account/Adv1",
      0,
      0,
      0,
      today_,
      today_ + 4*DAY,
      0, 1, 0 
    }, 
  };

  initialize_all_stats_(description, EXPECTED);
  
  const SumExpected SUM_EXPECTED[] =
  {
    { 0, 0 },
    { 0, 0 }
  };

  initialize_sum_stats_(
    description,
    adv_sum_stats_,
    adv_sum_diffs_,
    "UNIQUE/Account/Agency",
    &ORM::AdvertiserUserStats::Key::adv_account_id,
    SUM_EXPECTED);
      
  const Request UNIQUE_REQUESTS[] =
  {
     // 1
    {"UNIQUE/KWD/Display2", "UNIQUE/Tag", 0, "unit-test-imp", "UNIQUE/CC/Display2"},
     // 2
    {"UNIQUE/KWD/Text3", "UNIQUE/Tag", 60, "unit-test-imp", "UNIQUE/CC/Text3"},
     // 3
    {"UNIQUE/KWD/Display1", "UNIQUE/Tag", 120, "unit-test", "UNIQUE/CC/Display1"},
     // 4
    {"UNIQUE/KWD/Keyword4", "UNIQUE/Tag", DAY, "unit-test", "UNIQUE/CC/Keyword4"},
     // 5
    {"UNIQUE/KWD/Display4", "UNIQUE/Tag", DAY + 60, "unit-test", "UNIQUE/CC/Display4"},
     // 6
    {"UNIQUE/KWD/Display2", "UNIQUE/Tag", DAY + 120, "unit-test", "UNIQUE/CC/Display2"},
     // 7
    {"UNIQUE/KWD/Display2", "UNIQUE/Tag", DAY + 180, "unit-test", "UNIQUE/CC/Display2"},
     // 8
    {"UNIQUE/KWD/Text3", "UNIQUE/Tag", DAY + 240, "unit-test", "UNIQUE/CC/Text3"},
     // 9
    {"UNIQUE/KWD/Text1,UNIQUE/KWD/Keyword1", "UNIQUE/TagEx2", 2 * DAY,
     "unit-test", "UNIQUE/CC/Keyword1,UNIQUE/CC/Text1"},
    // 10
    {"UNIQUE/KWD/Text1", "UNIQUE/TagEx2", 3 * DAY, "unit-test", "UNIQUE/CC/Text1"},
    // 11
    {"UNIQUE/KWD/Keyword1", "UNIQUE/Tag", 3 * DAY + 60, "unit-test", "UNIQUE/CC/Keyword1"},
     // 12
    {"UNIQUE/KWD/Text1", "UNIQUE/TagEx2", 4 * DAY, "unit-test", "UNIQUE/CC/Text1"},
    // 13
    {"UNIQUE/KWD/Text1", "UNIQUE/TagEx1", 4 * DAY + 60, "unit-test", "UNIQUE/CC/Text1A"},
    // 14
    {"UNIQUE/KWD/Display1", "UNIQUE/Tag", 4 * DAY + 120, "unit-test", "UNIQUE/CC/Display1"}
  };

  AdClient client(AdClient::create_user(this));
  process_requests_(client, description, UNIQUE_REQUESTS, base_time);
}


void
AdvertiserUserStatsTest::last_usage_()
{
  std::string description("Last appearance date.");
  add_descr_phrase(description);
   
  const Expected EXPECTED[] =
  {
    {
      "LASTUSAGE/Account/Adv1",
      "LASTUSAGE/Campaign/Display1,LASTUSAGE/Campaign/Text1",
      "LASTUSAGE/CCG/Display1,LASTUSAGE/CCG/Text1",
      "LASTUSAGE/CC/Display1,LASTUSAGE/CC/Text1",
      ZERO_TIME,
      today_ - 32 * DAY,
      1, 1, 1
    },
    {
      "LASTUSAGE/Account/Adv2",
      "LASTUSAGE/Campaign/Display2",
      "LASTUSAGE/CCG/Display2",
      "LASTUSAGE/CC/Display2",
      ZERO_TIME,
      today_ - 31 * DAY,
      1, 1, 0
    },
    {
      "LASTUSAGE/Account/Adv1",
      "LASTUSAGE/Campaign/Text1",
      "LASTUSAGE/CCG/Text1",
      "LASTUSAGE/CC/Text1",
      ZERO_TIME,
      today_ - 30 * DAY,
      1, 0, 1
    },
    {
      "LASTUSAGE/Account/Adv1",
      "LASTUSAGE/Campaign/Display1,LASTUSAGE/Campaign/Text1",
      "LASTUSAGE/CCG/Display1,LASTUSAGE/CCG/Text1",
      "LASTUSAGE/CC/Display1,LASTUSAGE/CC/Text1",
      today_ - 31 * DAY,
      today_,
      1, 1, 1
    },
    {
      "LASTUSAGE/Account/Adv2",
      "LASTUSAGE/Campaign/Display2",
      "LASTUSAGE/CCG/Display2",
      "LASTUSAGE/CC/Display2",
      today_ - 31 * DAY,
      today_,
      1, 1, 0
    },
    {
      "LASTUSAGE/Account/Adv1",
      "LASTUSAGE/Campaign/Text1",
      "LASTUSAGE/CCG/Text1",
      "LASTUSAGE/CC/Text1",
      today_ - 30 * DAY,
      today_,
      1, 0, 1
    }
  };
  
  initialize_all_stats_(description, EXPECTED);

  const SumExpected SUM_EXPECTED[] =
  {
    { 0, 0 },
    { 0, 0 }
  };
   
  initialize_sum_stats_(
    description,
    adv_sum_stats_,
    adv_sum_diffs_,
    "LASTUSAGE/Account/Agency",
    &ORM::AdvertiserUserStats::Key::adv_account_id,
    SUM_EXPECTED);

  
  
  const Request LAST_USAGE_REQUESTS_USER_1[] =
  {
    {"LASTUSAGE/KWD/Display1", "LASTUSAGE/Tag", -32 * DAY, "unit-test", "LASTUSAGE/CC/Display1"},
    {"LASTUSAGE/KWD/Text1", "LASTUSAGE/Tag", -32 * DAY, "unit-test", "LASTUSAGE/CC/Text1"},
    {"LASTUSAGE/KWD/Display1", "LASTUSAGE/Tag", 0, "unit-test", "LASTUSAGE/CC/Display1"},
    {"LASTUSAGE/KWD/Text1", "LASTUSAGE/Tag", 0, "unit-test", "LASTUSAGE/CC/Text1"}
  };

  const Request LAST_USAGE_REQUESTS_USER_2[] =
  {
    {"LASTUSAGE/KWD/Display2", "LASTUSAGE/Tag", -31 * DAY, "unit-test", "LASTUSAGE/CC/Display2"},
    {"LASTUSAGE/KWD/Display2", "LASTUSAGE/FreeTag", -DAY, "unit-test", 0},
    {"LASTUSAGE/KWD/Display2", "LASTUSAGE/Tag", 0, "unit-test", "LASTUSAGE/CC/Display2"},
  };

  const Request LAST_USAGE_REQUESTS_USER_3[] =
  {
    {"LASTUSAGE/KWD/Text1", "LASTUSAGE/Tag", -30 * DAY, "unit-test", "LASTUSAGE/CC/Text1"},
    {"LASTUSAGE/KWD/Text1", "LASTUSAGE/Tag", 0, "unit-test", "LASTUSAGE/CC/Text1"}
  };

  {
    AdClient client(AdClient::create_user(this));
    process_requests_(
      client,
      description + " User#1.",
      LAST_USAGE_REQUESTS_USER_1,
      today_);
  }
  {
    AdClient client(AdClient::create_user(this));
    process_requests_(
      client,
      description + " User#2.",
      LAST_USAGE_REQUESTS_USER_2,
      today_);
  }
  {
    AdClient client(AdClient::create_user(this));
    process_requests_(
      client,
      description + " User#3.",
      LAST_USAGE_REQUESTS_USER_3,
      today_);
  }
}

void
AdvertiserUserStatsTest::timezones_()
{
  std::string description("Non-GMT timezone.");
  add_descr_phrase(description);

  AutoTest::Time base_time(
    today_.get_gm_time().get_date());

  const Expected EXPECTED[] =
  {
    {
      "TZ/Account/Adv1",
      "TZ/Campaign/1",
      "TZ/CCG/1",
      "TZ/CC/1",
      ZERO_TIME,
      today_ +  DAY,
      1, 0, 1
    },
    {
      "TZ/Account/Adv2",
      "TZ/Campaign/2",
      "TZ/CCG/2",
      "TZ/CC/2",
      ZERO_TIME,
      today_,
      1, 1, 0
    },
    {
      "TZ/Account/Adv2",
      "TZ/Campaign/2",
      "TZ/CCG/2",
      "TZ/CC/2",
      today_,
      today_ +  DAY,
      1, 1, 0
    }
  };

  initialize_all_stats_(description, EXPECTED);

  const SumExpected SUM_EXPECTED[] =
  {
    { 0, 0 },
    { 0, 0 }
  };
   
  initialize_sum_stats_(
    description,
    adv_sum_stats_,
    adv_sum_diffs_,
    "TZ/Account/Agency",
    &ORM::AdvertiserUserStats::Key::adv_account_id,
    SUM_EXPECTED);  

  const Request TZ_REQUESTS[] =
  {
    // Today 16:00
    {"TZ/KWD/1", "TZ/Tag", 16 * 60 * 60, "unit-test", "TZ/CC/1"},
    // Tomorrow 01:00
    {"TZ/KWD/2", "TZ/Tag", 25 * 60 * 60, "unit-test", "TZ/CC/2"},
    // Tomorrow 03:30
    {"TZ/KWD/2", "TZ/Tag", 27 * 60 * 60 + 30 * 60, "unit-test", "TZ/CC/2"}
  };

  AdClient client(AdClient::create_user(this));
  process_requests_(client, description, TZ_REQUESTS, base_time);
  
}

AdvertiserUserStatsTest::AdClient
AdvertiserUserStatsTest::async_part_1_()
{
  std::string description("Asynchronous logging#1.");
  add_descr_phrase(description);

  AutoTest::Time base_time(
    today_.get_gm_time().get_date());

  const Expected EXPECTED[] =
  {
    {
      "ASYNC/Account/Adv1",
      "ASYNC/Campaign/1",
      "ASYNC/CCG/1",
      "ASYNC/CC/1",
      ZERO_TIME,
      today_,
      1, 1, 0
    },
    {
      "ASYNC/Account/Adv2",
      "ASYNC/Campaign/2",
      "ASYNC/CCG/2",
      "ASYNC/CC/2",
      ZERO_TIME,
      today_,
      1, 0, 1
    }
  };

  initialize_all_stats_(description, EXPECTED);

  const SumExpected SUM_EXPECTED[] =
  {
    { 0, 0 }
  };
   
  initialize_sum_stats_(
    description,
    adv_sum_stats_,
    adv_sum_diffs_,
    "ASYNC/Account/Agency",
    &ORM::AdvertiserUserStats::Key::adv_account_id,
    SUM_EXPECTED);

  const Request ASYNC_REQUESTS[] =
  {
    {"ASYNC/KWD/1", "ASYNC/Tag", 60, "unit-test", "ASYNC/CC/1"},
    {"ASYNC/KWD/2", "ASYNC/Tag", 65, "unit-test", "ASYNC/CC/2"}
  };

  AdClient client(AdClient::create_user(this));
  process_requests_(
    client, description,
    ASYNC_REQUESTS, base_time);

  return client;
}


void
AdvertiserUserStatsTest::async_part_2_(
  AdClient& client)
{
  std::string description("Asynchronous logging#2.");
  add_descr_phrase(description);

  AutoTest::Time base_time(
    today_.get_gm_time().get_date());

  const Expected EXPECTED[] =
  {
    {
      "ASYNC/Account/Adv1",
      "ASYNC/Campaign/1",
      "ASYNC/CCG/1",
      "ASYNC/CC/1",
      ZERO_TIME,
      today_ - DAY,
      1, 1, 0
    },
    {
      "ASYNC/Account/Adv1",
      "ASYNC/Campaign/1",
      "ASYNC/CCG/1",
      "ASYNC/CC/1",
      ZERO_TIME,
      today_,
      -1, -1, 0
    },
    {
      "ASYNC/Account/Adv1",
      "ASYNC/Campaign/1",
      "ASYNC/CCG/1",
      "ASYNC/CC/1",
      today_ - DAY,
      today_,
      1, 1, 0
    },
    {
      "ASYNC/Account/Adv2",
      "ASYNC/Campaign/2",
      "ASYNC/CCG/2",
      "ASYNC/CC/2",
      ZERO_TIME,
      today_ - DAY,
      1, 0, 1
    },
    {
      "ASYNC/Account/Adv2",
      "ASYNC/Campaign/2",
      "ASYNC/CCG/2",
      "ASYNC/CC/2",
      ZERO_TIME,
      today_,
      -1, 0, -1
    },
    {
      "ASYNC/Account/Adv2",
      "ASYNC/Campaign/2",
      "ASYNC/CCG/2",
      "ASYNC/CC/2",
      today_ - DAY,
      today_,
      1, 0, 1
    }
  };

  initialize_all_stats_(description, EXPECTED);

  const SumExpected SUM_EXPECTED[] =
  {
    { 0, 0 }
  };
   
  initialize_sum_stats_(
    description,
    adv_sum_stats_,
    adv_sum_diffs_,
    "ASYNC/Account/Agency",
    &ORM::AdvertiserUserStats::Key::adv_account_id,
    SUM_EXPECTED);

  const Request ASYNC_REQUESTS[] =
  {
    // Yesterday 23:59:30
    {"ASYNC/KWD/1", "ASYNC/Tag", -30, "unit-test", "ASYNC/CC/1"},
    // Yesterday 23:59:35
    {"ASYNC/KWD/2", "ASYNC/Tag", -25, "unit-test", "ASYNC/CC/2"},
  };

  process_requests_(
    client, description,
    ASYNC_REQUESTS, base_time);
}

void
AdvertiserUserStatsTest::async_part_3_(
  AdClient& client)
{
  std::string description("Asynchronous logging#3.");
  add_descr_phrase(description);

  AutoTest::Time base_time(
    today_.get_gm_time().get_date());

  const Expected EXPECTED[] =
  {
    {
      "ASYNC/Account/Adv3",
      "ASYNC/Campaign/3",
      "ASYNC/CCG/3",
      "ASYNC/CC/3",
      ZERO_TIME,
      today_,
      1, 1, 0
    },
  };

  initialize_all_stats_(description, EXPECTED);

  const SumExpected SUM_EXPECTED[] =
  {
    { 0, 0 },
    { 0, 0 }
  };
   
  initialize_sum_stats_(
    description,
    adv_sum_stats_,
    adv_sum_diffs_,
    "ASYNC/Account/Adv",
    &ORM::AdvertiserUserStats::Key::adv_account_id,
    SUM_EXPECTED);

  initialize_sum_stats_(
    description,
    cmp_sum_stats_,
    cmp_sum_diffs_,
    "ASYNC/Campaign/",
    &ORM::CampaignUserStats::Key::campaign_id,
    SUM_EXPECTED);

  initialize_sum_stats_(
    description,
    ccg_sum_stats_,
    ccg_sum_diffs_,
    "ASYNC/CCG/",
    &ORM::CCGUserStats::Key::ccg_id,
    SUM_EXPECTED);
  
  initialize_sum_stats_(
    description,
    cc_sum_stats_,
    cc_sum_diffs_,
    "ASYNC/CC/",
    &ORM::CCUserStats::Key::cc_id,
    SUM_EXPECTED);


  const Request REQUESTS[] =
  {
    // Today 00:00:30   
    {"ASYNC/KWD/1", "ASYNC/Tag", 30, "unit-test", "ASYNC/CC/1"},
    // Today 00:00:40
    {"ASYNC/KWD/2", "ASYNC/Tag", 40, "unit-test", "ASYNC/CC/2"},
    // Today 00:00:30
    {"ASYNC/KWD/3", "ASYNC/Tag", 30, "unit-test", "ASYNC/CC/3"}
  };

  process_requests_(
    client, description,
    REQUESTS, base_time);
  
}


AdvertiserUserStatsTest::AdClient
AdvertiserUserStatsTest::big_date_diff_part_1_()
{
  std::string description("Big date difference#1.");
  add_descr_phrase(description);

  const Expected EXPECTED[] =
  {
    {
      "BIGDIFF/Account/Adv1",
      "BIGDIFF/Campaign/1",
      "BIGDIFF/CCG/1",
      "BIGDIFF/CC/1",
      ZERO_TIME,
      today_ - 60 * DAY,
      1, 1, 0
    },
    {
      "BIGDIFF/Account/Adv1",
      "BIGDIFF/Campaign/1",
      "BIGDIFF/CCG/1",
      "BIGDIFF/CC/1",
      today_ - 60 * DAY,
      today_ - 40 * DAY,
      1, 1, 0
    },
    {
      "BIGDIFF/Account/Adv1",
      "BIGDIFF/Campaign/1",
      "BIGDIFF/CCG/1",
      "BIGDIFF/CC/1",
      today_ - 31 * DAY,
      today_ ,
      1, 1, 0
    }
  };

  initialize_all_stats_(description, EXPECTED);
  
  const Request BIG_DIFF_REQUESTS_1[] =
  {
    {"BIGDIFF/KWD/1", "BIGDIFF/Tag", -60 * DAY, "unit-test", "BIGDIFF/CC/1"},
    {"BIGDIFF/KWD/1", "BIGDIFF/Tag", -40 * DAY, "unit-test", "BIGDIFF/CC/1"},
    {"BIGDIFF/KWD/1", "BIGDIFF/Tag", 0, "unit-test", "BIGDIFF/CC/1"}
  };
  
  AdClient client(AdClient::create_user(this));
  process_requests_(client, description, BIG_DIFF_REQUESTS_1, today_);

  return client;
}

void
AdvertiserUserStatsTest::big_date_diff_part_2_(
  AdClient& client)
{
  std::string description("Big date difference#2.");
  add_descr_phrase(description);

  const Expected EXPECTED[] =
  {
    {
      "BIGDIFF/Account/Adv1",
      "BIGDIFF/Campaign/1",
      "BIGDIFF/CCG/1",
      "BIGDIFF/CC/1",
      today_ - 31 * DAY,
      today_ ,
      -1, -1, 0
    },
    {
      "BIGDIFF/Account/Adv1",
      "BIGDIFF/Campaign/1",
      "BIGDIFF/CCG/1",
      "BIGDIFF/CC/1",
      today_ - 40 * DAY,
      today_ - 9 * DAY,
      1, 1, 0
    },
    {
      "BIGDIFF/Account/Adv1",
      "BIGDIFF/Campaign/1",
      "BIGDIFF/CCG/1",
      "BIGDIFF/CC/1",
      today_ - 9 * DAY,
      today_,
      1, 1, 0
    }
  };

  initialize_all_stats_(description, EXPECTED);

  const Request BIG_DIFF_REQUESTS_2[] =
  {
    {"BIGDIFF/KWD/1", "BIGDIFF/Tag", -9 * DAY, "unit-test", "BIGDIFF/CC/1"}
  };
  
  process_requests_(client, description, BIG_DIFF_REQUESTS_2, today_);
}


void
AdvertiserUserStatsTest::colo_()
{
  std::string description("Colo logging test.");
  add_descr_phrase(description);

  bool remote_2 =
    get_config().check_service(CTE_REMOTE2, STE_FRONTEND);

  AutoTest::Time base_time(
   today_.get_gm_time().get_date());

  const Expected EXPECTED[] =
  {
    {
      0,
      "COLO/Campaign/1",
      "COLO/CCG/1",
      "COLO/CC/1",
      ZERO_TIME,
      today_,
      1, 1, 0
    }
  };

  initialize_all_stats_(
    description,
    EXPECTED,
    remote_2?
      fetch_int("REMOTE_COLO"): fetch_int("DEFAULT_COLO"));

 
  const SumExpected SUM_EXPECTED[] =
  {
    {
      1,
      sum_change(today_, Generics::Time::ZERO, 1)
    }
  };

  initialize_sum_stats_(
    description,
    cmp_sum_stats_,
    cmp_sum_diffs_,
    "COLO/Campaign/",
    &ORM::CampaignUserStats::Key::campaign_id,
    SUM_EXPECTED);
  
  initialize_sum_stats_(
    description,
    ccg_sum_stats_,
    ccg_sum_diffs_,
    "COLO/CCG/",
    &ORM::CCGUserStats::Key::ccg_id,
    SUM_EXPECTED);
  
  initialize_sum_stats_(
    description,
    cc_sum_stats_,
    cc_sum_diffs_,
    "COLO/CC/",
    &ORM::CCUserStats::Key::cc_id,
    SUM_EXPECTED);

  const Request REQUESTS[] =
  {
    {"COLO/KWD/1", "COLO/Tag", 12*60*60, "unit-test", "COLO/CC/1"},
  };

  
  AdClient client(
    AdClient::create_user(
      this,
      remote_2? AutoTest::UF_FRONTEND_MINOR: 0));
  
  process_requests_(
    client,
    description,
    REQUESTS,
    base_time,
    fetch_int("ADS_COLO"));
}

void
AdvertiserUserStatsTest::temporary_user_()
{
  std::string description("Temporary user.");
  add_descr_phrase(description);

  const Expected EXPECTED[] =
  {
    {
      "TEMP/Account/Adv1",
      "TEMP/Campaign/1",
      "TEMP/CCG/1",
      "TEMP/CC/1",
      ZERO_TIME,
      today_ + DAY,
      1, 0, 1
    },
    {
      "TEMP/Account/Adv2",
      "TEMP/Campaign/2",
      "TEMP/CCG/2",
      "TEMP/CC/2",
      ZERO_TIME,
      today_ + DAY,
      1, 1, 0
    }
  };

  initialize_all_stats_(description, EXPECTED);

  const SumExpected SUM_EXPECTED[] =
  {
    { 0, 0 }
  };
   
  initialize_sum_stats_(
    description,
    adv_sum_stats_,
    adv_sum_diffs_,
    "TEMP/Account/Agency",
    &ORM::AdvertiserUserStats::Key::adv_account_id,
    SUM_EXPECTED);


  TemporaryAdClient temporary(TemporaryAdClient::create_user(this));

  temporary.process_request(
    NSLookupRequest().
      referer_kw(fetch_string("TEMP/KWD/1")).
      debug_time(today_));

  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      "TEMP/Channel/1",
      temporary.debug_info.history_channels).check(),
    description +
        " history_channels check");

  const Request BEFORE_MERGING[] =
  {
    {"TEMP/KWD/2", "TEMP/Tag", 0, "unit-test", "TEMP/CC/2"}
  };

  const Request AFTER_MERGING[] =
  {
    {0, "TEMP/Tag", 0, "unit-test", "TEMP/CC/1"}
  };
    
  AdClient persistent(AdClient::create_user(this));
  process_requests_(
    persistent, description + " Before merging.",
    BEFORE_MERGING, today_ + DAY);
  persistent.merge(temporary);
  process_requests_(
    persistent, description + " After merging",
    AFTER_MERGING, today_ + DAY);
}

void
AdvertiserUserStatsTest::optout_users_()
{
  std::string description("Optout users.");
  add_descr_phrase(description);

  const SumExpected SUM_EXPECTED[] =
  {
    { 0, 0 },
    { 0, 0 }
  };

  initialize_sum_stats_(
    description,
    adv_sum_stats_,
    adv_sum_diffs_,
    "OPTOUT/Account/Agency",
    &ORM::AdvertiserUserStats::Key::adv_account_id,
    SUM_EXPECTED);
  
  initialize_sum_stats_(
    description,
    adv_sum_stats_,
    adv_sum_diffs_,
    "OPTOUT/Account/Adv",
    &ORM::AdvertiserUserStats::Key::adv_account_id,
    SUM_EXPECTED);
  
  initialize_sum_stats_(
    description,
    cmp_sum_stats_,
    cmp_sum_diffs_,
    "OPTOUT/Campaign/",
    &ORM::CampaignUserStats::Key::campaign_id,
    SUM_EXPECTED);
  
  initialize_sum_stats_(
    description,
    ccg_sum_stats_,
    ccg_sum_diffs_,
    "OPTOUT/CCG/",
    &ORM::CCGUserStats::Key::ccg_id,
    SUM_EXPECTED);
  
  initialize_sum_stats_(
    description,
    cc_sum_stats_,
    cc_sum_diffs_,
    "OPTOUT/CC/",
    &ORM::CCUserStats::Key::cc_id,
    SUM_EXPECTED);

  const Request OPTOUT_REQUEST[] =
  {
    {"OPTOUT/KWD/1", "OPTOUT/Tag", 0, "unit-test", "OPTOUT/CC/1"},
    {"OPTOUT/KWD/2", "OPTOUT/Tag", 0, "unit-test", "OPTOUT/CC/2"}
  };

  AdClient users[] =
  {
    AdClient::create_user(this),
    AdClient::create_nonoptin_user(this),
    AdClient::create_optout_user(this)
  };

  users[0].set_probe_uid();

  for (size_t i = 0; i < countof(users); ++i)
  {
    process_requests_(
      users[i], description + "#" + strof(i+1),
      OPTOUT_REQUEST, today_, fetch_int("ADS_COLO"));

    if (
      get_config().check_service(CTE_CENTRAL, STE_FRONTEND) &&
      get_config().check_service(CTE_ALL_REMOTE, STE_FRONTEND))
    {
      users[i].change_base_url(
        get_config().get_service(CTE_CENTRAL, STE_FRONTEND).address.c_str());
      process_requests_(
        users[i], description + "#" + strof(i+1),
        OPTOUT_REQUEST, today_, fetch_int("ADS_COLO"));
    }
  }
}
