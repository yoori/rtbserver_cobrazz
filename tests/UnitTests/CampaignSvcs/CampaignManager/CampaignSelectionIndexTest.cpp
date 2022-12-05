/// @file CampaignSelectionIndexTest.cpp
#include "malloc.h"
#include <Logger/StreamLogger.hpp>
#include <Commons/Algs.hpp>
#include <CampaignSvcs/CampaignManager/CampaignIndex.hpp>

using namespace AdServer::CampaignSvcs;

Colocation_var
create_simple_colocation(unsigned long id, AccountDef* acc)
{
  Colocation_var colo = new Colocation();
  colo->colo_id = id;
  colo->colo_rate_id = id;
  colo->at_flags = 0;
  colo->account = ReferenceCounting::add_ref(acc);
  colo->revenue_share = RevenueDecimal::ZERO;
  colo->ad_serving = CS_ALL;
  return colo;
}

void add_campaign_(
  CampaignConfig& new_config,
  AccountDef* p_acc,
  Size* p_size,
  unsigned long id,
  unsigned long ecpm)
{
  Campaign_var campaign = new Campaign();

  campaign->campaign_id = id;
  campaign->campaign_group_id = id;
  campaign->account = ReferenceCounting::add_ref(p_acc);
  campaign->fc_id = 0;
  campaign->group_fc_id = 0;
  campaign->flags = CampaignFlags::US_NONE;
  campaign->mode = CM_NON_RANDOM;
  campaign->advertiser = ReferenceCounting::add_ref(p_acc);
  campaign->imp_revenue = RevenueDecimal::ZERO;
  campaign->click_revenue = RevenueDecimal::div(
    RevenueDecimal(false, ecpm, 0),
    RevenueDecimal(false, 100000, 0));
  campaign->click_sys_revenue = campaign->click_revenue;
  campaign->commision = RevenueDecimal::ZERO;
  campaign->ecpm_ = RevenueDecimal(ecpm);

  campaign->ccg_rate_id = 0;
  campaign->ccg_rate_type = CR_CPC;
  campaign->ctr = RevenueDecimal(false, 1, 0);

  campaign->status = 'A';
  campaign->eval_status = 'A';
  campaign->ccg_type = CT_DISPLAY;
  campaign->targeting_type = 'C';

  campaign->country = "ru";
  campaign->start_user_group_id = 0;
  campaign->end_user_group_id = MAX_TARGET_USERS_GROUPS;
  campaign->marketplace = 'A';

  ::system("echo TEST > ~test-template-file");

  CreativeTemplate c_templ(
    "~test-template-file",
    CreativeTemplateFactory::Handler::CTT_TEXT,
    "mime-format",
    false,
    0, // tokens
    0, // hidden tokens
    Generics::Time::ZERO);

  c_templ.status = 'A';

  new_config.creative_templates.insert(
    CreativeTemplateKey(
      "test-format",
      "test-size",
      "test-appformat"),
    c_templ);

  Creative_var creative(
    new Creative(
      campaign,
      1,
      1,
      0,
      1,
      "test-format",
      "",
      OptionValue(0, "test-url"),
      "test-url",
      "test-url",
      Creative::CategorySet()));

  Creative::Size creative_size;
  creative_size.size = ReferenceCounting::add_ref(p_size);
  creative_size.up_expand_space = 0;
  creative_size.right_expand_space = 0;
  creative_size.down_expand_space = 0;
  creative_size.left_expand_space = 0;
  creative_size.expandable = false;
  creative_size.available_appformats.insert("test-appformat");

  creative->sizes.insert(std::make_pair(1, creative_size));

  creative->status = 'A';

  campaign->add_creative(creative);

  new_config.campaigns.insert(std::make_pair(id, campaign));
}

