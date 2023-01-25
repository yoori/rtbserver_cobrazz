#include <sstream>
#include <Logger/StreamLogger.hpp>
#include <CampaignSvcs/CampaignServer/CampaignConfigModifier.hpp>

using namespace AdServer::CampaignSvcs;

AccountDef_var create_base_account(unsigned long id)
{
  AccountDef_var p_acc = new AccountDef;
  p_acc->account_id = id;
  p_acc->agency_account_id = 0;
  p_acc->role_id = 1;
  p_acc->internal_account_id = 0;
  p_acc->flags = 0;
  p_acc->at_flags = 0;
  p_acc->text_adserving = 'A';
  p_acc->currency_id = 0;
  p_acc->time_offset = Generics::Time::ZERO;
  p_acc->commision = RevenueDecimal::ZERO;
  p_acc->budget = RevenueDecimal(false, 100, 0);
  p_acc->paid_amount = RevenueDecimal::ZERO;
  p_acc->status = 'A';
  p_acc->eval_status = 'A';

  return p_acc;
}

Campaign_var create_base_campaign(
  unsigned long campaign_id, unsigned long account_id)
{
  Campaign_var campaign = new CampaignDef();

  campaign->campaign_group_id = campaign_id;
  campaign->account_id = account_id;
  campaign->advertiser_id = account_id;
  campaign->fc_id = 0;
  campaign->group_fc_id = 0;
  campaign->flags = 0;
  campaign->imp_revenue = RevenueDecimal::ZERO;
  campaign->click_revenue = RevenueDecimal(false, 1, 0);
  campaign->commision = RevenueDecimal::ZERO;
  campaign->max_pub_share = RevenueDecimal::ZERO;
  campaign->campaign_delivery_limits.budget = OptionalRevenueDecimal();
  campaign->campaign_delivery_limits.daily_budget = OptionalRevenueDecimal();
  campaign->campaign_delivery_limits.delivery_pacing = 'U';
  campaign->ccg_delivery_limits.budget = OptionalRevenueDecimal();
  campaign->ccg_delivery_limits.daily_budget = OptionalRevenueDecimal();
  campaign->ccg_delivery_limits.delivery_pacing = 'U';

  campaign->ccg_rate_id = 0;
  campaign->ccg_rate_type = 'C';

  campaign->status = 'A';
  campaign->eval_status = 'A';
  campaign->ccg_type = CT_DISPLAY;

  campaign->country = "ru";

  return campaign;
}

TagDef_var create_base_tag(unsigned long tag_id)
{
  TagDef_var tag = new TagDef();
  tag->tag_id = tag_id;
  return tag;
}

StatSource::Stat::CCGStat zero_ccg_stat() noexcept
{
  StatSource::Stat::CCGStat res;
  res.amount = RevenueDecimal::ZERO;
  res.comm_amount = RevenueDecimal::ZERO;
  res.daily_amount = RevenueDecimal::ZERO;
  res.daily_comm_amount = RevenueDecimal::ZERO;
  res.impressions = ImpRevenueDecimal::ZERO;
  res.clicks = ImpRevenueDecimal::ZERO;
  res.actions = ImpRevenueDecimal::ZERO;
  res.prev_hour_amount = RevenueDecimal::ZERO;
  res.cur_hour_amount = RevenueDecimal::ZERO;
  res.prev_hour_comm_amount = RevenueDecimal::ZERO;
  res.cur_hour_comm_amount = RevenueDecimal::ZERO;
  return res;
}

class TestModifyConfigSource: public ModifyConfigSource
{
public:
  ModifyConfig_var
  update() noexcept
  {
    ModifyConfig_var modify_config(new ModifyConfig());
    modify_config->countries["ru"] = ModifyConfig::CountryDef();
    return modify_config;
  }

protected:
  virtual ~TestModifyConfigSource() noexcept
  {}
};

class TestStatSource:
  public StatSource,
  public ReferenceCounting::AtomicImpl
{
public:
  TestStatSource() noexcept {}

  virtual Stat_var update(
    Stat* stat,
    bool& /*full_synch_required*/,
    const Generics::Time& now)
    /*throw(Exception)*/
  {
    StatMap::iterator sit = timed_stats_.find(now);

    if(sit != timed_stats_.end())
    {
      if(stat)
      {
//#define PRINT_STATS
#ifdef PRINT_STATS
        std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
        stat->print(std::cout, "");
        std::cout << "+++++++++++++++++++++++++++" << std::endl;
        sit->second->print(std::cout, "");
#endif

        stat->add(*sit->second);
        stat->check_time = now;

#ifdef PRINT_STATS
        std::cout << "===========================" << std::endl;
        stat->print(std::cout, "");
        std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
#endif

        return Stat_var(ReferenceCounting::add_ref(stat));
      }

      return sit->second;
    }

    if(stat)
    {
      stat->check_time = now;
      return Stat_var(ReferenceCounting::add_ref(stat));
    }

    Stat_var empty_stat(new Stat);
    empty_stat->check_time = now;
    return empty_stat;
  }

  void add(const Generics::Time& ts, Stat* stat) noexcept
  {
    Stat_var stat_copy(new Stat(*stat));
    stat_copy->check_time = ts;
    StatMap::value_type el(ts, stat_copy);
    timed_stats_.insert(el);
  }

protected:
  virtual ~TestStatSource() noexcept
  {}

private:
  typedef ReferenceCounting::Map<Generics::Time, Stat_var>
    StatMap;

  StatMap timed_stats_;
};

typedef ReferenceCounting::SmartPtr<TestStatSource> TestStatSource_var;

struct CheckCampaignStatus
{
  unsigned long id;
  char status;
};

bool check_statuses(
  const CampaignConfig& campaign_config,
  const CheckCampaignStatus* etalon_statuses,
  int etalon_statuses_size)
{
  bool res = true;
  for(int i = 0; i < etalon_statuses_size; ++i)
  {
    CampaignMap::ActiveMap::const_iterator cmp_it =
      campaign_config.campaigns.active().find(etalon_statuses[i].id);
    if(cmp_it == campaign_config.campaigns.active().end())
    {
      std::cerr << "Campaign #" << etalon_statuses[i].id << " not found" <<
        std::endl;
      res = false;
    }
    else if(cmp_it->second->eval_status != etalon_statuses[i].status)
    {
      std::cerr << "Campaign #" <<
        etalon_statuses[i].id << " have incorrect status: '" <<
        cmp_it->second->eval_status << "' instead '" <<
        etalon_statuses[i].status << "'" <<
        std::endl;
      res = false;
    }
  }

  return res;
}

