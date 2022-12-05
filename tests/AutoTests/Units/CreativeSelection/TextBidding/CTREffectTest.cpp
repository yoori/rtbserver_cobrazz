
#include "CTREffectTest.hpp"

REFLECT_UNIT(CTREffectTest) (
  "CreativeSelection",
  AUTO_TEST_FAST
);

namespace
{
  typedef AutoTest::NSLookupRequest NSL;
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::SelectedCreativesCCID SelectedCreativesCCID;
  typedef AutoTest::SelectedCreativesActualCPC SelectedCreativesActualCPC;
  typedef AutoTest::precisely_number precisely_number;;
}

template<size_t Count>
void CTREffectTest::process_requests(
  const TestCaseRequest(&requests)[Count],
  const std::string& prefix)
{
  for (size_t i = 0; i < Count; ++i)
  {
    AdClient client(AdClient::create_user(this));

    NSL request;
    request.tid = fetch_string(prefix + requests[i].tag);

    {
      std::string param_value;
      String::StringManip::SplitComma
        tokenizer(requests[i].request_param_value);
      String::SubString token;
      while (tokenizer.get_token(token))
      {
        String::StringManip::trim(token);
        if (!param_value.empty()) param_value += ",";
        param_value += fetch_string(prefix + token.str());
      }
      requests[i].request_param_name(request, param_value);
    }

    client.process_request(request);

    try
    {
      if (!requests[i].expected_ccids.empty())
      {
        // Check creatives
        {
          std::list<std::string> exp_ccids;
          String::StringManip::SplitComma tokenizer(requests[i].expected_ccids);
          String::SubString token;
          while (tokenizer.get_token(token))
          {
            String::StringManip::trim(token);
            exp_ccids.push_back(fetch_string(prefix + token.str()));
          }
          FAIL_CONTEXT(
            AutoTest::sequence_checker(
              exp_ccids,
              SelectedCreativesCCID(client)).check(),
            "expected ccids check#" + strof(i+1));
        }
        
        if (!requests[i].click_revenue.empty())
        {
          std::list<precisely_number> exp_click_revenue;
          String::StringManip::SplitComma tokenizer(requests[i].click_revenue);
          String::SubString token;
          while (tokenizer.get_token(token))
          {
            String::StringManip::trim(token);
            exp_click_revenue.push_back(
              precisely_number(fetch_float(prefix + token.str())));
          }
          FAIL_CONTEXT(
            AutoTest::sequence_checker(
              exp_click_revenue,
              SelectedCreativesActualCPC(client)).check(),
            "expected ccids check#" + strof(i+1));
        }
      }
      else
      {
        FAIL_CONTEXT(
          AutoTest::predicate_checker(
            client.debug_info.selected_creatives.empty()),
          "empty ccid check#" + strof(i+1));
      }
    }
    catch (const eh::Exception&)
    {
      AutoTest::AdminsArray<AutoTest::TagAdmin> admins;
      
      admins.initialize(
        this,
        CTE_ALL,
        STE_CAMPAIGN_MANAGER,
        fetch_int(prefix + requests[i].tag));
      
      admins.log(AutoTest::Logger::thlog());
      throw;
    }
  }
}


