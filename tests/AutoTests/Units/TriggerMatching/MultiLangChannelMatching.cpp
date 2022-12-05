#include "MultiLangChannelMatching.hpp"

REFLECT_UNIT(MultiLangChannelMatching) (
  "TriggerMatching",
  AUTO_TEST_FAST
);

namespace
{

  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::ChannelsCheck ChannelsCheck;
  
  const unsigned int REQUEST_SIZE = 2;

  struct UrlChannelCase
  {
    std::string description;
    const char* referer;
    const char* expected_channels;
  };


  const MultiLangChannelMatching::KeywordChannelCase SIMPLE_CASES[] =
  {
    {
      "Korean page.",
      "KEYWORD1",
      "KEYWORD1",
      "PAGECHANNEL1,SEARCHCHANNEL1,URLKWDCHANNEL1",
      0,
      0,
      0
    },
    {
      "Korean search.",
      0,
      "KEYWORD2",
      "SEARCHCHANNEL2,URLKWDCHANNEL2",
      0,
      0,
      0
    },
    {
      "Common case folding.",
      "FT3",
      "KEYWORD3",
      "PAGECHANNEL3,SEARCHCHANNEL3,URLKWDCHANNEL3",
      0,
      0,
      MultiLangChannelMatching::KCE_USE_FT
    },
    {
      "Non-Folded channel triggers and referer keywords.",
      "KEYWORD4_1",
      "KEYWORD4",
      "PAGECHANNEL4,SEARCHCHANNEL4,URLKWDCHANNEL4",
      0,
      0,
      0
    },
    {
      "Non-Folded referer keywords (case#1).",
      "KEYWORD5_1",
      "KEYWORD5_2",
      "PAGECHANNEL5,PAGECHANNEL6,PAGECHANNEL7,"
      "SEARCHCHANNEL5,SEARCHCHANNEL6,SEARCHCHANNEL7,"
      "URLKWDCHANNEL5,URLKWDCHANNEL6,URLKWDCHANNEL7",
      0,
      0,
      0
    },
    {
      "Non-Folded referer keywords (case#2).",
      "KEYWORD5_2",
      "KEYWORD5_1",
      "PAGECHANNEL5,PAGECHANNEL6,PAGECHANNEL7,"
      "SEARCHCHANNEL5,SEARCHCHANNEL6,SEARCHCHANNEL7,"
      "URLKWDCHANNEL5,URLKWDCHANNEL6,URLKWDCHANNEL7",
      0,
      0,
      0
    },
    {
      "Folded referer keywords.",
      "KEYWORD6_1",
      "KEYWORD6_2",
      "PAGECHANNEL5, PAGECHANNEL6, PAGECHANNEL7,"
      "SEARCHCHANNEL5,SEARCHCHANNEL6,SEARCHCHANNEL7,"
      "URLKWDCHANNEL5,URLKWDCHANNEL6,URLKWDCHANNEL7",
      0,
      0,
      0
    },
    {
      "Simple case folding (case#1).",
      "KEYWORD11_1",
      "KEYWORD11_1",
      "PAGECHANNEL8,SEARCHCHANNEL8,URLKWDCHANNEL8",
      0,
      0,
      0
    },
    {
      "Simple case folding (case#2).",
      "KEYWORD11_2",
      "KEYWORD11_2",
      "PAGECHANNEL8,SEARCHCHANNEL8,URLKWDCHANNEL8",
      0,
      0,
      0
    },
    {
      "Special case.",
      "KEYWORD12",
      "KEYWORD12",
      "PAGECHANNEL9,SEARCHCHANNEL9,URLKWDCHANNEL9",
      0,
      0,
      0
    }
  };

