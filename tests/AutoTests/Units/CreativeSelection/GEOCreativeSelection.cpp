
#include "GEOCreativeSelection.hpp"

REFLECT_UNIT(GEOCreativeSelection) (
  "CreativeSelection",
  AUTO_TEST_FAST);

namespace
{
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::ChannelsCheck ChannelsCheck;
  typedef AutoTest::SelectedCreativesCCID SelectedCreativesCCID;

  // Display. Ab Lench city. 
  const GEOCreativeSelection::TestRequest DISPLAYCITY1[] =
  {
    {"DisplayKWD", 0, 0, "DisplayChannelBP", 0},
    {0, "gb/Worcestershire-STATE/Ab-Lench-CITY",
     "DisplayCityTID", 0, "DisplayCityCC" }
  };

  // Display. Abberley city. 
  const GEOCreativeSelection::TestRequest DISPLAYCITY2[] =
  {
    {"DisplayKWD", 0, 0, "DisplayChannelBP", 0},
    {0, "gb/Worcestershire-STATE/Abberley-CITY",
     "DisplayCityTID", 0, "DisplayCityCC" }
  };

  // Display. Elmbridge city. 
  const GEOCreativeSelection::TestRequest DISPLAYCITY3[] =
  {
    {"DisplayKWD", 0, 0, "DisplayChannelBP", 0},
    {0, "gb/Worcestershire-STATE/Elmbridge-CITY",
     "DisplayCityTID", 0, 0 }
  };

  // Display. Worcestershire state level. 
  const GEOCreativeSelection::TestRequest DISPLAYSTATE[] =
  {
    {"DisplayKWD", 0, 0, "DisplayChannelBP", 0},
    {0, "gb/Worcestershire-STATE",
     "DisplayStateTID", 0, "DisplayStateCC" }
  };

  // Display. Country level. 
  const GEOCreativeSelection::TestRequest DISPLAYCOUNTRY[] =
  {
    {"DisplayKWD", 0, 0, "DisplayChannelBP", 0},
    {0, "gb", "DisplayStateTID", 0, 0 }
  };

  // Text. Abbotskerswell city.
  const GEOCreativeSelection::TestRequest TEXTCITY1[] =
  {
    {"ChannelText-1-KWD,ChannelText-2-KWD,Text-1-KWD,Text-2-KWD", 0, 0,
     "ChannelText-1-BP,ChannelText-2-BP,Text-1-BP,Text-2-BP,", 0},
    {0, "gb/Devon-STATE/Abbotskerswell-CITY",
     "TextTID", 0,
     "ChannelText-1-CC,ChannelText-2-CC,Text-1-CC,Text-2-CC"}
  };
  
  // Text. Aber city.
  const GEOCreativeSelection::TestRequest TEXTCITY2[] =
  {
    {"ChannelText-1-KWD,ChannelText-2-KWD,Text-1-KWD,Text-2-KWD", 0, 0,
     "ChannelText-1-BP,ChannelText-2-BP,Text-1-BP,Text-2-BP,", 0},
    {0, "gb/Conwy-STATE/Aber-CITY",
     "TextTID", 0,
     "ChannelText-1-CC,ChannelText-2-CC,Text-1-CC,Text-2-CC"}
  };

  // Text. Dorset state level.
  const GEOCreativeSelection::TestRequest TEXTSTATE1[] =
  {
    {"ChannelText-1-KWD,ChannelText-2-KWD,Text-1-KWD,Text-2-KWD", 0, 0,
     "ChannelText-1-BP,ChannelText-2-BP,Text-1-BP,Text-2-BP,", 0},
    {0, "gb/Dorset-STATE",
     "TextTID", 0,
     "ChannelText-1-CC,ChannelText-2-CC,Text-1-CC,Text-2-CC"}
  };

  // Text. Devon state level.
  const GEOCreativeSelection::TestRequest TEXTSTATE2[] =
  {
    {"ChannelText-1-KWD,ChannelText-2-KWD,Text-1-KWD,Text-2-KWD", 0, 0,
     "ChannelText-1-BP,ChannelText-2-BP,Text-1-BP,Text-2-BP,", 0},
    {0, "gb/Devon-STATE",
     "TextTID", 0,
     "ChannelText-2-CC,Text-1-CC,Text-2-CC"}
  };

  // Text. Different state.
  const GEOCreativeSelection::TestRequest DIFFSTATE[] =
  {
    {"ChannelText-1-KWD,ChannelText-2-KWD,Text-1-KWD,Text-2-KWD", 0, 0,
     "ChannelText-1-BP,ChannelText-2-BP,Text-1-BP,Text-2-BP,", 0},
    {0, "gb/Angus", "TextTID", 0, 0}
  };

  // Text. Same country, no state.
  const GEOCreativeSelection::TestRequest SAMECOUNTRYSTATE[] =
  {
    {"ChannelText-1-KWD,ChannelText-2-KWD,Text-1-KWD,Text-2-KWD", 0, 0,
     "ChannelText-1-BP,ChannelText-2-BP,Text-1-BP,Text-2-BP,", 0},
    {0, "gb", "TextTID", 0, 0}
  };

