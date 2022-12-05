#include "CampaignUpdateTest.hpp"

REFLECT_UNIT(CampaignUpdateTest) (
  "GranularUpdate",
  AUTO_TEST_SLOW
);

namespace
{

  typedef AutoTest::CampaignChecker CampaignChecker;

  AutoTest::Time now_trunc()
  {
    return
      AutoTest::Time().get_gm_time().
        format("%d-%m-%Y:%H-%M-00").c_str();
  }

  std::string
  make_channels_regexp(const AutoTest::ORM::PQ::Channel &channel)
  {
    std::string regexp;
    String::StringManip::Splitter<
      const String::AsciiStringManip::Char2Category<' ', '|'> > tokenizer(
      channel.expression.value());
    bool firstToken = true;
    unsigned short channelsCount = 0;
    String::SubString token;
    while (tokenizer.get_token(token))
    {
      if (!firstToken) regexp += " \\| ";
      regexp += "\\[";
      regexp += token.str();
      regexp += "\\]";
      firstToken = false;
      channelsCount++;
    }
    if (!channelsCount) return "0";
    if (channelsCount == 1) return regexp;
    return "\\(" + regexp + "\\)";
  }

}

void CampaignUpdateTest::add_campaign_case_()
{
  ORM::ORMRestorer<ORM::PQ::Campaign>* campaign =
    create<ORM::PQ::Campaign>();
  campaign->name = fetch_string("AddCampaign/NewName");
  campaign->account = fetch_int("Advertiser");
  campaign->flags = 16;
  campaign->budget = 100000;
  campaign->campaign_type = "D";
  campaign->sold_to_user = fetch_int("DefaultUser");
  campaign->bill_to_user = fetch_int("DefaultUser");
  campaign->date_start.set_now().trunc();
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      campaign->insert()),
    "inserting campaign");

  // Create CCG
  ORM::ORMRestorer<ORM::RatedCampaignCreativeGroup>* ccg =
    create<ORM::RatedCampaignCreativeGroup>();
  ccg->name = fetch_string("AddCampaign/NewName");
  ccg->ccg_type = "D";
  ccg->campaign = campaign->id();
  ccg->flags = 16;
  ccg->channel = fetch_int("UpdateChannel/BChannel1/ID");
  ccg->qa_status = "A";
  ccg->date_start.set_now().trunc();
  ccg->cur_date.set_now();
  ccg->tgt_type = "C";
  ccg->country_code = "GN";
  ccg->channel_target = "T";
  ccg->delivery_pacing = "F";
  ccg->daily_budget = 1000000;
  ccg->rate.cpm = 100;
  ccg->rate.cpc = 0;
  ccg->rate.cpa = 0;
  ccg->rate.rate_type = "CPM";
  ccg->rate.effective_date.set_now();
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      ccg->insert()),
    "inserting CampaignCreativeGroup");

  // Link with existing creative
  // (to avoid status 'I' for campaign)
  ORM::ORMRestorer<ORM::PQ::CampaignCreative>* cc =
    create<ORM::PQ::CampaignCreative>();
  cc->creative = fetch_int("AddCampaign/Creative#1");
  cc->ccg = ccg->id();
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      cc->insert()),
    "inserting CampaignCreative");

  ADD_WAIT_CHECKER(
    "Check campaign",
    CampaignChecker(
      this,
      ccg->id(),
      CampaignChecker::Expected().
        account_id(fetch_string("Advertiser")).
        account_currency_id(fetch_string("DefaultCurrency")).
        channels("\\[" + fetch_string("UpdateChannel/BChannel1/ID") + "\\]").
        ecpm(std::string("10000.0")). // cpm = 100, exchange rate = 1 (default)
        status("A").
        eval_status("A")));
}

