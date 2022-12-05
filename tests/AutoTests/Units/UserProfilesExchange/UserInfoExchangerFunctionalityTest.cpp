
#include "UserInfoExchangerFunctionalityTest.hpp"
#include "UserProfilesExchangeCommon.hpp"

REFLECT_UNIT(UserInfoExchangerFunctionalityTest) (
  "UserProfilesExchange",
  AUTO_TEST_SLOW
);

namespace
{
  typedef AutoTest::NSLookupRequest  NSLookupRequest;
  typedef AutoTest::UserProfilesExchange::CheckWaitHistoryChannel CheckWaitHistoryChannel;
}

UserInfoExchangerFunctionalityTest::CheckRequest::CheckRequest(
  const AdClient& client_,
  const char* request_,
  const char* colo,
  const char* channel,
  const char* description_,
  bool exists_) :
  client(client_),
  request(request_),
  expected_colo(colo),
  description(description_),
  exists(exists_)
{
  expected_history_channels.push_back(channel);
}

UserInfoExchangerFunctionalityTest::CheckRequest::CheckRequest(
  const AdClient& client_,
  const char* request_,
  const char* colo,
  const char** channels,
  size_t channels_size,
  const char* description_,
  bool exists_) :
  client(client_),
  request(request_),
  expected_colo(colo),
  description(description_),
  exists(exists_)
{
  for (size_t i = 0; i < channels_size; ++i)
  {
    expected_history_channels.push_back(channels[i]);
  }
}

bool 
UserInfoExchangerFunctionalityTest::run_test()
{
  s_channel       = fetch_string("CH/S");
  ht_channel      = fetch_string("CH/HT");
  h_channel       = fetch_string("CH/H");

  s_bp = fetch_string("BP/S");
  ht_bp = fetch_string("BP/HT");
  h_bp = fetch_string("BP/H");

  colo1_id = fetch_string("COLO1");
  colo2_id = fetch_string("COLO2");

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      get_config().check_service(CTE_REMOTE1, STE_FRONTEND)),
    "Remote#1.AdFrontend must set in the XML configuration file");

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      get_config().check_service(CTE_REMOTE2, STE_FRONTEND)),
    "Remote#1.AdFrontend must set in the XML configuration file");

  major_frontend_prefix =
    get_config().get_service(CTE_REMOTE1, STE_FRONTEND).address;

  minor_frontend_prefix =
    get_config().get_service(CTE_REMOTE2, STE_FRONTEND).address;

  s_keyword  = fetch_string("KeyS");
  ht_keyword = fetch_string("KeyHT");
  h_keyword = fetch_string("KeyH");

  colo_req_timeout =
    fetch_int("COLO_EXCHANGE_TIMEOUT") + 1;

  part1();
  part2();
  part3();
  part4();

  AdClient client(AdClient::create_user(this));

  part5_colo1(client);
  part5_colo2(client);

  merging_profiles_wait(
      client,
      minor_frontend_prefix.c_str(),
      colo2_id.c_str(),
      "CH/Marker1");
  
  verification();

  part5_return_to_colo1(client);
  
  return true;
}


//  1. Create user on Colo#1 
//  2. 1st S|HT request -> colo#1
//  3. 2nd S|HT request -> colo#2
//  4. Wait exchanging
//  5. Verification: 3d S|HT request -> colo#2 (S|HT channels appearance)
void
UserInfoExchangerFunctionalityTest::part1()
{
  add_descr_phrase("Part#1. Requests");

  std::string expected[] = { s_bp, ht_bp };
  const char* h_expected[] =
  {
    s_channel.c_str(),
    ht_channel.c_str()
  };

  AdClient client(AdClient::create_user(this));
  
  NSLookupRequest request;
  request.referer_kw = s_keyword + "," + ht_keyword;

  client.process_request(request, "Part#1. 1st Colo#1 request");

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      colo1_id,
      client.debug_info.colo_id).check(),
    "must receive Colo#1");

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      expected,
      client.debug_info.trigger_channels,
      AutoTest::SCE_ENTRY).check(),
    "trigger_channels");

  client.change_base_url(minor_frontend_prefix.c_str());
  client.process_request(request, "Part#1. 2nd Colo#2 request");
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      colo2_id,
      client.debug_info.colo_id).check(),
    "must receive Colo#2");

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      expected,
      client.debug_info.trigger_channels,
      AutoTest::SCE_ENTRY).check(),
    "trigger_channels");
  
  requests.push_back(CheckRequest(client,
                                  request.url().c_str(),
                                  colo2_id.c_str(),
                                  h_expected,
                                  countof(h_expected),
                                  "Part#1. Verification"));

}

