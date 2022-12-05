
#include "ChannelOverlapStats.hpp"

REFLECT_UNIT(ChannelOverlapStats) (
  "Statistics",
  AUTO_TEST_SLOW);

namespace
{
  typedef AutoTest::NSLookupRequest  NSLookupRequest;
  typedef AutoTest::ChannelsCheck ChannelsCheck;
  typedef AutoTest::SimpleChannelChecker SimpleChannelChecker;

  struct ChannelStatus
  {
    const char* channel;
    char status;
  };
}

bool
ChannelOverlapStats::run_test()
{

  add_descr_phrase("Setup.");

  // Today statistics

  daily_proc_case();
  non_optin_case();
  config_case();
  
  flags_case();
  
  // country_case();  // until ADSC-8125
  channel_type_case();
  expression_case();
  status_case();
  
  AdClient client(AdClient::create_user(this));
  now_ =
    AutoTest::Time(
      now_.get_gm_time().format("%d-%m-%Y:02-00-00"));
  history_today_case(client);

  check_stats("ChannelOverlapUserStats checks (today).");

  // Tomorrow (02:00:00) statistics
  now_ =
    AutoTest::Time(
      (now_ + Generics::Time::ONE_DAY).
        get_gm_time().format("%d-%m-%Y:02-00-00"));

  history_tomorrow_case(client);

  check_stats("ChannelOverlapUserStats checks (tomorrow).");

  return true;
}

template<size_t Count>
void
ChannelOverlapStats::send_requests(
  const std::string& description,
  AdClient* client,
  const Request(&requests) [Count])
{
  for (size_t i = 0;
       i < Count; ++i)
  {

    AutoTest::NSLookupRequest request(false);

    if (requests[i].kwds)
    {
      request.referer_kw = map_objects(requests[i].kwds);
    }

    request.debug_time = now_ + requests[i].time_ofset;
    
    client->process_request(request);

    FAIL_CONTEXT(
      ChannelsCheck(
        this, requests[i].expected,
        client->debug_info.history_channels).check(),
      description +
        " Expected history#" + strof(i+1));

    FAIL_CONTEXT(
      ChannelsCheck(
        this, requests[i].unexpected,
        client->debug_info.history_channels,
        AutoTest::SCE_NOT_ENTRY).check(),
      description +
        " Unexpected history#" + strof(i+1));
  }
}


void
ChannelOverlapStats::add_stat(
  const std::string& description,
  const char* channel1,
  const char* channel2,
  unsigned long users,
  unsigned long time_ofset)
{
    Stat::Key key;
    if (channel1)
    {
      key.channel1(fetch_int(channel1));
    }
    if (channel2)
    {    
      key.channel2(fetch_int(channel2));
    }
    key.sdate(now_ + time_ofset);
      
    Stat stat(key);

    stat.description(
      description.c_str());

    stat.select(conn_);

    Diff diff(users);
    
    if (users)
    {
      stats_.push_front(stat);
      diffs_.push_front(diff);
    }
    else
    {
      stats_.push_back(stat);
      diffs_.push_back(diff);
    }
}

template<size_t Count>
void
ChannelOverlapStats::add_stats(
  const std::string& description,
  const Expected(&expected) [Count],
  unsigned long time_ofset)
{
  for (size_t i = 0;
       i < Count; ++i)
  {
    add_stat(
      description + "#" + strof(i+1),
      expected[i].channel1,
      expected[i].channel2,
      expected[i].users,
      time_ofset);
  }
}

void
ChannelOverlapStats::check_stats(const std::string& description)
{
  add_descr_phrase(description);

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        conn_,
        diffs_,
        stats_)).check(),
    description);

  stats_.clear();
  diffs_.clear();
  
}


void
ChannelOverlapStats::flags_case()
{
  std::string description("Flags case.");
  add_descr_phrase(description);

  const Expected EXPECTED[] =
  {
    // 1
    {
      "Flags/Channel/B1",
      "Flags/Channel/B2",
      0
    },
    // 2
    {
      "Flags/Channel/B1",
      "Flags/Channel/B3",
      1
    },
    // 3
    {
      "Flags/Channel/B1",
      "Flags/Expr/E1",
      0
    },
    // 4
    {
      "Flags/Channel/B1",
      "Flags/Expr/E2",
      1
    },
    // 5
    {
      "Flags/Channel/B2",
      0,
      0
    },
    // 6
    {
      "Flags/Channel/B3",
      "Flags/Expr/E1",
      0
    },
    // 7
    {
      "Flags/Channel/B3",
      "Flags/Expr/E2",
      1
    },
    // 8
    {
      "Flags/Expr/E1",
      "Flags/Expr/E2",
      0
    }
  };

  add_stats(description, EXPECTED);
  

  const Request REQUESTS[] =
  {
    {
      "Flags/KWD/Kwd1,Flags/KWD/Kwd2",
      "Flags/Channel/B1,Flags/Channel/B2,Flags/Channel/B3",
      0,
      0
    }
  };

  AdClient client(AdClient::create_user(this));

  send_requests(description, &client, REQUESTS);
   
}

