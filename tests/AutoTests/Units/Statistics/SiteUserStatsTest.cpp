#include "SiteUserStatsTest.hpp"

REFLECT_UNIT(SiteUserStatsTest) (
  "Statistics",
  AUTO_TEST_SLOW);

namespace
{
  typedef AutoTest::TemporaryAdClient TemporaryAdClient;
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::ChannelsCheck ChannelsCheck;

  const unsigned int DAY = 24 * 60 * 60;
  const Generics::Time ZERO_TIME = Generics::Time::ZERO;

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

template <size_t Count>
void SiteUserStatsTest::add_stats_(
  const std::string& description,
  const Expected (&expected)[Count])
{
  std::map<unsigned long, Diff> sum_diffs;
  for (size_t i = 0; i < Count; ++i)
  {
    unsigned long site_id =
      fetch_int(expected[i].site);
    SiteUserStat stat;
    stat.key().
      site_id(site_id).
      colo_id(fetch_int(expected[i].colo)).
      last_appearance_date(expected[i].last_appearance).
      isp_sdate(expected[i].isp_sdate);
    stat.description(description + "#" + strof(i+1));
    stat.select(conn_);
    stats_.push_back(stat);
    diffs_.push_back(
      Diff().
        unique_users(expected[i].count));
    Diff& sum_diff =  sum_diffs[site_id];
    sum_diff.unique_users(
      (sum_diff.unique_users() == ORM::any_stats_diff?
        0: sum_diff.unique_users()) + expected[i].count);
    sum_diff.control_sum(
      (sum_diff.control_sum() == ORM::any_stats_diff?
        0: sum_diff.control_sum()) +
        sum_change(
          expected[i].isp_sdate,
          expected[i].last_appearance,
          expected[i].count));
  }

  std::map<unsigned long, Diff>::const_iterator it =
    sum_diffs.begin();

  for (size_t i = 0; it != sum_diffs.end(); ++it, ++i)
  {
    SiteUserStat stat;
    stat.key().
      site_id(it->first);
    stat.description(description + "#" + strof(i+1));
    stat.select(conn_);
    sum_stats_.push_back(stat);
    sum_diffs_.push_back(it->second);    
  }
}

template <size_t Count>
void SiteUserStatsTest::process_requests_(
  AdClient& client,
  const Request (&requests)[Count],
  const char* colo)
{
  for (size_t i = 0; i < Count; ++i)
  {
    client.process_request(
      NSLookupRequest().
        tid(fetch_int(requests[i].tag)).
        debug_time(requests[i].time).
        colo(colo? fetch_string(colo).c_str(): 0));
  }
}

void
SiteUserStatsTest::check_stats_()
{
  
  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        conn_, diffs_, stats_)).check(),
    "SiteUserStats check");

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        conn_, sum_diffs_, sum_stats_)).check(),
    "SiteUserStats sum and control check");

  diffs_.clear();
  stats_.clear();
  sum_diffs_.clear();
  sum_stats_.clear();
}


void
SiteUserStatsTest::unique_users_stats_()
{
  std::string description("Unique users stats.");
  add_descr_phrase(description);

  const Expected EXPECTED[] =
  {
    {
      "UNIQUE/SITE/1",
      "COLO",
      ZERO_TIME,
      base_time_ - DAY,
      1
    },
    {
      "UNIQUE/SITE/3",
      "COLO",
      ZERO_TIME,
      base_time_,
      1
    },
    {
      "UNIQUE/SITE/1",
      "COLO",
      base_time_ - DAY,
      base_time_ ,
      1
    }
  };

  add_stats_(description, EXPECTED);

  const Request REQUESTS[] =
  {
    { "UNIQUE/TID/1", base_time_ - DAY },
    { "UNIQUE/TID/1", base_time_ },
    { "UNIQUE/TID/2", base_time_ },
    { "UNIQUE/TID/3", base_time_ }
  };

  AdClient client(AdClient::create_user(this));

  process_requests_(client, REQUESTS);
}