void CTREffectTest::regular_adjustment_()
{
  const TestCaseRequest REQUESTS[] =
  {
    { "Site1/Tag1", &NSL::referer_kw, "DChannel-CPM/DKeywordCPM",
      "DisplayCPM/CCID", "" },
    { "Site1/Tag1", &NSL::referer_kw, "DChannel-CPC/DKeywordCPC",
      "DisplayCPC/CCID", "" },
    { "Site2/Tag1", &NSL::referer, "REFERER",
      "DisplayCPCRON/CCID", "" },
    { "Site1/Tag1", &NSL::referer_kw, "DChannel-CPA/DKeywordCPA",
      "DisplayCPA/CCID", "" },
    { "Site1/Tag1", &NSL::referer_kw, "CTChannel-CPM/CTKeywordCPM",
      "ChannelTargetedCPM/CCID", "" },
    { "Site1/Tag1", &NSL::referer_kw, "CTChannel-CPC/CTKeywordCPC",
      "ChannelTargetedCPC/CCID", "" },
    { "Site3/Tag1", &NSL::referer, "REFERER",
      "ChannelTargetedCPCRON/CCID", "" },
    { "Site1/Tag1", &NSL::referer_kw, "CTChannel-CPA/CTKeywordCPA",
      "ChannelTargetedCPA/CCID", "" },
    { "Site1/Tag1", &NSL::referer_kw, "KChannel1-1/KKeyword11",
      "KeywordTargeted/CCID", "" },
    { "Site1/Tag2", &NSL::referer_kw, "DChannel-CPM/DKeywordCPM",
      "", "" },
    { "Site1/Tag2", &NSL::referer_kw, "DChannel-CPC/DKeywordCPC",
      "DisplayCPC/CCID", "" },
    { "Site2/Tag2", &NSL::referer, "REFERER",
      "DisplayCPCRON/CCID", "" },
    { "Site1/Tag2", &NSL::referer_kw, "DChannel-CPA/DKeywordCPA",
      "", "" },
    { "Site1/Tag2", &NSL::referer_kw, "CTChannel-CPM/CTKeywordCPM",
      "", "" },
    { "Site1/Tag2", &NSL::referer_kw, "CTChannel-CPC/CTKeywordCPC",
      "ChannelTargetedCPC/CCID", "" },
    { "Site3/Tag2", &NSL::referer, "REFERER",
      "ChannelTargetedCPCRON/CCID", "" },
    { "Site1/Tag2", &NSL::referer_kw, "CTChannel-CPA/CTKeywordCPA",
      "", "" },
    { "Site1/Tag2", &NSL::referer_kw, "KChannel1-1/KKeyword11",
      "KeywordTargeted/CCID", "" },
    { "Site1/Tag3", &NSL::referer_kw, "DChannel-CPM/DKeywordCPM",
      "DisplayCPM/CCID", "" },
    { "Site1/Tag3", &NSL::referer_kw, "DChannel-CPC/DKeywordCPC",
      "", "" },
    { "Site1/Tag3", &NSL::referer_kw, "CTChannel-CPA/CTKeywordCPA",
      "ChannelTargetedCPA/CCID", ""},
    { "Site1/Tag3", &NSL::referer_kw, "KChannel1-1/KKeyword11",
      "", "" },
  };

  process_requests(REQUESTS, "TagAdjustment/");
}

void CTREffectTest::invalid_ctr_()
{
  const TestCaseRequest REQUESTS[] =
  {
    { "Site1/Tag4", &NSL::referer_kw, "DChannel-CPM/DKeywordCPM",
      "", "" },
    { "Site1/Tag4", &NSL::referer_kw, "DChannel-CPC/DKeywordCPC",
      "DisplayCPC/CCID", "DisplayCPC/CPC" },
    { "Site1/Tag4", &NSL::referer_kw, "CTChannel-CPC/CTKeywordCPC",
      "ChannelTargetedCPC/CCID", "ChannelTargetedCPC/CPC" },
    { "Site1/Tag4", &NSL::referer_kw,
      "KChannel1-1/KKeyword11,KChannel2/KKeyword2",
      "KeywordTargeted/CCID,KeywordTargeted#2/CCID",
      "KeywordTargeted/CLICK_REVENUE,KeywordTargeted#2/KKeyword2/CPC" },
    { "Site1/Tag5", &NSL::referer_kw,
      "CTChannel-CPC/CTKeywordCPC,KChannel1-1/KKeyword11", "", "" },
  };
  
  process_requests(REQUESTS, "TagAdjustment/");
}

void CTREffectTest::ccg_concurrence_()
{

  const TestCaseRequest REQUESTS[] =
  {
    { "Site1/Tag6", &NSL::referer_kw,
      "DChannel2/DKeyword2,DChannel-CPC/DKeywordCPC",
      "DisplayCPC/CCID", "" },
    { "Site1/Tag6", &NSL::referer_kw,
      "DChannel1/DKeyword1,DChannel-CPC/DKeywordCPC",
      "DisplayCPM#1/CCID", "" },
    { "Site1/Tag7", &NSL::referer_kw,
      "DChannel3/DKeyword3,DChannel-CPC/DKeywordCPC",
      "DisplayCPM#3/CCID", "" },
    { "Site1/Tag2", &NSL::referer_kw,
      "DChannel1/DKeyword1,DChannel-CPC/DKeywordCPC",
      "DisplayCPM#1/CCID", ""},
    { "Site1/Tag2", &NSL::referer_kw,
      "CTChannel-CPM#2/CTKeywordCPM2,CTChannel-CPC/CTKeywordCPC",
      "ChannelTargetedCPM#2/CCID,ChannelTargetedCPC/CCID", ""},
    { "Site1/Tag6", &NSL::referer_kw,
      "DChannel1/DKeyword1,KChannel1-1/KKeyword11",
      "DisplayCPM#1/CCID", ""},
    { "Site1/Tag7", &NSL::referer_kw,
      "DChannel3/DKeyword3,KChannel1-1/KKeyword11",
      "DisplayCPM#3/CCID", ""},
    { "Site1/Tag7", &NSL::referer_kw,
      "DChannel-CPM/DKeywordCPM,KChannel1-1/KKeyword11",
      "DisplayCPM/CCID", "" },
    { "Site1/Tag7", &NSL::referer_kw,
      "DChannel3/DKeyword3,KChannel1-2/KKeyword12",
      "KeywordTargeted/CCID", "" },
    { "Site1/Tag6", &NSL::referer_kw,
      "DChannel2/DKeyword2,KChannel1-1/KKeyword11",
      "KeywordTargeted/CCID", "" },
    { "Site1/Tag7", &NSL::referer_kw,
      "DChannel2/DKeyword2,KChannel1-1/KKeyword11,CTChannel-CPC/CTKeywordCPC",
      "DisplayCPM#2/CCID", "" },
    { "Site1/Tag6", &NSL::referer_kw,
      "DChannel1/DKeyword1,KChannel1-1/KKeyword11,CTChannel-CPC/CTKeywordCPC",
      "ChannelTargetedCPC/CCID,KeywordTargeted/CCID", "" }
  };

  process_requests(REQUESTS, "TagAdjustment/");
}