//  1. Create user on Colo#1 
//  2. 1st S|HT request -> colo#1
//  3. 2nd S|HT request -> colo#2
//  3. 3d S|HT request -> colo#2
//  4. Wait exchanging
//  5. Verification: 4th empty request -> colo#2 (S|HT channels appearance)
void
UserInfoExchangerFunctionalityTest::part2()
{
  add_descr_phrase("Part#2. Requests");
  
  std::string expected[] = { s_bp, ht_bp };
  const char* h_expected[] =
  {
    s_channel.c_str(),
    ht_channel.c_str()
  };


  AdClient client(AdClient::create_user(this));
  
  NSLookupRequest request;
  request.referer_kw = s_keyword + "," + ht_keyword;

  client.process_request(request, "Part#2. 1st Colo#1 request");

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      colo1_id,
      client.debug_info.colo_id).check(),
    "must receive Colo#1");

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      expected,
      client.debug_info.trigger_channels,
      AutoTest::SCE_ENTRY).check(),
    "trigger_channels");
    
  client.change_base_url(minor_frontend_prefix.c_str());
  client.process_request(request, "Part#2. 2nd Colo#2 request");

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      colo2_id,
      client.debug_info.colo_id).check(),
    "must receive Colo#2");

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      expected,
      client.debug_info.trigger_channels,
      AutoTest::SCE_ENTRY).check(),
    "trigger_channels");
                                
  client.process_request(request, "Part#2. 3d Colo#2 request");
  
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      colo2_id,
      client.debug_info.colo_id).check(),
    "must receive Colo#2");

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      expected,
      client.debug_info.trigger_channels,
      AutoTest::SCE_ENTRY).check(),
    "trigger_channels");

  requests.push_back(CheckRequest(client,
                                  NSLookupRequest().url().c_str(),
                                  colo2_id.c_str(),
                                  h_expected,
                                  countof(h_expected),
                                  "Part#2. Verification"));
  
}

//  1. Create user on Colo#1 
//  2. 1st H request -> colo#1
//  3. 2nd H request -> colo#2
//  4. 3d  H request -> colo#2
//  5. Wait exchanging
//  6. Verification: 4th empty next day request -> colo#2 (H channel appearance)
void
UserInfoExchangerFunctionalityTest::part3()
{
  add_descr_phrase("Part#3. Requests");

  NSLookupRequest request;
  request.referer_kw  = h_keyword;
  
  AdClient client(AdClient::create_user(this));

  client.process_request(request, "Part#3. 1st Colo#1 request");
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      colo1_id,
      client.debug_info.colo_id).check(),
    "must receive Colo#1");
  
  FAIL_CONTEXT(
    AutoTest::entry_checker(
      h_bp,
      client.debug_info.trigger_channels).check(),
    "trigger_channels");
  
  FAIL_CONTEXT(
    AutoTest::entry_checker(
      h_channel,
      client.debug_info.history_channels,
      AutoTest::SCE_NOT_ENTRY).check(),
    "Colo#1. history channel");

  client.change_base_url(minor_frontend_prefix.c_str());
    
  client.process_request(request, "Part#3. 2nd Colo#2 request");
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      colo2_id,
      client.debug_info.colo_id).check(),
    "must receive Colo#2");
  
  FAIL_CONTEXT(
    AutoTest::entry_checker(
      h_bp,
      client.debug_info.trigger_channels).check(),
    "trigger_channels");
  
  FAIL_CONTEXT(
    AutoTest::entry_checker(
      h_channel,
      client.debug_info.history_channels,
      AutoTest::SCE_NOT_ENTRY).check(), 
    "Colo#2. history channel");
                              
  client.process_request(request, "Part3. 3d request Colo#2");
  
  FAIL_CONTEXT(
    AutoTest::entry_checker(
      h_channel,
      client.debug_info.history_channels,
      AutoTest::SCE_NOT_ENTRY).check(), 
    "Colo#2. history channel");


  requests.push_back(CheckRequest(client,
                                  NSLookupRequest().
                                  debug_time(AutoTest::Time() + 25*60*60).
                                  url().c_str(),
                                  colo2_id.c_str(),
                                  h_channel.c_str(),
                                  "Part#3. Verification"));
}