  // Keyword unification for punctuation and combining characters
  const MultiLangChannelMatching::KeywordChannelCase KWD_UNIFICATION[] =
  {
    {
      "Keyword unification for punctuation"
      " and combining characters#1.",
      "KEYWORD14_1",
      "KEYWORD14",
      "PAGECHANNEL11,SEARCHCHANNEL11,URLKWDCHANNEL11",
      0,
      0,
      0
    },
    {
      "Keyword unification for punctuation"
      " and combining characters#2. "
      "Combining, dotless, uppercased.",
      "KEYWORD15_1",
      "KEYWORD15_2",
      "PAGECHANNEL12,PAGECHANNEL14,PAGECHANNEL15,"
      "SEARCHCHANNEL12,SEARCHCHANNEL14,SEARCHCHANNEL15,"
      "URLKWDCHANNEL12,URLKWDCHANNEL14,URLKWDCHANNEL15",
      0,
      0,
      0
    },
    {
      "Keyword unification for punctuation"
      " and combining characters#3.",
      "FT16",
      "KEYWORD16_1",
      "PAGECHANNEL13,SEARCHCHANNEL13,URLKWDCHANNEL13",
      0,
      0,
      MultiLangChannelMatching::KCE_USE_FT
    },
    {
      "Keyword unification for punctuation"
      " and combining characters#4. "
      "Normalization of hard trigger.",
      "KEYWORD17_1",
      "KEYWORD17_2",
      "SEARCHCHANNEL16,URLKWDCHANNEL16",
      "PAGECHANNEL16",
      0,
      0
    },
    {
      "Keyword unification for punctuation"
      " and combining characters#5. "
      "Soft to hard 1.",
      "KEYWORD18_1",
      "KEYWORD18_2",
      "SEARCHCHANNEL16,URLKWDCHANNEL16",
      "PAGECHANNEL16",
      0,
      0
    },
    {
      "Keyword unification for punctuation"
      " and combining characters#6. "
      "Soft to hard 2.",
      "KEYWORD19_1",
      "KEYWORD19_2",
      "PAGECHANNEL16",
      "SEARCHCHANNEL16,URLKWDCHANNEL16",
      0,
      0
    }
  };

  // Normalization of full and half width characters.
  const MultiLangChannelMatching::KeywordChannelCase NORMALIZATION[] =
  {
    {
      "Normalization of full and "
      "half width characters#1. "
      "Fullwidth: latin letters and digits.",
      "KEYWORD21_1",
      "KEYWORD21",
      "PAGECHANNEL17,SEARCHCHANNEL17,URLKWDCHANNEL17",
      0,
      0,
      0
    },
    {
      "Normalization of full and "
      "half width characters#2. "
      "Halfwidth: katakana.",
      "KEYWORD23",
      "KEYWORD24",
      "PAGECHANNEL17,PAGECHANNEL18,"
      "SEARCHCHANNEL17,SEARCHCHANNEL18,"
      "URLKWDCHANNEL17,URLKWDCHANNEL18",
      0,
      0,
      0
    },
    {
      "Normalization of full and "
      "half width characters#3.",
      "KEYWORD22",
      "KEYWORD25",
      "PAGECHANNEL17,PAGECHANNEL18,"
      "SEARCHCHANNEL17,SEARCHCHANNEL18,"
      "URLKWDCHANNEL17,URLKWDCHANNEL18",
      0,
      0,
      0
    }
  };

  // Segmentation
  const MultiLangChannelMatching::KeywordChannelCase SEGMENTATION[] =
  {
    {
      "Segmentation#1.",
      "FT26",
      "KEYWORD26",
      "PAGECHANNEL19,SEARCHCHANNEL19,URLKWDCHANNEL19",
      0,
      0,
      MultiLangChannelMatching::KCE_USE_FT
    },
    {
      "Segmentation#2. Segmented keyword.",
      "KEYWORD26_1",
      "KEYWORD26_1",
      "PAGECHANNEL19,SEARCHCHANNEL19,URLKWDCHANNEL19",
      0,
      0,
      MultiLangChannelMatching::KCE_USE_FT
    },
    {
      "Segmentation#3.",
      "KEYWORD27",
      "KEYWORD27",
      "PAGECHANNEL19,SEARCHCHANNEL19,URLKWDCHANNEL19",
      0,
      0,
      0
    },
    {
      "Segmentation#5. Exact matching.",
      0,
      "KEYWORD28",
      "SEARCHCHANNEL19",
      "URLKWDCHANNEL19",
      0,
      0
    },
    {
      "Segmentation#6. Exact matching (with stop words).",
      0,
      "KEYWORD29",
      "SEARCHCHANNEL19",
      "URLKWDCHANNEL19",
      0,
      0
    },
    {
      "Segmentation#7. Hard matching.",
      "KEYWORD30_1",
      "KEYWORD30_1",
      "PAGECHANNEL19,SEARCHCHANNEL19,URLKWDCHANNEL19",
      0,
      0,
      0
    },
    {
      "Segmentation#4.Use 'kn'.",
      "KEYWORD27",
      "KEYWORD27",
      "SEARCHCHANNEL19,URLKWDCHANNEL19",
      "PAGECHANNEL19",
      0,
      MultiLangChannelMatching::KCE_USE_KN
    }
  };

