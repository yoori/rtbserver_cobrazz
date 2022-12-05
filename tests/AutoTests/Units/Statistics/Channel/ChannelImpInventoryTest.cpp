#include "ChannelImpInventoryTest.hpp"
 
REFLECT_UNIT(ChannelImpInventoryTest) (
  "Statistics",
  AUTO_TEST_SLOW
);

namespace
{
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::NSLookupRequest NSLRequest;
  typedef AutoTest::DebugInfo::SelectedCreativesList SelectedCreativesList;

  const double PRECISION = 0.00001;
}

void 
ChannelImpInventoryTest::init_(
  ORM::StatsList<ORM::ChannelImpInventory>& stats,
  std::list<ORM::ChannelImpInventory::Diffs>& diffs,
  const std::string& prefix,
  size_t size)
{

  IntSeq channels;
  fetch_list_(channels, prefix + "/Channels");
 
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      channels.size() * 2,
      size).check(),
    prefix + ". Invalid test data");

  int colo = fetch_colo_(prefix);
  if (!colo)
    colo = default_colo_;

  for (size_t i = 0; i < channels.size()*2; ++i)
  {
    ORM::ChannelImpInventory stat(
      ORM::ChannelImpInventory::Key().
        channel_id(channels[i / 2]).
        ccg_type(i % 2? "T": "D").
        colo_id(colo));
    
    stat.description(prefix);
    stat.select(pq_conn_);
    stats.push_back(stat);
  }

  for (size_t i = 0; i < size; ++i)
  {
    std::string i_str(strof(i));
    diffs.push_back(
      ORM::ChannelImpInventory::Diffs().
        imps( 
          ORM::stats_diff_type( 
            static_cast<double>(fetch_float(prefix + i_str + "-0")), PRECISION)).
        clicks( 
          ORM::stats_diff_type(
            static_cast<double>(fetch_float(prefix + i_str + "-1")), PRECISION)).
        actions( 
          ORM::stats_diff_type(
            static_cast<double>(fetch_float(prefix + i_str + "-2")), PRECISION)).
        revenue( 
          ORM::stats_diff_type(
            static_cast<double>(fetch_float(prefix + i_str + "-3")), PRECISION)).
        impops_user_count( 
          ORM::stats_diff_type(
            static_cast<double>(fetch_float(prefix + i_str + "-4")), PRECISION)).
        imps_user_count( 
          ORM::stats_diff_type(
            static_cast<double>(fetch_float(prefix + i_str + "-5")), PRECISION)).
        imps_value( 
          ORM::stats_diff_type(
            static_cast<double>(fetch_float(prefix + i_str + "-6")), PRECISION)).
        imps_other( 
          ORM::stats_diff_type(
            static_cast<double>(fetch_float(prefix + i_str + "-7")), PRECISION)).
        imps_other_user_count(
          ORM::stats_diff_type(
            static_cast<double>(fetch_float(prefix + i_str + "-8")), PRECISION)).
        imps_other_value(
          ORM::stats_diff_type(
            static_cast<double>(fetch_float(prefix + i_str + "-9")), PRECISION)).
        impops_no_imp(
          ORM::stats_diff_type(
            static_cast<double>(fetch_float(prefix + i_str + "-10")), PRECISION)).
        impops_no_imp_user_count(
          ORM::stats_diff_type(
            static_cast<double>(fetch_float(prefix + i_str + "-11")), PRECISION)).
        impops_no_imp_value(
          ORM::stats_diff_type(
            static_cast<double>(fetch_float(prefix + i_str + "-12")), PRECISION)));
  }
}

void
ChannelImpInventoryTest::set_up()
{ }

