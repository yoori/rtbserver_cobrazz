#include "DailyRunTest.hpp"

REFLECT_UNIT(DailyRunTest) (
  "CreativeSelection",
  AUTO_TEST_FAST
);


Generics::Time
DailyRunTest::get_gmt(
  DailyRunTest::WeekDays wday,
  const char* local_tm,
  const char* time_zone)
{
  Generics::Time posed_time = Generics::Time::get_time_of_day();
  if (wday != DailyRunTest::WD_CURRENT)
  {
    posed_time -= Generics::Time::ONE_DAY * ((posed_time.get_gm_time().tm_wday + 6) % 7);
    posed_time += Generics::Time::ONE_DAY * wday;
  }
  Generics::Time time(
    (posed_time.get_gm_time().format("%d-%m-%Y") +
      ":" + local_tm), "%d-%m-%Y:%H-%M-%S");
  return time - AutoTest::ORM::get_tz_ofset(this, time_zone);
}


void
DailyRunTest::process_testcase(
  const char *keyword,
  const char *tz_name,
  const RequestTimeResult expect[],
  size_t expect_count)
{
  const std::string tid = fetch_string("Tag");
  const std::string cc_id = fetch_string(std::string("CC/") + keyword);

  AutoTest::AdClient client(AutoTest::AdClient::create_user(this));

  for (unsigned int i = 0; i < expect_count; ++i)
  {
    AutoTest::NSLookupRequest request;
    request.tid(tid).referer_kw(keyword);
    if (expect[i].time)
    {
      request.debug_time(
        get_gmt(
          expect[i].wday,
          expect[i].time,
          tz_name));
      
      add_descr_phrase(expect[i].time);
    }
    client.process_request(request);
    
    if (expect[i].have_cc)
    {
      FAIL_CONTEXT(
        AutoTest::equal_checker(
          cc_id,
          client.debug_info.ccid).check(),
        std::string(tz_name) + " NO empty ccid check#" + strof(i));
    }
    else
    {
      FAIL_CONTEXT(
        AutoTest::equal_checker(
          "0",
          client.debug_info.ccid).check(),
        std::string(tz_name) + " empty ccid check#" + strof(i));
    }
  }
}


bool 
DailyRunTest::run_test()
{

  const RequestTimeResult moscow[] = {
    { WD_MONDAY, "00-00-00", true },
    { WD_MONDAY, "01-00-00", true },
    { WD_TUESDAY, "01-00-00", false },
    { WD_MONDAY, "01-58-59", true },
    { WD_TUESDAY, "03-00-00", false },
    { WD_TUESDAY, "03-59-59", false },
    { WD_TUESDAY, "04-00-00", true },
    { WD_TUESDAY, "05-00-00", true },
    { WD_WEDNESDAY, "05-00-00", false },
    { WD_SATURDAY, "20-00-00", true },
    { WD_SUNDAY, "20-00-00", false },
    { WD_SATURDAY, "21-58-59", true },
    { WD_SATURDAY, "21-59-00", true },
    { WD_SATURDAY, "21-59-59", true },
    { WD_SATURDAY, "22-00-00", false },
    { WD_SUNDAY, "23-00-00", true },
    { WD_SUNDAY, "23-58-59", true }
  };

  NOSTOP_FAIL_CONTEXT(process_testcase(
    "Europe/Moscow",
    "Europe/Moscow",
    moscow,
    sizeof(moscow) / sizeof(*moscow)));

  const RequestTimeResult tokyo[] = {
    { WD_MONDAY, "05-30-01", true },
    { WD_TUESDAY, "08-00-00", true },
    { WD_TUESDAY, "09-30-00", true },
    { WD_SATURDAY, "11-15-00", true },
    { WD_CURRENT, "05-00-00", false },
    { WD_CURRENT, "14-45-00", false },
    { WD_CURRENT, "07-00-00", false },
    { WD_CURRENT, "20-20-00", false },
    { WD_CURRENT, "10-30-00", false }
  };

  NOSTOP_FAIL_CONTEXT(
    process_testcase(
      "Asia/Tokyo", "Asia/Tokyo", tokyo,
      sizeof(tokyo) / sizeof(*tokyo)));


  const RequestTimeResult any[] = {
    { WD_MONDAY, "20-30-01", true },
    { WD_TUESDAY, "23-00-00", true },
    { WD_WEDNESDAY, "00-30-00", true },
    { WD_THURSDAY, "02-15-00", true },
    { WD_FRIDAY, "19-59-59", true },
    { WD_SATURDAY, "05-45-00", true },
    { WD_SUNDAY, "21-59-59", true },
    { WD_CURRENT, "11-20-00", true },
    { WD_CURRENT, "01-30-00", true },
    { WD_CURRENT, 0, true }
  };

  NOSTOP_FAIL_CONTEXT(
    process_testcase(
      "America/Los_Angeles-any", "America/Los_Angeles", any,
      sizeof(any) / sizeof(*any)));


  const RequestTimeResult los_angeles[] = {
    { WD_CURRENT, "08-00-01", false },
    { WD_CURRENT, "15-00-01", false },
    { WD_CURRENT, "16-00-00", false },
    { WD_CURRENT, "16-29-59", false },
    { WD_CURRENT, "16-30-00", true },
    { WD_CURRENT, "16-58-59", true },
    { WD_CURRENT, "16-59-59", true },
    { WD_CURRENT, "17-00-00", false },
    { WD_CURRENT, "17-28-59", false },
    { WD_CURRENT, "17-29-00", false },
    { WD_CURRENT, "17-29-01", false },
    { WD_CURRENT, "21-16-34", false }
  };

  NOSTOP_FAIL_CONTEXT(
    process_testcase(
      "America/Los_Angeles", "America/Los_Angeles", los_angeles,
      sizeof(los_angeles) / sizeof(*los_angeles)));

  return true;
}
