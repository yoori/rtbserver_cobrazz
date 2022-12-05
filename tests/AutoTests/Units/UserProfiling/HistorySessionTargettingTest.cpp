#include "Common.hpp"
#include "HistorySessionTargettingTest.hpp"

REFLECT_UNIT(HistorySessionTargettingTest) (
  "UserProfiling",
  AUTO_TEST_FAST
);

namespace {
  typedef AutoTest::NSLookupRequest  NSLookupRequest;
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::ChannelsCheck ChannelsCheck;

  // Session channel#1.
  const HistorySessionTargettingTest::RequestType SESSION_1[] =
  {
    {
      0,
      "Session1Keyword",
      {"Session1BP", 0},
      {0, "Session1Channel"},
      false
    },
    {
      8*60 - 2,
      "Session1Keyword",
      {"Session1BP", "Session1Channel"},
      {0, 0},
      false
    },
    {
      16*60,
      "Session1Keyword",
      {"Session1BP", 0},
      {0, "Session1Channel"},
      false
    }
  };

  // Session channel#2.
  const HistorySessionTargettingTest::RequestType SESSION_2[] =
  {
    {
      0,
      "Session2Keyword",
      {"Session2BP", 0},
      {0, "Session2Channel"},
      false
    },
    {
      10,
      "Session2Keyword",
      {"Session2BP", 0},
      {0, "Session2Channel"},
      false
    },
    {
      20,
      "Session2Keyword",
      {"Session2BP", 0},
      {0, "Session2Channel"},
      false
    },
    {
      30,
      "Session2Keyword",
      {"Session2BP", 0},
      {0, "Session2Channel"},
      false
    },
    {
      40,
      "Session2Keyword",
      {"Session2BP", "Session2Channel"},
      {0, 0},
      false
    },
    {
      2*60 + 40,
      "Session2Keyword",
      {"Session2BP", 0},
      {0, "Session2Channel"},
      false
    },
    {
      3*60 + 10,
      "Session2Keyword",
      {"Session2BP", 0},
      {0, "Session2Channel"},
      false
    },
    {
      3*60 + 50,
      "Session2Keyword",
      {"Session2BP", "Session2Channel"},
      {0, 0},
      false
    }
  };

  // "Context" session channel
  const HistorySessionTargettingTest::RequestType CONTEXT_SESSION[] =
  {
    {
      0,
      "Session3Keyword",
      {"Session3BP", "Session3Channel"},
      {0, 0},
      false
    }
  };

  // Session channel with 2 visits count
  const HistorySessionTargettingTest::RequestType SESSION_VISITS_2[] =
  {
    {
      0,
      "Session1Keyword",
      {"Session1BP", 0},
      {0, "Session1Channel"},
      false
    },
    {
      0,
      0,
      {0, 0},
      {"Session1BP", "Session1Channel"},
      false
    },
    {
      0,
      "Session1Keyword" ,
      {"Session1BP", "Session1Channel"},
      {0, 0},
      false
    },
    {
      0,
      0,
      {0, "Session1Channel"},
      {"Session1BP", 0},
      false
    },
  };

  // Session channel with non-zero from time
  const HistorySessionTargettingTest::RequestType SESSION_TIME_FROM[] =
  {
    {
      0,
      "Session4Keyword",
      {"Session4BP", 0},
      {0, "Session4Channel"},
      false
    },
    {
      0,
      0,
      {0, 0},
      {"Session4BP", "Session4Channel"},
      false
    },
    {
      0,
      "Session4Keyword",
      {"Session4BP", 0},
      {0, "Session4Channel"},
      false
    },
    {
      0,
      0,
      {0, 0},
      {"Session4BP", "Session4Channel"},
      false
    },
    {
      2*60 + 30,
      0,
      {0, "Session4Channel"},
      {"Session4BP", 0},
      false
    }
  };