void
SiteUserStatsTest::last_appearance_date_()
{
  std::string description("Last appearance date.");
  add_descr_phrase(description);

  const Expected EXPECTED[] =
  {
    {
      "LASTUSAGE/SITE/1",
      "COLO",
      ZERO_TIME,
      base_time_ - 32 * DAY,
      1
    },
    {
      "LASTUSAGE/SITE/2",
      "COLO",
      ZERO_TIME,
      base_time_ - 31 * DAY,
      1
    },
    {
      "LASTUSAGE/SITE/2",
      "COLO",
      ZERO_TIME,
      base_time_ - 30 * DAY,
      1
    },
    {
      "LASTUSAGE/SITE/1",
      "COLO",
      base_time_ - 31 * DAY,
      base_time_,
      1
    },
    {
      "LASTUSAGE/SITE/2",
      "COLO",
      base_time_ - 31 * DAY,
      base_time_,
      1
    },
    {
      "LASTUSAGE/SITE/2",
      "COLO",
      base_time_ - 30 * DAY,
      base_time_,
      1
    }
  };

  add_stats_(description, EXPECTED);
  
  {
    AdClient client(AdClient::create_user(this));

    const Request REQUESTS[] =
    {
      { "LASTUSAGE/TID/1", base_time_ - 32 * DAY },
      { "LASTUSAGE/TID/1", base_time_ }
    };

    process_requests_(client, REQUESTS);
  }
  {
    AdClient client(AdClient::create_user(this));

    const Request REQUESTS[] =
    {
      { "LASTUSAGE/TID/2", base_time_ - 31 * DAY },
      { "LASTUSAGE/TID/2", base_time_ }
    };

    process_requests_(client, REQUESTS);

  }
  {
    AdClient client(AdClient::create_user(this));

    const Request REQUESTS[] =
    {
      { "LASTUSAGE/TID/2", base_time_ - 30 * DAY },
      { "LASTUSAGE/TID/2", base_time_ }
    };

    process_requests_(client, REQUESTS);

  }

}

void
SiteUserStatsTest::non_gmt_timezone_()
{
  if (get_config().check_service(CTE_REMOTE2, STE_FRONTEND))
  {
    std::string description("Non-GMT timezone test.");
    add_descr_phrase(description);
    
    AutoTest::Time base_time(
      base_time_.get_gm_time().get_date());

    Generics::Time tz_ofset(
      AutoTest::ORM::get_tz_ofset(
        this,
        fetch_string("TZ")));

    const Expected EXPECTED[] =
      {
        {
          "TZ/SITE/1",
          "TZ_COLO",
          ZERO_TIME,
          base_time - DAY,
          1
        },
        {
          "TZ/SITE/1",
          "TZ_COLO",
          base_time - DAY,
          base_time,
          1
        },
        {
          "TZ/SITE/1",
          "TZ_COLO",
          base_time,
          base_time + DAY,
          1
        }
      };
    
    add_stats_(description, EXPECTED);
    
    AdClient client(
      AdClient::create_user(
        this, AutoTest::UF_FRONTEND_MINOR));
    
    const Request REQUESTS[] =
    {
      { "TZ/TID/1", base_time - DAY + 23 * 60 *60 - tz_ofset.tv_sec},
      { "TZ/TID/1", base_time - DAY + 24 * 60 *60 - tz_ofset.tv_sec},
      { "TZ/TID/1", base_time + 24 * 60 *60 - tz_ofset.tv_sec}
    };
    
    process_requests_(client, REQUESTS);
  }
}

void
SiteUserStatsTest::colo_logging_()
{
  std::string description("Colo logging test.");
  add_descr_phrase(description);

  bool remote_2 =
    get_config().check_service(CTE_REMOTE2, STE_FRONTEND);

  AutoTest::Time base_time(
    base_time_.get_gm_time().get_date());

  const Expected EXPECTED[] =
  {
    {
      "COLO/SITE/1",
      remote_2? "TZ_COLO": "COLO",
      ZERO_TIME,
      base_time_,
      1
    }
  };

  add_stats_(description, EXPECTED);

  AdClient client(
    AdClient::create_user(
      this,
      remote_2? AutoTest::UF_FRONTEND_MINOR: 0));

  const Request REQUESTS_1[] =
  {
    { "COLO/TID/1", base_time + 12*60*60}
  };

  process_requests_(client, REQUESTS_1, "ADS_COLO");

}

SiteUserStatsTest::AdClient
SiteUserStatsTest::async_part_1_()
{
  std::string description("Asynchronous logging (part#1).");
  add_descr_phrase(description);

  AutoTest::Time base_time(
    base_time_.get_gm_time().get_date());

  const Expected EXPECTED[] =
  {
    {
      "ASYNC/SITE/1",
      "COLO",
      ZERO_TIME,
      base_time,
      1
    }
  };

  add_stats_(description, EXPECTED);

  const Request REQUESTS[] =
  {
    { "ASYNC/TID/1", base_time + 60*60}
  };

  AdClient client(AdClient::create_user(this));

  process_requests_(client, REQUESTS);

  return client;
}