 const UrlChannelCase URL_CASES[] =
 {
    {
      "Referer in IDNA 2003.",
      "URL1",
      "URLCHANNEL1"
    },
    {
      "Referer in IDNA 2008.",
      "URL2",
      "URLCHANNEL1"
    }
  };

  const MultiLangChannelMatching::KeywordChannelCase LINGJOIN_REMOTE[] =
  {
    {
      "LingJoin remote. Channel country/language and cluster "
      "configuration (lingjoin segmentation in ad request)",
      "KEYWORDLJ1_1",
      "KEYWORDLJ1_2",
      "PAGELJ1,SEARCHLJ1,URLKWDLJ1",
      "PAGELJ2,SEARCHLJ2,URLKWDLJ2",
      "PHRASELG1_1",
      MultiLangChannelMatching::KCE_USE_KN
    },
    {
      "LingJoin remote. Channel country/language and cluster "
      "configuration (wrong polyglot segmentation in ad request)",
      "KEYWORDLJ2",
      0,
      "PAGELJ2",
      "PAGELJ1",
      0,
      MultiLangChannelMatching::KCE_USE_KN
    },
    {
      "LingJoin remote. Hard trigger and ft parameter.",
      "FTLJ3",
      "KEYWORDLJ3",
      "PAGELJ1,SEARCHLJ1,URLKWDLJ1",
      0,
      "PHRASELG1_3",
      MultiLangChannelMatching::KCE_USE_FT
    },
    {
      "LingJoin remote. Exact trigger.",
      0,
      "KEYWORDLJ4",
      "SEARCHLJ1",
      0,
      "PHRASELG1_4",
      0
    },
    {
      "LingJoin remote. kn = 1.",
      "FTLJ5",
      "KEYWORDLJ5",
      "SEARCHLJ1,URLKWDLJ1",
      "PAGELJ1",
      "PHRASELG1_5",
      MultiLangChannelMatching::KCE_USE_KN | MultiLangChannelMatching::KCE_USE_FT
    }
  };
  
  const MultiLangChannelMatching::KeywordChannelCase LINGJOIN_CENTRAL[] =
  {
    {
      "LingJoin central. Channel country/language and cluster "
      "configuration (lingjoin segmentation in ad request)",
      "KEYWORDLJ1_1",
      "KEYWORDLJ1_2",
      "PAGELJ1,SEARCHLJ2,URLKWDLJ2",
      "PAGELJ2,SEARCHLJ1,URLKWDLJ1",
      "PHRASELG2_1",
      MultiLangChannelMatching::KCE_USE_KN
    },
    {
      "LingJoin central. Channel country/language and cluster "
      "configuration (wrong polyglot segmentation in ad request)",
      "KEYWORDLJ2",
      0,
      "PAGELJ2",
      "PAGELJ1",
      0,
      MultiLangChannelMatching::KCE_USE_KN
    },
    {
      "LingJoin central. Frontend test.",
      "FTLJ3",
      "KEYWORDLJ3",
      0,
      "PAGELJ1,SEARCHLJ1,URLKWDLJ1",
      "PHRASELG2_3",
      MultiLangChannelMatching::KCE_USE_FT
    }
  };
}