  // Session optimizing
  const HistorySessionTargettingTest::RequestType SESSION_OPTIMIZING[] =
  {
    {
      23*60*60 + 59*60,
      "Session1Keyword",
      {"Session1BP", 0},
      {0, "Session1Channel"},
      false
    },
    {
      23*60*60 + 59*60,
      "Session1Keyword",
      {"Session1BP", "Session1Channel"},
      {0, 0},
      false
    },
    {
      24*60*60 + 60,
      0,
      {0, "Session1Channel"},
      {"Session1BP", 0},
      false
    }
  };

  // Boundary time range
  // ATTENTION! This test could failed on NON GMT colocation
  const HistorySessionTargettingTest::RequestType SESSION_BOUNDARY[] =
  {
    {
      23*60*60 + 40*60,
      "SessionBoundary1Keyword,"
      "SessionBoundary2Keyword,"
      "HistoryBoundaryKeyword",
      { "SessionBoundary1BP,"
        "SessionBoundary2BP,"
        "HistoryBoundaryBP" , 0},
      {0,
       "SessionBoundary1Channel,"
       "SessionBoundary2Channel,"
       "HistoryBoundaryChannel" },
      true
    },
    {
      24*60*60 + 20*60,
      "SessionBoundary1Keyword,"
      "SessionBoundary2Keyword,"
      "HistoryBoundaryKeyword",
      { "SessionBoundary1BP,"
        "SessionBoundary2BP,"
        "HistoryBoundaryBP" , 0},
      {0,
       "SessionBoundary1Channel,"
       "SessionBoundary2Channel,"
       "HistoryBoundaryChannel" },
      true
    },
    {
      24*60*60 +  12*60*60 + 30*60,
      0,
      {0 ,
       "SessionBoundary1Channel,"
       "SessionBoundary2Channel"},
      {"SessionBoundary1BP,"
       "SessionBoundary2BP,"
       "HistoryBoundaryBP",
       "HistoryBoundaryChannel" },
      true
    },
    {
      24*60*60 +  23*60*60 + 30*60,
      0,
      {0 , "SessionBoundary1Channel"},
      {"SessionBoundary1BP,"
       "SessionBoundary2BP,"
       "HistoryBoundaryBP",
       "SessionBoundary2Channel,"
       "HistoryBoundaryChannel" },
      true
    },
    {
      24*60*60 +  23*60*60 + 50*60,
      0,
      {0 , 0},
      {"SessionBoundary1BP,"
       "SessionBoundary2BP,"
       "HistoryBoundaryBP",
       "SessionBoundary1Channel,"
       "SessionBoundary2Channel,"
       "HistoryBoundaryChannel" },
      true
    }
  };

  // Regular H+T channel.
  const HistorySessionTargettingTest::RequestType REGULAR_HT[] =
  {
    {
      0,
      "HT1Keyword",
      {"HT1BP", 0},
      {0, "HT1Channel"},
      false
    },
    {
      2*60*60,
      "HT1Keyword",
      {"HT1BP", "HT1Channel"},
      {0, 0},
      false
    }
  };

  // H+T channel#2.
  const HistorySessionTargettingTest::RequestType HT_2[] =
  {
    {
      0,
      "HT2Keyword",
      {"HT2BP", 0},
      {0, "HT2Channel"},
      false
    },
    {
      24*60*60,
      "HT2Keyword",
      {"HT2BP", 0},
      {0, "HT2Channel"},
      false
    },
    {
      2*24*60*60,
      "HT2Keyword",
      {"HT2BP", "HT2Channel"},
      {0, 0},
      false
    }
  };

