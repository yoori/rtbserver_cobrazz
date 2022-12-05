#include "TextAdAutoCategories.hpp"

REFLECT_UNIT(TextAdAutoCategories) (
  "CreativeSelection",
  AUTO_TEST_FAST
);

bool
TextAdAutoCategories::run_test()
{
  std::vector<std::string> ccid_exp;
  std::vector<AutoTest::Money> actual_cpc_exp;

  const int total_creatives = fetch_int("TotalCreatives");
  for (int i = 0; i < total_creatives; ++i)
  {
    ccid_exp.push_back(fetch_string("CC" + strof(i + 1)));
    actual_cpc_exp.push_back(fetch_string("ActualCPC" + strof(i + 1)));
  }

  AutoTest::NSLookupRequest request;
  request.tid = fetch_string("Tag1");
  request.referer_kw = fetch_string("Keyword1");

  AutoTest::AdClient client(AutoTest::AdClient::create_user(this));
  client.process_request(request);
  client.process_request(request);

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      ccid_exp,
      AutoTest::SelectedCreativesCCID(client)).check(),
    "selected_creatives cc_id");
  
  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      actual_cpc_exp,
      AutoTest::SelectedCreativesActualCPC(client)).check(),
    "selected_creatives actual_cpc");

  return true;
}