void CampaignUpdateTest::currency_change_case_()
{
  unsigned int account_id = fetch_int("CurrencyChange/Campaign/ACCOUNT");
  unsigned int ccg_id = fetch_int("CurrencyChange/Campaign/CCG");

  ORM::ORMRestorer<ORM::PQ::Account>* account =
    create<ORM::PQ::Account>(account_id);

  FAIL_CONTEXT(
    CampaignChecker(
      this,
      ccg_id,
      CampaignChecker::Expected().
        account_id(strof(account_id)).
        account_currency_id(fetch_string("DefaultCurrency")).
        status("A").
        eval_status("A")).check(),
    "Initial");

  unsigned int new_currency = fetch_int("CurrencyChange/Currency#1");
  account->currency = new_currency;

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      account->update()),
    "updating currency");

  ADD_WAIT_CHECKER(
    "Check campaign",
    CampaignChecker(
      this,
      ccg_id,
      CampaignChecker::Expected().
        account_id(strof(account_id)).
        account_currency_id(strof(new_currency)).
        status("A").
        eval_status("A")));
}

void
CampaignUpdateTest::update_channel_case_()
{
  const unsigned int channel1 = fetch_int("UpdateChannel/BChannel1/ID");
  const unsigned int channel2 = fetch_int("UpdateChannel/BChannel2/ID");
  const unsigned int targeting_full = fetch_int("UpdateChannel/TargetingFull/ID");
  const unsigned int geo = fetch_int("UpdateChannel/GEO/ID");
  const unsigned int platform = fetch_int("UpdateChannel/Device/EXP");

  // Add channel
  {
    std::string description("Link channel.");

    std::string account_id = fetch_string("UpdateChannel/AddChannel/ACCOUNT");
    ORM::ORMRestorer<ORM::PQ::CampaignCreativeGroup>* ccg =
      create<ORM::PQ::CampaignCreativeGroup>(
        fetch_int("UpdateChannel/AddChannel/CCG"));

    FAIL_CONTEXT(CampaignChecker(this, ccg->id(),
        CampaignChecker::Expected().
          account_id(account_id).
          channels("NULL").
          status("A").
          eval_status("A")).check(),
      description + " - Initial");

    ccg->channel = channel1;
    ccg->targeting_channel_id = targeting_full;
    ccg->channel_target = "T";
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        ccg->update()),
      "updating ccg.channel");

    std::string regexp("\\(\\[" + strof(channel1) + "\\] & \\[" +
      strof(geo) + "\\] & \\[" + strof(platform) + "\\]\\)");

    ADD_WAIT_CHECKER(
      description,
      CampaignChecker(
        this,
        ccg->id(),
        CampaignChecker::Expected().
          account_id(account_id).
          channels(regexp).
          status("A").
          eval_status("A")));
  }

  // Change channel
  {
    std::string description("Change channel.");

    std::string account_id = fetch_string("UpdateChannel/ChangeChannel/ACCOUNT");
    ORM::ORMRestorer<ORM::PQ::CampaignCreativeGroup>* ccg =
      create<ORM::PQ::CampaignCreativeGroup>(
        fetch_int("UpdateChannel/ChangeChannel/CCG"));

    FAIL_CONTEXT(CampaignChecker(this, ccg->id(),
        CampaignChecker::Expected().
          account_id(account_id).
          channels("\\[" + strof(channel1) + "\\]").
          status("A").
          eval_status("A")).check(),
      description + " - Initial");

    ccg->channel = channel2;
    ccg->targeting_channel_id = channel2;
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        ccg->update()),
      "updating ccg.channel");

    ADD_WAIT_CHECKER(
      description,
      CampaignChecker(
        this,
        ccg->id(),
        CampaignChecker::Expected().
          account_id(account_id).
          channels("\\[" + strof(channel2) + "\\]").
          status("A").
          eval_status("A")));
  }

  // Delete channel link
  {
    std::string description("Unlink channel.");
    std::string account_id = fetch_string("UpdateChannel/NullChannel/ACCOUNT");
    ORM::ORMRestorer<ORM::PQ::CampaignCreativeGroup>* ccg =
      create<ORM::PQ::CampaignCreativeGroup>(
        fetch_int("UpdateChannel/NullChannel/CCG"));

    FAIL_CONTEXT(CampaignChecker(this, ccg->id(),
        CampaignChecker::Expected().
          account_id(account_id).
          channels("\\[" + strof(channel1) + "\\]").
          status("A").
          eval_status("A")).check(),
      description + " - Initial");

    ccg->channel.null();
    ccg->targeting_channel_id.null();
    ccg->channel_target = "N";
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        ccg->update()),
      "updating ccg.channel");
    
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        ccg->set_display_status(
          AutoTest::ORM::DS_NOT_LIVE_BY_CHANNEL_TARGET)),
      "updating ccg.display_status changed");

    ADD_WAIT_CHECKER(
      description,
      CampaignChecker(
        this,
        ccg->id(),
        CampaignChecker::Expected().
          account_id(account_id).
          channels("NULL").
          status("I").
          eval_status("A")));
  }

  // Change channel expression
  {
    std::string description("Change channel expression.");

    const unsigned int ccg_id = fetch_int("UpdateChannel/ExprChannel/CCG");

    ORM::ORMRestorer<ORM::PQ::Channel>* channel =
      create<ORM::PQ::Channel>(
        fetch_int("UpdateChannel/EChannel/ID"));

    {
      std::string regexp = make_channels_regexp(*channel);
      FAIL_CONTEXT(CampaignChecker(this, ccg_id,
          CampaignChecker::Expected().
            channels(regexp)).check(),
        description + " Initial");
    }

    channel->expression = fetch_string("UpdateChannel/B1forExpr/ID");
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        channel->update()),
      "updating expression channel");

    std::string regexp = make_channels_regexp(*channel);

    ADD_WAIT_CHECKER(
      description,
      CampaignChecker(
        this,
        ccg_id,
        CampaignChecker::Expected().channels(regexp)));
  }

  // Untargeting
  {
    std::string description("Untargeting.");

    const unsigned int ccg_id = fetch_int("UpdateChannel/Untargeting/CCG");
    ORM::ORMRestorer<ORM::PQ::CampaignCreativeGroup>* ccg =
      create<ORM::PQ::CampaignCreativeGroup>(ccg_id);

    std::string regexp("\\(\\[" + strof(channel1) + "\\] & \\[" +
      strof(geo) + "\\] & \\[" + strof(platform) + "\\]\\)");

    FAIL_CONTEXT(
      CampaignChecker(
        this,
        ccg->id(),
        CampaignChecker::Expected().
          channels(regexp).
          status("A").
          eval_status("A")).check(),
      description + " - Initial");

    ccg->channel.null();
    ccg->targeting_channel_id =
      fetch_int("UpdateChannel/TargetingUntarget/ID");
    ccg->channel_target = "U";

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        ccg->update()),
      "updating ccg.channel");

    regexp = "\\(\\[" +  strof(geo) + "\\] & \\[" +
      strof(platform) + "\\]\\)";

    ADD_WAIT_CHECKER(
      description,
      CampaignChecker(
        this,
        ccg->id(),
        CampaignChecker::Expected().
          channels(regexp).
          status("A").
          eval_status("A")));
  }
}