  // Text. Conwy state level.
  const GEOCreativeSelection::TestRequest TEXTSTATE3[] =
  {
    {"ChannelText-1-KWD,ChannelText-2-KWD,Text-1-KWD,Text-2-KWD", 0, 0,
     "ChannelText-1-BP,ChannelText-2-BP,Text-1-BP,Text-2-BP,", 0},
    {0, "gb/Conwy-STATE",
     "TextTID", 0, "Text-1-CC"}
  };

  // Alternative Geo-Channel Name. Hallingbury city.
  const GEOCreativeSelection::TestRequest ALTNAME1[] =
  {
    {"AltNameKWD", 0, 0, "AltNameBP", 0},
    {0, "gb/Hertford-STATE/Hallingbury-CITY",
     "AltNameTID", 0, "AltNameCC" }
  };

  // Alternative Geo-Channel Name.
  // Great Hallingbury - alternative name.
  const GEOCreativeSelection::TestRequest ALTNAME2[] =
  {
    {"AltNameKWD", 0, 0, "AltNameBP", 0},
    {0, "gb/Hertford-STATE/Hallingbury-ALTCITY",
     "AltNameTID", 0, "AltNameCC" }
  };

  // GEO Competition.
  const GEOCreativeSelection::TestRequest COMPETITION[] =
  {
    {"CompetitionKWD", 0, 0, "CompetitionBP", 0},
    {0, "gb/Highland-STATE", "CompetitionTID",
     0, "CompetitionStateCC" },
    {0, "gb/Highland-STATE/Acharacle-CITY",
     "CompetitionTID", 0, "CompetitionCityCC" }
  };
}

bool
GEOCreativeSelection::run_test()
{
  NOSTOP_FAIL_CONTEXT(
    testcase(
      "Display. Ab Lench city.",
      DISPLAYCITY1));

  NOSTOP_FAIL_CONTEXT(
    testcase(
      "Display. Abberley city.",
      DISPLAYCITY2));

  NOSTOP_FAIL_CONTEXT(
    testcase(
      "Display. Elmbridge city.",
      DISPLAYCITY3));

  NOSTOP_FAIL_CONTEXT(
    testcase(
      "Display. Worcestershire state level.",
      DISPLAYSTATE));
  
  NOSTOP_FAIL_CONTEXT(
    testcase(
      "Display. Country level.",
      DISPLAYCOUNTRY));

  NOSTOP_FAIL_CONTEXT(
    testcase(
      "Text. Abbotskerswell city.",
      TEXTCITY1));

  NOSTOP_FAIL_CONTEXT(
    testcase(
      "Text. Aber city.",
      TEXTCITY2));

  NOSTOP_FAIL_CONTEXT(
    testcase(
      "Text. Dorset state level.",
      TEXTSTATE1));

  NOSTOP_FAIL_CONTEXT(
    testcase(
      "Text. Devon state level.",
      TEXTSTATE2));

  NOSTOP_FAIL_CONTEXT(
    testcase(
      "Text. Conwy state level.",
      TEXTSTATE3));
  
  NOSTOP_FAIL_CONTEXT(
    testcase(
      "Text. Different state.",
      DIFFSTATE));

  NOSTOP_FAIL_CONTEXT(
    testcase(
      "Text. Same country, no state.",
      SAMECOUNTRYSTATE));

  NOSTOP_FAIL_CONTEXT(
    testcase(
      "Alternative Geo-Channel Name. "
      "Hallingbury city.",
      ALTNAME1));

  NOSTOP_FAIL_CONTEXT(
    testcase(
      "Alternative Geo-Channel Name. "
      "Great Hallingbury - alternative name.",
      ALTNAME2));

  NOSTOP_FAIL_CONTEXT(
    testcase(
      "GEO Competition.",
      COMPETITION));

  return true;
}

template <size_t COUNT>
void GEOCreativeSelection::testcase(
  const std::string& description,
  const TestRequest (&requests)[COUNT])
{
  add_descr_phrase(description);
  AdClient client(AdClient::create_user(this));
  for (size_t i = 0; i <  COUNT; ++i)
  {
    NSLookupRequest request;
    set_param(request.loc_name, requests[i].location, "/");
    set_param(request.tid,  requests[i].tid);
    set_param(request.referer_kw,  requests[i].keyword);
    client.process_request(request);

    FAIL_CONTEXT(
      ChannelsCheck(
        this,
        requests[i].expected_channels,
        client.debug_info.trigger_channels).check(),
      description +
        " Expected trigger_channels#" + strof(i+1));
    
    if (requests[i].expected_ccids)
    {
      std::list<std::string> ccids;

      String::SubString s(requests[i].expected_ccids);
      String::StringManip::SplitComma tokenizer(s);

      String::SubString token;
      
      while (tokenizer.get_token(token))
      {
        ccids.push_back(fetch_string(token.str()));
      }
      
      FAIL_CONTEXT(
        AutoTest::sequence_checker(
          ccids,
          SelectedCreativesCCID(client)).check(),
        description +
        " Expected creatives#" +
        strof(i+1));
    }
    else
    {
      FAIL_CONTEXT(
        AutoTest::equal_checker(
          "0",
          client.debug_info.ccid).check(),
        description +
        " Expected creatives#" +
        strof(i+1));    
    }
  }
}

void
GEOCreativeSelection::set_param(
  NSLookupRequest::NSLookupParam& param,
  const char* param_name,
  const char* separator)
{
  if (param_name)
  {
    param = map_objects(param_name, separator);
  }
}