//  1. Create user on Colo#1 
//  2. 1st H request -> colo#1
//  3. 1st S request -> colo#1
//  4. 2nd H request -> colo#1
//  5. 2nd S and 3d H request -> colo#2 (need merge)
//  6. Wait exchanging
//  7. Verification#1: 3d S request -> colo#2 (S channel appearance)
//  8. Verification#2: empty request -> colo#2 (H channel absent)
//  7. Verification#3: empty next day request -> colo#2 (H channel appearance)
//  8. Verification#4: empty next day request -> colo#2 (S channel disappearance)
void
UserInfoExchangerFunctionalityTest::part4()
{
  add_descr_phrase("Part#4. Requests");

  NSLookupRequest s_validating_request;
  s_validating_request.referer_kw  = s_keyword;

  NSLookupRequest h_validating_request;
  h_validating_request.referer_kw  = h_keyword;
    
  AdClient client(AdClient::create_user(this));
 
  std::string not_channels[] = {
    s_channel, h_channel, ht_channel
  };

  // 1st H request (Colo#1)
  client.process_request(h_validating_request, "Part#4. 1st Colo#1 H request");
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      colo1_id,
      client.debug_info.colo_id).check(),
    "must receive Colo#1");

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      not_channels,
      client.debug_info.history_channels,
      AutoTest::SCE_NOT_ENTRY).check(),
    "mustn't have history channels");
  
  // 1st S request (Colo#1)
  client.process_request(s_validating_request,"Part#4. 1st Colo#1 S request");
  
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      colo1_id,
      client.debug_info.colo_id).check(),
    "must receive expected Colo#1");

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      not_channels,
      client.debug_info.history_channels,
      AutoTest::SCE_NOT_ENTRY).check(),
    "mustn't have history channels");

  // 2nd H request (Colo#1)
  client.process_request(h_validating_request, "Part#4. 2nd Colo#1 H request");
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      colo1_id,
      client.debug_info.colo_id).check(),
    "must receive Colo#1");

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      not_channels,
      client.debug_info.history_channels,
      AutoTest::SCE_NOT_ENTRY).check(),
    "mustn't have history channels");
  
  // 2nd S & 3d H request (Colo#2) - request for matching
  NSLookupRequest request;
  request.referer_kw = s_keyword + "," + h_keyword;
  client.change_base_url(minor_frontend_prefix.c_str());
  client.process_request(request,"Part#4. 2nd S & 3d H Colo#2 request");
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      colo2_id,
      client.debug_info.colo_id).check(),
    "must receive Colo#2");

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      not_channels,
      client.debug_info.history_channels,
      AutoTest::SCE_NOT_ENTRY).check(),
    "mustn't have history channels");
  
  // Verification requests
  requests.push_back(CheckRequest(client,
                                  s_validating_request.url().c_str(),
                                  colo2_id.c_str(),
                                  s_channel.c_str(),
                                  "Part#4. Verification S channel appear"));

  requests.push_back(CheckRequest(client,
                                  NSLookupRequest().url().c_str(),
                                  colo2_id.c_str(),
                                  h_channel.c_str(),
                                  "Part#4. Verification H channel absent",
                                  false));

  requests.push_back(CheckRequest(client,
                                  NSLookupRequest().
                                  debug_time( AutoTest::Time() + 25*60*60).
                                  url().c_str(),
                                  colo2_id.c_str(),
                                  h_channel.c_str(),
                                  "Part#4. Verification H channel appear"));

  requests.push_back(CheckRequest(client,
                                  NSLookupRequest().
                                  debug_time( AutoTest::Time() + 25*60*60).
                                  url().c_str(),
                                  colo2_id.c_str(),
                                  s_channel.c_str(),
                                  "Part#4. Verification S channel disappear",
                                  false));

}

//  Part5. First Colo#1 requests
//  1. Create user on Colo#1 
//  2. 1st HT, H request -> colo#1
void
UserInfoExchangerFunctionalityTest::part5_colo1(AdClient& client)
{
    add_descr_phrase("Part#5. Colo#1 requests");

    std::string expected[] = { ht_bp, h_bp, fetch_string("BP/Marker1") };

    NSLookupRequest request;
    request.referer_kw = ht_keyword + "," + h_keyword +
        "," + fetch_string("KeyMarker1");

    client.process_request(request,
      "Part#5. 1st Colo#1 request to HT &H (also match Marker1)");
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        colo1_id,
        client.debug_info.colo_id).check(),
      "must receive Colo#1");

    FAIL_CONTEXT(
      AutoTest::sequence_checker(
        expected,
        client.debug_info.trigger_channels,
        AutoTest::SCE_ENTRY).check(),
      "trigger_channels");

    FAIL_CONTEXT(
      AutoTest::entry_checker(
        fetch_string("CH/Marker1"),
        client.debug_info.history_channels).check(), 
      "Colo#1. marker channel");
}