void add_tag_(
  CampaignConfig& new_config,
  AccountDef* p_acc,
  Size* p_size,
  unsigned long id,
  unsigned long cpm = 0)
{
  Site_var p_site = new Site();

  p_site->site_id = id;
  p_site->account = ReferenceCounting::add_ref(p_acc);
  p_site->freq_cap_id = 0;
  p_site->noads_timeout = 0;
  p_site->status = 'A';
  p_site->flags = 0;

//p_site->approved_campaigns.insert(1);

  new_config.sites[p_site->site_id] = p_site;

  Tag_var p_tag = new Tag();

  p_tag->tag_id = id;
  p_tag->site = p_site;
  p_tag->adjustment = RevenueDecimal(false, 1, 0);
  p_tag->marketplace = 'A';
  Tag::Size_var tag_size = new Tag::Size();
  tag_size->size = ReferenceCounting::add_ref(p_size);
  tag_size->max_text_creatives = 1;
  p_tag->sizes.insert(std::make_pair(1, tag_size));

  Tag::TagPricing tag_pricing;
  tag_pricing.site_rate_id = 0;
  tag_pricing.cpm = RevenueDecimal(false, cpm, 0);
  p_tag->tag_pricings.insert(std::make_pair(
    Tag::TagPricingKey("", CT_ALL, CR_ALL), tag_pricing));
  p_tag->country_tag_pricings.insert(std::make_pair("", tag_pricing));

  new_config.tags[p_tag->tag_id] = p_tag;
}

Account_var
add_account_(
  CampaignConfig& new_config,
  unsigned long account_id,
  Currency* currency)
{
  Account_var acc = new AccountDef();
  acc->account_id = account_id;
  acc->internal_account_id = account_id;
  acc->flags = 0;
  acc->at_flags = 0;
  acc->text_adserving = 'A';
  acc->currency = ReferenceCounting::add_ref(currency);
  acc->country = "ru";
  acc->time_offset = Generics::Time::ZERO;
  acc->commision = RevenueDecimal::ZERO;
  acc->budget = RevenueDecimal::ZERO;
  acc->paid_amount = RevenueDecimal::ZERO;
  acc->status = 'A';
  acc->eval_status = 'A';
  new_config.accounts[account_id] = acc;

  return acc;
}

Size_var
create_size_()
{
  Size_var size = new Size;
  size->size_id = 1;
  size->protocol_name = "test-size";
  size->width = 1;
  size->height = 1;
  return size;
}

namespace Test1
{
  static const char TEST_NAME[] = "Test1";

  void fill(CampaignConfig& new_config)
  {
    Currency_var p_currency = new Currency();

    p_currency->currency_id = 1;
    p_currency->currency_exchange_id = 1;
    p_currency->effective_date = 0;
    p_currency->rate = RevenueDecimal(false, 1, 0);
    p_currency->fraction = 1;

    new_config.currencies[p_currency->currency_id] = p_currency;

    Account_var p_acc = add_account_(new_config, 1, p_currency);
    Account_var p_acc_2 = add_account_(new_config, 2, p_currency);

    Colocation_var colo1 = create_simple_colocation(1, p_acc);
    new_config.colocations.insert(std::make_pair(1, colo1));
    Colocation_var colo2 = create_simple_colocation(2, p_acc_2);
    new_config.colocations.insert(std::make_pair(2, colo2));

    Size_var size = create_size_();
    add_campaign_(new_config, p_acc, size, 1, 1000);
    add_tag_(new_config, p_acc, size, 100);

    add_campaign_(new_config, p_acc_2, size, 2, 32);
    add_tag_(new_config, p_acc_2, size, 2);
  }

