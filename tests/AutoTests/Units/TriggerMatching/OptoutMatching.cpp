
#include "OptoutMatching.hpp"

REFLECT_UNIT(OptoutMatching) (
  "TriggerMatching",
  AUTO_TEST_FAST | AUTO_TEST_SLOW);

namespace
{

  enum UserBehaviour
  {
    UB_AllMatch        = 0,  // All channels should match.
    UB_NoMatch         = 1   // Channels shouldn't match.
  };

  struct TestCase
  {
    std::string colo;
    UserBehaviour optin;
    UserBehaviour undef;
    UserBehaviour optout;
    UserBehaviour temp;
    UserBehaviour probe;
  };

  const TestCase testcases[] =
  {
    {
      "OPTINCOLO",
      UB_AllMatch,
      UB_AllMatch,
      UB_AllMatch,
      UB_AllMatch,
      UB_AllMatch
    },
    {
      "ALLCOLO",
      UB_AllMatch,
      UB_AllMatch,
      UB_AllMatch,
      UB_AllMatch,
      UB_AllMatch
    },
    {
      "NONOPTOUTCOLO",
      UB_AllMatch,
      UB_AllMatch,
      UB_AllMatch,
      UB_AllMatch,
      UB_AllMatch
    },
    {
      "NOADSCOLO",
      UB_NoMatch,
      UB_NoMatch,
      UB_NoMatch,
      UB_NoMatch,
      UB_NoMatch
    },
    {
      "DELETEDCOLO",
      UB_NoMatch,
      UB_NoMatch,
      UB_NoMatch,
      UB_NoMatch,
      UB_NoMatch
    }
  };


  typedef AutoTest::TemporaryAdClient TemporaryAdClient;
  typedef AutoTest::OptOutRequest OptOutRequest;

  typedef UserBehaviour TestCase::* UserBehaviourMember;

  enum UserEnum
  {
    CE_Optin     = 0, // Optin client
    CE_Undef     = 1, // Undefined client
    CE_Optout    = 2, // Optout client
    CE_Temporary = 3, // Temporary client
    CE_Probe     = 4  // Probe UID user
  };

  const UserBehaviourMember userMap[] =
  {
      &TestCase::optin,
      &TestCase::undef,
      &TestCase::optout,
      &TestCase::temp,
      &TestCase::probe
  };

  const std::string userDsc[] =
  {
      "Optin user",
      "Undefined user",
      "Optout user",
      "Temporary user",
      "Probe UID user"
  };

}


void
OptoutMatching::run_case(unsigned int i, unsigned int user, AdClient& client, NSLookupRequest& request)
{
  UserBehaviour behaviour = testcases[i].*(userMap[user]);

  request.colo = fetch_string(testcases[i].colo);

  client.process_request(request);
  
  std::list<std::string> matched_bps;
  std::list<std::string> unmatched_bps;
  
  std::list<std::string> matched_channels;
  std::list<std::string> unmatched_channels;
  
  switch (behaviour)
  {
  case UB_AllMatch:
    {
      fill_expected(matched_channels, "CHANNEL");
      fill_expected(matched_bps, "BP");
      break;
    }
    
  case UB_NoMatch:
    {
      fill_expected(unmatched_bps, "BP");
      fill_expected(unmatched_channels, "CHANNEL");
      break;
    }
  }

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      matched_bps,
      client.debug_info.trigger_channels,
      AutoTest::SCE_ENTRY).check(),
    userDsc[user] +
      ". Matched trigger_channels check#" + strof(i));

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      unmatched_bps,
      client.debug_info.trigger_channels,
      AutoTest::SCE_NOT_ENTRY).check(),
    userDsc[user] +
      ". Unmatched trigger_channels check#" + strof(i));

  if (!client.uses_profiling_cluster())
  {
    FAIL_CONTEXT(
      AutoTest::sequence_checker(
        matched_channels,
        client.debug_info.history_channels,
        AutoTest::SCE_ENTRY).check(),
      userDsc[user] +
        ". Matched history_channels check#" + strof(i));
        
    FAIL_CONTEXT(
      AutoTest::sequence_checker(
        unmatched_channels,
        client.debug_info.history_channels,
        AutoTest::SCE_NOT_ENTRY).check(),
      userDsc[user] +
        ". Unmatched history_channels check#" + strof(i));  
  }
}