bool check_tag_deliveries(
  const char* error_prefix,
  const TagDeliveryMap& etalon_tag_deliveries,
  const TagDeliveryMap& tag_deliveries)
{
  if(etalon_tag_deliveries.size() != tag_deliveries.size() ||
     !std::equal(etalon_tag_deliveries.begin(),
       etalon_tag_deliveries.end(),
       tag_deliveries.begin()))
  {
    std::cerr << error_prefix << ", unexpected tag deliveries: ";
    for(TagDeliveryMap::const_iterator td_it = tag_deliveries.begin();
      td_it != tag_deliveries.end(); ++td_it)
    {
      std::cerr << " (" << td_it->first << ", " << td_it->second << ")";
    }
    std::cerr << "  instead: ";
    for(TagDeliveryMap::const_iterator td_it = etalon_tag_deliveries.begin();
      td_it != etalon_tag_deliveries.end(); ++td_it)
    {
      std::cerr << " (" << td_it->first << ", " << td_it->second << ")";
    }
    std::cerr << std::endl;
    return false;
  }

  return true;
}

int acc_budgets_test()
{
  static const char* TEST_NAME = "acc_budgets_test()";

  int res = 0;

  Generics::Time now(String::SubString("2011-04-02 22:31:30"),
    "%Y-%m-%d %H:%M:%S");

  CampaignConfig_var campaign_config(new CampaignConfig());

  AccountDef_var account1 = create_base_account(1);
  account1->budget = RevenueDecimal(false, 10, 0);
  campaign_config->accounts.activate(1, account1);

  AccountDef_var account2 = create_base_account(2);
  account2->budget = RevenueDecimal(false, 10, 0);
  campaign_config->accounts.activate(2, account2);

  Campaign_var cmp1 = create_base_campaign(1, 1);
  campaign_config->campaigns.activate(1, cmp1);

  Campaign_var cmp2 = create_base_campaign(2, 2);
  campaign_config->campaigns.activate(2, cmp2);

  Logging::Logger_var logger(
    new Logging::OStream::Logger(Logging::OStream::Config(std::cout)));

  TestStatSource_var inc_stat_source(new TestStatSource);

  CampaignConfigModifier_var campaign_config_modifier(
    new CampaignConfigModifier(
      logger,
      StatSource_var(new TestStatSource),
      Generics::Time::ONE_HOUR,
      inc_stat_source,
      ModifyConfigSource_var(new TestModifyConfigSource()),
      true // enable delivery thresholds
      ));

  {
    StatSource::Stat_var stat_1(new StatSource::Stat());

    StatSource::Stat::Amount acc1_stat;
    acc1_stat.amount = RevenueDecimal("3.4");
    acc1_stat.comm_amount = RevenueDecimal("0.0");
    stat_1->account_amounts.insert(std::make_pair(1, acc1_stat));

    StatSource::Stat::Amount acc2_stat;
    acc2_stat.amount = RevenueDecimal("2");
    acc2_stat.comm_amount = RevenueDecimal("1.4");
    stat_1->account_amounts.insert(std::make_pair(2, acc2_stat));

    inc_stat_source->add(now, stat_1);
    inc_stat_source->add(now + Generics::Time::ONE_DAY, stat_1);
    inc_stat_source->add(now + Generics::Time::ONE_DAY + 1, stat_1);
  }

  campaign_config_modifier->update(campaign_config, now);

  {
    const CheckCampaignStatus CHECK[] = { { 1, 'A' }, { 2, 'A' } };

    if(!check_statuses(*campaign_config,
         CHECK, sizeof(CHECK) / sizeof(CHECK[0])))
    {
      std::cerr << TEST_NAME << ": step #1 failed" << std::endl;
      ++res;
    }
  }

  campaign_config_modifier->update(campaign_config, now + Generics::Time::ONE_DAY);

  {
    const CheckCampaignStatus CHECK[] = { { 1, 'A' }, { 2, 'A' } };

    if(!check_statuses(*campaign_config,
         CHECK, sizeof(CHECK) / sizeof(CHECK[0])))
    {
      std::cerr << TEST_NAME << ": step #2 failed" << std::endl;
      ++res;
    }
  }

  campaign_config_modifier->update(campaign_config, now + Generics::Time::ONE_DAY + 1);

  {
    const CheckCampaignStatus CHECK[] = { { 1, 'I' }, { 2, 'I' } };

    if(!check_statuses(*campaign_config,
         CHECK, sizeof(CHECK) / sizeof(CHECK[0])))
    {
      std::cerr << TEST_NAME << ": step #3 failed" << std::endl;
      ++res;
    }
  }

  return res;
}

int simple_cmp_budgets_test()
{
  static const char* TEST_NAME = "simple_cmp_budgets_test()";

  int res = 0;

  CampaignConfig_var campaign_config(new CampaignConfig());

  campaign_config->accounts.activate(1, create_base_account(1));

  Campaign_var cmp1 = create_base_campaign(1, 1);
  cmp1->campaign_delivery_limits.budget = RevenueDecimal(false, 5, 0);
  cmp1->campaign_delivery_limits.daily_budget = OptionalRevenueDecimal();
  cmp1->campaign_delivery_limits.delivery_pacing = 'U';
  campaign_config->campaigns.activate(1, cmp1);

  Campaign_var cmp2 = create_base_campaign(2, 1);
  cmp2->campaign_delivery_limits.budget = OptionalRevenueDecimal();
  cmp2->campaign_delivery_limits.daily_budget = RevenueDecimal(false, 10, 0);
  cmp2->campaign_delivery_limits.delivery_pacing = 'F';
  campaign_config->campaigns.activate(2, cmp2);

  Logging::Logger_var logger(new Logging::OStream::Logger(
    Logging::OStream::Config(std::cout/*, Logging::Logger::TRACE*/)));

  TestStatSource_var inc_stat_source(new TestStatSource);

  CampaignConfigModifier_var campaign_config_modifier(
    new CampaignConfigModifier(
      logger,
      StatSource_var(new TestStatSource),
      Generics::Time::ONE_HOUR,
      inc_stat_source,
      ModifyConfigSource_var(new TestModifyConfigSource()),
      true // enable delivery thresholds
      ));

  {
    StatSource::Stat_var stat_1(new StatSource::Stat());
    StatSource::Stat::CampaignStat cmp1_stat;
    cmp1_stat.amount = RevenueDecimal(false, 10, 0);
    cmp1_stat.comm_amount = RevenueDecimal::ZERO;
    cmp1_stat.daily_amount = RevenueDecimal(false, 1, 0);
    cmp1_stat.daily_comm_amount = RevenueDecimal::ZERO;
    stat_1->campaign_stats.insert(std::make_pair(1, cmp1_stat));
    inc_stat_source->add(Generics::Time(1), stat_1);

    StatSource::Stat_var stat_2(new StatSource::Stat());
    StatSource::Stat::CampaignStat cmp2_stat;
    cmp2_stat.amount = RevenueDecimal(false, 10, 0);
    cmp2_stat.comm_amount = RevenueDecimal::ZERO;
    cmp2_stat.daily_amount = RevenueDecimal(false, 10, 0);
    cmp2_stat.daily_comm_amount = RevenueDecimal::ZERO;
    stat_2->campaign_stats.insert(std::make_pair(2, cmp2_stat));
    inc_stat_source->add(Generics::Time(2), stat_2);
  }

  campaign_config_modifier->update(campaign_config, Generics::Time(1));

  {
    const CheckCampaignStatus CHECK[] = { { 1, 'I' }, { 2, 'A' } };

    if(!check_statuses(
         *campaign_config, CHECK, sizeof(CHECK) / sizeof(CHECK[0])))
    {
      std::cerr << TEST_NAME << ": step #1 failed" << std::endl;
      ++res;
    }
  }

  campaign_config_modifier->update(campaign_config, Generics::Time(2));

  {
    const CheckCampaignStatus CHECK[] = { { 1, 'I' }, { 2, 'I' } };

    if(!check_statuses(
         *campaign_config, CHECK, sizeof(CHECK) / sizeof(CHECK[0])))
    {
      std::cerr << TEST_NAME << ": step #2 failed" << std::endl;
      ++res;
    }
  }

  return res;
}