  // History channel.
  const HistorySessionTargettingTest::RequestType HISTORY[] =
  {
    {
      0,
      "History1Keyword",
      {"History1BP", 0},
      {0, "History1Channel"},
      false
    },
    {
      24*60*60,
      "History1Keyword",
      {"History1BP", 0},
      {0, "History1Channel"},
      false
    },
    {
      2*24*60*60,
      "History1Keyword",
      {"History1BP", 0},
      {0, "History1Channel"},
      false
    },
    {
      3*24*60*60,
      "History1Keyword",
      {"History1BP", 0},
      {0, "History1Channel"},
      false
    },
    {
      5*24*60*60 - 2,
      "History1Keyword",
      {"History1BP", 0},
      {0, "History1Channel"},
      false
    },
    {
      7*24*60*60 - 2,
      "History1Keyword",
      {"History1BP", 0},
      {0, "History1Channel"},
      false
    },
    {
      7*24*60*60 - 2,
      "History1Keyword",
      {"History1BP", 0},
      {0, "History1Channel"},
      false
    },
    {
      8*24*60*60 - 2,
      "History1Keyword",
      {"History1BP", 0},
      {0, "History1Channel"},
      true
    },
    {
      9*24*60*60 - 3,
      "History1Keyword",
      {"History1BP", "History1Channel"},
      {0, 0},
      true
    },
  };


  // Combined case.
  const HistorySessionTargettingTest::RequestType COMBINED[] =
  {
    {
      0,
      "History1Keyword,Session2Keyword,HT2Keyword",
      {"History1BP,Session2BP,HT2BP", 0},
      {0, "History1Channel,Session2Channel,HT2Channel"},
      false
    },
    {
      10,
      "Session2Keyword,HT2Keyword",
      {"HT2BP,Session2BP", 0},
      {"History1BP", "History1Channel,Session2Channel,HT2Channel"},
      false
    },
    {
      20,
      "History1Keyword,HT2Keyword",
      {"History1BP,HT2BP", "HT2Channel"},
      {"Session2BP", "History1Channel,Session2Channel"},
      false
    },
    {
      30,
      "HT2Keyword",
      {"HT2BP", "HT2Channel"},
      {"History1BP,Session2BP", "History1Channel,Session2Channel"},
      false
    },
    {
      90,
      "Session2Keyword,HT2Keyword",
      {"HT2BP,Session2BP", "Session2Channel,HT2Channel"},
      {"History1BP", "History1Channel"},
      false
    },
    {
      24*60*60,
      "History1Keyword",
      {"History1BP", "HT2Channel"},
      {"HT2BP,Session2BP", "History1Channel,Session2Channel"},
      false
    },
    {
      2*24*60*60,
      "Session1Keyword",
      {"Session1BP", "HT2Channel,History1Channel"},
      {"History1BP,HT2BP,Session2BP",
       "Session1Channel,Session2Channel"},
      false
    }
  };

  // "Context" H+T channel.
  const HistorySessionTargettingTest::RequestType CONTEXT_HT[] =
  {
    {
      0,
      "HT3Keyword",
      {"HT3BP", "HT3Channel"},
      {0, 0},
      false
    }
  };

  // History+Today and history optimizing procedure.
  // Visits in base and history profile.
  const HistorySessionTargettingTest::RequestType HT_OPTIMIZE[] =
  {
    {
      0,
      "HT2Keyword",
      {"HT2BP", 0},
      {0, "HT2Channel"},
      false
    },
    {
      0,
      "HT2Keyword",
      {"HT2BP", 0},
      {0, "HT2Channel"},
      false
    },
    {
      24*60*60,
      0,
      {0, 0},
      {"HT2BP", "HT2Channel"},
      false
    },
    {
      24*60*60,
      "HT2Keyword",
      {"HT2BP", "HT2Channel"},
      {0, 0},
      false
    }
  };

  // History+Today. Visits only in history profile.
  const HistorySessionTargettingTest::RequestType HT_HISTORY[] =
  {
    {
      0,
      "HT4Keyword",
      {"HT4BP", "HT4Channel"},
      {0, 0},
      false
    },
    {
      24*60*60,
      0,
      {0, "HT4Channel"},
      {"HT4BP", 0},
      false
    },
    {
      2*24*60*60,
      0,
      {0, "HT4Channel"},
      {"HT4BP", 0},
      false
    }  
  };