void
ChannelOverlapStats::country_case()
{
  std::string description("Country case.");
  add_descr_phrase(description);

  const Expected EXPECTED[] =
  {
    // 1
    {
      0,
      "Country/Channel/B2",
      0
    },
    // 2
    {
      "Country/Channel/B1",
      "Country/Channel/B3",
      1
    },
    // 3
    {
      "Country/Channel/B2",
      0,
      0
    }
  };

  add_stats(description, EXPECTED);

  const Request REQUESTS[] =
  {
    {
      "Country/KWD/Kwd",
      "Country/Channel/B1,Country/Channel/B2,Country/Channel/B3",
      0,
      0
    }
  };

  AdClient client(AdClient::create_user(this));

  send_requests(description, &client, REQUESTS);
}

void
ChannelOverlapStats::channel_type_case()
{
  std::string description("Channel type case.");
  add_descr_phrase(description);

  const Expected EXPECTED[] =
  {
    // 1
    {
      "ChannelType/Channel/Keyword",
      0,
      0
    },
    // 2
    {
      "ChannelType/Channel/Behavioral1",
      "ChannelType/Channel/Behavioral2",
      1
    },
    // 3
    {
      "ChannelType/Channel/Behavioral1",
      "ChannelType/Expr/EXPR",
      1
    },
    // 4
    {
      "ChannelType/Channel/Behavioral1",
      0,
      2
    },
    // 5
    {
      "ChannelType/Channel/Behavioral2",
      "ChannelType/Expr/EXPR",
      1
    },
    // 6
    {
      "ChannelType/Channel/Behavioral2",
      0,
      1
    },
    // 7
    {
      "ChannelType/Device",
      0,
      0
    },
    // 8
    {
      "ChannelType/Channel/Guinea",
      0,
      0
    },
    // 9
    {
      "ChannelType/Channel/Mamou",
      0,
      0
    },
    // 10
    {
      "ChannelType/Expr/EXPR",
      0,
      0
    },
    // 11
    {
      "ChannelType/Targeting/A1",
      0,
      0
    }
  };

  add_stats(description, EXPECTED);

  AdClient client(AdClient::create_user(this));

  NSLookupRequest request;
  
  request.referer_kw =
    fetch_string("ChannelType/KWD/Kwd");
  request.debug_time = now_;
  request.loc_name =
    "gn/" + fetch_string("ChannelType/LOC/Guinea") +
      "/" + fetch_string("ChannelType/LOC/Mamou");

  client.process_request(request);
  
  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      "ChannelType/Channel/Behavioral1,"
      "ChannelType/Channel/Behavioral2,"
      "ChannelType/Channel/Keyword",
      client.debug_info.history_channels).check(),
    description +
        " Expected history");

   FAIL_CONTEXT(
    ChannelsCheck(
      this,
      "ChannelType/Device",
      client.debug_info.device_channels).check(),
    description +
        " Expected history");

  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      "ChannelType/Channel/Guinea,"
      "ChannelType/Channel/Mamou",
      client.debug_info.geo_channels).check(),
    description +
        " Expected geo_channels");

}

void ChannelOverlapStats::non_optin_case()
{
  std::string description("Non optin case.");
  add_descr_phrase(description);

  const Expected EXPECTED[] =
  {
    // 1
    {
      "NonOptin/Channel/B1",
      "NonOptin/Channel/B2",
      0
    }
  };

  add_stats(description, EXPECTED);

  AdClient CLIENTS[] =
  {
    AdClient::create_user(this),
    AdClient::create_nonoptin_user(this),
    AdClient::create_optout_user(this)
  };

  CLIENTS[0].set_probe_uid();

  for (size_t i = 0; i < countof(CLIENTS); ++i)
  {
    NSLookupRequest request;
  
    request.referer =
      fetch_string("NonOptin/URL/Url");
    request.debug_time = now_;
    request.colo = fetch_string("ADSCOLO");

    CLIENTS[i].process_request(request);
  
    FAIL_CONTEXT(
      ChannelsCheck(
        this,
        "NonOptin/Channel/B1,"
        "NonOptin/Channel/B2",
        CLIENTS[i].debug_info.history_channels).check(),
      description +
        " Expected history#" + strof(i+1));

    if (
      get_config().check_service(CTE_CENTRAL, STE_FRONTEND) &&
      get_config().check_service(CTE_ALL_REMOTE, STE_FRONTEND))
    {
      CLIENTS[i].change_base_url(
        get_config().get_service(CTE_CENTRAL, STE_FRONTEND).address.c_str());

      CLIENTS[i].process_request(request);

      FAIL_CONTEXT(
        ChannelsCheck(
          this,
          "NonOptin/Channel/B1,"
          "NonOptin/Channel/B2",
          CLIENTS[i].debug_info.history_channels).check(),
        description +
            " Expected history (central)#" + strof(i+1));
    }
  }
}