int dynamic_cmp_budgets_test()
{
  static const char* TEST_NAME = "dynamic_cmp_budgets_test()";

  int res = 0;

  CampaignConfig_var campaign_config(new CampaignConfig());

  campaign_config->accounts.activate(
    1,
    create_base_account(1));

  Generics::Time now(String::SubString("2011-04-02 22:31:30"),
    "%Y-%m-%d %H:%M:%S");
  Generics::Time date_end(String::SubString("2011-04-07 22:31:30"),
    "%Y-%m-%d %H:%M:%S");

  Campaign_var cmp1 = create_base_campaign(1, 1);
  cmp1->campaign_delivery_limits.budget = RevenueDecimal(false, 11, 0);
  cmp1->campaign_delivery_limits.daily_budget = OptionalRevenueDecimal();
  cmp1->campaign_delivery_limits.date_end = date_end;
  cmp1->campaign_delivery_limits.delivery_pacing = 'D';
  campaign_config->campaigns.activate(1, cmp1);

  Logging::Logger_var logger(
    new Logging::OStream::Logger(Logging::OStream::Config(std::cout)));

  TestStatSource_var inc_stat_source(new TestStatSource);

  CampaignConfigModifier_var campaign_config_modifier(
    new CampaignConfigModifier(
      logger,
      StatSource_var(new TestStatSource),
      Generics::Time::ONE_HOUR,
      inc_stat_source,
      ModifyConfigSource_var(new TestModifyConfigSource()),
      true // enable delivery thresholds
      ));

  {
    StatSource::Stat_var stat_1(new StatSource::Stat());
    StatSource::Stat::CampaignStat cmp1_stat;
    cmp1_stat.amount = RevenueDecimal("1.8");
    cmp1_stat.comm_amount = RevenueDecimal::ZERO;
    cmp1_stat.daily_amount = RevenueDecimal("1.8");
    cmp1_stat.daily_comm_amount = RevenueDecimal::ZERO;
    stat_1->campaign_stats.insert(std::make_pair(1, cmp1_stat));

    StatSource::Stat_var stat_2(new StatSource::Stat());
    StatSource::Stat::CampaignStat cmp2_stat;
    cmp2_stat.amount = RevenueDecimal("0.04");
    cmp2_stat.comm_amount = RevenueDecimal::ZERO;
    cmp2_stat.daily_amount = RevenueDecimal("0.04");
    cmp2_stat.daily_comm_amount = RevenueDecimal::ZERO;
    stat_2->campaign_stats.insert(std::make_pair(1, cmp2_stat));

    inc_stat_source->add(now, stat_1);
    inc_stat_source->add(now + 1, stat_2);
    inc_stat_source->add(now + Generics::Time::ONE_DAY, stat_1);
    inc_stat_source->add(now + Generics::Time::ONE_DAY + 1, stat_2);
  }

  campaign_config_modifier->update(campaign_config, now);

  {
    const CheckCampaignStatus CHECK[] = { { 1, 'A' } };

    if(!check_statuses(*campaign_config,
         CHECK, sizeof(CHECK) / sizeof(CHECK[0])))
    {
      std::cerr << TEST_NAME << ": step #1 failed" << std::endl;
      ++res;
    }
  }

  campaign_config_modifier->update(campaign_config, now + 1);

  {
    const CheckCampaignStatus CHECK[] = { { 1, 'I' } };

    if(!check_statuses(*campaign_config,
         CHECK, sizeof(CHECK) / sizeof(CHECK[0])))
    {
      std::cerr << TEST_NAME << ": step #2 failed" << std::endl;
      ++res;
    }
  }

  campaign_config_modifier->update(campaign_config, now + Generics::Time::ONE_DAY);

  {
    const CheckCampaignStatus CHECK[] = { { 1, 'A' } };

    if(!check_statuses(*campaign_config,
         CHECK, sizeof(CHECK) / sizeof(CHECK[0])))
    {
      std::cerr << TEST_NAME << ": step #3 failed" << std::endl;
      ++res;
    }
  }

  campaign_config_modifier->update(campaign_config, now + Generics::Time::ONE_DAY + 1);

  {
    const CheckCampaignStatus CHECK[] = { { 1, 'I' } };

    if(!check_statuses(*campaign_config,
         CHECK, sizeof(CHECK) / sizeof(CHECK[0])))
    {
      std::cerr << TEST_NAME << ": step #4 failed" << std::endl;
      ++res;
    }
  }

  return res;
}

