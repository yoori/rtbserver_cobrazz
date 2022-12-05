
#include "BrokenChannelTriggers.hpp"
 
REFLECT_UNIT(BrokenChannelTriggers) (
  "TriggerMatching",
  AUTO_TEST_FAST
);

namespace {
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::ChannelsCheck ChannelsCheck;

  // Incorrect trigger definition test
  const BrokenChannelTriggers::TestCase INCORRECT_TRIGGER[] =
  {
    {
      "Keywords 1.",
      "ITR/KWD1", "ITR/REF1",
      "ITR/Channel1U", "ITR/Channel2P"
    },
    {
      "Keywords 2.",
      "ITR/KWD2", "ITR/REF2",
      "ITR/Channel1U,ITR/Channel2P", 0
    },
    {
      "Keywords 3.",
      "ITR/KWD3", "ITR/REF3",
      "ITR/Channel1U,ITR/Channel2P", 0
    },
    {
      "Keywords 4 (stop words).",
      "ITR/KWD4", "ITR/SEARCH4", 0,
      "ITR/Channel1U,ITR/Channel2U,"
      "ITR/Channel2P,ITR/Channel2S"
    },
    {
      "URLs 1.",
      "ITR/KWD5", "ITR/REF5",
      "ITR/Channel2P", "ITR/Channel1U"
    },
    {
      "URLs 2.",
      "ITR/KWD6", "ITR/REF6",
      "ITR/Channel1U,ITR/Channel2P", 0
    },
    {
      "Exact keywords.",
      "ITR/KWD7", "ITR/SEARCH7",
      "ITR/Channel1P,ITR/Channel2S",
      "ITR/Channel2P"
    },
  };

  // Trigger length limit test. Part#1.
  const BrokenChannelTriggers::TestCase TRIGGER_LENGTH_LIMIT1[] =
  {
    {
      "Keyword of 510 bytes.",
      "LEN/KWD1", 0,
      "LEN/ChannelP", 0
    },
    {
      "Keyword of 512 bytes.",
      "LEN/KWD2", 0,
      "LEN/ChannelP", 0
    },
    {
      "Keyword of 514 bytes.",
      "LEN/KWD3", 0,
      0, "LEN/ChannelP"
    },
    { "Ad request with normal "
      "and very long keywords.",
      "LEN/KWD4", 0,
      "LEN/ChannelP", 0
    }
  };

  // Trigger length limit test. Part#2.
  const BrokenChannelTriggers::TestCase TRIGGER_LENGTH_LIMIT2[] =
  {
    {
      "Ad request with referer "
      "of 2046 bytes.",
      0, "LEN/REF5",
      "LEN/ChannelU", 0
    },
    {
      "Ad request with referer "
      "of 2048 bytes.",
      0, "LEN/REF6",
      "LEN/ChannelU", 0
    },
    {
      "Ad request with referer "
      "of 2050 bytes.",
      0, "LEN/REF7",
      0, "LEN/ChannelU"
    },
    {
      "Ad request with exact keyword of 512 bytes.",
      0, "LEN/SEARCH8",
      "LEN/ChannelS", 0
    },
    {
      "Ad request with exact keyword of 513 bytes.",
      0, "LEN/SEARCH9",
      0, "LEN/ChannelS"
    }
  };

  // Triggers word count limit test
  const BrokenChannelTriggers::TestCase TRIGGER_WORDS_LIMIT[] =
  {
    {
      "Keyword = 20 words",
      "WORDS/KWD1", "WORDS/SEARCH1",
      "WORDS/ChannelP,WORDS/ChannelS,WORDS/ChannelR",
      0
    },
    {
      "Keyword > 20 words",
      "WORDS/KWD2", "WORDS/SEARCH2", 0,
      "WORDS/ChannelP,WORDS/ChannelS,WORDS/ChannelR"
    },
    {
      "Negative keyword > 20 words",
      "WORDS/KWD3", 0,
      "WORDS/ChannelP", 0
    },
    {
      "Hard trigger inside soft",
      "WORDS/KWD4", 0,
      "WORDS/ChannelP", 0
    },
    {
      "Trigger before segmentation > 20 "
      "words after < 20 (korean)",
      "WORDS/KWD5", 0,
      "WORDS/ChannelP", 0
    },
    {
      "Exact keyword > 20 words "
      "(9 words of them in quotes)",
      0, "WORDS/SEARCH6", 0, 
      "WORDS/ChannelP,WORDS/ChannelS,WORDS/ChannelR"      
    }
  };

  // Triggers word count limit test
  const BrokenChannelTriggers::TestCase HARD_WORDS_LIMIT[] =
  {
    {
      "20 words in hard keyword",
      "HARD/KWD1", "HARD/SEARCH1",
      "HARD/ChannelP,HARD/ChannelS,HARD/ChannelR",
      0
    },
    {
      "21 words in hard keyword",
      "HARD/KWD2", "HARD/SEARCH2", 0,
      "HARD/ChannelP,HARD/ChannelS,HARD/ChannelR",
    },
    {
      "21 korean words in hard keyword",
      "HARD/KWD3", "HARD/SEARCH3", 0,
      "HARD/ChannelP,HARD/ChannelS,HARD/ChannelR",
    }
  };
}
 
bool 
BrokenChannelTriggers::run_test()
{

  AUTOTEST_CASE(
    test_group(INCORRECT_TRIGGER),
    "Incorrect trigger definition");
  
  AUTOTEST_CASE(
    test_group(TRIGGER_LENGTH_LIMIT1),
    "Trigger length limit");
  
  AUTOTEST_CASE(
    test_group(TRIGGER_LENGTH_LIMIT2),
    "Trigger length limit");
  
  AUTOTEST_CASE(
    test_group(TRIGGER_WORDS_LIMIT),
    "Triggers word count limit");

  AUTOTEST_CASE(
    test_group(HARD_WORDS_LIMIT),
    "Hard keyword words count limit test");

  return true;
}

template<size_t Count>
void
BrokenChannelTriggers::test_group(
  const TestCase(&tests)[Count])
{
  AdClient client(AdClient::create_user(this));
  for (size_t i = 0; i < Count; ++i)
  {
    NOSTOP_FAIL_CONTEXT(test_case(client, tests[i]));
  }
}

void
BrokenChannelTriggers::test_case(
  AdClient& client,
  const TestCase& test)
{
  add_descr_phrase(test.description);
  NSLookupRequest request;
  if (test.referer)
  {
    if (strstr(test.referer, "SEARCH"))
    {
      request.search = fetch_string(test.referer);
    }
    else
    {
      request.referer = fetch_string(test.referer);
    }
  }
  if (test.referer_kw)
  {
    request.referer_kw = fetch_string(test.referer_kw);
  }
  client.process_request(request);
  
  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      test.matched,
      client.debug_info.trigger_channels).check(),
    test.description +
      " Expected trigger_channels");
  
  FAIL_CONTEXT(
    ChannelsCheck(
      this,
      test.unmatched,
      client.debug_info.trigger_channels,
      AutoTest::SCE_NOT_ENTRY).check(),
    test.description +
      " Unexpected trigger_channels");
}


