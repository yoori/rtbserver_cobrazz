
#include "LemmatisationTest.hpp"

REFLECT_UNIT(LemmatisationTest) (
  "TriggerMatching",
  AUTO_TEST_FAST);

namespace
{
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::ChannelsCheck ChannelsCheck;
  typedef AutoTest::SpecialEffectsChecker SpecialEffectsChecker;

  enum RequestSpecific
  {
    RS_SEARCH = 1,   // Use search referer
    RS_KN = 2,       // Use 'kn' (keyword normalization) parameter
    RS_NO_ADV = 4,   // Check NO ADV effect
    RS_NO_TRACK = 8, // Check NO TRACK effect
    RS_FT = 16,      // Use ft (full text mode)
    RS_CHECK_TRIGGERS = 32 // Check Debug-Info triggers fiels
  };
  
  
  struct TestCase
  {
    std::string description;
    const char* kwd;
    const char* expected_channels;
    const char* unexpected_channels;
    unsigned short flags;
  };

  const TestCase TEST_CASES[] =
  {
    {
      "Single word triggers#1.",
      "KWD/1",
      "BPPage/1,BPPage/4", 0, 0
    },
    {
      "Single word triggers#2.",
      "KWD/2",
      "BPPage/1", "BPChannel/4", 0
    },
    {
      "Hard triggers#1.",
      "KWD/3",
      "BPPage/4,BPPage/5,BPPage/9", 0, 0
    },
    {
      "Hard triggers#2.",
      "KWD/4",
      "BPPage/9", "BPChannel/4,BPChannel/5", 0
    },
    {
      "Hard & complex triggers.",
      "KWD/5",
      "BPPage/5,BPPage/9", "BPChannel/4", RS_FT
    },
    {
      "Dictionary bounds (first and last lemmas).",
      "KWD/6",
      "BPPage/9", 0, 0
    },
    {
      "Non-first form in trigger.",
      "KWD/7",
      "BPSearch/3,BPUrkKwd", 0, RS_SEARCH
    },
    {
      "Mixed language lemmas with "
      "defined channel lang: en lemmas only.",
      "KWD/8",
      "BPPage/5",0 , 0
    },
    {
      "Mixed language lemmas with defined "
      "channel lang: en and non-en lemmas.",
      "KWD/9",
      0, "BPChannel/5", 0
    },
    {
      "CJK: JP original hard trigger.",
      "KWD/10",
      "BPPage/6", 0, RS_KN
    },
    {
      "CJK: JP lemmatized hard trigger.",
      "KWD/11",
      0, "BPChannel/6", RS_KN
    },
    {
      "CJK: JP lemmatized soft trigger.",
      "KWD/12",
      "BPPage/6", 0, RS_KN
    },
    {
      "Lemmas from EXT dictionary.",
      "KWD/13",
      "BPPage/2", 0, 0
    },
    {
      "One trigger in channels with different "
      "language; words in one dictionary#1.",
      "KWD/14",
      "BPPage/7,BPPage/8", 0, RS_FT
    },
    {
      "One trigger in channels with different "
      "language; words in one dictionary#2.",
      "KWD/15",
      0, "BPChannel/7,BPChannel/8", 0
    },
    {
      "One trigger in channels with different "
      "language; words in both dictionaries#1.",
      "KWD/16",
      "BPPage/7", "BPChannel/8", 0
    },
    {
      "One trigger in channels with different "
      "language; words in both dictionaries#2.",
      "KWD/17",
      "BPPage/8", "BPChannel/7", 0
    },
    {
      "Lemmatization performed after normalization.",
      "KWD/18",
      "BPPage/8", 0, 0
    },
    {
      "Lemmatization performed after "
      "normalization+segmentation.",
      "KWD/19",
      "BPPage/6", 0, RS_KN
    },
    {
      "Exact triggers#1.",
      "KWD/20",
      "BPSearch/4,BPSearch/9", 0, RS_SEARCH 
    },
    {
      "Exact triggers#2.",
      "KWD/21",
      "BPTrigger/9/4", "BPTrigger/9/5",
      RS_SEARCH | RS_CHECK_TRIGGERS
    },
    {
      "Channel with undefined language.",
      "KWD/22",
      0, 0, RS_NO_ADV | RS_NO_TRACK
    }
  };

}

bool
LemmatisationTest::run_test()
{
  for (size_t i = 0; i < countof(TEST_CASES); ++i)
  {
    add_descr_phrase(TEST_CASES[i].description);
    AdClient client(AdClient::create_user(this));
    NSLookupRequest request;
    NSLookupRequest::Member member =
      TEST_CASES[i].flags & RS_SEARCH?
        NSLookupRequest::Member(&NSLookupRequest::search):
        TEST_CASES[i].flags & RS_FT?
          NSLookupRequest::Member(&NSLookupRequest::ft):
          NSLookupRequest::Member(&NSLookupRequest::referer_kw);

    member(request, fetch_string(TEST_CASES[i].kwd));

    if ( TEST_CASES[i].flags & RS_KN )
    {
      request.kn = 1;
    }
    request.loc_name.clear();

    unsigned long special_effects = 0;
    
    if (TEST_CASES[i].flags & RS_NO_ADV)
    {
      special_effects |= SpecialEffectsChecker::SE_NO_ADV;
    }

    if (TEST_CASES[i].flags & RS_NO_TRACK)
    {
      special_effects |= SpecialEffectsChecker::SE_NO_TRACK;
    }

    if (special_effects)
    {
      FAIL_CONTEXT(
        SpecialEffectsChecker(
          client,
          request,
          "0",
          special_effects).check(),
        TEST_CASES[i].description +
        " Check special effects");
    }
    else
    {
      client.process_request(request);

      std::list<std::string> got_channels;
      if (TEST_CASES[i].flags & RS_CHECK_TRIGGERS)
      {
        got_channels = client.debug_info.triggers.list();
      }
      else
      {
        got_channels.assign(
          client.debug_info.trigger_channels.begin(),
          client.debug_info.trigger_channels.end());
      }

      std::list<std::string> got_history_channels;
      got_history_channels.assign(
          client.debug_info.history_channels.begin(),
          client.debug_info.history_channels.end());
      

      FAIL_CONTEXT(
        ChannelsCheck(
          this,
          TEST_CASES[i].expected_channels,
          got_channels).check(),
        TEST_CASES[i].description +
          " Expected channels check");
      
      FAIL_CONTEXT(
        ChannelsCheck(
          this,
          TEST_CASES[i].unexpected_channels,
          TEST_CASES[i].flags & RS_CHECK_TRIGGERS?
            got_channels: got_history_channels,
          AutoTest::SCE_NOT_ENTRY).check(),
        TEST_CASES[i].description +
          " Unexpected channels check");
    }
  }  
  return true;
}