int simple_ccg_budgets_test()
{
  static const char* TEST_NAME = "simple_ccg_budgets_test()";

  int res = 0;

  Generics::Time now(String::SubString("2011-04-02 22:31:30"),
    "%Y-%m-%d %H:%M:%S");

  CampaignConfig_var campaign_config(new CampaignConfig());

  campaign_config->accounts.activate(1, create_base_account(1));

  Campaign_var cmp1 = create_base_campaign(1, 1);
  cmp1->ccg_delivery_limits.budget = RevenueDecimal(false, 10, 0);
  cmp1->ccg_delivery_limits.daily_budget = OptionalRevenueDecimal();
  cmp1->ccg_delivery_limits.delivery_pacing = 'U';
  campaign_config->campaigns.activate(1, cmp1);

  Campaign_var cmp2 = create_base_campaign(2, 1);
  cmp2->ccg_delivery_limits.budget = OptionalRevenueDecimal();
  cmp2->ccg_delivery_limits.daily_budget = RevenueDecimal(false, 5, 0);
  cmp2->ccg_delivery_limits.delivery_pacing = 'F';
  campaign_config->campaigns.activate(2, cmp2);

  Logging::Logger_var logger(new Logging::OStream::Logger(
    Logging::OStream::Config(std::cout/*, Logging::Logger::TRACE*/)));

  TestStatSource_var inc_stat_source(new TestStatSource);

  CampaignConfigModifier_var campaign_config_modifier(
    new CampaignConfigModifier(
      logger,
      StatSource_var(new TestStatSource),
      Generics::Time::ONE_HOUR,
      inc_stat_source,
      ModifyConfigSource_var(new TestModifyConfigSource()),
      true // enable delivery thresholds
      ));

  {
    StatSource::Stat_var stat(new StatSource::Stat());
    StatSource::Stat::CCGStat ccg_stat = zero_ccg_stat();
    ccg_stat.amount = RevenueDecimal(false, 3, 0);
    ccg_stat.comm_amount = RevenueDecimal::ZERO;
    ccg_stat.daily_amount = RevenueDecimal(false, 3, 0);
    ccg_stat.daily_comm_amount = RevenueDecimal::ZERO;

    StatSource::Stat::CampaignStat cmp_stat1;
    cmp_stat1.ccgs.insert(std::make_pair(1, ccg_stat));
    stat->campaign_stats.insert(std::make_pair(1, cmp_stat1));

    StatSource::Stat::CampaignStat cmp_stat2;
    cmp_stat2.ccgs.insert(std::make_pair(2, ccg_stat));
    stat->campaign_stats.insert(std::make_pair(2, cmp_stat2));

    inc_stat_source->add(now, stat);
    inc_stat_source->add(now + 1, stat);
    inc_stat_source->add(now + Generics::Time::ONE_DAY, stat);
    inc_stat_source->add(now + Generics::Time::ONE_DAY + 1, stat);
  }

  campaign_config_modifier->update(campaign_config, now);

  {
    const CheckCampaignStatus CHECK[] = { { 1, 'A' }, { 2, 'A' } };

    if(!check_statuses(*campaign_config,
         CHECK, sizeof(CHECK) / sizeof(CHECK[0])))
    {
      std::cerr << TEST_NAME << ": step #1 failed" << std::endl;
      ++res;
    }
  }

  campaign_config_modifier->update(campaign_config, now + 1);

  {
    const CheckCampaignStatus CHECK[] = { { 1, 'A' }, { 2, 'I' } };

    if(!check_statuses(*campaign_config,
         CHECK, sizeof(CHECK) / sizeof(CHECK[0])))
    {
      std::cerr << TEST_NAME << ": step #2 failed" << std::endl;
      ++res;
    }
  }

  campaign_config_modifier->update(campaign_config, now + Generics::Time::ONE_DAY);

  {
    const CheckCampaignStatus CHECK[] = { { 1, 'A' }, { 2, 'A' } };

    if(!check_statuses(*campaign_config,
         CHECK, sizeof(CHECK) / sizeof(CHECK[0])))
    {
      std::cerr << TEST_NAME << ": step #3 failed" << std::endl;
      ++res;
    }
  }

  campaign_config_modifier->update(campaign_config, now + Generics::Time::ONE_DAY + 1);

  {
    const CheckCampaignStatus CHECK[] = { { 1, 'I' }, { 2, 'I' } };

    if(!check_statuses(*campaign_config,
         CHECK, sizeof(CHECK) / sizeof(CHECK[0])))
    {
      std::cerr << TEST_NAME << ": step #4 failed" << std::endl;
      ++res;
    }
  }

  return res;
}

