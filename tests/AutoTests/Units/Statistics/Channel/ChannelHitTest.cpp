#include "ChannelHitTest.hpp"

REFLECT_UNIT(ChannelHitTest) (
  "Statistics",
  AUTO_TEST_SLOW
);

namespace
{
  const int DAY = 24*60*60;
}

void
ChannelHitTest::tear_down()
{}

static const int hits_cs = 2;
static const int hits_s = 3;
static const int hits_x = 5;
static const int hits_u = 7;
static const int hits_ps = 11;

void
ChannelHitTest::run_case(const AutoTest::Time& date,
  unsigned long colo)
{
  std::ostringstream description;
  description << "Run for '" << date << "' date and '"
    << (colo ? colo: default_colo_) << "' colo_id.";
  add_descr_phrase(description.str());
  const std::string kw1 = fetch_string("KW1");
  const std::string kw2 = fetch_string("KW2");
  const std::string kw3 = fetch_string("KW3");
  const std::string url1 = fetch_string("URL1");
  const std::string page = fetch_string("ONLYPAGE");
  const std::string search = fetch_string("ONLYSEARCH");

  const unsigned long channels[] = { fetch_int("CH1") };
  const unsigned long tag = fetch_int("Tag");

  for (size_t i = 0; i < countof(channels); ++i)
  {
    Stat stat(Stat::Key().
      channel_id(channels[i]).
      colo_id(colo ? colo : default_colo_).
      sdate(date));
    stat.description(description.str());
    stat.select(pq_conn_);
    stats_.push_back(stat);
  }
  {
    AutoTest::AdClient client(AutoTest::AdClient::create_user(this));
    AutoTest::NSLookupRequest request;
    request.debug_time(date).ft(kw1 + "\n" + kw2 + "\n" + kw3).referer(
      std::string("http://www.google.ru/search?hl=ru&q=") + kw2);
    if (colo)
    { request.colo = colo; }

    std::string expected[] = {
      strof(channels[0]) + "P",
      strof(channels[0]) + "S"
    };

    for (int i = 0; i < hits_cs; ++i)
    {
      client.process_request(request);
      
      FAIL_CONTEXT(
        AutoTest::sequence_checker(
          expected,
          client.debug_info.trigger_channels,
          AutoTest::SCE_ENTRY).check(),
        "trigger_channels check");
    }
  }

  {
    AutoTest::AdClient client(AutoTest::AdClient::create_user(this));
    AutoTest::NSLookupRequest request;
    request.debug_time(date).referer(
      std::string("http://www.google.ru/search?hl=ru&q=") + kw2);
    if (colo)
    { request.colo = colo; }

    std::string expected[] = { strof(channels[0]) + "S" };

    for (int i = 0; i < hits_s; ++i)
    {
      client.process_request(request);
      FAIL_CONTEXT(
        AutoTest::sequence_checker(
          expected,
          client.debug_info.trigger_channels,
          AutoTest::SCE_ENTRY).check(),
        "trigger_channels check");
    }
  }

  {
    AutoTest::AdClient client(AutoTest::AdClient::create_user(this));
    AutoTest::NSLookupRequest request;
    request.debug_time(date).referer_kw(kw3);
    if (colo)
    { request.colo = colo; }
    //client.process_request(request);

    std::string expected[] = { strof(channels[0]) + "P" };

    for (int i = 0; i < hits_x; ++i)
    {
      client.process_request(request);
      FAIL_CONTEXT(
        AutoTest::sequence_checker(
          expected,
          client.debug_info.trigger_channels,
          AutoTest::SCE_ENTRY).check(),
        "trigger_channels check");
    }
  }

  {
    AutoTest::AdClient client(AutoTest::AdClient::create_user(this));
    AutoTest::NSLookupRequest request;
    request.debug_time(date).referer(url1);
    if (colo)
    { request.colo = colo; }

    std::string expected[] = {
      strof(channels[0]) + "U"
    };

    for (int i = 0; i < hits_u; ++i)
    {
      if (i % 2)
      { request.tid = tag; }
      else
      { request.tid.clear(); }
      client.process_request(request);
      
      FAIL_CONTEXT(
        AutoTest::sequence_checker(
          expected,
          client.debug_info.trigger_channels,
          AutoTest::SCE_ENTRY).check(),
        "trigger_channels check");
    }
  }

  {
    AutoTest::AdClient client(AutoTest::AdClient::create_user(this));
    AutoTest::NSLookupRequest request;
    request.debug_time(date).search(page + " " + search).
      referer_kw(page + "," + search);
    if (colo)
    { request.colo = colo; }
    for (int i = 0; i < hits_ps; ++i)
    {
      std::string expected[] = {
        strof(channels[0]) + "P",
        strof(channels[0]) + "S"
      };

      client.process_request(request);
      
      FAIL_CONTEXT(
        AutoTest::sequence_checker(
          expected,
          client.debug_info.trigger_channels,
          AutoTest::SCE_ENTRY).check(),
        "trigger_channels check");
    }
  }

  for (size_t i = 0; i < countof(channels); ++i)
  {
    diffs_.push_back(Diff().
      hits(hits_cs + hits_x + hits_s + hits_u + hits_ps).
      hits_urls(hits_u).
      hits_kws(hits_cs + hits_x + hits_ps).
      hits_search_kws(hits_s + hits_cs + hits_ps));
  }
}

bool
ChannelHitTest::run()
{
  // today
  run_case(base_time_);
  // today at different colo
  run_case(base_time_, fetch_int("NonDefaultColo"));
  // tomorrow
  run_case(base_time_ + DAY);

  FAIL_CONTEXT(AutoTest::wait_checker(
    AutoTest::stats_diff_checker(
      pq_conn_, diffs_, stats_)).check(),
    "ChannelInventory hits check");

  return true;
}