void ChannelOverlapStats::config_case()
{
  if (get_config().check_service(CTE_REMOTE2, STE_FRONTEND))
  {
    std::string description("Config case.");
    add_descr_phrase(description);
    
    const Expected EXPECTED[] =
    {
      // 1
      {
        "Config/Channel/B1",
        "Config/Channel/B2",
        0
      }
    };
    
    add_stats(description, EXPECTED);

    AdClient client(
      AdClient::create_user(
        this, AutoTest::UF_FRONTEND_MINOR));

    client.process_request(
      NSLookupRequest().
        referer_kw(fetch_string("Config/KWD/Kwd")).
        debug_time(now_));

      
    FAIL_CONTEXT(
      ChannelsCheck(
        this,
        "Config/Channel/B1,"
        "Config/Channel/B2",
        client.debug_info.history_channels).check(),
      description + " Expected history");
  }
}

void
ChannelOverlapStats::expression_case()
{
  std::string description("Expression case.");
  add_descr_phrase(description);

  const Expected EXPECTED[] =
  {
    // 1
    {
      "Expression/Channel/Channel1",
      "Expression/Channel/Channel2",      
      0
    },
    // 2
    {
      "Expression/Channel/Channel1",
      "Expression/Expr/E1",      
      1
    },
    // 3
    {
      "Expression/Channel/Channel1",
      "Expression/Expr/E2",      
      1
    },
    // 4
    {
      "Expression/Channel/Channel1",
      "Expression/Expr/E3",      
      1
    },
    // 5
    {
      "Expression/Channel/Channel1",
      "Expression/Expr/E5",      
      1
    },
    // 6
    {
      "Expression/Channel/Channel2",
      0,
      0
    },
    // 7
    {
      "Expression/Expr/E1",
      "Expression/Expr/E2",
      1
    },
    // 8
    {
      "Expression/Expr/E1",
      "Expression/Expr/E3",
      1
    },
    // 9
    {
      "Expression/Expr/E1",
      "Expression/Expr/E5",
      1
    },
    // 10
    {
      "Expression/Expr/E2",
      "Expression/Expr/E3",
      1
    },
    // 11
    {
      "Expression/Expr/E2",
      "Expression/Expr/E5",
      1
    },
    // 12
    {
      "Expression/Expr/E3",
      "Expression/Expr/E5",
      1
    },
    // 13
    {
      "Expression/Expr/E4",
      0,
      0
    },
    // 14
    {
      0,
      "Expression/Expr/E4",
      0
    }
  };

  add_stats(description, EXPECTED);

  const Request REQUESTS[] =
  {
    {
      "Expression/KWD/Kwd1",
      "Expression/Channel/Channel1",
      "Expression/Channel/Channel2",
      0
    }
  };

  AdClient client(AdClient::create_user(this));
  send_requests(description, &client, REQUESTS);
  
}

void ChannelOverlapStats::history_today_case(AdClient& client)
{
  std::string description("History case (today).");
  add_descr_phrase(description);


  const Expected EXPECTED[] =
  {
    // 1
    {
      "History/Channel/Session1",
      "History/Channel/HT",
      1
    },
    // 2
    {
      "History/Channel/Session1",
      "History/Channel/Session2",
      1
    },
    // 3
    {
      "History/Channel/HT",
      "History/Channel/Session2",
      1
    },
    // 4
    {
      0,
      "History/Channel/History",
      0
    }

  };

  add_stats(description, EXPECTED);

  const Request REQUESTS[] =
  {
    {
      "History/KWD/Kwd1,"
      "History/KWD/Kwd2,"
      "History/KWD/Kwd3,"
      "History/KWD/Kwd4",
      "History/Channel/Session1,"
      "History/Channel/HT",
      "History/Channel/Session2,"
      "History/Channel/History",
      1
    },
    {
      "History/KWD/Kwd3",
      "History/Channel/Session1,"
      "History/Channel/HT,"
      "History/Channel/Session2",
      "History/Channel/History",
      2
    }
    
  };

  send_requests(description, &client, REQUESTS);

}