//  Part5. First Colo#2 requests
//  2. 2nd HT, H request + 1st Marker2 -> colo#2
void
UserInfoExchangerFunctionalityTest::part5_colo2(AdClient& client)
{
  add_descr_phrase("Part#5. Colo#2 requests");
  std::string expected[] = { ht_bp, h_bp, fetch_string("BP/Marker2") };
  
  client.change_base_url(minor_frontend_prefix.c_str());
  NSLookupRequest request;
  request.referer_kw =
    ht_keyword + "," + h_keyword + "," + fetch_string("KeyMarker2");

  client.process_request(request,
     "Part#5. 2nd Colo#2 request to HT, H (also match Marker2)");
    
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      colo2_id,
      client.debug_info.colo_id).check(),
    "must receive Colo#2");

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      expected,
      client.debug_info.trigger_channels,
      AutoTest::SCE_ENTRY).check(),
    "trigger_channels");
}

//  Part5. First Colo#2 requests
//  3. 3d HT, H request -> colo#1
void
UserInfoExchangerFunctionalityTest::part5_return_to_colo1(AdClient& client)
{
  add_descr_phrase("Part#5. Return to Colo#1 & verification");

  std::string expected[] = { ht_channel, h_channel };
  
  client.change_base_url(major_frontend_prefix.c_str());
  NSLookupRequest request;
  request.referer_kw =
      ht_keyword + "," + h_keyword;
  request.debug_time =  AutoTest::Time() + 25*60*60;  
  client.process_request(request, "Part#5. 3d Colo#1 request to HT, H");

  request.referer_kw.clear();

  FAIL_CONTEXT(
    CheckWaitHistoryChannel(
      client,
      fetch_int("CH/Marker2"),
      colo1_id.c_str(),
      colo_req_timeout,
      request).check(),
    "Check marker channel in adjacent colo history");
  
  request.debug_time =  AutoTest::Time() + 49*60*60;
  client.process_request(request, "Part#5. Verification");

  FAIL_CONTEXT(
    AutoTest::sequence_checker(
      expected,
      client.debug_info.history_channels,
      AutoTest::SCE_ENTRY).check(),
    "Colo#1. history channel");
  
}

// Wait exchanger merging cicle
void
UserInfoExchangerFunctionalityTest::merging_profiles_wait(
  AdClient& client,
  const char* frontend_dst,
  const char* colo_dst,
  const char* marker_channel_name)
{
  add_descr_phrase("Waiting marker channel");
  
  client.change_base_url(frontend_dst);
  try
  {

    FAIL_CONTEXT(
      CheckWaitHistoryChannel(
        client,
        fetch_int(marker_channel_name),
        colo_dst,
        colo_req_timeout).check(),
      "Check marker channel in adjacent colo history");
  }
  catch (...)
  {
    AutoTest::AdminsArray<AutoTest::UserInfoAdminLog> admin;
    
    admin.initialize(
      this, CTE_ALL,
      STE_USER_INFO_MANAGER_CONTROLLER,
      client.debug_info.uid.value().c_str(),
      AutoTest::UserInfoManagerController);
    
    admin.log(AutoTest::Logger::thlog());
    
    throw;
  }

}

// Final verifcation
void
UserInfoExchangerFunctionalityTest::verification()
{
  add_descr_phrase("Verification");
  for (CheckRequests::iterator it = requests.begin();
       it != requests.end(); ++it)
  {
    (it->client).process_request(it->request.c_str(),
                               it->description.c_str());
    try
    {
      FAIL_CONTEXT(
        AutoTest::equal_checker(
          it->expected_colo,
          it->client.debug_info.colo_id).check(),
        "must have expected colo");
      
      FAIL_CONTEXT(
        AutoTest::sequence_checker(
          it->expected_history_channels,
          it->client.debug_info.history_channels, 
          it->exists? AutoTest::SCE_ENTRY: AutoTest::SCE_NOT_ENTRY).check(),
        "history channel");
    }
    catch (...)
    {
      AutoTest::AdminsArray<AutoTest::UserInfoAdminLog> admin;

      admin.initialize(
        this, CTE_ALL,
        STE_USER_INFO_MANAGER_CONTROLLER,
        it->client.debug_info.uid.value().c_str(),
        AutoTest::UserInfoManagerController);

      admin.log(AutoTest::Logger::thlog());
      
      throw;
    }
        
  }
}