int ccg_delivery_tags_test()
{
  static const char* TEST_NAME = "ccg_delivery_tags_test()";

  static const RevenueDecimal UNUSED_COMM_AMOUNT(false, 1, 0);

  int res = 0;

  Generics::Time now(String::SubString("2011-04-02 22:30:00"),
    "%Y-%m-%d %H:%M:%S");

  CampaignConfig_var campaign_config(new CampaignConfig());
  campaign_config->master_stamp = now;

  TagDef_var tag2 = create_base_tag(2);
  campaign_config->accounts.activate(1, create_base_account(1));
  campaign_config->tags.activate(2, tag2);

  Campaign_var cmp1 = create_base_campaign(1, 1);
  cmp1->ccg_delivery_limits.budget = OptionalRevenueDecimal();
  cmp1->ccg_delivery_limits.daily_budget = RevenueDecimal(false, 12 * 24, 0);
  cmp1->ccg_delivery_limits.delivery_pacing = 'F';
  campaign_config->campaigns.activate(1, cmp1);

  Logging::Logger_var logger(new Logging::OStream::Logger(
    Logging::OStream::Config(std::cout/*, Logging::Logger::TRACE*/)));

  TestStatSource_var inc_stat_source(new TestStatSource);

  CampaignConfigModifier_var campaign_config_modifier(
    new CampaignConfigModifier(
      logger,
      StatSource_var(new TestStatSource),
      Generics::Time::ONE_HOUR,
      inc_stat_source,
      ModifyConfigSource_var(new TestModifyConfigSource()),
      true // enable delivery thresholds
      ));

  {
    StatSource::Stat_var stat_1(new StatSource::Stat());
    StatSource::Stat::CCGStat ccg1_stat;
    ccg1_stat.amount = RevenueDecimal(false, 24, 0);
    ccg1_stat.comm_amount = RevenueDecimal::ZERO;
    ccg1_stat.daily_amount = RevenueDecimal(false, 24, 0);
    ccg1_stat.daily_comm_amount = RevenueDecimal::ZERO;
    ccg1_stat.prev_hour_amount = RevenueDecimal(false, 12, 0);
    ccg1_stat.prev_hour_comm_amount = RevenueDecimal::ZERO;
    ccg1_stat.cur_hour_amount = RevenueDecimal(false, 12, 0);
    ccg1_stat.cur_hour_comm_amount = RevenueDecimal::ZERO;
    StatSource::Stat::CCGStat::TagStat tag1_stat;
    tag1_stat.prev_hour_stat.isp_pub_amount = RevenueDecimal(false, 1, 0);
    tag1_stat.prev_hour_stat.adv_amount = RevenueDecimal(false, 4, 0);
    tag1_stat.prev_hour_stat.adv_comm_amount = UNUSED_COMM_AMOUNT;
    tag1_stat.current_hour_stat.isp_pub_amount = RevenueDecimal(false, 1, 0);
    tag1_stat.current_hour_stat.adv_amount = RevenueDecimal(false, 4, 0);
    tag1_stat.current_hour_stat.adv_comm_amount = UNUSED_COMM_AMOUNT;
    ccg1_stat.tag_stats.insert(std::make_pair(1, tag1_stat));
    StatSource::Stat::CCGStat::TagStat tag2_stat;
    tag2_stat.prev_hour_stat.isp_pub_amount = RevenueDecimal("2");
    tag2_stat.prev_hour_stat.adv_amount = RevenueDecimal(false, 4, 0);
    tag2_stat.prev_hour_stat.adv_comm_amount = UNUSED_COMM_AMOUNT;
    tag2_stat.current_hour_stat.isp_pub_amount = RevenueDecimal("2");
    tag2_stat.current_hour_stat.adv_amount = RevenueDecimal(false, 4, 0);
    tag2_stat.current_hour_stat.adv_comm_amount = UNUSED_COMM_AMOUNT;
    ccg1_stat.tag_stats.insert(std::make_pair(2, tag2_stat));
    StatSource::Stat::CCGStat::TagStat tag3_stat;
    tag3_stat.prev_hour_stat.isp_pub_amount = RevenueDecimal("3");
    tag3_stat.prev_hour_stat.adv_amount = RevenueDecimal(false, 4, 0);
    tag3_stat.prev_hour_stat.adv_comm_amount = UNUSED_COMM_AMOUNT;
    tag3_stat.current_hour_stat.isp_pub_amount = RevenueDecimal("3");
    tag3_stat.current_hour_stat.adv_amount = RevenueDecimal(false, 4, 0);
    tag3_stat.current_hour_stat.adv_comm_amount = UNUSED_COMM_AMOUNT;
    ccg1_stat.tag_stats.insert(std::make_pair(3, tag3_stat));

    StatSource::Stat::CampaignStat cmp_stat;
    cmp_stat.ccgs.insert(std::make_pair(1, ccg1_stat));
    stat_1->campaign_stats.insert(std::make_pair(1, cmp_stat));

    inc_stat_source->add(now, stat_1);
  }

  campaign_config_modifier->update(campaign_config, now);

  {
    CampaignMap::ActiveMap::const_iterator cmp_it =
      campaign_config->campaigns.active().find(1);
    if(cmp_it == campaign_config->campaigns.active().end())
    {
      std::cerr << TEST_NAME << ": Campaign #1 not found" << std::endl;
      res = false;
    }
    else
    {
      TagDeliveryMap etalon_tag_deliveries;
      etalon_tag_deliveries.insert(std::make_pair(2, 5000));
      etalon_tag_deliveries.insert(std::make_pair(3, 0));

      res &= check_tag_deliveries(
        (std::string(TEST_NAME) + ": step #1").c_str(),
        etalon_tag_deliveries,
        cmp_it->second->exclude_tags);
    }
  }

  {
    // 3 excluded, 2 excluded for 50%
    StatSource::Stat_var stat_1(new StatSource::Stat());
    StatSource::Stat::CCGStat ccg1_stat;
    ccg1_stat.amount = RevenueDecimal(false, 6, 0);
    ccg1_stat.comm_amount = RevenueDecimal::ZERO;
    ccg1_stat.daily_amount = RevenueDecimal(false, 6, 0);
    ccg1_stat.daily_comm_amount = RevenueDecimal::ZERO;
    ccg1_stat.prev_hour_amount = RevenueDecimal::ZERO;
    ccg1_stat.prev_hour_comm_amount = RevenueDecimal::ZERO;
    ccg1_stat.cur_hour_amount = RevenueDecimal(false, 6, 0);
    ccg1_stat.cur_hour_comm_amount = RevenueDecimal::ZERO;
    StatSource::Stat::CCGStat::TagStat tag1_stat;
    tag1_stat.current_hour_stat.isp_pub_amount = RevenueDecimal(false, 1, 0);
    tag1_stat.current_hour_stat.adv_amount = RevenueDecimal(false, 4, 0);
    tag1_stat.current_hour_stat.adv_comm_amount = UNUSED_COMM_AMOUNT;
    ccg1_stat.tag_stats.insert(std::make_pair(1, tag1_stat));
    StatSource::Stat::CCGStat::TagStat tag2_stat;
    // amount source must divide amounts to delivery threshold
    tag2_stat.current_hour_stat.isp_pub_amount = RevenueDecimal("2");
    tag2_stat.current_hour_stat.adv_amount = RevenueDecimal(false, 4, 0);
    tag2_stat.current_hour_stat.adv_comm_amount = UNUSED_COMM_AMOUNT;
    ccg1_stat.tag_stats.insert(std::make_pair(2, tag2_stat));

    StatSource::Stat::CampaignStat cmp_stat;
    cmp_stat.ccgs.insert(std::make_pair(1, ccg1_stat));
    stat_1->campaign_stats.insert(std::make_pair(1, cmp_stat));

    inc_stat_source->add(now + Generics::Time::ONE_HOUR, stat_1);
  }

  campaign_config_modifier->update(
    campaign_config, now + Generics::Time::ONE_HOUR);

  {
    // no delivery changes
    CampaignMap::ActiveMap::const_iterator cmp_it =
      campaign_config->campaigns.active().find(1);
    if(cmp_it == campaign_config->campaigns.active().end())
    {
      std::cerr << TEST_NAME << ": Campaign #1 not found" <<
        std::endl;
      res = false;
    }
    else
    {
      TagDeliveryMap etalon_tag_deliveries;
      etalon_tag_deliveries.insert(std::make_pair(2, 5000));
      etalon_tag_deliveries.insert(std::make_pair(3, 0));

      res &= check_tag_deliveries(
        (std::string(TEST_NAME) + ": step #2").c_str(),
        etalon_tag_deliveries,
        cmp_it->second->exclude_tags);
    }
  }

  {
    // under delivering: 3 excluded, 2 excluded for 50% ->
    StatSource::Stat_var stat_1(new StatSource::Stat());
    StatSource::Stat::CCGStat ccg1_stat;
    ccg1_stat.amount = RevenueDecimal(false, 6, 0);
    ccg1_stat.comm_amount = RevenueDecimal::ZERO;
    ccg1_stat.daily_amount = RevenueDecimal(false, 6, 0);
    ccg1_stat.daily_comm_amount = RevenueDecimal::ZERO;
    ccg1_stat.prev_hour_amount = RevenueDecimal::ZERO;
    ccg1_stat.prev_hour_comm_amount = RevenueDecimal::ZERO;
    ccg1_stat.cur_hour_amount = RevenueDecimal(false, 5, 0);
    ccg1_stat.cur_hour_comm_amount = RevenueDecimal::ZERO;

    StatSource::Stat::CCGStat::TagStat tag1_stat;
    tag1_stat.current_hour_stat.isp_pub_amount = RevenueDecimal(false, 1, 0);
    tag1_stat.current_hour_stat.adv_amount = RevenueDecimal(false, 4, 0);
    tag1_stat.current_hour_stat.adv_comm_amount = UNUSED_COMM_AMOUNT;
    ccg1_stat.tag_stats.insert(std::make_pair(1, tag1_stat));

    StatSource::Stat::CCGStat::TagStat tag2_stat;
    tag2_stat.current_hour_stat.isp_pub_amount = RevenueDecimal("1");
    tag2_stat.current_hour_stat.adv_amount = RevenueDecimal(false, 2, 0); // 1 amount / 0.5 delivery
    tag2_stat.current_hour_stat.adv_comm_amount = UNUSED_COMM_AMOUNT;
    ccg1_stat.tag_stats.insert(std::make_pair(2, tag2_stat));

    StatSource::Stat::CampaignStat cmp_stat;
    cmp_stat.ccgs.insert(std::make_pair(1, ccg1_stat));
    stat_1->campaign_stats.insert(std::make_pair(1, cmp_stat));

    inc_stat_source->add(now + Generics::Time::ONE_HOUR * 2, stat_1);
  }

  campaign_config_modifier->update(
    campaign_config, now + Generics::Time::ONE_HOUR * 2);

  {
    CampaignMap::ActiveMap::const_iterator cmp_it =
      campaign_config->campaigns.active().find(1);
    if(cmp_it == campaign_config->campaigns.active().end())
    {
      std::cerr << TEST_NAME << ": Campaign #1 not found" <<
        std::endl;
      res = false;
    }
    else
    {
      TagDeliveryMap etalon_tag_deliveries;
      etalon_tag_deliveries.insert(std::make_pair(3, 0));

      res &= check_tag_deliveries(
        (std::string(TEST_NAME) + ": step #3").c_str(),
        etalon_tag_deliveries,
        cmp_it->second->exclude_tags);
    }
  }

  // tag #2 must be reallowed - tag pricings changed (
  //   up previous state of #3 - before full exclusion)
  campaign_config->master_stamp = now + Generics::Time::ONE_HOUR * 2 + 1;
  tag2->tag_pricings_timestamp = campaign_config->master_stamp;

  campaign_config_modifier->update(
    campaign_config, now + Generics::Time::ONE_HOUR * 2 + 1);

  {
    CampaignMap::ActiveMap::const_iterator cmp_it =
      campaign_config->campaigns.active().find(1);
    if(cmp_it == campaign_config->campaigns.active().end())
    {
      std::cerr << TEST_NAME << ": Campaign #1 not found" <<
        std::endl;
      res = false;
    }
    else
    {
      TagDeliveryMap etalon_tag_deliveries;
      etalon_tag_deliveries.insert(std::make_pair(3, 5005));

      res &= check_tag_deliveries(
        (std::string(TEST_NAME) + ": step #4").c_str(),
        etalon_tag_deliveries,
        cmp_it->second->exclude_tags);
    }
  }

  return res;
}

