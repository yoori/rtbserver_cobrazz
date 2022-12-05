#include "CreativeFilesPresenceTest.hpp"

REFLECT_UNIT(CreativeFilesPresenceTest) (
  "CreativeInstantiation",
  AUTO_TEST_FAST
);

typedef AutoTest::CreativeChecker CreativeChecker;
typedef AutoTest::CreativeTemplatesChecker TemplateChecker;
typedef AutoTest::SelectedCreativesCCID SelectedCreativesCCID;

bool 
CreativeFilesPresenceTest::run_test()
{
  NOSTOP_FAIL_CONTEXT(file_not_present_case());
  NOSTOP_FAIL_CONTEXT(normal_case());
  NOSTOP_FAIL_CONTEXT(url_not_present_case());
  NOSTOP_FAIL_CONTEXT(file_url_present_case());
  NOSTOP_FAIL_CONTEXT(template_file_not_present_case());
  NOSTOP_FAIL_CONTEXT(w_notblock_a_case());
  NOSTOP_FAIL_CONTEXT(w_notblock_a_size_format_case());
  NOSTOP_FAIL_CONTEXT(text_2campaign_case());
  NOSTOP_FAIL_CONTEXT(text_campaign_case());
  NOSTOP_FAIL_CONTEXT(ADSC_8367());
  return true;
}
 
void
CreativeFilesPresenceTest::file_not_present_case()
{
  std::string description("Absent creative file."); 
  add_descr_phrase(description);

  unsigned long cc_id = fetch_int("CCID1");

  FAIL_CONTEXT(
    CreativeChecker(
      this, cc_id,
      CreativeChecker::Expected().
      status("W")).check(),
    description +
      " Check creative status");
  
  AutoTest::AdClient client(AutoTest::AdClient::create_user(this));
  
  AutoTest::NSLookupRequest request;
  request.referer_kw = fetch_string("KEYWORD1");
  request.tid = fetch_string("TID1");
  request.format = "unit-test-imp";
  client.process_request(request);
  
  FAIL_CONTEXT(
    AutoTest::entry_checker(
      strof(cc_id),
      SelectedCreativesCCID(client),
      AutoTest::SCE_NOT_ENTRY).check(),
    description +
      " Check ccid");
}

void
CreativeFilesPresenceTest::normal_case()
{
  std::string description("Normal case."); 
  add_descr_phrase(description);

  unsigned long cc_id = fetch_int("CCID2");

  FAIL_CONTEXT(
    CreativeChecker(
      this, cc_id,
      CreativeChecker::Expected().
        status("A")).check(),
    description +
      " Check creative status");
  
  AutoTest::AdClient client(AutoTest::AdClient::create_user(this));
  
  AutoTest::NSLookupRequest request;
  request.referer_kw = fetch_string("KEYWORD2");
  request.tid = fetch_string("TID2");
  request.format = "unit-test-imp";
  client.process_request(request);
  
  FAIL_CONTEXT(
    AutoTest::entry_checker(
      strof(cc_id),
      SelectedCreativesCCID(client)).check(),
    description +
      " Check ccid");
}

void
CreativeFilesPresenceTest::url_not_present_case()
{
  std::string description("URL option absent."); 
  add_descr_phrase(description);

  unsigned long cc_id = fetch_int("CCID3");

  FAIL_CONTEXT(
    CreativeChecker(
      this, cc_id,
      CreativeChecker::Expected().
        status("W")).check(),
    description +
      " Check creative status");
  
  AutoTest::AdClient client(AutoTest::AdClient::create_user(this));

  AutoTest::NSLookupRequest request;
  request.referer_kw = fetch_string("KEYWORD3");
  request.tid = fetch_string("TID3");
  request.format = "unit-test-imp";
  client.process_request(request);
  
  FAIL_CONTEXT(
    AutoTest::entry_checker(
      strof(cc_id),
      SelectedCreativesCCID(client),
      AutoTest::SCE_NOT_ENTRY).check(),
    description +
      " Check ccid");
}

void
CreativeFilesPresenceTest::file_url_present_case()
{
  std::string description("URL option & creative file present."); 
  add_descr_phrase(description);

  unsigned long cc_id = fetch_int("CCID4");
  
  FAIL_CONTEXT(
    CreativeChecker(
      this, cc_id,
      CreativeChecker::Expected().
        status("A")).check(),
    description +
      " Check creative status");
  
  AutoTest::AdClient client(AutoTest::AdClient::create_user(this));
  AutoTest::NSLookupRequest request;
  request.referer_kw = fetch_string("KEYWORD4");
  request.tid = fetch_string("TID4");
  request.format = "unit-test-imp";
  client.process_request(request);
  
  FAIL_CONTEXT(
    AutoTest::entry_checker(
      strof(cc_id),
      SelectedCreativesCCID(client)).check(),
    description +
      " Check ccid");
}