void OptoutMatching::set_up()
{

}

void OptoutMatching::pre_condition()
{
  add_descr_phrase("Select initial stats");

  trigger_stats_[0].key().channel_trigger_id(fetch_int("PAGETRIGGER"));
  trigger_stats_[1].key().channel_trigger_id(fetch_int("SEARCHTRIGGER"));
  trigger_stats_[2].key().channel_trigger_id(fetch_int("URLTRIGGER"));
  trigger_stats_.select(conn_);

  channel_stats_[0].key().channel_id(fetch_int("PAGECHANNEL"));
  channel_stats_[1].key().channel_id(fetch_int("SEARCHCHANNEL"));
  channel_stats_[2].key().channel_id(fetch_int("URLCHANNEL"));
  channel_stats_.select(conn_);
}

bool
OptoutMatching::run()
{

  NSLookupRequest request;
  request.referer_kw = fetch_string("KEYWORD");
  request.referer    = fetch_string("SEARCH");

  for (unsigned int i=0; i < countof(testcases); ++i)
  {
    add_descr_phrase("Colo " +  testcases[i].colo);
    for (unsigned int j=0; j < countof(userMap); ++j)
    {
      if (j == CE_Optin)
      {
        AdClient client = AdClient::create_user(this);
        run_case(i, j, client, request);
      }
      else if (j == CE_Undef)
      {
        AdClient client = AdClient::create_nonoptin_user(this);
        run_case(i, j, client, request);
      }
      else if (j == CE_Optout)
      {
        AdClient client = AdClient::create_nonoptin_user(this);
        client.process_request(OptOutRequest().op("out"));
        run_case(i, j, client, request);
      }
      else if (j == CE_Temporary)
      {
        TemporaryAdClient client = TemporaryAdClient::create_user(this);
        run_case(i, j, client, request);
      }
      else if (j == CE_Probe)
      {
        AdClient client = AdClient::create_nonoptin_user(this);
        client.set_probe_uid();
        run_case(i, j, client, request);
      }
    }
  }
  return true;
}

void OptoutMatching::post_condition()
{
  add_descr_phrase("Check generated stats");

  // Stats collected only for optin and probe_uid users (x3 for differents colo)
  ChannelDiff diffs[] = {
    ChannelDiff().hits(6).hits_kws(6).hits_search_kws(0).hits_urls(0),
    ChannelDiff().hits(6).hits_kws(0).hits_search_kws(6).hits_urls(0),
    ChannelDiff().hits(6).hits_kws(0).hits_search_kws(0).hits_urls(6),
  };

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        conn_, diffs, channel_stats_)).check(),
    "ChannelInventory check");

  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_each_diff_checker(
        conn_, TriggerDiff().hits(6), trigger_stats_)).check(),
    "ChannelTriggerStats check");
}

void OptoutMatching::tear_down()
{

}

void OptoutMatching::fill_expected(std::list<std::string>& list,
                                   const char* namesuffix,
                                   const char* names)
{

  std::string prefixes[] = {"PAGE", "SEARCH", "URL"};
  
  if ( !names )
  {
    for (unsigned int i = 0; i < countof(prefixes); ++i)
    {
      list.push_back(fetch_string(prefixes[i]+namesuffix));
    }
  }
  else
  {
    std::string names_(names);
    String::StringManip::SplitComma tokenizer(names_);
    String::SubString token;
    while (tokenizer.get_token(token))
    {
      String::StringManip::trim(token);
      list.push_back(fetch_string(token.str()+namesuffix));
    }    
  }


}