int one_hour_dynamic_test()
{
  static const char* TEST_NAME = "one_hour_dynamic_test()";

  static const RevenueDecimal UNUSED_COMM_AMOUNT(false, 1, 0);

  int res = 0;

  const Generics::Time now(String::SubString("2011-04-02 22:03:00"),
    "%Y-%m-%d %H:%M:%S");
  const Generics::Time STEP2_TIME(String::SubString("2011-04-02 22:32:00"),
    "%Y-%m-%d %H:%M:%S");
  const Generics::Time STEP3_TIME(String::SubString("2011-04-02 22:50:00"),
    "%Y-%m-%d %H:%M:%S");

  CampaignConfig_var campaign_config(new CampaignConfig());
  campaign_config->master_stamp = now;

  TagDef_var tag = create_base_tag(2);
  campaign_config->accounts.activate(1, create_base_account(1));
  campaign_config->tags.activate(2, tag);

  Campaign_var cmp1 = create_base_campaign(1, 1);
  cmp1->ccg_delivery_limits.budget = OptionalRevenueDecimal();
  // 30 min budget = 6
  cmp1->ccg_delivery_limits.daily_budget = RevenueDecimal(false, 12 * 24, 0);
  cmp1->ccg_delivery_limits.delivery_pacing = 'F';
  campaign_config->campaigns.activate(1, cmp1);

  Logging::Logger_var logger(new Logging::OStream::Logger(
    Logging::OStream::Config(std::cout/*, Logging::Logger::TRACE*/)));

  TestStatSource_var inc_stat_source(new TestStatSource);

  CampaignConfigModifier_var campaign_config_modifier(
    new CampaignConfigModifier(
      logger,
      StatSource_var(new TestStatSource),
      Generics::Time::ONE_HOUR,
      inc_stat_source,
      ModifyConfigSource_var(new TestModifyConfigSource()),
      true // enable delivery thresholds
      ));

  {
    StatSource::Stat_var stat_1(new StatSource::Stat());
    StatSource::Stat::CCGStat ccg1_stat;
    ccg1_stat.amount = RevenueDecimal(false, 8, 0); // 8/6 - over delivering
    ccg1_stat.comm_amount = RevenueDecimal::ZERO;
    ccg1_stat.daily_amount = RevenueDecimal(false, 8, 0);
    ccg1_stat.daily_comm_amount = RevenueDecimal::ZERO;
    ccg1_stat.prev_hour_amount = RevenueDecimal(false, 8, 0);
    ccg1_stat.prev_hour_comm_amount = RevenueDecimal::ZERO;
    ccg1_stat.cur_hour_amount = RevenueDecimal(false, 8, 0);
    ccg1_stat.cur_hour_comm_amount = RevenueDecimal::ZERO;
    StatSource::Stat::CCGStat::TagStat tag1_stat;
    tag1_stat.prev_hour_stat.isp_pub_amount = RevenueDecimal(false, 1, 0);
    tag1_stat.prev_hour_stat.adv_amount = RevenueDecimal(false, 4, 0); // 4/6
    tag1_stat.prev_hour_stat.adv_comm_amount = UNUSED_COMM_AMOUNT;
    tag1_stat.current_hour_stat.isp_pub_amount = RevenueDecimal(false, 1, 0);
    tag1_stat.current_hour_stat.adv_amount = RevenueDecimal(false, 4, 0);
    tag1_stat.current_hour_stat.adv_comm_amount = UNUSED_COMM_AMOUNT;
    ccg1_stat.tag_stats.insert(std::make_pair(1, tag1_stat));
    StatSource::Stat::CCGStat::TagStat tag2_stat;
    tag2_stat.prev_hour_stat.isp_pub_amount = RevenueDecimal("2");
    tag2_stat.prev_hour_stat.adv_amount = RevenueDecimal(false, 4, 0); // 4/6
    tag2_stat.prev_hour_stat.adv_comm_amount = UNUSED_COMM_AMOUNT;
    tag2_stat.current_hour_stat.isp_pub_amount = RevenueDecimal("2");
    tag2_stat.current_hour_stat.adv_amount = RevenueDecimal(false, 4, 0);
    tag2_stat.current_hour_stat.adv_comm_amount = UNUSED_COMM_AMOUNT;
    ccg1_stat.tag_stats.insert(std::make_pair(2, tag2_stat));

    StatSource::Stat::CampaignStat cmp_stat;
    cmp_stat.ccgs.insert(std::make_pair(1, ccg1_stat));
    stat_1->campaign_stats.insert(std::make_pair(1, cmp_stat));

    inc_stat_source->add(now, stat_1);
  }

  campaign_config_modifier->update(campaign_config, now);

  {
    CampaignMap::ActiveMap::const_iterator cmp_it =
      campaign_config->campaigns.active().find(1);
    if(cmp_it == campaign_config->campaigns.active().end())
    {
      std::cerr << TEST_NAME << ": Campaign #1 not found" << std::endl;
      res = false;
    }
    else
    {
      TagDeliveryMap etalon_tag_deliveries;
      etalon_tag_deliveries.insert(std::make_pair(2, 344));

      res &= check_tag_deliveries(
        (std::string(TEST_NAME) + ": step #1").c_str(),
        etalon_tag_deliveries,
        cmp_it->second->exclude_tags);
    }
  }

  // small delivery changes
  inc_stat_source->add(STEP2_TIME, StatSource::Stat_var(new StatSource::Stat()));
  campaign_config_modifier->update(campaign_config, STEP2_TIME);

  {
    CampaignMap::ActiveMap::const_iterator cmp_it =
      campaign_config->campaigns.active().find(1);
    if(cmp_it == campaign_config->campaigns.active().end())
    {
      std::cerr << TEST_NAME << ": Campaign #1 not found" <<
        std::endl;
      res = false;
    }
    else
    {
      TagDeliveryMap etalon_tag_deliveries;
      etalon_tag_deliveries.insert(std::make_pair(2, 6000));

      res &= check_tag_deliveries(
        (std::string(TEST_NAME) + ": step #2").c_str(),
        etalon_tag_deliveries,
        cmp_it->second->exclude_tags);
    }
  }

  // big delivery changes
  inc_stat_source->add(STEP3_TIME, StatSource::Stat_var(new StatSource::Stat()));
  campaign_config_modifier->update(campaign_config, STEP3_TIME);

  {
    CampaignMap::ActiveMap::const_iterator cmp_it =
      campaign_config->campaigns.active().find(1);
    if(cmp_it == campaign_config->campaigns.active().end())
    {
      std::cerr << TEST_NAME << ": Campaign #1 not found" <<
        std::endl;
      res = false;
    }
    else
    {
      TagDeliveryMap etalon_tag_deliveries;

      res &= check_tag_deliveries(
        (std::string(TEST_NAME) + ": step #3").c_str(),
        etalon_tag_deliveries,
        cmp_it->second->exclude_tags);
    }
  }

  return res;
}