bool
ChannelImpInventoryTest::run()
{
  count_ =  fetch_int("COUNT");
  default_colo_ = fetch_int("DefaultColo");

  AUTOTEST_CASE(
    test_case("ImpsScenario", 6, SEND_ALL_ACTIONS),
    "Impressions, clicks & actions (display)");

  AUTOTEST_CASE(
    test_case("NoImps", 2),
    "Counter no_imps");

  AUTOTEST_CASE(
    test_case("NoImpsOpportunity", 2, SEND_NO_ACTIONS),
    "No impression opportunity");

  AUTOTEST_CASE(
    test_case("ImpsOtherChannel", 4),
    "Counter imps_other (Text channel targeted)");

  AUTOTEST_CASE(
    test_case("ImpsOtherKeyword", 10, SEND_TID|SEND_CLICKS),
    "4 Text Creatives Served in 4-slot banner");

  AUTOTEST_CASE(
    test_case(
      "NoCookies",
      2,
      SEND_TID|SEND_CLICKS|SEND_NO_COOKIES|SEND_REFERER),
    "No Cookies");

  AUTOTEST_CASE(
    test_case(
      "OOUser",
      2,
      SEND_TID|SEND_CLICKS|SEND_NO_COOKIES|SEND_OO_USER|SEND_REFERER),
    "OO User");

  AUTOTEST_CASE(
    test_case("ImpsOtherDisplay", 6),
    "Display Creative Served in 2-slot banner");

  AUTOTEST_CASE(
    test_case("NotVerifiedImpressions", 4, SEND_TID|SEND_FORMAT),
    "Not verified impressions");

  AUTOTEST_CASE(
    test_case("1Keyword", 4),
    "Text Creative Served in 4-slot banner");

  AUTOTEST_CASE(
    test_case("2KeywordsOpportunity", 8),
    "2 Text Creatives Served in 4-slot banner");

  AUTOTEST_CASE(
    test_case("2KeywordsDisplay", 4),
    "Display CCG eCPM taken as impression "
    "opportunity value");

  AUTOTEST_CASE(
    test_case("2KeywordsDifference", 8),
    "Top ad ecpm calculated as the difference "
    "to cover minimum cpm");

  AUTOTEST_CASE(
    test_case("NoAdKeyword", 4),
    "No Ad Served in 4-slot banner");

  AUTOTEST_CASE(
    test_case("1Channel2Keywords", 6, SEND_TID|SEND_CLICKS),
    "2 Text Creatives Served on the same "
    "keyword in 4-slot banner");

  AUTOTEST_CASE(
    test_case("ChannelTargeted", 6),
    "Channel Targeted Text CCGs");

  AUTOTEST_CASE(
    test_case("KeywordvsChannelTargeted", 4),
    "Keyword & Channel Targeted Text CCGs");

  AUTOTEST_CASE(
    test_case("1ChannelTextDisplay", 4),
    "Channel used in the Text and Display CCGs");

  AUTOTEST_CASE(
    test_case("NotServeTextAds", 2),
    "Channel Inventory on tags which can't serve text ads");

  AUTOTEST_CASE(
    test_case("NonDefaultCurrency", 4),
    "Campaigns, tags with non-system currency");

  AUTOTEST_CASE(
    test_case("TagAdjustment", 6),
    "Tag with adjustment (ADSC-5502)");

  AUTOTEST_CASE(
    colo_case(),
    "Colo id logging (ADSC-7276)");

  return true;  
}

void
ChannelImpInventoryTest::tear_down()
{ }

unsigned long
ChannelImpInventoryTest::fetch_colo_(const std::string& prefix)
{
  std::string object_name = prefix + "/COLO";
  
  Locals all_locals = get_local_params();
  size_t locals_len = all_locals.DataElem().size();
    
  for (size_t ind = 0; ind < locals_len; ++ind)
  {
    if (all_locals.DataElem()[ind].Name() == object_name)
    {
      DataElemObjectRef object = all_locals.DataElem()[ind];
      unsigned long ret;
      Stream::Parser istr(object.Value());
      istr >> ret;
      return ret;
    }
  }

  return 0;
}

