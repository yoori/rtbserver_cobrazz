#include "Common.hpp"
#include "UserProfileMergingTest.hpp"

REFLECT_UNIT(UserProfileMergingTest) (
  "UserProfiling",
  AUTO_TEST_FAST
);


namespace
{  
  const std::string referer = "www.google.com";
  typedef AutoTest::UserInfoAdminLog UserInfoAdmin;
  typedef AutoTest::ChannelsCheck ChannelsCheck;

  // Merging two empty profiles.
  const UserProfileMergingTest::RequestType EMPTY_PROFILES[] =
  {
    {
      0,
      UserProfileMergingTest::RE_TEMPORARY,
      "ContextKeyword",
      {"ContextBP", "ContextChannel"},
      {0, 0}
    },
    {
      0,
      UserProfileMergingTest::RE_PERSISTENT,
      "ContextKeyword",
      {"ContextBP", "ContextChannel"},
      {0, 0}
    },
    {
      0,
      UserProfileMergingTest::RE_MERGING,
      0,
      {0, 0},
      {"ContextBP", "ContextChannel"}
    }
  };

  // Merging with empty profile.
  const UserProfileMergingTest::RequestType EMPTY_TEMP_PROFILE[] =
  {
    {
      0,
      UserProfileMergingTest::RE_TEMPORARY,
      "ContextKeyword",
      {"ContextBP", "ContextChannel"},
      {0, 0}
    },
    {
      0,
      UserProfileMergingTest::RE_PERSISTENT,
      "Session2Keyword",
      {"Session2BP", "Session2Channel"},
      {0, 0}
    },
    {
      0,
      UserProfileMergingTest::RE_MERGING,
      0,
      {0, "Session2Channel" },
      {"ContextBP,Session2BP", "ContextChannel"}
    },
    {
      0,
      UserProfileMergingTest::RE_PERSISTENT,
      0,
      {0, "Session2Channel" },
      {"ContextBP,Session2BP", "ContextChannel"}
    }
  };

    // Merging with unknown profile.
  const UserProfileMergingTest::RequestType UNKNOWN_TEMP_PROFILE[] =
  {
    {
      0,
      UserProfileMergingTest::RE_PERSISTENT,
      "Session2Keyword,ContextKeyword",
      {"Session2BP,ContextBP",
       "Session2Channel,ContextChannel"},
      {0, 0}
    },
    {
      0,
      UserProfileMergingTest::RE_MERGING,
      0,
      {0, "Session2Channel" },
      {"ContextBP,Session2BP", "ContextChannel"}
    },
    {
      0,
      UserProfileMergingTest::RE_PERSISTENT,
      0,
      {0, "Session2Channel" },
      {"ContextBP,Session2BP", "ContextChannel"}
    }
  };
  

  // 'H+T' channel (visits=2).
  const UserProfileMergingTest::RequestType HT_CHANNEL[] =
  {
    {
      0,
      UserProfileMergingTest::RE_TEMPORARY,
      "HTKeyword",
      {"HTBP", 0},
      {0, "HTChannel"}
    },
    {
      0,
      UserProfileMergingTest::RE_PERSISTENT,
      "HTKeyword",
      {"HTBP", 0},
      {0, "HTChannel"}
    },
    {
      0,
      UserProfileMergingTest::RE_MERGING,
      0,
      {0, "HTChannel"},
      {0, 0}
    }
  };

  // 'Session' channel (visits=3).
  const UserProfileMergingTest::RequestType SESSION_CHANNEL_1[] =
  {
    {
      0,
      UserProfileMergingTest::RE_TEMPORARY,
      "Session1Keyword",
      {"Session1BP", 0},
      {0, "Session1Channel"}
    },
    {
      0,
      UserProfileMergingTest::RE_PERSISTENT,
      "Session1Keyword",
      {"Session1BP", 0},
      {0, "Session1Channel"}
    },
    {
      0,
      UserProfileMergingTest::RE_MERGING,
      "Session1Keyword",
      {"Session1BP", "Session1Channel"},
      {0, 0}
    }
  };