  // History channels matching
  const HistorySessionTargettingTest::RequestType HISTORY_MATCHING[] =
  {
    {
      0,
      "History3Keyword",
      {"History3BP", 0},
      {0, "History3Channel"},
      false
    },
    {
      0,
      "History3Keyword",
      {"History3BP", 0},
      {0, "History3Channel"},
      false
    },
    {
      0,
      "History3Keyword",
      {"History3BP", 0},
      {0, "History3Channel"},
      false
    },
    {
      0,
      0,
      {0, 0},
      {"History3BP", "History3Channel"},
      false
    },
    {
      24*60*60,
      0,
      {0, "History3Channel"},
      {"History3BP", 0},
      false
    },
    {
      2*24*60*60,
      0,
      {0, 0},
      {"History3BP", "History3Channel"},
      false
    }
  };

  // Navigation history persistence.
  const HistorySessionTargettingTest::RequestType HISTORY_PERSISTENCE[] =
  {
    {
      0,
      "History2Keyword",
      {"History2BP", 0},
      {0, "History2Channel"},
      false
    },
    {
      24*60*60,
      "History2Keyword",
      {"History2BP", 0},
      {0, "History2Channel"},
      false
    },
    {
      2*24*60*60,
      0,
      {0, "History2Channel"},
      {"History2BP", 0},
      false
    }
  };

  // History channel with time_from > 1 day,
  // history profile record filtration.
  const HistorySessionTargettingTest::RequestType HISTORY_FILTRATION[] =
  {
    {
      0,
      "History4Keyword",
      {"History4BP", 0},
      {0, "History4Channel"},
      false
    },
    {
      0,
      "History4Keyword",
      {"History4BP", 0},
      {0, "History4Channel"},
      false
    },
    {
      24*60*60,
      0,
      {0, 0},
      {"History4BP", "History4Channel"},
      false
    },
    {
      2*24*60*60,
      0,
      {0, "History4Channel"},
      {"History4BP", 0},
      false
    }
  };

  // Minimum visits condition for history channels
  const HistorySessionTargettingTest::RequestType HISTORY_VISITS[] =
  {
    {
      0,
      "History1Keyword",
      {"History1BP", 0},
      {0, "History1Channel"},
      false
    },
    {
      0,
      "History1Keyword",
      {"History1BP", 0},
      {0, "History1Channel"},
      false
    },
    {
      24*60*60,
      "History1Keyword",
      {"History1BP", 0},
      {0, "History1Channel"},
      false
    },
    {
      24*60*60,
      "History1Keyword",
      {"History1BP", 0},
      {0, "History1Channel"},
      false
    },
    {
      2*24*60*60,
      0,
      {0, "History1Channel"},
      {"History1BP", 0},
      false
    }    
  };

  // Double optimization
  const HistorySessionTargettingTest::RequestType DOUBLE_OPTIMIZATION[] =
  {
    {
      0,
      "History2Keyword",
      {"History2BP", 0},
      {0, "History2Channel"},
      false
    },
    {
      0,
      "History2Keyword",
      {"History2BP", 0},
      {0, "History2Channel"},
      false
    },
    {
      24*60*60,
      0,
      {0, "History2Channel"},
      {"History2BP", 0},
      false
    },
    {
      2*24*60*60,
      0,
      {0, "History2Channel"},
      {"History2BP", 0},
      false
    },
    {
      3*24*60*60,
      0,
      {0, 0},
      {"History2BP", "History2Channel"},
      false
    }
  };