void
ChannelImpInventoryTest::fetch_list_(
  StringSeq& seq, 
  const std::string& prefix)
{
  DataElemObjectPtr object;
  while (next_list_item(object, prefix))
  {
    seq.push_back(object->Value());
  }
}

void
ChannelImpInventoryTest::fetch_list_(
  IntSeq& seq, 
  const std::string& prefix)
{
  DataElemObjectPtr object;
  while (next_list_item(object, prefix))
  {
    int value = 0;
    Stream::Parser istr(object->Value());
    istr >> value;
    seq.push_back(value);
  }
}

void
ChannelImpInventoryTest::check_channels_(
  const std::string& prefix,
  AutoTest::AdClient& client)
{
  StringSeq channels;
  fetch_list_(channels, prefix);
  if (channels.empty())
  {
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        client.debug_info.history_channels.empty()),
      "must got empty history channels: " + prefix);
  }
  else
  {
    if (channels.size() > 1)
    {
      for (size_t ch = 0; ch < channels.size(); ++ch)
      {
        FAIL_CONTEXT(
          AutoTest::entry_checker(
            channels[ch],
            client.debug_info.history_channels).check(),
          "channel (" + channels[ch]  +
            ") must present in server response: " + prefix);
      }
    }
    else
    {
      if (channels[0] != "0")
      {
        FAIL_CONTEXT(
          AutoTest::entry_checker(
            channels[0],
            client.debug_info.history_channels).check(),
          "channel (" + channels[0] +
            ") must present in server response: " + prefix);
      }
    }
  }  
}

void
ChannelImpInventoryTest::check_cretives_(
  const std::string& prefix,
  AutoTest::AdClient& client)
{
  StringSeq creatives;
  fetch_list_(creatives, prefix);
  if (creatives.empty())
  {
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        client.debug_info.selected_creatives.empty()),
      "must got empty selected creatives: " + prefix);
  }
  else
  {
    if (creatives.size() > 1)
    {
      FAIL_CONTEXT(
        AutoTest::sequence_checker(
          creatives,
          AutoTest::SelectedCreativesCCID(client)).check(),
        "must got expected creatives: " + prefix);
    }
    else
    {
      if (creatives[0] != "0")
      {
        FAIL_CONTEXT(
          AutoTest::equal_checker(
            creatives[0],
            client.debug_info.ccid).check(),
          "must got expected ccid (" +
            creatives[0] + ") : " + prefix);
      }
    }
  }
}