void CampaignUpdateTest::change_status_case_()
{
  // Deactivating
  {
    std::string description("Campaign deactivation.");

    std::string account_id = fetch_string("ChangeStatus/CampaignA/ACCOUNT");
    const unsigned int ccg_id = fetch_int("ChangeStatus/CampaignA/CCG");

    ORM::ORMRestorer<ORM::PQ::Campaign>* campaign =
      create<ORM::PQ::Campaign>(fetch_int("ChangeStatus/CampaignA/CAMPAIGN"));

    FAIL_CONTEXT(CampaignChecker(this, ccg_id,
        CampaignChecker::Expected().
          account_id(account_id).
          status("A").
          eval_status("A")).check(),
      description + " Initial");
    
    campaign->status = "D";
    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        campaign->update()),
      "updating campaign.status");

    ADD_WAIT_CHECKER(
      description,
      CampaignChecker(
        this,
        ccg_id,
        CampaignChecker::NOT_PRESENT));
  }

  // Activation
  {
    std::string description("Campaign activation.");

    std::string account_id = fetch_string("ChangeStatus/CampaignD/ACCOUNT");
    const unsigned int ccg_id = fetch_int("ChangeStatus/CampaignD/CCG");

    ORM::ORMRestorer<ORM::PQ::Campaign>* campaign =
      create<ORM::PQ::Campaign>(
        fetch_int("ChangeStatus/CampaignD/CAMPAIGN"));

    FAIL_CONTEXT(
      CampaignChecker(
        this,
        ccg_id,
        CampaignChecker::NOT_PRESENT).check(),
      description + " Initial");

    campaign->status = "A";

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        campaign->update()),
      "updating campaign.status");

    ADD_WAIT_CHECKER(
      description,
      CampaignChecker(
        this,
        ccg_id,
        CampaignChecker::Expected().
          account_id(account_id).
          status("A").
          eval_status("A")));
  }
}