bool 
MultiLangChannelMatching::run_test()
{
  {
    AdClient client(AdClient::create_user(this));
    
    AUTOTEST_CASE(
      keyword_channels(client, SIMPLE_CASES),
      "Simple cases");

    AUTOTEST_CASE(
      keyword_channels(client, KWD_UNIFICATION),
      "Keyword unification for punctuation and combining characters");

    AUTOTEST_CASE(
      keyword_channels(client, NORMALIZATION),
      "Normalization of full and half width characters");

    AUTOTEST_CASE(
      keyword_channels(client, SEGMENTATION),
      "Segmentation");
  }

  if (get_config().check_service(CTE_CENTRAL, STE_FRONTEND))
  {
    AdClient client(AdClient::create_user(this));
    client.change_base_url(
      get_config().get_service(CTE_CENTRAL, STE_FRONTEND).address.c_str());

    AUTOTEST_CASE(
      keyword_channels(
        client,
        LINGJOIN_CENTRAL,
        // to avoid NOADS colocation trouble
        fetch_int("COLO")),
      "Lingjoin central");
  }

  if (get_config().check_service(CTE_REMOTE2, STE_FRONTEND))
  {
    AdClient client(
      AdClient::create_user(
        this, AutoTest::UF_FRONTEND_MINOR));
    
    AUTOTEST_CASE(
      keyword_channels(client, LINGJOIN_REMOTE),
      "Lingjoin remote");
  }

  AUTOTEST_CASE(
    url_channels(),
    "URL cases");
  return true;
}

template<size_t Count>
void
MultiLangChannelMatching::keyword_channels(
  AdClient& client,
  const KeywordChannelCase(&tests)[Count],
  unsigned long colo)
{
  for (unsigned int i = 0; i < countof(tests); ++i)
  {
    add_descr_phrase(tests[i].description);
    NSLookupRequest request;
    if (colo)
    {
      request.colo = colo;
    }
    if (tests[i].referer_kw)
    {
      if (tests[i].flags & KCE_USE_FT)
      {
        request.ft = fetch_string(tests[i].referer_kw);
      }
      else
      {
        request.referer_kw = fetch_string(tests[i].referer_kw);
      }
    }
    if (tests[i].search_kw)
    {
      request.search = fetch_string(tests[i].search_kw);
    }
    if (tests[i].flags & KCE_USE_KN)
    {
      request.kn = 1;
    }
    client.process_request(request);
    
    FAIL_CONTEXT(
      ChannelsCheck(
        this,
        tests[i].expected_channels,
        client.debug_info.trigger_channels).check(),
      tests[i].description +
        " Expected trigger_channels check");
    
    if (tests[i].expected_search_phrase)
    {
      FAIL_CONTEXT(
        AutoTest::equal_checker(
          fetch_string(tests[i].expected_search_phrase),
          client.debug_info.search_phrase).check(),
        tests[i].description +
          " Expected search-phrase check")
    }

    if (tests[i].unexpected_channels)
    {
      FAIL_CONTEXT(
        ChannelsCheck(
          this,
          tests[i].unexpected_channels,
          client.debug_info.trigger_channels,
          AutoTest::SCE_NOT_ENTRY).check(),
        tests[i].description +
            " Unexpected trigger_channels check");
    }
   
  }  
}

void
MultiLangChannelMatching::url_channels()
{
  AdClient client(AdClient::create_user(this));
  for (unsigned int i = 0; i < countof(URL_CASES); ++i)
  {
    add_descr_phrase(URL_CASES[i].description);
    NSLookupRequest request;
    if (URL_CASES[i].referer)
    {
      request.referer = fetch_string(URL_CASES[i].referer);
    }
    client.process_request(request);

    FAIL_CONTEXT(
      ChannelsCheck(
        this,
        URL_CASES[i].expected_channels,
        client.debug_info.trigger_channels).check(),
      URL_CASES[i].description +
          " Expected trigger_channels check");
  }  
}
