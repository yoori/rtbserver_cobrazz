#include "ContextChannelsTest.hpp"

REFLECT_UNIT(ContextChannelsTest) (
  "TriggerMatching",
  AUTO_TEST_FAST
);

typedef AutoTest::NSLookupRequest NSLookupRequest;
typedef AutoTest::AdClient AdClient;


namespace
{
  const char SEARCH_REFERER[]            = "http://www.google.ru/search?hl=ru&q=";
}

 
bool 
ContextChannelsTest::run_test()
{
  // simple test cases
  NOSTOP_FAIL_CONTEXT(url_request_test_case());
  NOSTOP_FAIL_CONTEXT(page_request_test_case());
  NOSTOP_FAIL_CONTEXT(search_request_test_case());

  //composite test case
  NOSTOP_FAIL_CONTEXT(composite_request_test_case());
  
  return true;
}

void ContextChannelsTest::url_request_test_case()
{
  add_descr_phrase("'ContextChannels' enable URL triggers");
  ChannelList exp_context_channels;
  ChannelList exp_noncontext_channels;
  NSLookupRequest request;
  request.referer=fetch_string("Referer/01");
  test_client_.process_request(request, "url channels request");

  exp_context_channels.push_back(get_object_by_name("Cntx01").Value());
  exp_noncontext_channels.push_back(get_object_by_name("NCntx01").Value());
  
  // check results

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      exp_context_channels,
      test_client_.debug_info.trigger_channels,
      AutoTest::SCE_ENTRY).check(),
    "context channels");

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      exp_noncontext_channels,
      test_client_.debug_info.trigger_channels,
      AutoTest::SCE_ENTRY).check(),
    "noncontext channels");
}

void ContextChannelsTest::page_request_test_case()
{
  add_descr_phrase("'ContextChannels' enable PAGE triggers");
  ChannelList exp_context_channels;
  ChannelList exp_noncontext_channels;
  NSLookupRequest request;
  request.referer_kw = fetch_string("PageKwd/01");
  test_client_.process_request(request, "page channels request");
  exp_context_channels.push_back(get_object_by_name("Cntx02").Value());
  exp_noncontext_channels.push_back(get_object_by_name("NCntx02").Value());

  // check results

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      exp_context_channels,
      test_client_.debug_info.trigger_channels,
      AutoTest::SCE_ENTRY).check(),
    "context channels");

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      exp_noncontext_channels,
      test_client_.debug_info.trigger_channels,
      AutoTest::SCE_ENTRY).check(),
    "noncontext channels");
}

void ContextChannelsTest::search_request_test_case()
{
  add_descr_phrase("'ContextChannels' enable SEARCH triggers");
  ChannelList exp_context_channels;
  ChannelList exp_noncontext_channels;
  NSLookupRequest request;
  request.referer = SEARCH_REFERER + fetch_string("SearchKwd/01");
  
  test_client_.process_request(request, "search channels request");
  
  exp_context_channels.push_back(get_object_by_name("Cntx03").Value());
  exp_noncontext_channels.push_back(get_object_by_name("NCntx03").Value());

  // check results
  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      exp_context_channels,
      test_client_.debug_info.trigger_channels,
      AutoTest::SCE_ENTRY).check(),
    "context channels");

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      exp_noncontext_channels,
      test_client_.debug_info.trigger_channels,
      AutoTest::SCE_ENTRY).check(),
    "noncontext channels");
}


void ContextChannelsTest::composite_request_test_case()
{

  add_descr_phrase("'ContextChannels' enable ALL triggers");
  ChannelList exp_context_channels;
  ChannelList exp_noncontext_channels;

  NSLookupRequest request;
  request.referer_kw = fetch_string("PageKwd/01");
  request.referer = SEARCH_REFERER + fetch_string("SearchKwd/01");
  
  test_client_.process_request(request, "composite request");

  // expected result
  Locals all_locals = get_local_params();
  
  const ulong test_param_size = all_locals.DataElem().size();

  for (ulong idx=0; idx < test_param_size; idx++)
    {
      std::string param_name = all_locals.DataElem()[idx].Name();
      if (param_name.find("Cntx") == 0)
        exp_context_channels.push_back(all_locals.DataElem()[idx].Value());
      if (param_name.find("NCntx") == 0)
        exp_noncontext_channels.push_back(all_locals.DataElem()[idx].Value());
    }
  
  // check results
  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      exp_context_channels,
      test_client_.debug_info.trigger_channels,
      AutoTest::SCE_ENTRY).check(),
    "context channels");

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      exp_noncontext_channels,
      test_client_.debug_info.trigger_channels,
      AutoTest::SCE_ENTRY).check(),
    "noncontext channels");
}
