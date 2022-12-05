#include "CreativeExclusionTest.hpp"

REFLECT_UNIT(CreativeExclusionTest) (
  "CreativeSelection",
  AUTO_TEST_FAST
);

namespace
{
  typedef AutoTest::SelectedCreativesCCID SelectedCreativesCCID;
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::AdClient AdClient;
  const TestCaseType site_excl_test_cases[] =
  {
    // Site exclusion test cases
    // Stage#1
    {"1-se1",
     "Starting part 1: 'R' in SiteCampaignApproval "
     "and ExclusionFlag is set in AccountType",
     {1, 1, 0, 0, 1, 0, 0, 0, 0, 0}
    },

    // Stage#2
    {"1-se2",
     "Starting part 2: 'A' in SiteCampaignApproval "
     "and ExclusionFlag is set in AccountType",
     {1, 1, 1, 0, 1, 1, 0, 1, 0, 0}
    },

    // Stage#3
    {"1-ne1",
     "Starting part 3: 'R' in SiteCampaignApproval "
     "and ExclusionFlag is not set in AccountType",       
     {1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
    },

    // Stage#4
    {"1-ne2",
     "Starting part 4: 'A' in SiteCampaignApproval "
     "and ExclusionFlag is not set in AccountType",       
     {1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
    },

    // Stage#5
    {"1-wse1",
     "Starting part 5: WalledGarden flag is set, 'R' "
     "in SiteCampaignApproval and ExclusionFlag is set in AccountType",
     {1, 1, 0, 0, 1, 0, 0, 0, 0, 0}
    },

    // Stage#6
    {"1-wse2",
     "Starting part 6: WalledGarden flag is set, 'A' "
     "in SiteCampaignApproval and ExclusionFlag is set in AccountType",       
    {1, 1, 1, 0, 1, 1, 0, 1, 0, 0}
    },

    // Stage#7
    {"1-wne1",
     "Starting part 7: WalledGarden flag is set, 'R' "
     "in SiteCampaignApproval and ExclusionFlag is not set in AccountType",
     {1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
    },

    // Stage#8
    {"1-wne2",
     "Starting part 8: WalledGarden flag is set, 'A' "
     "in SiteCampaignApproval and ExclusionFlag is not set in AccountType",
     {1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
    }
  };

  const unsigned short site_excl_cases_count =
    sizeof(site_excl_test_cases) / sizeof(*site_excl_test_cases);

  const TestCaseType tags_excl_test_cases[] =
  {
    //Tags exclusion test cases
    // Stage#1
    {"2-ne1",
     "Starting part 1: 'R' in SiteCampaignApproval and WALLED_GARDEN, "
     "SITE_EXCLUSION and TAGS_EXCLUSION are not set in AccountType",
     {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
    },

    // Stage#2
    {"2-ne2",
     "Starting part 2: 'A' in SiteCampaignApproval and WALLED_GARDEN, "
     "SITE_EXCLUSION and TAGS_EXCLUSION are not set in AccountType",
     {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
    },

    // Stage#5
    {"2-se1",
     "Starting part 5: 'R' in SiteCampaignApproval and "
     "SITE_EXCLUSION flag is set in AccountType",
     {1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 1}
    },

    // Stage#6
    {"2-se2",
     "Starting part 6: 'A' in SiteCampaignApproval and "
     "SITE_EXCLUSION flag is set in AccountType",
     {1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1}
    },

    // Stage#7
    {"2-ste1",
     "Starting part 7: 'R' in SiteCampaignApproval, "
     "SITE_EXCLUSION and TAGS_EXCLUSION flags are set in AccountType",
     {1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0}
    },

    // Stage#8
    {"2-ste2",
     "Starting part 8: 'A' in SiteCampaignApproval, "
     "SITE_EXCLUSION and TAGS_EXCLUSION flags are set in AccountType",
     {1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0}
    },

    // Stage#9
    {"2-wne1",
     "Starting part 9: 'R' in SiteCampaignApproval and "
     "WALLED_GARDEN flag is set in AccountType",
     {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
    },

    // Stage#10
    {"2-wne2",
     "Starting part 10: 'A' in SiteCampaignApproval and "
     "WALLED_GARDEN flag is set in AccountType",
     {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
    },

    // Stage#13
    {"2-wse1",
     "Starting part 13: 'R' in SiteCampaignApproval, "
     "WALLED_GARDEN and SITE_EXCLUSION flags are set in AccountType",
     {1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 1}
    },

    // Stage#14
    {"2-wse2",
     "Starting part 14: 'A' in SiteCampaignApproval, "
     "WALLED_GARDEN and SITE_EXCLUSION flags are set in AccountType",
     {1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1}
    },

    // Stage#15
    {"2-wste1",
     "Starting part 15: 'R' in SiteCampaignApproval, WALLED_GARDEN, "
     "SITE_EXCLUSION and TAGS_EXCLUSION flags are set in AccountType",
     {1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0}
    },

    // Stage#16
    {"2-wste2",
     "Starting part 16: 'A' in SiteCampaignApproval, WALLED_GARDEN, "
     "SITE_EXCLUSION and TAGS_EXCLUSION flags are set in AccountType",
     {1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0}
    }
  };

  const unsigned short tags_excl_cases_count =
    sizeof(tags_excl_test_cases) / sizeof(*tags_excl_test_cases);

  const NSLookupRequest::Member req_params[] = {
    &NSLookupRequest::referer,
    &NSLookupRequest::orig
  };
}


void 
CreativeExclusionTest::exclusion(const TestCaseType* test_cases,
                                 const unsigned short& cases_count,
                                 const unsigned short tags_count)
{
  for (size_t testcase_id = 0; testcase_id < cases_count; ++testcase_id)
  {
    add_descr_phrase(test_cases[testcase_id].description);

    std::string prefix(test_cases[testcase_id].name_prefix);
  
    request.referer_kw = fetch_string("kwd/" + prefix);

    for (size_t ind = 0; ind < tags_count; ++ind)
    { 

      AutoTest::AdClient client(AutoTest::AdClient::create_user(this));

      std::string tag_name(prefix+"/Tag Id/"+strof(ind+1));
      std::string ccid_name(prefix+"/CC Id/"+strof(ind+1));
      request.tid = get_object_by_name(tag_name).Value();
      client.process_request(request);
      std::string exp_ccid =
          test_cases[testcase_id].ccid_appearances[ind]?
          get_object_by_name(ccid_name).Value(): "0";
      std::string exp_tid  = exp_ccid == "0"?
          "0" : get_object_by_name(tag_name).Value();
      FAIL_CONTEXT(
        AutoTest::equal_checker(
          exp_ccid,
          client.debug_info.ccid).check(),
        test_cases[testcase_id].description +
          ". Unexpected ccid#" + strof(ind));
    }
  }  
}

void
CreativeExclusionTest::excluding_higher_weight_creative_()
{
  std::string description =
    "AdServer Ad Categories Test Plan "
    "(Test 3. Many creatives in campaign)";
  add_descr_phrase(description);

  NSLookupRequest request;
  request.tid = fetch_string("ADSC-5543-4/TAG");
  request.referer_kw = fetch_string("ADSC-5543-4/KWD");

  std::string ccid = fetch_string("ADSC-5543-4/CCID1");

  AutoTest::AdClient client(AutoTest::AdClient::create_user(this));

  client.process_request(request);

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      ccid,
      client.debug_info.ccid).check(),
    description + ". Expected ccid");
}

void
CreativeExclusionTest::excluding_by_creative_template_and_tag_()
{
  std::string description =
    "AdServer Ad Categories Test Plan "
    "(Test 4. Categories assigned to creative template)";
  add_descr_phrase(description);

  AutoTest::AdClient user(AutoTest::AdClient::create_user(this));

  NSLookupRequest request;
  request.referer_kw = fetch_string("ADSC-5543-5/KWD");
  request.tid = fetch_string("ADSC-5543-5/TAG2");
  user.process_request(
    request,
    "request for no creative (rejected template)");
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      user.debug_info.selected_creatives.empty()),
    "server must return empty cc_id (rejected template)");

  request.tid = fetch_string("ADSC-5543-5/TAG3");
  user.process_request(
    request,
    "request for no creative (rejected tag)");
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      user.debug_info.selected_creatives.empty()),
    "server must return empty cc_id (rejected tag)");

  std::string ccid = fetch_string("ADSC-5543-5/CCID");
  request.tid = fetch_string("ADSC-5543-5/TAG1");
  user.process_request(request);

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      ccid,
      user.debug_info.ccid).check(),
    description + ". Expected ccid");
}