  // Periodic history optimization
  const HistorySessionTargettingTest::RequestType PERIODIC_OPTIMIZATION[] =
  {
    // today 04:01
    {
      4*60*60 + 60 ,
      "History2Keyword,HT7Keyword",
      {"History2BP,HT7BP", 0},
      {0, 0},
      false
    },
    // today 04:01
    {
      4*60*60 + 60 ,
      "History2Keyword,HT7Keyword",
      {"History2BP,HT7BP", 0},
      {0, 0},
      false
    },
    // today 05:02
    // first periodic optimization
    {
      5*60*60 + 2*60 ,
      0,
      {0, "HT7Channel"},
      {0, "History2Channel"},
      false
    },
    // today 05:02
    {
      5*60*60 + 2*60 ,
      "History6Keyword,HT6Keyword",
      {"History6BP,HT6BP", "HT7Channel"},
      {0, "History2Channel,History6Channel,HT6Channel"},
      false
    },
    // tomorrow 04:03
    // daily after periodic
    {
      24*60*60 + 4*60*60 + 3*60 ,
      "History5Keyword,HT5Keyword",
      {"History5BP,HT5BP", "History2Channel,HT7Channel"},
      {0, "History5Channel,HT5Channel,History6Channel,HT6Channel"},
      false
    },
    // tomorrow 04:03
    {
      24*60*60 + 4*60*60 + 3*60 ,
      0,
      {0, "History2Channel,HT7Channel"},
      {0, "History5Channel,HT5Channel,History6Channel,HT6Channel"},
      false
    },
    // tomorrow 04:04
    // periodic after daily
    {
      24*60*60 + 4*60*60 + 4*60 ,
      "History5Keyword,History6Keyword,HT5Keyword,HT6Keyword",
      {"History5BP,HT5BP,History6BP,HT6BP",
       "History2Channel,HT7Channel,"
       "HT6Channel,HT5Channel"},
      {0, "History5Channel,History6Channel"},
      false
    },
    // tomorrow 05:05
    // second periodic optimization on the day
    {
      24*60*60 + 5*60*60 + 5*60 ,
      0,
      {0,
       "History2Channel,HT7Channel,HT5Channel,HT6Channel"},
      {0, "History5Channel,History6Channel"},
      false
    },
    // tomorrow 06:06
    {
      24*60*60 + 6*60*60 + 6*60 ,
      0,
      {0,
       "History2Channel,HT7Channel,HT5Channel,HT6Channel"},
      {0, "History5Channel,History6Channel"},
      false
    },
    // 2 days later 04:07
    {
      2*24*60*60 + 4*60*60 + 7*60 ,
      0,
      {0,
       "History2Channel,History5Channel,"
       "History6Channel,HT7Channel,"
       "HT5Channel,HT6Channel"},
      {0, 0},
      false
    },
  };
  
}
 
bool 
HistorySessionTargettingTest::run_test()
{
 
  AutoTest::Time today_base(
    (base_time.get_gm_time().format("%d-%m-%Y") +
       ":" + "00-00-00").c_str());
  
  AUTOTEST_CASE(
    check(
      SESSION_1,
      countof(SESSION_1),
      base_time),
    "Session channel#1");

  AUTOTEST_CASE(
    check(
      SESSION_2,
      countof(SESSION_2),
      base_time),
    "Session channel#2");

  AUTOTEST_CASE(
    check(
      CONTEXT_SESSION,
      countof(CONTEXT_SESSION),
      base_time),
    "Context session channel");

  AUTOTEST_CASE(
    check(
      SESSION_VISITS_2,
      countof(SESSION_VISITS_2),
      base_time),
    "Session channel with 2 visits count");

  AUTOTEST_CASE(
    check(
      SESSION_TIME_FROM,
      countof(SESSION_TIME_FROM),
      base_time),
    "Session channel with non-zero from time");

  AUTOTEST_CASE(
    check(
      SESSION_OPTIMIZING,
      countof(SESSION_OPTIMIZING),
      today_base),
    "Session optimizing");

  AUTOTEST_CASE(
    check(
      SESSION_BOUNDARY,
      countof(SESSION_BOUNDARY),
      today_base),
    "Boundary time range");

  AUTOTEST_CASE(
    check(
      REGULAR_HT,
      countof(REGULAR_HT),
      base_time),
    "Regular H+T channel");

  AUTOTEST_CASE(
    check(
      HT_2,
      countof(HT_2),
      base_time),
    "H+T channel#2");

  AUTOTEST_CASE(
    check(
      HISTORY,
      countof(HISTORY),
      base_time),
    "History channel");

  AUTOTEST_CASE(
    check(
      COMBINED,
      countof(COMBINED),
      base_time),
    "Combined case");

  AUTOTEST_CASE(
    check(
      CONTEXT_HT,
      countof(CONTEXT_HT),
      base_time),
    "Context H+T channel");
  
  AUTOTEST_CASE(
    check(
      HT_OPTIMIZE,
      countof(HT_OPTIMIZE),
      base_time),
    "History+Today. Visits in base and history profile");

  AUTOTEST_CASE(
    check(
      HT_HISTORY,
      countof(HT_HISTORY),
      base_time),
    "History+Today. Visits only in history profile");

  AUTOTEST_CASE(
    check(
       HISTORY_MATCHING,
      countof(HISTORY_MATCHING),
       base_time),
    "History channels matching");

  AUTOTEST_CASE(
    check(
      HISTORY_PERSISTENCE,
      countof(HISTORY_PERSISTENCE),
      base_time),
    "Navigation history persistence");

  AUTOTEST_CASE(
    check(
      HISTORY_FILTRATION,
      countof(HISTORY_FILTRATION),
      base_time),
    "History channel with time_from > 1 day, "
    "history profile record filtration");

  AUTOTEST_CASE(
    check(
      HISTORY_VISITS,
      countof(HISTORY_VISITS),
      base_time),
    "Minimum visits condition for history channels");

  AUTOTEST_CASE(
    check(
      DOUBLE_OPTIMIZATION,
      countof(DOUBLE_OPTIMIZATION),
      base_time),
    "Double optimization");

  AUTOTEST_CASE(
    check(
      PERIODIC_OPTIMIZATION,
      countof(PERIODIC_OPTIMIZATION),
      today_base),
    "Periodic history optimization");

  return true;
}


