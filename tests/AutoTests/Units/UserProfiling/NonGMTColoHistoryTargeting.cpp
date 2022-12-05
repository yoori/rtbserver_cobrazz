
#include "Common.hpp"
#include "NonGMTColoHistoryTargeting.hpp"

REFLECT_UNIT(NonGMTColoHistoryTargeting) (
  "UserProfiling",
  AUTO_TEST_FAST);

namespace
{
  typedef AutoTest::NSLookupRequest  NSLookupRequest;
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::ChannelsCheck ChannelsCheck;

  // Ad matching after history optimization
  const NonGMTColoHistoryTargeting::RequestInfo OPTIMIZATION_AFTER[] =
  {
    {
      NonGMTColoHistoryTargeting::GTE_TZ_MIDNIGHT,
      0,
      true,
      AutoTest::SCE_ENTRY,
      AutoTest::SCE_NOT_ENTRY
    },
    {
      NonGMTColoHistoryTargeting::GTE_GMT_HIGH,
      0,
      true,
      AutoTest::SCE_ENTRY,
      AutoTest::SCE_NOT_ENTRY
    },
    {
      NonGMTColoHistoryTargeting::GTE_SAME_DAY,
      0,
      true,
      AutoTest::SCE_ENTRY,
      AutoTest::SCE_NOT_ENTRY
    },
    {
      NonGMTColoHistoryTargeting::GTE_SAME_DAY,
      0,
      false,
      AutoTest::SCE_NOT_ENTRY,
      AutoTest::SCE_NOT_ENTRY
    },
    {
      NonGMTColoHistoryTargeting::GTE_TZ_HIGH,
      0,
      true,
      AutoTest::SCE_ENTRY,
      AutoTest::SCE_ENTRY
    },
    {
      NonGMTColoHistoryTargeting::GTE_TZ_HIGH,
      0,
      false,
      AutoTest::SCE_NOT_ENTRY,
      AutoTest::SCE_ENTRY
    }
  };

  // Ad matching on history optimization
  const NonGMTColoHistoryTargeting::RequestInfo OPTIMIZATION[] =
  {
    {
      NonGMTColoHistoryTargeting::GTE_SAME_DAY,
      0,
      true,
      AutoTest::SCE_ENTRY,
      AutoTest::SCE_NOT_ENTRY
    },
    {
      NonGMTColoHistoryTargeting::GTE_SAME_DAY,
      0,
      true,
      AutoTest::SCE_ENTRY,
      AutoTest::SCE_NOT_ENTRY
    },
    {
      NonGMTColoHistoryTargeting::GTE_TZ_HIGH,
      0,
      false,
      AutoTest::SCE_NOT_ENTRY,
      AutoTest::SCE_ENTRY
    }
  };

  // Clearing old records
  const NonGMTColoHistoryTargeting::RequestInfo CLEARING[] =
  {
    {
      NonGMTColoHistoryTargeting::GTE_SAME_DAY,
      0,
      true,
      AutoTest::SCE_ENTRY,
      AutoTest::SCE_NOT_ENTRY
    },
    {
      NonGMTColoHistoryTargeting::GTE_SAME_DAY,
      0,
      true,
      AutoTest::SCE_ENTRY,
      AutoTest::SCE_NOT_ENTRY
    },
    {
      NonGMTColoHistoryTargeting::GTE_TZ_HIGH,
      0,
      false,
      AutoTest::SCE_NOT_ENTRY,
      AutoTest::SCE_ENTRY
    },
    {
      NonGMTColoHistoryTargeting::GTE_SAME_DAY,
      2*24*60*60,
      false,
      AutoTest::SCE_NOT_ENTRY,
      AutoTest::SCE_ENTRY
    },
    {
      NonGMTColoHistoryTargeting::GTE_TZ_HIGH,
      2*24*60*60,
      false,
      AutoTest::SCE_NOT_ENTRY,
      AutoTest::SCE_NOT_ENTRY
    }
  };
}


bool
NonGMTColoHistoryTargeting::run_test()
{
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      get_config().check_service(CTE_REMOTE2, STE_FRONTEND)),
    "Remote#2.AdFrontend need for this test");

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      1,
      AutoTest::ORM::get_tz_ofset(
        this,
        fetch_string("TZName")) > Generics::Time::ZERO).check(),
    "Remote#2.colo must have positive timezone ofset");

  NOSTOP_FAIL_CONTEXT(
    check(
      "Ad matching after history optimization",
      "H3",
      OPTIMIZATION_AFTER,
      countof(OPTIMIZATION_AFTER)));

  NOSTOP_FAIL_CONTEXT(
    check(
      "Ad matching on history optimization",
      "H1",
      OPTIMIZATION,
      countof(OPTIMIZATION)));

  NOSTOP_FAIL_CONTEXT(
    check(
      "Clearing old records",
      "H2",
      CLEARING,
      countof(CLEARING)));

  return true;
}

void
NonGMTColoHistoryTargeting::check(
  const std::string& description,
  const std::string& prefix,
  const RequestInfo* requests,
  size_t requests_size)
{
  add_descr_phrase(description);
  AdClient client(
    AdClient::create_user(
      this, AutoTest::UF_FRONTEND_MINOR));

  for (unsigned long i = 0; i < requests_size; ++i)
  {
    NSLookupRequest request;
    if (requests[i].need_referer)
      request.referer = fetch_string(prefix+"Ref");
    request.debug_time =
      get_time(
        requests[i].date_type,
        base_time + requests[i].time_ofset);

    client.process_request(request);

    FAIL_CONTEXT(
      ChannelsCheck(
        this,
        (prefix+"BP").c_str(),
        client.debug_info.trigger_channels,
        requests[i].trigger_match).check(),
      description + " Triggrer_channels check#" + strof(i));

    FAIL_CONTEXT(
      ChannelsCheck(
        this, (prefix + "Channel").c_str(),
        client.debug_info.history_channels,
        requests[i].history_match).check(),
      prefix + " History_channels check#" + strof(i));
  }
}

Generics::Time
NonGMTColoHistoryTargeting::get_time(
  GMTvsTZEnum date_type,
  const AutoTest::Time& time)
{
  Generics::Time midnight(time.get_gm_time().get_date());
  Generics::Time tz_ofset =
    AutoTest::ORM::get_tz_ofset(
      this,
      fetch_string("TZName"));
  switch (date_type)
  {
  case GTE_TZ_MIDNIGHT:
    return midnight - tz_ofset;
  case GTE_SAME_DAY:
    return midnight + tz_ofset + 30*60;
  case GTE_TZ_HIGH:
    return midnight + 24*60*60 - tz_ofset + 30*60;      
  case GTE_GMT_HIGH:
    return midnight - tz_ofset + 30*60;
  }
  return midnight;
}