  bool check_select_(
    CampaignConfig* campaign_config,
    unsigned long expected_size,
    const char* /*test_name*/,
    const char* check_name,
    AdServer::CampaignSvcs::CampaignIndex* campaign_index,
    unsigned long tag_id,
    const char* country_code,
    UserStatus user_status,
    bool none_user_status)
  {
    TagMap::const_iterator tag_it =
      campaign_config->tags.find(tag_id);
    if(tag_it == campaign_config->tags.end())
    {
      return false;
    }

    CampaignIndex::Key key(tag_it->second);
    key.country_code = country_code;
    key.format = "test-appformat";
    key.user_status = user_status;
    key.none_user_status = none_user_status;
    key.test_request = false;

    CampaignIndex::CampaignSelectionCellPtrList wg_ch_cmps;
    CampaignIndex::CampaignSelectionCellPtrList ch_cmps;
    CampaignIndex::CampaignCellPtrList text_cmps;
    CampaignIndex::CampaignCellPtrList kw_cmps;

    campaign_index->get_campaigns(
      key,
      wg_ch_cmps,
      ch_cmps,
      text_cmps,
      kw_cmps,
      0,
      0);

    int res = 0;

    if(ch_cmps.size() != expected_size)
    {
      std::cerr << TEST_NAME << ": " << check_name <<
        ": unexpected select size = " << ch_cmps.size() << " instead " <<
        expected_size << std::endl;

      res = 1;

      std::cerr << "trace_tree:" << std::endl;
      campaign_index->trace_tree(std::cerr);
    }

    return res;
  }

  int run()
  {
    CampaignConfig_var campaign_config(new CampaignConfig());

    fill(*campaign_config);

    Logging::Logger_var logger(
      new Logging::OStream::Logger(Logging::OStream::Config(std::cout)));

    AdServer::CampaignSvcs::CampaignIndex_var campaign_index(
      new AdServer::CampaignSvcs::CampaignIndex(campaign_config, logger));

    campaign_index->index_campaigns();
//  campaign_index->trace_tree(std::cout);

    int ret = check_select_(
      campaign_config,
      1,
      TEST_NAME,
      "1",
      campaign_index,
      1,
      "ru",
      US_OPTOUT,
      true // none_user_status
      );

    ret += check_select_(
      campaign_config,
      0,
      TEST_NAME,
      "2",
      campaign_index,
      1,
      "",
      US_OPTIN,
      true // none_user_status
      );

    ret += check_select_(
      campaign_config,
      0,
      TEST_NAME,
      "3",
      campaign_index,
      1,
      "ru",
      US_OPTIN,
      true // none_user_status
      );

    /*
    {
      CampaignIndex::Key key(1);
      key.country_code = "ru";
      key.format = "test-appformat";
      key.colo_id = 1;
      key.optout = true;
      key.test_request = false;

      CampaignIndex_var trace_campaign_index(
        new CampaignIndex(*campaign_index, logger));

      trace_campaign_index->trace_indexing(
        key,
        Generics::Time::ZERO,
        FreqCapIdSet(),
        "",
        "",
        ChannelIdSet(),
        (++campaign_config->campaigns.begin())->second,
        campaign_config->tags.begin()->second,
        std::cout);
    }
    */

    /*
    std::cout << ">>>>>>>>>>>" << std::endl;
    campaign_index->trace_margin_tree(std::cout);
    std::cout << "<<<<<<<<<<<" << std::endl;
    */

    if(ret == 0)
    {
      std::cout << TEST_NAME << ": success." << std::endl;
    }

    return ret;
  }
}