void HistorySessionTargettingTest::check(
  const RequestType* requests,
  size_t requests_size,
  const AutoTest::Time& base_time)
{
  // Create client
  AdClient client(AdClient::create_user(this));
  
  for (size_t i = 0; i < requests_size; ++i)
  {
    NSLookupRequest request;
    AutoTest::UserProfiling::make_request(
      this,
      request,
      requests[i],
      base_time);

    client.process_request(request);

    if (requests[i].log_request &&
        get_config().check_service(
          CTE_ALL,
          STE_USER_INFO_MANAGER_CONTROLLER))
    {
      AutoTest::Logger::thlog().log("Persistent user profile:");

      AutoTest::AdminsArray<AutoTest::UserInfoAdminLog> admin;

      admin.initialize(
        this, CTE_ALL,
        STE_USER_INFO_MANAGER_CONTROLLER,
        AutoTest::prepare_uid(
          client.get_uid(),
          AutoTest::UUE_ADMIN_PARAMVALUE).c_str(),
        AutoTest::UserInfoManagerController);

      admin.log(AutoTest::Logger::thlog());
    }

    // Check channels
    if (requests[i].expected.triggers)
    {
      FAIL_CONTEXT(
        ChannelsCheck(
          this,
          requests[i].expected.triggers,
          client.debug_info.trigger_channels).check(),
        "Expected trigger_channels#" + strof(i));
    }

    if (requests[i].expected.history)
    {
      FAIL_CONTEXT(
        ChannelsCheck(
          this,
          requests[i].expected.history,
          client.debug_info.history_channels).check(),
        "Expected history#" + strof(i));
    }

    if (requests[i].unexpected.triggers)
    {
      FAIL_CONTEXT(
        ChannelsCheck(
          this,
          requests[i].unexpected.triggers,
          client.debug_info.trigger_channels,
          AutoTest::SCE_NOT_ENTRY).check(),
        "Unexpected trigger_channels#" + strof(i));
    }

    if (requests[i].unexpected.history)
    {
      FAIL_CONTEXT(
        ChannelsCheck(
          this,
          requests[i].unexpected.history,
          client.debug_info.history_channels,
          AutoTest::SCE_NOT_ENTRY).check(),
        "Unexpected history#" + strof(i));
    }
  }
}