void
CreativeFilesPresenceTest::template_file_not_present_case()
{
  std::string description("Template file absent."); 
  add_descr_phrase(description);
  unsigned long cc_id = fetch_int("CCID5");

  FAIL_CONTEXT(
    TemplateChecker(
      this,
      TemplateChecker::Expected().
        creative_format(".*CreativeFilesPresenceTest-5")).check(),
    description +
      " Check creative template");
  
  FAIL_CONTEXT(
    CreativeChecker(
      this, cc_id,
      CreativeChecker::Expected().
        status("A")).check(),
    description +
      " Check creative status");
  
  AutoTest::AdClient client(AutoTest::AdClient::create_user(this));
  
  AutoTest::NSLookupRequest request;
  request.referer_kw = fetch_string("KEYWORD5");
  request.tid = fetch_string("TID5");
  request.format = "unit-test-imp";
  client.process_request(request);
  
  FAIL_CONTEXT(
    AutoTest::entry_checker(
      strof(cc_id),
      SelectedCreativesCCID(client),
      AutoTest::SCE_NOT_ENTRY).check(),
    description +
      " Check ccid");
}

void
CreativeFilesPresenceTest::w_notblock_a_case()
{
  std::string description("'W' creative is not block 'A'."); 
  add_descr_phrase(description);
  unsigned long cc_id_a = fetch_int("CCID6A");
  unsigned long cc_id_w = fetch_int("CCID6W");
  std::string ccg_id = fetch_string("CCG6");

  FAIL_CONTEXT(
    CreativeChecker(
      this, cc_id_a,
      CreativeChecker::Expected().
        campaign_id(ccg_id).
        weight("1").
        status("A")).check(),
    description +
      " Check creative#1");

  FAIL_CONTEXT(
    CreativeChecker(
      this, cc_id_w,
      CreativeChecker::Expected().
        campaign_id(ccg_id).
        weight("10000").
        status("W")).check(),
    description +
      " Check creative#2");

  AutoTest::AdClient client(AutoTest::AdClient::create_user(this));
  
  AutoTest::NSLookupRequest request;
  request.referer_kw = fetch_string("KEYWORD6");
  request.tid = fetch_string("TID6");
  request.format = "unit-test-imp";
  client.process_request(request);

  FAIL_CONTEXT(
    AutoTest::entry_checker(
      strof(cc_id_a),
      SelectedCreativesCCID(client)).check(),
    description +
      " Check ccid#1");
  
  FAIL_CONTEXT(
    AutoTest::entry_checker(
      strof(cc_id_w),
      SelectedCreativesCCID(client),
      AutoTest::SCE_NOT_ENTRY).check(),
    description +
      " Check ccid#2");

}

void
CreativeFilesPresenceTest::w_notblock_a_size_format_case()
{
  std::string description("'W' template is not block creative."); 
  add_descr_phrase(description);
  
  unsigned long cc_id = fetch_int("CCID7");

  FAIL_CONTEXT(
    TemplateChecker(
      this,
      TemplateChecker::Expected().
        creative_format(".*CreativeFilesPresenceTest-7").
        app_format("unit-test").
        status("A")).check(),
    description +
      " Check creative template#1");

    FAIL_CONTEXT(
    TemplateChecker(
      this,
      TemplateChecker::Expected().
        creative_format(".*CreativeFilesPresenceTest-7").
        app_format("unit-test-imp").
        status("W")).check(),
    description +
      " Check creative template#2");
    
    FAIL_CONTEXT(
    CreativeChecker(
      this, cc_id,
      CreativeChecker::Expected().
        status("A")).check(),
    description +
      " Check creative status");

  AutoTest::AdClient client(AutoTest::AdClient::create_user(this));
  AutoTest::NSLookupRequest request;
  request.referer_kw = fetch_string("KEYWORD7");
  request.tid = fetch_string("TID7");

  request.format = "unit-test";
  client.process_request(request);
  
  FAIL_CONTEXT(
    AutoTest::entry_checker(
      strof(cc_id),
      SelectedCreativesCCID(client)).check(),
    description +
      " Check ccid size#1");

  request.format     = "unit-test-imp";
  client.process_request(request);

  FAIL_CONTEXT(
    AutoTest::entry_checker(
      strof(cc_id),
      SelectedCreativesCCID(client),
      AutoTest::SCE_NOT_ENTRY).check(),
    description +
      " Check ccid size#2");
}