  // 'Session' channel (visits=1).
  const UserProfileMergingTest::RequestType SESSION_CHANNEL_2[] =
  {
    {
      0,
      UserProfileMergingTest::RE_TEMPORARY,
      "Session2Keyword",
      {"Session2BP", "Session2Channel"},
      {0, 0}
    },
    {
      0,
      UserProfileMergingTest::RE_MERGING,
      0,
      {0, "Session2Channel"},
      {"Session2BP", 0}
    }
  };

  // Base profile and history profile (merging on Periodic Optimization).
  const UserProfileMergingTest::RequestType PERIODIC_OPTIMIZATION[] =
  {
    {
      0,
      UserProfileMergingTest::RE_TEMPORARY,
      "HistoryKeyword,HTKeyword",
      {"HistoryBP,HTBP", 0},
      {0, "HistoryChannel,HTChannel"}
    },
    {
      -24*60*60,
      UserProfileMergingTest::RE_PERSISTENT,
      "HistoryKeyword,HTKeyword",
      {"HistoryBP,HTBP", 0},
      {0, "HistoryChannel,HTChannel"}
    },
    {
      0,
      UserProfileMergingTest::RE_PERSISTENT,
      0,
      {0, 0},
      {"HistoryBP,HTBP",
       "HistoryChannel,HTChannel"}
    },
    {
      60*60 + 1,
      UserProfileMergingTest::RE_MERGING,
      0,
      {0, "HTChannel"},
      {"HistoryBP, HTBP", "HistoryChannel"}
    }
  };


  // Merging two history profiles.
  const UserProfileMergingTest::RequestType HISTORY_PROFILE[] =
  {
    {
      0,
      UserProfileMergingTest::RE_TEMPORARY,
      "HistoryKeyword",
      {"HistoryBP", 0},
      {0, "HistoryChannel"}
    },
    {
      24*60*60,
      UserProfileMergingTest::RE_TEMPORARY,
      0,
      {0, 0},
      {"HistoryBP", "HistoryChannel"}
    },
    {
      0,
      UserProfileMergingTest::RE_PERSISTENT,
      0,
      {0, 0},
      {"HistoryBP", "HistoryChannel"}
    },
    {
      24*60*60,
      UserProfileMergingTest::RE_PERSISTENT,
      "HistoryKeyword",
      {"HistoryBP", 0},
      {0, "HistoryChannel"}
    },
    {
      2*24*60*60,
      UserProfileMergingTest::RE_PERSISTENT,
      0,
      {0, 0},
      {"HistoryBP", "HistoryChannel"}
    },
    {
      2*24*60*60,
      UserProfileMergingTest::RE_MERGING,
      0,
      {0, "HistoryChannel"},
      {"HistoryBP", 0}
    }
  };
}     
 
bool 
UserProfileMergingTest::run_test()
{
  NOSTOP_FAIL_CONTEXT(
    check(
      "Merging two empty profiles.",
      EMPTY_PROFILES,
      countof(EMPTY_PROFILES)));

  NOSTOP_FAIL_CONTEXT(
    check(
      "Merging with empty profile.",
      EMPTY_TEMP_PROFILE,
      countof(EMPTY_TEMP_PROFILE)));

  NOSTOP_FAIL_CONTEXT(
    check(
      "Merging with unknown profile.",
      UNKNOWN_TEMP_PROFILE,
      countof(UNKNOWN_TEMP_PROFILE)));

  NOSTOP_FAIL_CONTEXT(
    check(
      "H+T channel (visits=2).",
      HT_CHANNEL,
      countof(HT_CHANNEL)));
  
  NOSTOP_FAIL_CONTEXT(
    check("Session channel (visits=3).",
      SESSION_CHANNEL_1,
      countof(SESSION_CHANNEL_1)));
   
  NOSTOP_FAIL_CONTEXT(
    check(
      "Session channel (visits=1).",
      SESSION_CHANNEL_2,
      countof(SESSION_CHANNEL_2)));

  NOSTOP_FAIL_CONTEXT(
    check(
      "Base profile and history profile "
      "(merging on Periodic Optimization).",
      PERIODIC_OPTIMIZATION,
      countof(PERIODIC_OPTIMIZATION),
      AutoTest::Time()));

  NOSTOP_FAIL_CONTEXT(
    check(
      "Merging two history profiles.",
      HISTORY_PROFILE,
      countof(HISTORY_PROFILE),
      base_time));

  return true;
}

