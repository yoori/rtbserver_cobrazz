
#include "ProfilesContentCleanupTest.hpp"
 
REFLECT_UNIT(ProfilesContentCleanupTest) (
  "UserProfiling",
  AUTO_TEST_SLOW
);

namespace
{
  typedef AutoTest::NSLookupRequest NSLRequest;
  typedef AutoTest::UserKeywordProfileChecker UserKeywordChecker;

  const char GOOGLE_SEARCH[] = "http://www.google.com/search?hl=en&q=";
  const size_t REQUESTS_CAPACITY = 30;
  const size_t REQUESTS_OVER_CAPACITY = 2;
  struct MyTimestamp
  {
    size_t days_ago;
    const char* time;
    bool stored_in_profile;
  };

  const MyTimestamp REQUESTS_DEBUG_TIME[] = {
    {8, "00-00-00", false},
    {8, "23-59-59", false},
    {7, "00-00-00", true},
    {7, "00-00-01", true},
    {7, "23-59-59", true},
    {0, "00-00-00", true}
  };
};

void
ProfilesContentCleanupTest::cleanup_by_visits_count()
{
  add_descr_phrase("Cleanup by visits count");

  AutoTest::AdClient user(AutoTest::AdClient::create_user(this));

  std::string expected_channels[] = {
    channel_ + "P",
    channel_ + "S"
  };

  std::string expected_timestamps;

  NSLRequest request;
  request.referer_kw = fetch_string("KEYWORD");
  request.referer = GOOGLE_SEARCH + fetch_string("KEYWORD");

  // Profiling requests
  for (size_t i = 0; i < REQUESTS_CAPACITY + REQUESTS_OVER_CAPACITY; ++i)
  {
    request.debug_time = base_time_ + i * 60;
    user.process_request(request, ("profiling request #" + strof(i)).c_str());

    FAIL_CONTEXT(
      AutoTest::sequence_checker(
        expected_channels,
        user.debug_info.trigger_channels,
        AutoTest::SCE_ENTRY).check(),
      "matching 'P' and 'S' channels");
    
    if (i >= REQUESTS_OVER_CAPACITY)
    {
      expected_timestamps += (base_time_ + i * 60).
                               get_gm_time().
                               format("%Y-%m-%d %H:%M:%S ");
    }
  }

  request.tid = fetch_string("TAG");
  request.referer_kw.clear();
  request.referer.clear();

  // Advertising requests
  for (size_t i = 0; i < REQUESTS_CAPACITY + REQUESTS_OVER_CAPACITY; ++i)
  {
    request.debug_time = base_time_ + i * 60;
    user.process_request(request, ("advertising  request #" + strof(i)).c_str());
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        cc_id_,
        user.debug_info.ccid).check(),
      "matching 'P' and 'S' channels");
  }

}

void
ProfilesContentCleanupTest::cleanup_by_visits_date()
{
  add_descr_phrase("Cleanup by visits date");

  AutoTest::AdClient user(AutoTest::AdClient::create_user(this));

  std::string expected_channels[] = {
    channel_ + "P",
    channel_ + "S"
  };

  std::string expected_timestamps;

  NSLRequest request;

  for (size_t i = 0;
       i < sizeof(REQUESTS_DEBUG_TIME)/sizeof(*REQUESTS_DEBUG_TIME);
       ++i)
  {
    request.tid.clear();
    request.referer_kw = fetch_string("KEYWORD");
    request.referer = GOOGLE_SEARCH + fetch_string("KEYWORD");
    request.debug_time =
      (base_time_ - REQUESTS_DEBUG_TIME[i].days_ago * 24 * 60 * 60).
      get_gm_time().
      format((std::string("%d-%m-%Y:") + REQUESTS_DEBUG_TIME[i].time).c_str());
    user.process_request(request, ("Profiling request #" + strof(i)).c_str());

    FAIL_CONTEXT(
      AutoTest::sequence_checker(
        expected_channels,
        user.debug_info.trigger_channels,
        AutoTest::SCE_ENTRY).check(),
      "matching 'P' and 'S' channels");
    
    request.tid = fetch_string("TAG");
    request.referer_kw.clear();
    request.referer.clear();
    user.process_request(request, ("Advertising request #" + strof(i)).c_str());
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        cc_id_,
        user.debug_info.ccid).check(),
      "matching 'P' and 'S' channels");
    if (REQUESTS_DEBUG_TIME[i].stored_in_profile)
    {
      expected_timestamps += request.debug_time.
                                     get_gm_time().
                                     format("%Y-%m-%d %H:%M:%S ");
    }
  }

}

bool
ProfilesContentCleanupTest::run_test()
{
  add_descr_phrase("Test started");

  channel_ = fetch_string("CHANNEL");
  cc_id_ = fetch_string("CCID");
  ccg_id_ = fetch_string("CCGID");

  NOSTOP_FAIL_CONTEXT(cleanup_by_visits_count());
  NOSTOP_FAIL_CONTEXT(cleanup_by_visits_date());

  return true;
}