void
ChannelImpInventoryTest::do_case_requests_(
  const std::string& prefix, 
  int flags)
{
  StringSeq tags;
  StringSeq keywords;
  fetch_list_(keywords, prefix + "/Keywords");
  fetch_list_(tags, prefix + "/Tags");
  int colo = fetch_colo_(prefix);
  for (size_t i = 0; i < count_; ++i)
  {
    AdClient client = (flags & SEND_NO_COOKIES) ? 
      AdClient::create_nonoptin_user(this)
      : AdClient::create_user(this);
    
    if (flags & SEND_OO_USER)
    {
      client.process_request(AutoTest::OptOutRequest()
        .op("out"), "opt out");
    }
    for (size_t j = 0; j < tags.size(); ++j)
    {
      NSLRequest request;
      if (colo)
        request.colo = colo;
      std::string j_str = strof(j);
      if (((tags.size() - j) == 1) 
        && (flags & SEND_FORMAT))
      {
        if (flags & SEND_REFERER)
        {
          client.process_request(
            request.tid(tags[j])
            .referer("www." + keywords[j] + ".com").format("unit-test-imp"),
            (prefix + ": unit-test-imp request in tag: " + tags[j]
             + " with referer: " + keywords[j]).c_str());
        }
        else
        {
          client.process_request(
            request.tid(tags[j])
            .referer_kw(keywords[j]).format("unit-test-imp"),
            (prefix + ": unit-test-imp request in tag: " + tags[j]
             + " with: " + keywords[j]).c_str());
        }
      }
      else
      {
        if (flags & SEND_TID)
        {
          if (flags & SEND_REFERER)
          {
            client.process_request(
              request.tid(tags[j]).referer("www." + keywords[j] + ".com"),
              (prefix + ": request in tag: " + tags[j]
               + " with referer: " + keywords[j]).c_str());
          }
          else
          {
            client.process_request(
              request.tid(tags[j]).referer_kw(keywords[j]),
              (prefix + ": request in tag: " + tags[j]
               + " with: " + keywords[j]).c_str());
          }
        }
        else
        {
          if (flags & SEND_REFERER)
          {
            client.process_request(
              request.referer("www." + keywords[j] + ".com"),
              (prefix + ": request with referer: " + keywords[j]).c_str());
          }
          else
          {
            client.process_request(
              request.referer_kw(keywords[j]),
              (prefix + ": request with: " + keywords[j]).c_str());
          }
        }
      }
      check_channels_(prefix + "/ReqChannels-" + j_str, client);
      check_cretives_(prefix + "/ReqCreatives-" + j_str, client);
    }//for tags
     
    std::string track_pixel_url
      = client.debug_info.track_pixel_url;
    std::string click_url =
      client.debug_info.selected_creatives.size()  == 0 ? "":
      client.debug_info.selected_creatives.first().click_url.value();
    
    std::string action_adv_url =
      client.debug_info.selected_creatives.size() == 0 ? "":
      client.debug_info.selected_creatives.first().action_adv_url.value();
    
    if (flags & SEND_TRACK)
    {
      FAIL_CONTEXT(
        AutoTest::predicate_checker(
          !track_pixel_url.empty()),
        "response must have valid track_pixel_url: " + prefix);
      if (i % 2 == 1)
      {
        client.process_request(track_pixel_url,
          (prefix + ": Activate track pixel url").c_str());
      }
    }

    if (flags & SEND_CLICK)
    {        
      FAIL_CONTEXT(
        AutoTest::predicate_checker(
          !click_url.empty()),
        "response must have valid click_url: " + prefix);
      if (i % 3 == 2)
      {
        client.process_request(click_url,
          (prefix + ": Activate click url").c_str());
      }
    }

    if (flags & SEND_ACTION)
    {
      FAIL_CONTEXT(
        AutoTest::predicate_checker(
          !action_adv_url.empty()),
        "response must have valid action_adv_url: " + prefix);
      if (i % 5 == 4)
      {
        client.process_request(action_adv_url,
          (prefix + ": Activate action adv url").c_str());
      }
    }

    if (flags & SEND_CLICKS)
    {
      for (SelectedCreativesList::const_iterator it =
        client.debug_info.selected_creatives.begin();
           it != client.debug_info.selected_creatives.end(); ++it)
      {
        if (!it->click_url.empty())
        {
          AdClient client_c(client);
          client_c.process_request(it->click_url,
          (prefix + ": do click").c_str());
        }
      }
    }
  }//for count_
}

void
ChannelImpInventoryTest::test_case(
  const char* prefix,
  size_t size,
  int flags)
{
  ORM::StatsList<ORM::ChannelImpInventory> stats;
  std::list<ORM::ChannelImpInventory::Diffs> diffs;

  init_(stats, diffs, prefix, size);
  do_case_requests_(prefix, flags);

  ADD_WAIT_CHECKER(
    "Check ChannelImpInventory stats",
    AutoTest::stats_diff_checker(
      pq_conn_, diffs, stats));
}

void
ChannelImpInventoryTest::colo_case()
{
  ORM::StatsList<ORM::ChannelImpInventory> stats;
  std::list<ORM::ChannelImpInventory::Diffs> diffs;

  init_(stats, diffs, "DifferentColo", 4);
  init_(stats, diffs, "DifferentColoUsers", 4);
  do_case_requests_("DifferentColo");

  ADD_WAIT_CHECKER(
    "Check ChannelImpInventory stats",
    AutoTest::stats_diff_checker(
      pq_conn_, diffs, stats));
}