void fill_test_campaign_config(
  CampaignConfig& new_config,
  unsigned long colocation_count,
  unsigned long tag_count,
  unsigned long campaign_count)
{
  Currency_var p_currency = new Currency();

  p_currency->currency_id = 1;
  p_currency->currency_exchange_id = 1;
  p_currency->effective_date = 0;
  p_currency->rate = RevenueDecimal(false, 1, 0);
  p_currency->fraction = 1;

  new_config.currencies[p_currency->currency_id] = p_currency;

  Size_var p_size = create_size_();

  Account_var p_acc = add_account_(new_config, 1, p_currency);

  {
    for(unsigned long i = 1; i <= colocation_count; ++i)
    {
      Colocation_var colo = create_simple_colocation(i, p_acc);
      new_config.colocations.insert(std::make_pair(i, colo));
    }
  }

  for(unsigned long i = 1; i <= campaign_count; ++i)
  {
    Campaign_var campaign = new Campaign();

    campaign->campaign_id = i;
    campaign->campaign_group_id = i;
    campaign->account = p_acc;
    campaign->fc_id = 0;
    campaign->group_fc_id = 0;
    campaign->flags = 0;
    campaign->advertiser = p_acc;
    campaign->imp_revenue = RevenueDecimal::ZERO;
    campaign->click_revenue = RevenueDecimal::ZERO;
    campaign->click_sys_revenue = RevenueDecimal::ZERO;
    campaign->commision = RevenueDecimal::ZERO;
    campaign->ecpm_ = RevenueDecimal(1000);

    campaign->ccg_rate_id = 0;
    campaign->ccg_rate_type = CR_CPM;
    campaign->ctr = RevenueDecimal::ZERO;
    campaign->status = 'A';
    campaign->eval_status = 'A';
    campaign->ccg_type = CT_DISPLAY;
    campaign->targeting_type = 'C';

    campaign->country = "ru";
    campaign->start_user_group_id = 0;
    campaign->end_user_group_id = MAX_TARGET_USERS_GROUPS;
    campaign->marketplace = 'A';

//  ::system("echo TEST > ~test-template-file");

    CreativeTemplate c_templ(
      "~test-template-file",
      CreativeTemplateFactory::Handler::CTT_TEXT,
      "mime_format",
      false,
      0, // tokens
      0, // hidden tokens
      Generics::Time::ZERO);

    c_templ.status = 'A';

    new_config.creative_templates.insert(
      CreativeTemplateKey(
        "test-format",
        "test-size",
        "test-appformat"),
      c_templ);

    Creative_var creative(
      new Creative(
        campaign,
        i,
        i,
        0,
        1,
        "test-format",
        "",
        OptionValue(0, "test-url"),
        "test-url",
        "test-url",
        Creative::CategorySet()));

    Creative::Size creative_size;
    creative_size.size = ReferenceCounting::add_ref(p_size);
    creative_size.up_expand_space = 0;
    creative_size.right_expand_space = 0;
    creative_size.down_expand_space = 0;
    creative_size.left_expand_space = 0;
    creative_size.expandable = false;
    creative_size.available_appformats.insert("test-appformat");
    creative->sizes.insert(std::make_pair(1, creative_size));

    creative->status = 'A';

    campaign->add_creative(creative);

    new_config.campaigns.insert(std::make_pair(i, campaign));
  }

  for(unsigned long i = 1; i <= tag_count; ++i)
  {
    Site_var p_site = new Site();

    p_site->site_id = i;
    p_site->account = p_acc;
    p_site->freq_cap_id = 0;
    p_site->noads_timeout = 0;
    p_site->status = 'A';
    p_site->flags = 0;

    p_site->approved_creatives.insert(1);

    new_config.sites[p_site->site_id] = p_site;

    Tag_var p_tag = new Tag();

    p_tag->tag_id = i;
    p_tag->site = p_site;
    p_tag->adjustment = RevenueDecimal(false, 1, 0);
    p_tag->marketplace = 'A';

    Tag::Size_var tag_size = new Tag::Size();
    tag_size->size = ReferenceCounting::add_ref(p_size);
    tag_size->max_text_creatives = 1;
    p_tag->sizes.insert(std::make_pair(1, tag_size));

    Tag::TagPricing tag_pricing;
    tag_pricing.site_rate_id = 0;
    tag_pricing.cpm = RevenueDecimal::ZERO;
    p_tag->tag_pricings.insert(std::make_pair(
      Tag::TagPricingKey("", CT_ALL, CR_ALL), tag_pricing));
    p_tag->country_tag_pricings.insert(std::make_pair("", tag_pricing));

    new_config.tags[p_tag->tag_id] = p_tag;
  }
}