void ChannelOverlapStats::status_case()
{
  std::string description("Status case.");
  add_descr_phrase(description);

  const ChannelStatus CHANNEL_STATUS[] =
  {
    { "Status/Channel/Channel1", 'A' },
    { "Status/Channel/Channel2", 'A' },
    { "Status/Channel/Channel3", 'W' }
  };

  for (size_t i = 0; i < countof(CHANNEL_STATUS); ++i)
  {
    FAIL_CONTEXT(
      SimpleChannelChecker(
        this,
        fetch_int(CHANNEL_STATUS[i].channel),
        SimpleChannelChecker::Expected().
          status(std::string(1, CHANNEL_STATUS[i].status))).check(),
      description +
        "Channel#" + strof(i+1) + " status check");
  }

  const Expected EXPECTED[] =
  {
    // 1
    {
      "Status/Channel/Channel1",
      "Status/Channel/Channel2",
      1
    },
    // 2
    {
      "Status/Channel/Channel1",
      "Status/Channel/Channel3",
      1
    },
    // 3
    {
      "Status/Channel/Channel2",
      "Status/Channel/Channel3",
      1
    }
  };

  add_stats(description, EXPECTED);

  const Request REQUESTS[] =
  {
    {
      "Status/KWD/Kwd",
      "Status/Channel/Channel1,"
      "Status/Channel/Channel2,"
      "Status/Channel/Channel3",
      0,
      0
    }
  };

  AdClient client(AdClient::create_user(this));
  send_requests(description, &client, REQUESTS);
}

void ChannelOverlapStats::history_tomorrow_case(AdClient& client)
{
  std::string description("History case (tomorrow).");
  add_descr_phrase(description);

  {
    // Today stats
    const Expected EXPECTED[] =
    {
      // 1
      {
        "History/Channel/Session1",
        0,
        0
      },
      // 2
      {
        "History/Channel/HT",
        0,
        0
      },
      // 3
      {
        "History/Channel/Session2",
        0,
        0
      }
    };

    add_stats(description, EXPECTED, -24*60*60);
  }
  
  {
    // Tomorrow stats
    const Expected EXPECTED[] =
    {
      // 1
      {
        "History/Channel/Session1",
        0,
        0
      },
      // 2
      {
        "History/Channel/HT",
        "History/Channel/Session2",
        1
      },
      // 3
      {
        "History/Channel/HT",
        "History/Channel/History",
        1
      },
      // 4
      {
        "History/Channel/Session2",
        "History/Channel/History",
        1
      }
    };

    add_stats(description, EXPECTED);
  }

  const Request REQUESTS[] =
  {
    {
      "History/KWD/Kwd1,"
      "History/KWD/Kwd2,"
      "History/KWD/Kwd3",
      "History/Channel/Session1,"
      "History/Channel/Session2,"
      "History/Channel/HT",
      "History/Channel/History",
      -24*60*60  + 3
    },
    {
      0,
      "History/Channel/HT,"
      "History/Channel/Session2,"
      "History/Channel/History",
      "History/Channel/Session1",
      0
    }
  };

  send_requests(description, &client, REQUESTS); 
}

void ChannelOverlapStats::daily_proc_case()
{
  std::string description("Daily processing.");
  add_descr_phrase(description);

  add_stat(
    description + " Yesterday",
    "DailyProc/Channel/History1",
    "DailyProc/Channel/History2",
    0, -24 * 60 * 60);
  add_stat(
    description + " Today",
    "DailyProc/Channel/History1",
    "DailyProc/Channel/History2",
    0, 0);
  add_stat(
    description + " Tomorrow",
    "DailyProc/Channel/History1",
    "DailyProc/Channel/History2",
    0, 24 * 60 * 60);
  add_stat(
    description + " Day after tomorrow",
    "DailyProc/Channel/History1",
    "DailyProc/Channel/History2",
    1, 2 * 24 * 60 * 60);
  
  AdClient client(AdClient::create_user(this));
 
  {
    // Yesterday request
    
    const Request REQUESTS[] =
    {
      {
        "DailyProc/KWD/Kwd",
        0,
        "DailyProc/Channel/History1,"
        "DailyProc/Channel/History2",
        -24 * 60 * 60
      }
    };
    
    send_requests(
      description, &client, REQUESTS);
  }

  // Daily processing

  AutoTest::DailyProcess::execute(this);

  {
    // Day after tomorrow request
   
    const Request REQUESTS[] =
    {
      {
        0,
        "DailyProc/Channel/History1,"
        "DailyProc/Channel/History2",
        0,
        2 * 24 * 60 * 60
      }
    };
    
    send_requests(
      description, &client, REQUESTS);   
  }
  
}