int zero_adv_amount_test()
{
  // regression for ADSC-5441
  static const char* TEST_NAME = "zero_adv_amount_test()";

  static const RevenueDecimal UNUSED_COMM_AMOUNT(false, 1, 0);

  int res = 0;

  const Generics::Time now(String::SubString("2011-04-02 22:32:00"),
    "%Y-%m-%d %H:%M:%S");

  CampaignConfig_var campaign_config(new CampaignConfig());
  campaign_config->master_stamp = now;

  campaign_config->accounts.activate(1, create_base_account(1));

  TagDef_var tag1 = create_base_tag(1);
  campaign_config->tags.activate(1, tag1);

  TagDef_var tag2 = create_base_tag(2);
  campaign_config->tags.activate(2, tag2);

  Campaign_var cmp1 = create_base_campaign(1, 1);
  cmp1->ccg_delivery_limits.budget = OptionalRevenueDecimal();
  // 30 min budget = 6
  cmp1->ccg_delivery_limits.daily_budget = RevenueDecimal(false, 12 * 24, 0);
  cmp1->ccg_delivery_limits.delivery_pacing = 'F';
  campaign_config->campaigns.activate(1, cmp1);

  Logging::Logger_var logger(new Logging::OStream::Logger(
    Logging::OStream::Config(std::cout/*, Logging::Logger::TRACE*/)));

  TestStatSource_var inc_stat_source(new TestStatSource);

  CampaignConfigModifier_var campaign_config_modifier(
    new CampaignConfigModifier(
      logger,
      StatSource_var(new TestStatSource),
      Generics::Time::ONE_HOUR,
      inc_stat_source,
      ModifyConfigSource_var(new TestModifyConfigSource()),
      true // enable delivery thresholds
      ));

  {
    StatSource::Stat_var stat_1(new StatSource::Stat());
    StatSource::Stat::CCGStat ccg1_stat;

    RevenueDecimal add_adv_amount(false, 8, 0);

    ccg1_stat.amount = add_adv_amount;
    ccg1_stat.comm_amount = RevenueDecimal::ZERO;
    ccg1_stat.daily_amount = add_adv_amount;
    ccg1_stat.daily_comm_amount = RevenueDecimal::ZERO;
    ccg1_stat.prev_hour_amount = add_adv_amount;
    ccg1_stat.prev_hour_comm_amount = RevenueDecimal::ZERO;
    ccg1_stat.cur_hour_amount = add_adv_amount;
    ccg1_stat.cur_hour_comm_amount = RevenueDecimal::ZERO;

    StatSource::Stat::CCGStat::TagStat tag1_stat;
    tag1_stat.prev_hour_stat.isp_pub_amount = RevenueDecimal(false, 1, 0);
    tag1_stat.prev_hour_stat.adv_amount = add_adv_amount;
    tag1_stat.prev_hour_stat.adv_comm_amount = RevenueDecimal::ZERO;
    tag1_stat.current_hour_stat.isp_pub_amount = RevenueDecimal(false, 1, 0);
    tag1_stat.current_hour_stat.adv_amount = add_adv_amount;
    tag1_stat.current_hour_stat.adv_comm_amount = RevenueDecimal::ZERO;
    ccg1_stat.tag_stats.insert(std::make_pair(1, tag1_stat));

    StatSource::Stat::CCGStat::TagStat tag2_stat;
    tag2_stat.prev_hour_stat.isp_pub_amount = RevenueDecimal(false, 2, 0);
    tag2_stat.prev_hour_stat.adv_amount = RevenueDecimal::ZERO;
    tag2_stat.prev_hour_stat.adv_comm_amount = RevenueDecimal::ZERO;
    tag2_stat.current_hour_stat.isp_pub_amount = RevenueDecimal(false, 2, 0);
    tag2_stat.current_hour_stat.adv_amount = RevenueDecimal::ZERO;
    tag2_stat.current_hour_stat.adv_comm_amount = RevenueDecimal::ZERO;
    ccg1_stat.tag_stats.insert(std::make_pair(2, tag2_stat));

    StatSource::Stat::CampaignStat cmp_stat;
    cmp_stat.ccgs.insert(std::make_pair(1, ccg1_stat));
    stat_1->campaign_stats.insert(std::make_pair(1, cmp_stat));

    inc_stat_source->add(now, stat_1);
  }

  campaign_config_modifier->update(campaign_config, now);

  {
    CampaignMap::ActiveMap::const_iterator cmp_it =
      campaign_config->campaigns.active().find(1);
    if(cmp_it == campaign_config->campaigns.active().end())
    {
      std::cerr << TEST_NAME << ": Campaign #1 not found" << std::endl;
      res = false;
    }
    else
    {
      TagDeliveryMap etalon_tag_deliveries;
      etalon_tag_deliveries.insert(std::make_pair(1, 8000));

      res &= check_tag_deliveries(
        (std::string(TEST_NAME) + ": step #1").c_str(),
        etalon_tag_deliveries,
        cmp_it->second->exclude_tags);
    }
  }

  return res;
}