/*
void index(const char* name, CampaignConfig* campaign_config)
{
  Logging::Logger_var logger(
    new Logging::OStream::Logger(Logging::OStream::Config(std::cout)));

  CampaignSelectionIndex_var config_index(
    new CampaignSelectionIndex(campaign_config, logger));

  std::cerr << name << " : start heap state: " << std::endl;
  ::malloc_stats();

  Generics::Timer timer;
  timer.start();
  config_index->index_campaigns();
  timer.stop();

  std::cerr << name << " : stop heap state: " << std::endl;
  ::malloc_stats();

  std::cout << name << " : indexing time " << timer.elapsed_time() << std::endl;

  CampaignSelectionIndex::Key key(1);
  key.format = "test-appformat";
  key.colo_id = 1;
  key.optout = 0;
  key.test_request = 0;

  config_index->trace_indexing(
    key,
    Generics::Time::ZERO,
    ClientFreqCaps(campaign_config),
    "",
    "",
    ChannelIdSet(),
    CCGIdSet(),
    campaign_config->campaigns.begin()->second,
    campaign_config->tags.begin()->second,
    std::cout);

  std::cout << ">>>>>>>>>>" << std::endl;
  config_index->trace_tree(std::cout);
  std::cout << "==========" << std::endl;
}

*/
using namespace AdServer::CampaignSvcs;

void index_(const char* name, CampaignConfig* campaign_config)
{
  Logging::Logger_var logger(
    new Logging::OStream::Logger(Logging::OStream::Config(std::cout)));

  CampaignIndex_var config_index(
    new AdServer::CampaignSvcs::CampaignIndex(campaign_config, logger));

#ifdef _MEMUSAGE_TRACE_
  std::cerr << name << " : start heap state: " << std::endl;
  ::malloc_stats();
#endif

  Generics::Timer timer;
  timer.start();
  config_index->index_campaigns();
  timer.stop();

/*
  CampaignIndex::Key key(1);
  key.format = "test-appformat";
  key.country = "ru";
  key.colo_id = 1;
  key.optout = 0;
  key.test_request = 0;

  config_index->trace_indexing(
    key,
    Generics::Time::ZERO,
    FreqCapIdSet(),
    "",
    "",
    ChannelIdSet(),
    campaign_config->campaigns.begin()->second,
    campaign_config->tags.begin()->second,
    std::cout);
*/
/*
  std::cout << ">>>>>>>>>>>" << std::endl;
  config_index->trace_margin_tree(std::cout);
  std::cout << "<<<<<<<<<<<" << std::endl;
*/

#ifdef _MEMUSAGE_TRACE_
  std::cerr << name << " : stop heap state: " << std::endl;
  ::malloc_stats();
#endif

  std::cout << name << " : indexing time = " << timer.elapsed_time() << ", " <<
    "index size = " << config_index->size() << std::endl;

  /*
  std::cout << ">>>>>>>>>>" << std::endl;
  config_index->trace_tree(std::cout);
  std::cout << "==========" << std::endl;
  */
}

int main() noexcept
{
/*
  {
    CampaignConfig_var campaign_config(new CampaignConfig());
    fill_test_campaign_config(*campaign_config, 1, 1, 1);
//  index("1x1x1", campaign_config);
    index_("1x1x1:2", campaign_config);
  }

  {
    CampaignConfig_var campaign_config(new CampaignConfig());
    fill_test_campaign_config(*campaign_config, 1, 1, 100);
//  index("1x1x100", campaign_config);
    index_("1x1x100:2", campaign_config);
  }

  {
    CampaignConfig_var campaign_config(new CampaignConfig());
    fill_test_campaign_config(*campaign_config, 1, 100, 100);
//  index("1x100x100", campaign_config);
    index_("1x100x100:2", campaign_config);
  }

  {
    CampaignConfig_var campaign_config(new CampaignConfig());
    fill_test_campaign_config(*campaign_config, 100, 1, 100);
//  index("100x1x100", campaign_config);
    index_("100x1x100:2", campaign_config);
  }

  {
    CampaignConfig_var campaign_config(new CampaignConfig());
    fill_test_campaign_config(*campaign_config, 100, 100, 100);
//  index("100x100x100", campaign_config);
    index_("100x100x100:2", campaign_config);
  }
*/

  int ret = 0;
  ret += Test1::run();

  return ret;
}
