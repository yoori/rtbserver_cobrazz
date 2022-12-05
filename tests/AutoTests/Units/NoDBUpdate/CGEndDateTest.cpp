
#include "CGEndDateTest.hpp"
 
REFLECT_UNIT(CGEndDateTest) (
  "NoDBUpdate",
  AUTO_TEST_QUIET
);

namespace
{
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::CampaignChecker CampaignChecker;
  typedef AutoTest::SelectedCreativeChecker SelectedCreativeChecker;
}

template <size_t COUNT>
void
CGEndDateTest::process_requests_(
  const std::string& description,
  unsigned long tag,
  const TestRequest (&requests)[COUNT])
{
  add_descr_phrase(description);
  
  for (size_t i = 0; i < COUNT; ++i)
  {
    if (requests[i].ccgid)
    {
      FAIL_CONTEXT(
        CampaignChecker(
          this,
          requests[i].ccgid,
          CampaignChecker::Expected().
          eval_status(requests[i].flags & CCG_INACTIVE? "I": "A").
          force_remote_present(true)).check(),
        description +
        " Check CCG status#" + strof(i+1));
    }
    
    AdClient client = AdClient::create_user(this);

    FAIL_CONTEXT(
      SelectedCreativeChecker(
        client,
        NSLookupRequest().
          referer_kw(requests[i].kwd).
          tid(tag),
        strof(requests[i].ccid)).check(),
      description +
        " Check CC#" + strof(i+1))
  }
}

void
CGEndDateTest::gmt_case_()
{
  std::string kwd1 = fetch_string("GMT/KWD1");
  std::string kwd2 = fetch_string("GMT/KWD2");
  std::string kwd3 = fetch_string("GMT/KWD3");
  std::string kwd4 = fetch_string("GMT/KWD4");
  std::string kwd5 = fetch_string("GMT/KWD5");
  std::string kwd6 = fetch_string("GMT/KWD6");
  std::string kwd7 = fetch_string("GMT/KWD7");
  unsigned long cc1 = fetch_int("GMT/CC1");
  unsigned long cc4 = fetch_int("GMT/CC4");
  unsigned long cc6 = fetch_int("GMT/CC6");
  unsigned long ccg1 = fetch_int("GMT/CCG1");
  unsigned long ccg2 = fetch_int("GMT/CCG2");
  unsigned long ccg3 = fetch_int("GMT/CCG3");
  unsigned long ccg4= fetch_int("GMT/CCG4");
  unsigned long ccg5= fetch_int("GMT/CCG5");
  unsigned long ccg6= fetch_int("GMT/CCG6");
  unsigned long ccg7= fetch_int("GMT/CCG7");
  
  const TestRequest START_DATE[] =
  {
    { kwd1 , cc1, ccg1, CCG_ACTIVE },
    { kwd2 , 0, ccg2, CCG_INACTIVE },
    { kwd3 , 0, ccg3, CCG_INACTIVE },
    { kwd4 , cc4, ccg4, CCG_ACTIVE },
    { kwd5 , 0, ccg5, CCG_INACTIVE },
    { kwd6 , cc6, ccg6, CCG_ACTIVE },
    { kwd7 , 0, ccg7, CCG_INACTIVE }
  };

  process_requests_(
    "GMT cases.",
    fetch_int("GMT/TAG"),
    START_DATE);
}

bool 
CGEndDateTest::run_test()
{
  NOSTOP_FAIL_CONTEXT(gmt_case_());

  return true;
}