int exclusion_with_zero_coef_test()
{
  // regression for ADSC-5699
  // specific case when sum of few first by priority tags amount strictly equal budget_limit
  // and next tag must be excluded with 0 coef
  static const char* TEST_NAME = "exclusion_with_zero_coef_test()";

  int res = 0;

  const Generics::Time now(String::SubString("2011-04-02 22:30:00"),
    "%Y-%m-%d %H:%M:%S");

  CampaignConfig_var campaign_config(new CampaignConfig());
  campaign_config->master_stamp = now;

  campaign_config->accounts.activate(1, create_base_account(1));

  TagDef_var tag1 = create_base_tag(1);
  campaign_config->tags.activate(1, tag1);

  TagDef_var tag2 = create_base_tag(2);
  campaign_config->tags.activate(2, tag2);

  Campaign_var cmp1 = create_base_campaign(1, 1);
  cmp1->ccg_delivery_limits.budget = OptionalRevenueDecimal();
  // 30 min budget = 6
  cmp1->ccg_delivery_limits.daily_budget = RevenueDecimal(false, 12 * 24, 0);
  cmp1->ccg_delivery_limits.delivery_pacing = 'F';
  campaign_config->campaigns.activate(1, cmp1);

  Logging::Logger_var logger(new Logging::OStream::Logger(
    Logging::OStream::Config(std::cout/*, Logging::Logger::TRACE*/)));

  TestStatSource_var inc_stat_source(new TestStatSource);

  CampaignConfigModifier_var campaign_config_modifier(
    new CampaignConfigModifier(
      logger,
      StatSource_var(new TestStatSource),
      Generics::Time::ONE_HOUR,
      inc_stat_source,
      ModifyConfigSource_var(new TestModifyConfigSource()),
      true // enable delivery thresholds
      ));

  {
    StatSource::Stat_var stat_1(new StatSource::Stat());
    StatSource::Stat::CCGStat ccg1_stat;

    RevenueDecimal add_adv_amount(false, 12, 0);

    ccg1_stat.amount = add_adv_amount;
    ccg1_stat.comm_amount = RevenueDecimal::ZERO;
    ccg1_stat.daily_amount = add_adv_amount;
    ccg1_stat.daily_comm_amount = RevenueDecimal::ZERO;
    ccg1_stat.prev_hour_amount = add_adv_amount;
    ccg1_stat.prev_hour_comm_amount = RevenueDecimal::ZERO;
    ccg1_stat.cur_hour_amount = add_adv_amount;
    ccg1_stat.cur_hour_comm_amount = RevenueDecimal::ZERO;

    RevenueDecimal tag_add_adv_amount(false, 6, 0);

    StatSource::Stat::CCGStat::TagStat tag1_stat;
    tag1_stat.prev_hour_stat.isp_pub_amount = RevenueDecimal(false, 1, 0);
    tag1_stat.prev_hour_stat.adv_amount = tag_add_adv_amount;
    tag1_stat.prev_hour_stat.adv_comm_amount = RevenueDecimal::ZERO;
    tag1_stat.current_hour_stat.isp_pub_amount = RevenueDecimal(false, 1, 0);
    tag1_stat.current_hour_stat.adv_amount = tag_add_adv_amount;
    tag1_stat.current_hour_stat.adv_comm_amount = RevenueDecimal::ZERO;
    ccg1_stat.tag_stats.insert(std::make_pair(1, tag1_stat));

    StatSource::Stat::CCGStat::TagStat tag2_stat;
    tag2_stat.prev_hour_stat.isp_pub_amount = RevenueDecimal(false, 2, 0);
    tag2_stat.prev_hour_stat.adv_amount = tag_add_adv_amount;
    tag2_stat.prev_hour_stat.adv_comm_amount = RevenueDecimal::ZERO;
    tag2_stat.current_hour_stat.isp_pub_amount = RevenueDecimal(false, 2, 0);
    tag2_stat.current_hour_stat.adv_amount = tag_add_adv_amount;
    tag2_stat.current_hour_stat.adv_comm_amount = RevenueDecimal::ZERO;
    ccg1_stat.tag_stats.insert(std::make_pair(2, tag2_stat));

    StatSource::Stat::CampaignStat cmp_stat;
    cmp_stat.ccgs.insert(std::make_pair(1, ccg1_stat));
    stat_1->campaign_stats.insert(std::make_pair(1, cmp_stat));

    inc_stat_source->add(now, stat_1);
  }

  const unsigned long CHECKS_COUNT = 4;

  for(unsigned long i = 1; i < CHECKS_COUNT; ++i)
  {
    StatSource::Stat_var empty_stat(new StatSource::Stat());
    inc_stat_source->add(now + Generics::Time::ONE_SECOND * i / 1000, empty_stat);
  }

  for(unsigned long i = 0; i < CHECKS_COUNT; ++i)
  {
    campaign_config_modifier->update(
      campaign_config,
      now + Generics::Time::ONE_SECOND * i / 1000);

    CampaignMap::ActiveMap::const_iterator cmp_it =
      campaign_config->campaigns.active().find(1);
    if(cmp_it == campaign_config->campaigns.active().end())
    {
      std::cerr << TEST_NAME << ": Campaign #1 not found" << std::endl;
      res = false;
    }
    else
    {
      TagDeliveryMap etalon_tag_deliveries;
      etalon_tag_deliveries.insert(std::make_pair(2, 0));

      std::ostringstream ostr;
      ostr << TEST_NAME << ": step #" << i;
      res &= check_tag_deliveries(
        ostr.str().c_str(),
        etalon_tag_deliveries,
        cmp_it->second->exclude_tags);
    }
  }

  return res;
}

int main(int /*argc*/, char** /*argv*/)
{
  int result = 0;

  result += acc_budgets_test();
  result += simple_cmp_budgets_test();
  result += dynamic_cmp_budgets_test();
  result += simple_ccg_budgets_test();
  result += ccg_delivery_tags_test();
  result += one_hour_dynamic_test();
  result += zero_adv_amount_test();
  result += exclusion_with_zero_coef_test();

  return result;
}
