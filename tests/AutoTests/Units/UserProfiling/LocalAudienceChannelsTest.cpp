
#include "Common.hpp"
#include "LocalAudienceChannelsTest.hpp"

REFLECT_UNIT(LocalAudienceChannelsTest) (
  "UserProfiling",
  AUTO_TEST_SLOW);

namespace
{
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::TemporaryAdClient TemporaryAdClient;
  typedef AutoTest::ChannelsCheck ChannelsCheck;

  const char TEST_1_1[] = "Persistent UID matching";
}

void LocalAudienceChannelsTest::persistent_uid_matching_setup_()
{
  
}

void LocalAudienceChannelsTest::persistent_uid_matching_()
{

}

void LocalAudienceChannelsTest::stage1()
{
  AUTOTEST_CASE(persistent_uid_matching_setup_(), TEST_1_1);
  persistent_uid_matching_();
}

void LocalAudienceChannelsTest::stage2()
{
  AUTOTEST_CASE(persistent_uid_matching_(), TEST_1_1);
}

bool
LocalAudienceChannelsTest::run_test()
{
  stage1();
  check();
  stage2();
  check();
  return true;
}