void
CreativeFilesPresenceTest::text_2campaign_case()
{
  std::string description("Text. 2nd creative 'W'."); 
  add_descr_phrase(description);
  
  unsigned long cc_id1 = fetch_int("CCID8_1");
  unsigned long cc_id2 = fetch_int("CCID8_2");
  
  FAIL_CONTEXT(
    CreativeChecker(
      this, cc_id1,
      CreativeChecker::Expected().
        status("A")).check(),
    description +
      " Check creative#1 status");

  FAIL_CONTEXT(
    CreativeChecker(
      this, cc_id2,
      CreativeChecker::Expected().
        status("W")).check(),
    description +
      " Check creative#2 status");

  AutoTest::AdClient client(AutoTest::AdClient::create_user(this));
  
  AutoTest::NSLookupRequest request;
  request.referer_kw =
    fetch_string("KEYWORD8_1") + "," + fetch_string("KEYWORD8_2");
  request.tid = fetch_string("TID8");
  request.format = "unit-test";
  client.process_request(request);
  client.repeat_request ();

  FAIL_CONTEXT(
    AutoTest::entry_checker(
      strof(cc_id1),
      SelectedCreativesCCID(client)).check(),
    description +
      " Check ccid#1");
  
  FAIL_CONTEXT(
    AutoTest::entry_checker(
      strof(cc_id2),
      SelectedCreativesCCID(client),
      AutoTest::SCE_NOT_ENTRY).check(),
    description +
      " Check ccid#2");
}

void
CreativeFilesPresenceTest::text_campaign_case()
{
  std::string description("Text. 1st creative 'W'."); 
  add_descr_phrase(description);
  
  unsigned long cc_id1 = fetch_int("CCID9_1");
  unsigned long cc_id2 = fetch_int("CCID9_2");

  FAIL_CONTEXT(
    CreativeChecker(
      this, cc_id1,
      CreativeChecker::Expected().
        status("W")).check(),
    description +
      " Check creative#1 status");

  FAIL_CONTEXT(
    CreativeChecker(
      this, cc_id2,
      CreativeChecker::Expected().
        status("A")).check(),
    description +
      " Check creative#2 status");

  AutoTest::AdClient client(AutoTest::AdClient::create_user(this));
  
  AutoTest::NSLookupRequest request;
  request.referer_kw = fetch_string("KEYWORD9");
  request.tid = fetch_string("TID9");
  request.format = "unit-test";
  client.process_request(request);
  client.repeat_request ();


  FAIL_CONTEXT(
    AutoTest::entry_checker(
      strof(cc_id1),
      SelectedCreativesCCID(client),
      AutoTest::SCE_NOT_ENTRY).check(),
    description +
      " Check ccid#1");
  
  FAIL_CONTEXT(
    AutoTest::entry_checker(
      strof(cc_id2),
      SelectedCreativesCCID(client)).check(),
    description +
      " Check ccid#2");
}

// 'Dynamic File' type option
void CreativeFilesPresenceTest::ADSC_8367()
{
  const std::string description("ADSC-8367: 'Dynamic File' type option");
  add_descr_phrase(description);

  unsigned long cc_A = fetch_int("ADSC-8367/CCID/A");
  unsigned long cc_W = fetch_int("ADSC-8367/CCID/W");

  FAIL_CONTEXT(
    CreativeChecker(this, cc_A,
      CreativeChecker::Expected().status("A")).check(),
    description + " Check creative#1 (A) status");

  FAIL_CONTEXT(
    CreativeChecker(this, cc_W,
      CreativeChecker::Expected().status("W")).check(),
    description +  " Check creative#2 (W) status");

  AutoTest::AdClient client(AutoTest::AdClient::create_user(this));

  AutoTest::NSLookupRequest request;
  request.referer_kw = fetch_string("ADSC-8367/KEYWORD");
  request.tid = fetch_string("ADSC-8367/TID");
  client.process_request(request);

  FAIL_CONTEXT(
    AutoTest::entry_checker(
      strof(cc_A),
      SelectedCreativesCCID(client)).check(),
    description + " Check ccid#1 (with status A) appearance");

  FAIL_CONTEXT(
    AutoTest::entry_checker(
      strof(cc_W),
      SelectedCreativesCCID(client),
      AutoTest::SCE_NOT_ENTRY).check(),
    description +" Check ccid#2 (with status W) not appearance");
}