void CampaignUpdateTest::change_date_interval_case_()
{
  // Change campaign dates interval
  {
    std::string description("Change campaign date interval.");

    const unsigned int ccg_id = fetch_int("ChangeDateInterval/CMPOutside/CCG");

    ORM::ORMRestorer<ORM::PQ::Campaign>* campaign =
      create<ORM::PQ::Campaign>(
        fetch_int("ChangeDateInterval/CMPOutside/CAMPAIGN"));

    FAIL_CONTEXT(CampaignChecker(this, ccg_id,
        CampaignChecker::Expected().
          cmp_date_start("01-01-1970:00-00-00.000000").
          cmp_date_end("02-01-1970:00-00-00.000000").
          status("A").
          eval_status("I").
          force_remote_present(true)).check(),
      description + " - Initial");

    AutoTest::Time now = now_trunc();

    AutoTest::Time date_start = now - 10*60;
    AutoTest::Time date_end = now + 2*60*60;

    campaign->date_start = date_start;
    campaign->date_end = date_end;

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        campaign->update()),
      "updating campaign.dates");

    ADD_WAIT_CHECKER(
      description,
      CampaignChecker(
        this,
        ccg_id,
        CampaignChecker::Expected().
          cmp_date_start(
            date_start.get_gm_time().format("%d-%m-%Y:%H-%M-00.000000")).
          cmp_date_end(
            date_end.get_gm_time().format("%d-%m-%Y:%H-%M-00.000000")).
          status("A").
          eval_status("A")));
  }

  // Change ccg dates interval
  {
    std::string description("Change ccg date interval.");

    ORM::ORMRestorer<ORM::PQ::CampaignCreativeGroup>* ccg =
      create<ORM::PQ::CampaignCreativeGroup>(
        fetch_int("ChangeDateInterval/CCGOutside/CCG"));

    FAIL_CONTEXT(CampaignChecker(this, ccg->id(),
        CampaignChecker::Expected().
          date_start("01-01-1970:00-00-00.000000").
          date_end("02-01-1970:00-00-00.000000").
          status("A").
          eval_status("I").
          force_remote_present(true)).check(),
      description + " - Initial");

    AutoTest::Time now = now_trunc();
    AutoTest::Time date_start = now - 10*60;
    AutoTest::Time date_end = now + 2*60*60;

    ccg->date_start = date_start;
    ccg->date_end = date_end;

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        ccg->update()),
      "updating ccg.dates");

    ADD_WAIT_CHECKER(
      description,
      CampaignChecker(
        this,
        ccg->id(),
        CampaignChecker::Expected().
          date_start(
            date_start.get_gm_time().format("%d-%m-%Y:%H-%M-00.000000")).
          date_end(
            date_end.get_gm_time().format("%d-%m-%Y:%H-%M-00.000000")).
          status("A").
          eval_status("A")));
  }

  // Campaign expiration
  {
    std::string description("Campaign expiration.");

    const unsigned int ccg_id = fetch_int("ChangeDateInterval/CMPExpiration/CCG");

    ORM::ORMRestorer<ORM::PQ::Campaign>* campaign =
      create<ORM::PQ::Campaign>(
        fetch_int("ChangeDateInterval/CMPExpiration/CAMPAIGN"));

    FAIL_CONTEXT(CampaignChecker(this, ccg_id,
        CampaignChecker::Expected().
          cmp_date_end("01-01-1970:00-00-00.000000").
          status("A").
          eval_status("A")).check(),
      description + " - Initial");

    AutoTest::Time date_end = (now_trunc() + 2*60);

    campaign->date_end = date_end;

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        campaign->update()),
      "updating campaign.date_end");

    ADD_WAIT_CHECKER(
      description,
      CampaignChecker(
        this,
        ccg_id,
        CampaignChecker::Expected().
          cmp_date_end(
            date_end.get_gm_time().format("%d-%m-%Y:%H-%M-00.000000")).
          status("A").
          eval_status("I").
          force_remote_present(true)));
  }

  // Creative group expiration
  {
    std::string description("Creative group expiration.");

    ORM::ORMRestorer<ORM::PQ::CampaignCreativeGroup>* ccg =
      create<ORM::PQ::CampaignCreativeGroup>(
        fetch_int("ChangeDateInterval/CCGExpiration/CCG"));

    FAIL_CONTEXT(CampaignChecker(this, ccg->id(),
        CampaignChecker::Expected().
          date_end("01-01-1970:00-00-00.000000").
          status("A").
          eval_status("A")).check(),
      description + " - Initial");

    AutoTest::Time date_end = (now_trunc() + 2*60);

    ccg->date_end = date_end;

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        ccg->update()),
      "updating ccg.date_end");

    ADD_WAIT_CHECKER(
      description,
      CampaignChecker(
        this,
        ccg->id(),
        CampaignChecker::Expected().
          date_end(
            date_end.get_gm_time().format("%d-%m-%Y:%H-%M-00.000000")).
          status("A").
          eval_status("I").
          force_remote_present(true)));
  }
}