void CTREffectTest::negative_ctr_()
{
  const TestCaseRequest REQUESTS[] =
  {
    { "Site1/Tag9", &NSL::referer_kw,
      "DChannel-CPC-Negative/DKeywordCPCNegative",
      "DisplayCPCNegative/CCID", "" },
    { "Site1/Tag9", &NSL::referer_kw,
      "DChannel-CPA-Negative/DKeywordCPANegative",
      "DisplayCPANegative/CCID", "" },
    { "Site1/Tag9", &NSL::referer_kw,
      "KChannelNegative/KKeywordNegative",
      "KeywordTargeted/CCID", "" },
    { "Site1/Tag8", &NSL::referer_kw,
      "DChannel-CPC/DKeywordCPC",
      "DisplayCPC/CCID", "" }
  };
  
  process_requests(REQUESTS, "TagAdjustment/");
}

void CTREffectTest::time_of_week_()
{
  const TestCaseRequest REQUESTS[] =
  {
    { "TagAdjustment/Site1/Tag3", &NSL::referer_kw,
      "TOWCoefficient/KChannel1-2/Keyword12",
      "", "" },
    { "TagAdjustment/Site1/Tag3", &NSL::referer_kw,
      "TOWCoefficient/KChannel1-1/Keyword11",
      "TOWCoefficient/KeywordTargeted#1/CCID", "" },
    { "TagAdjustment/Site1/Tag1", &NSL::referer_kw,
      "TOWCoefficient/KChannel1-1/Keyword11,"
      "TOWCoefficient/KChannel2-1/Keyword21",
      "TOWCoefficient/KeywordTargeted#1/CCID,"
      "TOWCoefficient/KeywordTargeted#2/CCID", ""},
    { "TagAdjustment/Site1/Tag1", &NSL::referer_kw,
      "TOWCoefficient/KChannel1-1/Keyword11,"
      "TOWCoefficient/KChannel2-1/Keyword21,"
      "TOWCoefficient/DChannel/DKeyword",
      "TOWCoefficient/KeywordTargeted#1/CCID,"
      "TOWCoefficient/KeywordTargeted#2/CCID", ""},
    { "TagAdjustment/Site1/Tag1", &NSL::referer_kw,
      "TOWCoefficient/KChannel1-3/Keyword13,"
      "TOWCoefficient/KChannel2-2/Keyword22",
      "TOWCoefficient/KeywordTargeted#1/CCID,"
      "TOWCoefficient/KeywordTargeted#2/CCID",
      "TOWCoefficient/KeywordTargeted#1/Keyword13/CLICK_REVENUE,"
      "TOWCoefficient/KeywordTargeted#2/Keyword22/CPC"},
    { "TagAdjustment/Site1/Tag1", &NSL::referer_kw,
      "TOWCoefficient/KChannel1-4/Keyword14,"
      "TOWCoefficient/KChannel2-2/Keyword22",
      "TOWCoefficient/KeywordTargeted#2/CCID,"
      "TOWCoefficient/KeywordTargeted#1/CCID",
      "TOWCoefficient/KeywordTargeted#2/Keyword22/CLICK_REVENUE,"
      "TOWCoefficient/KeywordTargeted#1/Keyword14/CPC"},
    { "TagAdjustment/Site1/Tag5", &NSL::referer_kw,
      "TOWCoefficient/KChannel1-4/Keyword14,"
      "TOWCoefficient/KChannel2-2/Keyword22",
      "", ""},
  };
  
  process_requests(REQUESTS);
}

bool 
CTREffectTest::run_test()
{
  AUTOTEST_CASE(
    regular_adjustment_(),
    "Regular Tag adjustment");

  AUTOTEST_CASE(
    invalid_ctr_(),
    "Tag adjustment with CTR > 1 and CTR = 0");

  AUTOTEST_CASE(
    ccg_concurrence_(),
    "CCG concurrence with tag adjustment");

  AUTOTEST_CASE(
    negative_ctr_(),
    "Negative CTR/AR");

  AUTOTEST_CASE(
    time_of_week_(),
    "Time of week");

  return true;
}