void
CreativeExclusionTest::tags_exclusion_for_text_creatives_()
{
  std::string description =
    "AdServer Ad Categories Test Plan "
    "(Test 5. Tags Exclusion for Text creatives)";
  add_descr_phrase(description);

  size_t cases_count = fetch_int("TEST-5-6/TCcount");

  NSLookupRequest request;
  request.tid = fetch_string("TEST-5-6/TAG");

  for (size_t i = 1; i <= cases_count; ++i)
  {
    request.referer_kw = fetch_string("TEST-5-6/KWD-" + strof(i));

    AutoTest::AdClient client(AutoTest::AdClient::create_user(this));

    client.process_request(request);
    client.process_request(request);

    FAIL_CONTEXT(
      AutoTest::equal_checker(
        fetch_string("TEST-5-6/EXP_CCID-" + strof(i)),
        client.debug_info.ccid).check(),
      description + ". Expected ccid#" + strof(i));
  }
}

void
CreativeExclusionTest::
exclusion_text_by_domain_match_within_creative_token_()
{
  std::string description =
    "AdServer Ad Categories Test Plan "
    "(Test 5.8. Exclusion text campaign by "
    "main-domain matching within creative token)";
  add_descr_phrase(description);

  size_t cases_count = fetch_int("TEST-5-8/TCcount");

  NSLookupRequest request;
  request.tid = fetch_string("TEST-5-8/TAG");

  for (size_t i = 1; i <= cases_count; ++i)
  {
    request.referer_kw = fetch_string("TEST-5-8/KWD-" + strof(i));

    AutoTest::AdClient client(AutoTest::AdClient::create_user(this));

    client.process_request(request);
    client.process_request(request);
    
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        fetch_string("TEST-5-8/EXP_CCID-" + strof(i)),
        client.debug_info.ccid).check(),
      description + ". Expected ccid#" + strof(i));
  }
}