void UserProfileMergingTest::check(
  const std::string& description,
  const RequestType* requests,
  size_t requests_size,
  const AutoTest::Time& base_time)
{
  add_descr_phrase(description);
  // Create clients
  TemporaryAdClient t_client(
    TemporaryAdClient::create_user(this));
  AdClient p_client(AdClient::create_user(this));
  
  for (size_t i = 0; i < requests_size; ++i)
  {

    AdClient* client =
      requests[i].req_type == RE_TEMPORARY?
      &t_client: &p_client;
    NSLookupRequest request;
    AutoTest::UserProfiling::make_request(
      this,
      request,
      requests[i],
      base_time);

    if (requests[i].req_type == RE_MERGING)
    {
      log_admin_output(
        p_client,
        t_client,
        RE_TEMPORARY);

      client->merge(t_client, request);
    }
    else
    {
      client->process_request(request);
    }

    log_admin_output(
      p_client,
      t_client,
      requests[i].req_type);

    // Check channels 
    FAIL_CONTEXT(
      ChannelsCheck(
        this,
        requests[i].expected.triggers,
        client->debug_info.trigger_channels).check(),
      description +
        " Expected trigger_channels#" + strof(i));

    FAIL_CONTEXT(
      ChannelsCheck(
        this,
        requests[i].expected.history,
        client->debug_info.history_channels).check(),
      description +
        " Expected history#" + strof(i));

    FAIL_CONTEXT(
      ChannelsCheck(
        this,
        requests[i].unexpected.triggers,
        client->debug_info.trigger_channels,
        AutoTest::SCE_NOT_ENTRY).check(),
      description +
        " Unexpected trigger_channels#" + strof(i));

    FAIL_CONTEXT(
      ChannelsCheck(
        this,
        requests[i].unexpected.history,
        client->debug_info.history_channels,
        AutoTest::SCE_NOT_ENTRY).check(),
      description +
        " Unexpected history#" + strof(i));
  }
}


void UserProfileMergingTest::log_admin_output(
  AdClient& pclient,
  TemporaryAdClient& tclient,
  RequestEnum request_type)
{
  if (get_config().check_service(CTE_ALL, STE_USER_INFO_MANAGER_CONTROLLER))
  {

    if (request_type != RE_PERSISTENT)
    {
      AutoTest::Logger::thlog().log("Temporary user profile:");

      AutoTest::AdminsArray<AutoTest::UserInfoAdminLog> admin;

      admin.initialize(
        this, CTE_ALL,
        STE_USER_INFO_MANAGER_CONTROLLER,
        AutoTest::prepare_uid(
          tclient.get_tuid(),
          AutoTest::UUE_ADMIN_PARAMVALUE, true).c_str(),
        AutoTest::UserInfoManagerController, true);

      admin.log(AutoTest::Logger::thlog());
    }

    if (request_type != RE_TEMPORARY)
    {
      AutoTest::Logger::thlog().log("Persistent user profile:");

      AutoTest::AdminsArray<AutoTest::UserInfoAdminLog> admin;

      admin.initialize(
        this, CTE_ALL,
        STE_USER_INFO_MANAGER_CONTROLLER,
        AutoTest::prepare_uid(
          pclient.get_uid(),
          AutoTest::UUE_ADMIN_PARAMVALUE).c_str(),
        AutoTest::UserInfoManagerController);

      admin.log(AutoTest::Logger::thlog());
    }
  }
}