void
CampaignUpdateTest::change_max_pub_share_case_()
{
  const unsigned int ccg_id = fetch_int("ChangeMaxPubShare/Campaign/CCG");
  std::string account_id = fetch_string("ChangeMaxPubShare/Campaign/ACCOUNT");

  ORM::ORMRestorer<ORM::PQ::Campaign>* campaign =
    create<ORM::PQ::Campaign>(
      fetch_int("ChangeMaxPubShare/Campaign/CAMPAIGN"));

  const char* old_max_pub_share = "1.0";
  const char* new_max_pub_share = "0.5";

  FAIL_CONTEXT(CampaignChecker(this, ccg_id,
      CampaignChecker::Expected().
        account_id(account_id).
        account_currency_id(fetch_string("DefaultCurrency")).
        status("A").
        eval_status("A").
        max_pub_share(old_max_pub_share)).check(),
    "Initial");

  campaign->max_pub_share = 0.5;
  
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      campaign->update()),
    "updating campaign.max_pub_share");

  ADD_WAIT_CHECKER(
    "Check campaign",
    CampaignChecker(this, ccg_id,
      CampaignChecker::Expected().
        account_id(account_id).
        status("A").
        eval_status("A").
        max_pub_share(new_max_pub_share)));  
}

void CampaignUpdateTest::set_up()
{
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      get_config().check_service(
        CTE_ALL, STE_CAMPAIGN_MANAGER)),
    "CampaignManager must set in the XML configuration file");
}

bool
CampaignUpdateTest::run()
{
  AUTOTEST_CASE(
    add_campaign_case_(),
    "Add campaign");
  
  AUTOTEST_CASE(
    currency_change_case_(),
    "Update currency");
  
  AUTOTEST_CASE(
    update_channel_case_(),
    "Update channel");
  
  AUTOTEST_CASE(
    change_status_case_(),
    "Change status");
  
  AUTOTEST_CASE(
    change_date_interval_case_(),
    "Change date interval");
  
  AUTOTEST_CASE(
    change_max_pub_share_case_(),
    "Change campaign max_pub_share");

  return true;
}

void CampaignUpdateTest::tear_down()
{}