void
CreativeExclusionTest::
exclusion_by_ccg_keyword_click_url_()
{

  std::string description =
    "AdServer Ad Categories Test Plan "
    "(Test 5.9 Exclusion by CCG Keyword Click URL)";
  
  add_descr_phrase(description);

  size_t cases_count = fetch_int("TEST-5-9/TCcount");

  NSLookupRequest request;
  request.tid = fetch_string("TEST-5-9/TAG");

  for (size_t i = 1; i <= cases_count; ++i)
  {
    request.referer_kw = fetch_string("TEST-5-9/KWD-" + strof(i));

    AutoTest::AdClient client(AutoTest::AdClient::create_user(this));

    client.process_request(request, "keyword context request");
    client.process_request(request, "request for creative");

    FAIL_CONTEXT(
      AutoTest::equal_checker(
        fetch_string("TEST-5-9/EXP_CCID-" + strof(i)),
        client.debug_info.ccid).check(),
      description + ". Expected ccid#" + strof(i));
  }
}

bool
CreativeExclusionTest::run_test()
{
  add_descr_phrase("Site exclusion test cases started.");
  NOSTOP_FAIL_CONTEXT(exclusion(site_excl_test_cases,
                          site_excl_cases_count,
                          TAGS_COUNT_1));
  add_descr_phrase("Tags exclusion test cases started.");
  NOSTOP_FAIL_CONTEXT(exclusion(tags_excl_test_cases,
                          tags_excl_cases_count,
                          TAGS_COUNT_2));

  NOSTOP_FAIL_CONTEXT(excluding_higher_weight_creative_());
  NOSTOP_FAIL_CONTEXT(excluding_by_creative_template_and_tag_());
  NOSTOP_FAIL_CONTEXT(tags_exclusion_for_text_creatives_());

  NOSTOP_FAIL_CONTEXT(exclusion_text_by_domain_match_within_creative_token_());
  NOSTOP_FAIL_CONTEXT(exclusion_by_ccg_keyword_click_url_());
  return true;
}