void
SiteUserStatsTest::async_part_2_(
  AdClient& client)
{
  std::string description("Asynchronous logging (part#2).");
  add_descr_phrase(description);

  AutoTest::Time base_time(
    base_time_.get_gm_time().get_date());

  const Expected EXPECTED[] =
  {
    {
      "ASYNC/SITE/1",
      "COLO",
      ZERO_TIME,
      base_time - DAY,
      1
    },
    {
      "ASYNC/SITE/1",
      "COLO",
      ZERO_TIME,
      base_time,
      -1
    },
    {
      "ASYNC/SITE/1",
      "COLO",
      base_time - DAY,
      base_time,
      1
    }
  };

  add_stats_(description, EXPECTED);

  const Request REQUESTS[] =
  {
    { "ASYNC/TID/1", base_time - 30}
  };

  process_requests_(client, REQUESTS);
}

void
SiteUserStatsTest::async_part_3_(
  AdClient& client)
{
  std::string description("Asynchronous logging (part#3).");
  add_descr_phrase(description);

  AutoTest::Time base_time(
    base_time_.get_gm_time().get_date());

  const Expected EXPECTED[] =
  {
    {
      "ASYNC/SITE/1",
      "COLO",
      base_time - DAY,
      base_time,
      0
    },
    {
      "ASYNC/SITE/2",
      "COLO",
      ZERO_TIME,
      base_time,
      1
    }
  };

  add_stats_(description, EXPECTED);

  const Request REQUESTS[] =
  {
    { "ASYNC/TID/1", base_time + 30},
    { "ASYNC/TID/2", base_time + 30}
  };

  process_requests_(client, REQUESTS);
}

void
SiteUserStatsTest::temporary_user_()
{
  std::string description("Temporary user test.");
  add_descr_phrase(description);

  const Expected EXPECTED[] =
  {
    {
      "TEMP/SITE/1",
      "COLO",
      ZERO_TIME,
      base_time_ + DAY,
      1
    }
  };

  add_stats_(description, EXPECTED);

  TemporaryAdClient temporary(
    TemporaryAdClient::create_user(this));
  AdClient persistent(
    AdClient::create_user(this));
  
  temporary.process_request(
    NSLookupRequest().
      referer_kw(fetch_string("KWD")).
      debug_time(base_time_));

  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      "CHANNEL",
      temporary.debug_info.history_channels).check(),
    description +
      " history_channels check (before merging)");

  persistent.merge(
    temporary,
     NSLookupRequest().
      debug_time(base_time_ + DAY).
      tid(fetch_int("TEMP/TID/1")));

  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      "CHANNEL",
      persistent.debug_info.history_channels).check(),
    description +
      " history_channels check (after merging)");

}

void
SiteUserStatsTest::non_opted_in_users_()
{
  std::string description("Non-opted-in users test.");
  add_descr_phrase(description);

  const Expected EXPECTED[] =
  {
    {
      "OPTOUT/SITE/1",
      "COLO",
      ZERO_TIME,
      base_time_,
      0
    }
  };
    
  add_stats_(description, EXPECTED);

  AdClient users[] =
  {
    AdClient::create_user(this),
    AdClient::create_nonoptin_user(this),
    AdClient::create_optout_user(this)
  };

  users[0].set_probe_uid();

  for (size_t i = 0; i < countof(users); ++i)
  {
    users[i].process_request(
      NSLookupRequest().
        debug_time(base_time_).
        tid(fetch_int("OPTOUT/TID/1")));
  }
}

bool
SiteUserStatsTest::run_test()
{
  non_opted_in_users_();
  unique_users_stats_();
  last_appearance_date_();
  non_gmt_timezone_();
  colo_logging_();
  AdClient client(async_part_1_());
  temporary_user_();

  add_descr_phrase("Check stats#1");
  check_stats_();

  async_part_2_(client);
  
  add_descr_phrase("Check stats#2");
  check_stats_();

  async_part_3_(client);
  
  add_descr_phrase("Check stats#3");
  check_stats_();

  return true;
}

