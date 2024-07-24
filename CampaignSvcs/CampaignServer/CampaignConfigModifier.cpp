
#include "ExecutionTimeTracer.hpp"
#include "CampaignConfigModifier.hpp"

namespace Aspect
{
  const char CAMPAIGN_CONFIG_MODIFIER[] = "CampaignConfigModifier";
}

namespace AdServer
{
namespace CampaignSvcs
{
  namespace
  {
    const RevenueDecimal TAG_DELIVERY_MAX_DECIMAL(false, TAG_DELIVERY_MAX, 0);
    const RevenueDecimal REVENUEDECIMAL_ONE_DAY(
      false, Generics::Time::ONE_DAY.tv_sec, 0);
    const StatSource::Stat::CCGStat::TagHourStat ZERO_TAG_HOUR_STAT;

    const ImpRevenueDecimal MIN_IMP_FOR_CREATIVE_WEIGHT_CALC(false, 2000, 0);
  }

  // CampaignConfigModifier
  CampaignConfigModifier::CampaignConfigModifier(
    Logging::Logger* logger,
    StatSource* stamp_stat_source,
    const Generics::Time& stamp_sync_period,
    StatSource* increment_stat_source,
    ModifyConfigSource* modify_config_source,
    bool enable_delivery_thresholds)
    noexcept
    : logger_(ReferenceCounting::add_ref(logger)),
      stamp_stat_source_(ReferenceCounting::add_ref(stamp_stat_source)),
      increment_stat_source_(ReferenceCounting::add_ref(increment_stat_source)),
      modify_config_source_(ReferenceCounting::add_ref(modify_config_source)),
      STAMP_SYNC_PERIOD_(stamp_sync_period),
      ENABLE_DELIVERY_THRESHOLDS_(enable_delivery_thresholds),
      full_synch_required_(false),
      state_(new State())
  {
    delivery_calc_params_.under_coef = RevenueDecimal("0.9");
    delivery_calc_params_.over_coef = RevenueDecimal("1.1");
    delivery_calc_params_.calc_period = Generics::Time::ONE_MINUTE * 30;
    delivery_calc_params_.day_part =
      RevenueDecimal(false, delivery_calc_params_.calc_period.tv_sec, 0);
  }

  void
  CampaignConfigModifier::update_stat(
    CampaignConfig* campaign_config,
    const Generics::Time& now)
    /*throw(Exception)*/
  {
    SyncPolicy::WriteGuard update_lock(update_stat_lock_);

    StatSource::Stat_var campaign_stat;

    if(campaign_stat_.get().in())
    {
      campaign_stat = new StatSource::Stat(*campaign_stat_.get());
      adapt_stat_(*campaign_stat, campaign_config, now);
    }

    campaign_stat = update_stat_i_(campaign_stat, now, true);
    campaign_stat_ = campaign_stat;
  }

  void
  CampaignConfigModifier::update(
    CampaignConfig* campaign_config,
    const Generics::Time& now)
    /*throw(Exception)*/
  {
    StatSource::Stat_var campaign_stat;
    CModifyConfig_var modify_config;

    {
      SyncPolicy::WriteGuard update_lock(update_stat_lock_);

      if(campaign_stat_.get().in())
      {
        StatSource::Stat_var adapt_campaign_stat(
          new StatSource::Stat(*campaign_stat_.get()));
        adapt_stat_(*adapt_campaign_stat, campaign_config, now);
        campaign_stat = adapt_campaign_stat;
      }

      campaign_stat = update_stat_i_(campaign_stat, now, false);
      campaign_stat_ = campaign_stat;
    }

    {
      // update modify config
      ModifyConfig_var new_modify_config =
        modify_config_source_->update();

      SyncPolicy::WriteGuard lock(modify_config_lock_);

      if(new_modify_config.in())
      {
        modify_config_ = new_modify_config;
      }

      modify_config = modify_config_;
    }

    if(campaign_stat.in() && modify_config.in()) // first initialization success
    {
      SyncPolicy::WriteGuard update_config_lock(update_config_lock_);

      const State_var old_state = state_.get();
      State_var new_state(new State());
      new_state->timestamp = campaign_config->master_stamp;

      update_config_(
        *new_state,
        *campaign_config,
        *campaign_stat,
        *modify_config,
        old_state.in(),
        now);

      state_ = new_state;
    }
  }

  StatSource::CStat_var
  CampaignConfigModifier::stat() const /*throw(NotReady)*/
  {
    const StatSource::CStat_var campaign_stat = campaign_stat_.get();

    if(!campaign_stat.in())
    {
      throw NotReady("");
    }

    return campaign_stat;
  }

  CampaignConfigModifier::CState_var
  CampaignConfigModifier::state() const /*throw(NotReady)*/
  {
    const CampaignConfigModifier::CState_var state = state_.get();

    if(!state.in())
    {
      throw NotReady("");
    }

    return state;
  }

  StatSource::Stat_var
  CampaignConfigModifier::update_stat_i_(
    StatSource::Stat* campaign_stat,
    const Generics::Time& now,
    bool full_update)
    /*throw(Exception)*/
  {
    StatSource::Stat_var res_campaign_stat(
      ReferenceCounting::add_ref(campaign_stat));

    if(!res_campaign_stat.in() ||
       full_synch_required_ ||
       full_update ||
       STAMP_SYNC_PERIOD_ == Generics::Time::ZERO ||
       now >= stamp_last_update_ + STAMP_SYNC_PERIOD_)
    {
      // try update stats from DB
      StatSource::Stat_var full_campaign_stat = stamp_stat_source_->update(
        res_campaign_stat, full_synch_required_, now);

      if(full_campaign_stat.in())
      {
        stamp_last_update_ = now;
        res_campaign_stat = full_campaign_stat;
      }
    }

    // synchronize with LogGeneralizer
    if(res_campaign_stat.in()) // first initialization success
    {
      bool full_synch_required;

      res_campaign_stat = increment_stat_source_->update(
        res_campaign_stat, full_synch_required, now);

      if(full_synch_required)
      {
        full_synch_required_ = full_synch_required;
      }
    }

    return res_campaign_stat;
  }

  void
  CampaignConfigModifier::update_config_(
    State& new_state,
    CampaignConfig& campaign_config,
    StatSource::Stat& stats,
    const ModifyConfig& modify_config,
    const State* old_state,
    const Generics::Time& sysdate)
    /*throw(Exception)*/
  {
//  static const char* FUN = "CampaignConfigModifier::update_config_()";

    typedef std::list<std::pair<unsigned long, Campaign_var> >
      CampaignPtrList;
    typedef std::map<unsigned long, CampaignPtrList> CampaignCCGMap;
    typedef std::map<unsigned long, Campaign_var>
      ModifiedCampaignMap;

    // check account budget limitations
    for(AccountMap::ActiveMap::iterator acc_it =
          campaign_config.accounts.active().begin();
        acc_it != campaign_config.accounts.active().end(); ++acc_it)
    {
      const AccountDef* account = acc_it->second;

      if((account->is_advertiser() && (
           account->use_self_budget() || account->agency_account_id == 0)) ||
         (account->is_agency() && !account->use_self_budget()))
      {
        RevenueDecimal amount = RevenueDecimal::ZERO;

        StatSource::Stat::AccountAmountMap::const_iterator acc_stat_it =
          stats.account_amounts.find(acc_it->first);

        if(acc_stat_it != stats.account_amounts.end())
        {
          // stats contains account_id that realy used for pay amount
          // comm_amount contains payable commision amount (invoice commision)
          amount = acc_stat_it->second.amount +
            acc_stat_it->second.comm_amount;
        }

        bool is_active;
        try
        {
          is_active = amount < account->budget + account->paid_amount;
        }
        catch(const RevenueDecimal::Overflow&)
        {
          is_active = true;
        }

        char new_eval_status = is_active ? 'A' : 'I';

        if(new_eval_status != account->eval_status)
        {
          AccountDef_var new_account(new AccountDef(*account));
          new_account->eval_status = new_eval_status;
          new_account->timestamp = sysdate;
          acc_it->second = new_account;
        }
      }
    }

    Stream::Dynamic deactivate_trace_str_holder(4096);
    Stream::Dynamic* deactivate_trace_str = 0;
    if(logger_->log_level() >= Logging::Logger::TRACE)
    {
      deactivate_trace_str = &deactivate_trace_str_holder;
    }

    RevenueDecimal prev_hour_factor;
    RevenueDecimal cur_hour_factor;

    fill_campaign_delivery_check_params_(
      prev_hour_factor,
      cur_hour_factor,
      sysdate);

    // group ccg by campaigns
    CampaignCCGMap grouped_campaigns;

    for(CampaignMap::ActiveMap::iterator ccg_it =
          campaign_config.campaigns.active().begin();
        ccg_it != campaign_config.campaigns.active().end(); ++ccg_it)
    {
      const CampaignDef* campaign = ccg_it->second;
      grouped_campaigns[campaign->campaign_group_id].push_back(
        std::make_pair(ccg_it->first, ccg_it->second));
    }

    // check campaigns
    for(CampaignCCGMap::iterator cmp_group_it = grouped_campaigns.begin();
        cmp_group_it != grouped_campaigns.end();
        ++cmp_group_it)
    {
      assert(!cmp_group_it->second.empty());

      bool account_is_active = false;
      const AccountDef* account = 0;

      {
        // check accounts
        const CampaignDef* first_campaign = cmp_group_it->second.begin()->second;

        if(first_campaign->advertiser_id)
        {
          AccountMap::ActiveMap::const_iterator adv_it =
            campaign_config.accounts.active().find(first_campaign->advertiser_id);
          account_is_active = adv_it != campaign_config.accounts.active().end() &&
            adv_it->second->eval_status == 'A';
        }

        AccountMap::ActiveMap::const_iterator acc_it =
          campaign_config.accounts.active().find(first_campaign->account_id);

        if(acc_it != campaign_config.accounts.active().end())
        {
          account = acc_it->second;
          account_is_active = account_is_active && (
            account->eval_status == 'A');
        }
        else
        {
          account_is_active = false;
        }
      }

      ModifiedCampaignMap modified_ccgs;
      bool campaign_daily_budget_defined = false;
      RevenueDecimal campaign_daily_budget;
      // summarized period adv amount that include delivery changes
      RevenueDecimal period_campaign_adv_amount = RevenueDecimal::ZERO;

      const CampaignDef* first_campaign = cmp_group_it->second.begin()->second;

      bool campaign_is_active = account_is_active && campaign_is_active_(
        deactivate_trace_str,
        campaign_daily_budget_defined,
        campaign_daily_budget,
        sysdate,
        account,
        cmp_group_it->first,
        first_campaign,
        stats);

      for(CampaignPtrList::const_iterator cmp_it = cmp_group_it->second.begin();
          cmp_it != cmp_group_it->second.end(); ++cmp_it)
      {
        const unsigned long ccg_id = cmp_it->first;
        const CampaignDef* campaign = cmp_it->second;

        bool is_active = campaign_is_active;
        AccountIdSet exclude_pub_accounts;
        CreativeList creatives;
        TagDeliveryMap exclude_tags;
        bool creatives_changed = false;

        bool ccg_daily_budget_defined;
        RevenueDecimal ccg_daily_budget;
        Generics::Time today_open_time;
        CampaignMode new_campaign_mode = campaign->mode;

        if(is_active)
        {
          is_active = ccg_is_active_(
            deactivate_trace_str,
            ccg_daily_budget_defined,
            ccg_daily_budget,
            today_open_time,
            new_campaign_mode,
            sysdate,
            account,
            ccg_id,
            campaign,
            stats,
            modify_config);
        }

        if(is_active)
        {
          StatSource::Stat::CampaignStatMap::iterator cmp_stats_it =
            stats.campaign_stats.find(campaign->campaign_group_id);

          StatSource::Stat::CCGStat* ccg_stats = 0;

          if(cmp_stats_it != stats.campaign_stats.end())
          {
            StatSource::Stat::CCGStatMap::iterator ccg_stats_it =
              cmp_stats_it->second.ccgs.find(ccg_id);

            if(ccg_stats_it != cmp_stats_it->second.ccgs.end())
            {
              ccg_stats = &ccg_stats_it->second;
            }
          }

          if(ccg_stats)
          {
            if(campaign->max_pub_share > RevenueDecimal::ZERO &&
               campaign->max_pub_share < RevenueDecimal(false, 1, 0))
            {
              fill_campaign_exclude_pub_accounts_(
                exclude_pub_accounts,
                campaign,
                ccg_stats->publisher_amounts);
            }

            if(ENABLE_DELIVERY_THRESHOLDS_)
            {
              if(ccg_daily_budget_defined || campaign_daily_budget_defined)
              {
                const RevenueDecimal daily_budget_limit =
                  ccg_daily_budget_defined && campaign_daily_budget_defined ?
                  std::min(ccg_daily_budget, campaign_daily_budget) : (
                    ccg_daily_budget_defined ? ccg_daily_budget : campaign_daily_budget);

                RevenueDecimal predictable_period_adv_amount;

                fill_ccg_delivery_tags_(
                  predictable_period_adv_amount,
                  new_state,
                  exclude_tags,
                  *ccg_stats,
                  old_state,
                  prev_hour_factor,
                  cur_hour_factor,
                  campaign_config.tags.active(),
                  ccg_id,
                  *campaign,
                  account,
                  daily_budget_limit,
                  today_open_time,
                  sysdate);

                period_campaign_adv_amount += predictable_period_adv_amount;
              }
              else
              {
                period_campaign_adv_amount += eval_period_amount_(
                  account,
                  ccg_stats->prev_hour_amount,
                  ccg_stats->cur_hour_amount,
                  ccg_stats->prev_hour_comm_amount,
                  ccg_stats->cur_hour_comm_amount,
                  prev_hour_factor,
                  cur_hour_factor);
              }
            }
          }

          if(campaign->creative_optimization())
          {
            creatives_changed = fill_campaign_creative_weights_(
              creatives,
              campaign,
              ccg_stats ? &ccg_stats->creatives : 0);
          }
        }

        if(deactivate_trace_str && is_active)
        {
          *deactivate_trace_str << "activate ccg #" << cmp_it->first << std::endl;
        }

        char new_eval_status = is_active ? 'A' : 'I';

        if(new_campaign_mode != campaign->mode ||
           new_eval_status != campaign->eval_status ||
           creatives_changed ||
           exclude_pub_accounts.size() != campaign->exclude_pub_accounts.size() ||
           !std::equal(
             exclude_pub_accounts.begin(),
             exclude_pub_accounts.end(),
             campaign->exclude_pub_accounts.begin()) ||
           exclude_tags.size() != campaign->exclude_tags.size() ||
           !std::equal(
             exclude_tags.begin(),
             exclude_tags.end(),
             campaign->exclude_tags.begin(),
             Algs::PairEqual()))
        {
          Campaign_var new_campaign(new CampaignDef(*campaign));
          new_campaign->mode = new_campaign_mode;
          new_campaign->eval_status = new_eval_status;
          new_campaign->exclude_pub_accounts.swap(exclude_pub_accounts);
          new_campaign->exclude_tags.swap(exclude_tags);
          if(creatives_changed)
          {
            new_campaign->creatives.swap(creatives);
          }
          new_campaign->timestamp = sysdate;
          modified_ccgs.insert(std::make_pair(ccg_id, new_campaign));          
        }
      }

      unsigned long delivery_coef = TAG_DELIVERY_MAX;

      if(ENABLE_DELIVERY_THRESHOLDS_ && campaign_daily_budget_defined)
      {
        // reeval delivery coefs by campaign daily budget
        Generics::Time campaign_open_time = campaign_open_time_(
          first_campaign, sysdate);

        const RevenueDecimal period_budget_limit = eval_period_budget_limit_(
          campaign_daily_budget,
          campaign_open_time);

        // check only over delivering, under delivering conrolled on ccg level
        if(period_campaign_adv_amount != RevenueDecimal::ZERO &&
           period_campaign_adv_amount >= RevenueDecimal::mul(
             period_budget_limit,
             delivery_calc_params_.over_coef,
             Generics::DMR_FLOOR))
        {
          delivery_coef = RevenueDecimal::mul(
            RevenueDecimal::div(period_budget_limit, period_campaign_adv_amount),
            TAG_DELIVERY_MAX_DECIMAL,
            Generics::DMR_FLOOR).integer<unsigned long>();
        }
      }

      // apply ccgs changes
      for(CampaignPtrList::const_iterator cmp_it = cmp_group_it->second.begin();
          cmp_it != cmp_group_it->second.end(); ++cmp_it)
      {
        bool modified = false;
        ReferenceCounting::SmartPtr<CampaignDef> orig;
        ModifiedCampaignMap::const_iterator mcmp_it = modified_ccgs.find(
          cmp_it->first);
        if(mcmp_it != modified_ccgs.end())
        {
          orig = mcmp_it->second;
          modified = true;
        }
        else
        {
          orig = cmp_it->second;
        }

        if(orig->delivery_coef != delivery_coef)
        {
          Campaign_var new_campaign(new CampaignDef(*orig));
          new_campaign->delivery_coef = delivery_coef;
          new_campaign->timestamp = sysdate;
          orig = new_campaign;
          modified = true;
        }

        if(modified)
        {
          campaign_config.campaigns.activate(cmp_it->first, orig);
        }
      }
    }

    campaign_config.master_stamp = sysdate;

    if(deactivate_trace_str)
    {
      logger_->log(
        deactivate_trace_str->str(),
        Logging::Logger::TRACE,
        Aspect::CAMPAIGN_CONFIG_MODIFIER);
    }
  }

  Generics::Time
  CampaignConfigModifier::campaign_open_time_(
    const CampaignDef* campaign,
    const Generics::Time& now)
    noexcept
  {
    Generics::Time today_open_time_start = now.get_gm_time().get_date();
    Generics::Time today_open_time_end = today_open_time_start +
      Generics::Time::ONE_DAY - Generics::Time::ONE_SECOND;

    today_open_time_start = std::max(
      today_open_time_start,
      campaign->campaign_delivery_limits.date_start);
    if(campaign->campaign_delivery_limits.date_end != Generics::Time::ZERO)
    {
      today_open_time_end = std::min(
        today_open_time_end,
        campaign->campaign_delivery_limits.date_end - Generics::Time::ONE_SECOND);
    }

    if(today_open_time_end > today_open_time_start)
    {
      return today_open_time_end - today_open_time_start;
    }

    return Generics::Time::ZERO;
  }

  unsigned long
  CampaignConfigModifier::time_to_week_offset_(
    const Generics::Time& time)
    noexcept
  {
    Generics::ExtendedTime ex_time = time.get_gm_time();
    return (ex_time.tm_wday + 6) % 7 * 60 * 24 +
      ex_time.tm_hour * 60 +
      ex_time.tm_min;
  }

  bool
  CampaignConfigModifier::get_daily_budget_(
    RevenueDecimal& daily_budget,
    const Generics::Time& now,
    const CampaignDeliveryLimits& delivery_limits,
    const Generics::Time& time_offset,
    const RevenueDecimal& amount,
    const RevenueDecimal& daily_amount)
    noexcept
  {
    if(delivery_limits.delivery_pacing == 'F')
    {
      if(delivery_limits.daily_budget)
      {
        daily_budget = *delivery_limits.daily_budget;
        return true;
      }
    }
    else if(delivery_limits.delivery_pacing == 'D' &&
      delivery_limits.date_end != Generics::Time::ZERO)
    {
      if(delivery_limits.budget)
      {
        RevenueDecimal remain_days(
          ((delivery_limits.date_end + time_offset).get_gm_time().get_date() -
           (now + time_offset).get_gm_time().get_date()).tv_sec /
          Generics::Time::ONE_DAY.tv_sec + 1);
        daily_budget = (remain_days > RevenueDecimal::ZERO ?
          RevenueDecimal::div(
            *delivery_limits.budget - amount + daily_amount, remain_days) :
          RevenueDecimal::ZERO);
        return true;
      }
    }

    return false; // unlimited daily budget
  }

  template<typename OStream>
  bool
  CampaignConfigModifier::campaign_is_active_(
    OStream* deactivate_trace_str,
    bool& campaign_daily_budget_defined,
    RevenueDecimal& campaign_daily_budget,
    const Generics::Time& now,
    const AccountDef* account,
    unsigned long id,
    const CampaignDef* campaign,
    const StatSource::Stat& stat)
    noexcept
  {
    campaign_daily_budget_defined = false;

    if(account->eval_status != 'A')
    {
      if(deactivate_trace_str)
      {
        *deactivate_trace_str << "deactivate campaign #" << id <<
          ": " << (account->eval_status != 'A' ? "account status" : "") <<
          std::endl;
      }

      return false;
    }

    // Campaign budget check
    RevenueDecimal amount = RevenueDecimal::ZERO;
    RevenueDecimal daily_amount = RevenueDecimal::ZERO;

    StatSource::Stat::CampaignStatMap::const_iterator cmp_stat_it =
      stat.campaign_stats.find(campaign->campaign_group_id);

    if(cmp_stat_it != stat.campaign_stats.end())
    {
      amount = cmp_stat_it->second.amount +
        (account->is_gross() ? cmp_stat_it->second.comm_amount :
         RevenueDecimal::ZERO);

      daily_amount = cmp_stat_it->second.daily_amount +
        (account->is_gross() ? cmp_stat_it->second.daily_comm_amount :
         RevenueDecimal::ZERO);
    }

    if(campaign->campaign_delivery_limits.budget &&
         *campaign->campaign_delivery_limits.budget <= amount)
    {
      if(deactivate_trace_str)
      {
        *deactivate_trace_str << "deactivate campaign #" << id <<
          ": ccg delivery passing limits" << std::endl;
      }
      return false;
    }
    else if((campaign_daily_budget_defined =
      get_daily_budget_(
        campaign_daily_budget,
        now,
        campaign->campaign_delivery_limits,
        account->time_offset,
        amount,
        daily_amount)) && daily_amount >= campaign_daily_budget)
    {
      if(deactivate_trace_str)
      {
        *deactivate_trace_str << "deactivate ccg #" << id <<
          ": campaign delivery passing limits" << std::endl;
      }
      return false;
    }

    return true;
  }

  template<typename OStream>
  bool
  CampaignConfigModifier::ccg_is_active_(
    OStream* deactivate_trace_str,
    bool& ccg_daily_budget_defined,
    RevenueDecimal& ccg_daily_budget,
    Generics::Time& today_open_time,
    CampaignMode& campaign_mode,
    const Generics::Time& now,
    const AccountDef* account,
    unsigned long id,
    const CampaignDef* campaign,
    const StatSource::Stat& stat,
    const ModifyConfig& modify_config)
    noexcept
  {
    ccg_daily_budget_defined = false;

    StatSource::Stat::CampaignStatMap::const_iterator cmp_stat_it =
      stat.campaign_stats.find(campaign->campaign_group_id);

    ImpRevenueDecimal ccg_ctr_reset_imps = ImpRevenueDecimal::ZERO;

    if(cmp_stat_it != stat.campaign_stats.end())
    {
      StatSource::Stat::CCGStatMap::const_iterator ccg_stats_it =
        cmp_stat_it->second.ccgs.find(id);

      if(ccg_stats_it != cmp_stat_it->second.ccgs.end())
      {
        RevenueDecimal amount = ccg_stats_it->second.amount +
          (account->is_gross() ? ccg_stats_it->second.comm_amount :
           RevenueDecimal::ZERO);
        RevenueDecimal daily_amount = ccg_stats_it->second.daily_amount +
          (account->is_gross() ? ccg_stats_it->second.daily_comm_amount :
           RevenueDecimal::ZERO);

        if(campaign->ctr_reset_id == 0)
        {
          ccg_ctr_reset_imps = ccg_stats_it->second.impressions;
        }
        else
        {
          StatSource::Stat::CCGStat::
            CtrResetStatMap::const_iterator ctr_reset_it =
              ccg_stats_it->second.ctr_reset_stats.find(campaign->ctr_reset_id);

          if(ctr_reset_it != ccg_stats_it->second.ctr_reset_stats.end())
          {
            ccg_ctr_reset_imps = ctr_reset_it->second.impressions;
          }
        }

        ccg_daily_budget_defined = get_daily_budget_(
          ccg_daily_budget,
          now,
          campaign->ccg_delivery_limits,
          account->time_offset,
          amount,
          daily_amount);

        // CCG budget check
        if((ccg_daily_budget_defined &&
             daily_amount >= ccg_daily_budget) ||
           (campaign->ccg_delivery_limits.budget &&
             *campaign->ccg_delivery_limits.budget <= amount))
        {
          if(deactivate_trace_str)
          {
            *deactivate_trace_str << "deactivate ccg #" << id <<
              ": ccg budget limits" << std::endl;
          }

          return false;
        }
      }
    }

    // fill today open time if campaign is active
    Generics::Time today_open_time_start = now.get_gm_time().get_date();
    Generics::Time today_open_time_end = today_open_time_start +
      Generics::Time::ONE_DAY - Generics::Time::ONE_SECOND;

    today_open_time_start = std::max(
      today_open_time_start, campaign->ccg_delivery_limits.date_start);
    today_open_time_start = std::max(
      today_open_time_start, campaign->campaign_delivery_limits.date_start);

    if(campaign->ccg_delivery_limits.date_end != Generics::Time::ZERO)
    {
      today_open_time_end = std::min(
        today_open_time_end,
        campaign->ccg_delivery_limits.date_end - Generics::Time::ONE_SECOND);
    }

    if(campaign->campaign_delivery_limits.date_end != Generics::Time::ZERO)
    {
      today_open_time_end = std::min(
        today_open_time_end,
        campaign->campaign_delivery_limits.date_end - Generics::Time::ONE_SECOND);
    }

    // cross result [today_open_time_start, today_open_time_end)
    // with weekly run intervals
    unsigned long today_open_time_start_offset =
      time_to_week_offset_(today_open_time_start);
    unsigned long today_open_time_end_offset =
      time_to_week_offset_(today_open_time_end);

    if(today_open_time_end_offset <= today_open_time_start_offset)
    {
      today_open_time = Generics::Time::ZERO;
    }
    else if(!campaign->weekly_run_intervals.empty())
    {
      // weekly_run_intervals contains intervals in minutes
      WeeklyRunIntervalSet intervals;
      intervals.insert(WeeklyRunIntervalDef(
        today_open_time_start_offset,
        today_open_time_end_offset + 1));
      intervals.cross(campaign->weekly_run_intervals);

      today_open_time = Generics::Time::ZERO;
      for(WeeklyRunIntervalSet::const_iterator it = intervals.begin();
          it != intervals.end(); ++it)
      {
        today_open_time += Generics::Time::ONE_MINUTE *
          (it->max - it->min);
      }
    }
    else
    {
      today_open_time = today_open_time_end >= today_open_time_start ?
        today_open_time_end - today_open_time_start +
          Generics::Time::ONE_SECOND :
        Generics::Time::ZERO;
    }

    ModifyConfig::CountryMap::const_iterator country_it =
      modify_config.countries.find(campaign->country);

    if(country_it == modify_config.countries.end())
    {
      return false;
    }

    // eval campaign mode
    if(campaign->ccg_rate_type == CR_CPM ||
       campaign->ccg_rate_type == CR_MAXBID ||
       (campaign->ccg_rate_type == CR_CPC &&
        ccg_ctr_reset_imps >= ImpRevenueDecimal(false, country_it->second.cpc_random_imps, 0)) ||
       (campaign->ccg_rate_type == CR_CPA &&
        ccg_ctr_reset_imps >= ImpRevenueDecimal(false, country_it->second.cpa_random_imps, 0)))
    {
      campaign_mode = CM_NON_RANDOM;
    }
    else
    {
      campaign_mode = CM_RANDOM;
    }

    return true;
  }

  void
  CampaignConfigModifier::adapt_stat_(
    StatSource::Stat& stat,
    const CampaignConfig* campaign_config,
    const Generics::Time& now)
    noexcept
  {
    for(CampaignMap::ActiveMap::const_iterator ccg_it =
          campaign_config->campaigns.active().begin();
        ccg_it != campaign_config->campaigns.active().end(); ++ccg_it)
    {
      AccountMap::ActiveMap::const_iterator acc_it =
        campaign_config->accounts.active().find(ccg_it->second->account_id);
      if(acc_it != campaign_config->accounts.active().end())
      {
        Generics::ExtendedTime acc_time = Generics::Time(
          now + acc_it->second->time_offset).get_gm_time();

        Generics::ExtendedTime acc_hour(
          acc_time.tm_year + 1900,
          acc_time.tm_mon + 1,
          acc_time.tm_mday,
          acc_time.tm_hour,
          0,
          0,
          0);

        Generics::ExtendedTime acc_check_time = Generics::Time(
          stat.check_time + acc_it->second->time_offset).get_gm_time();

        Generics::ExtendedTime acc_check_hour(
          acc_check_time.tm_year + 1900,
          acc_check_time.tm_mon + 1,
          acc_check_time.tm_mday,
          acc_check_time.tm_hour,
          0,
          0,
          0);

        StatSource::Stat::CampaignStat* campaign_stat =
          find_campaign_stat_(stat, ccg_it->second);

        if (campaign_stat)
        {
          if(acc_hour.get_date() != acc_check_hour.get_date())
          {
            campaign_stat->daily_amount = RevenueDecimal::ZERO;
            campaign_stat->daily_comm_amount = RevenueDecimal::ZERO;
          }

          StatSource::Stat::CCGStat* ccg_stat = find_ccg_stat_(
            *campaign_stat, ccg_it->first);

          if(ccg_stat)
          {
            if(acc_hour.get_date() != acc_check_hour.get_date())
            {
              ccg_stat->daily_amount = RevenueDecimal::ZERO;
              ccg_stat->daily_comm_amount = RevenueDecimal::ZERO;

              for(StatSource::Stat::CCGStat::PublisherStatMap::iterator pit =
                    ccg_stat->publisher_amounts.begin();
                  pit != ccg_stat->publisher_amounts.end(); ++pit)
              {
                pit->second.daily_amount = RevenueDecimal::ZERO;
              }
            }

            if(acc_hour != acc_check_hour)
            {
              if(acc_hour - acc_check_hour == Generics::Time::ONE_HOUR)
              {
                ccg_stat->prev_hour_amount = ccg_stat->cur_hour_amount;
                ccg_stat->cur_hour_amount = RevenueDecimal::ZERO;
                ccg_stat->prev_hour_comm_amount = ccg_stat->cur_hour_comm_amount;
                ccg_stat->cur_hour_comm_amount = RevenueDecimal::ZERO;

                for(StatSource::Stat::CCGStat::TagStatMap::iterator ts_it =
                      ccg_stat->tag_stats.begin();
                    ts_it != ccg_stat->tag_stats.end(); ++ts_it)
                {
                  ts_it->second.prev_hour_stat = ts_it->second.current_hour_stat;
                  ts_it->second.current_hour_stat = ZERO_TAG_HOUR_STAT;
                }
              }
              else
              {
                ccg_stat->prev_hour_amount = RevenueDecimal::ZERO;
                ccg_stat->cur_hour_amount = RevenueDecimal::ZERO;
                ccg_stat->prev_hour_comm_amount = RevenueDecimal::ZERO;
                ccg_stat->cur_hour_comm_amount = RevenueDecimal::ZERO;

                for(StatSource::Stat::CCGStat::TagStatMap::iterator ts_it =
                      ccg_stat->tag_stats.begin();
                    ts_it != ccg_stat->tag_stats.end(); ++ts_it)
                {
                  ts_it->second.prev_hour_stat = ZERO_TAG_HOUR_STAT;
                  ts_it->second.current_hour_stat = ZERO_TAG_HOUR_STAT;
                }
              }
            }
          }
        }
      }
    }

    stat.check_time = now;
  }

  void
  CampaignConfigModifier::fill_campaign_exclude_pub_accounts_(
    AccountIdSet& exclude_pub_accounts,
    const CampaignDef* campaign,
    const StatSource::Stat::CCGStat::PublisherStatMap& publisher_amounts)
    noexcept
  {
    for(StatSource::Stat::CCGStat::PublisherStatMap::const_iterator pub_it =
          publisher_amounts.begin();
        pub_it != publisher_amounts.end(); ++pub_it)
    {
      if((campaign->ccg_delivery_limits.budget &&
         pub_it->second.amount >= RevenueDecimal::mul(
           *campaign->ccg_delivery_limits.budget,
           campaign->max_pub_share,
           Generics::DMR_FLOOR)) ||
         (campaign->ccg_delivery_limits.daily_budget &&
         pub_it->second.daily_amount >= RevenueDecimal::mul(
           *campaign->ccg_delivery_limits.daily_budget,
           campaign->max_pub_share,
           Generics::DMR_FLOOR)))
      {
        exclude_pub_accounts.insert(pub_it->first);
      }
    }
  }

  bool CampaignConfigModifier::fill_campaign_creative_weights_(
    CreativeList& creatives,
    const CampaignDef* campaign,
    const StatSource::Stat::CCGStat::CreativeStatMap* creative_stats)
    noexcept
  {
    bool creatives_changed = false;
    const ImpRevenueDecimal WEIGHT_BASE = ImpRevenueDecimal(false, 10000, 0);

    // calculate creative weights
    for(CreativeList::const_iterator cc_it = campaign->creatives.begin();
        cc_it != campaign->creatives.end(); ++cc_it)
    {
      unsigned long new_weight = 10000;

      if(creative_stats)
      {
        StatSource::Stat::CCGStat::CreativeStatMap::const_iterator cc_stat_it =
          creative_stats->find((*cc_it)->ccid);

        if(cc_stat_it == creative_stats->end() ||
          cc_stat_it->second.impressions < MIN_IMP_FOR_CREATIVE_WEIGHT_CALC)
        {
          new_weight = 10000;
        }
        else if(cc_stat_it->second.clicks < cc_stat_it->second.impressions)
        {
          const ImpRevenueDecimal new_weight_dec = ImpRevenueDecimal::div(
            ImpRevenueDecimal::mul(
              WEIGHT_BASE, std::max(cc_stat_it->second.clicks, ImpRevenueDecimal::ZERO), Generics::DMR_FLOOR),
            cc_stat_it->second.impressions,
            Generics::DDR_FLOOR).ceil(0);
          new_weight_dec.to_integer(new_weight);
        }
        else
        {
          new_weight = 10000;
        }

        new_weight = std::max(new_weight, 1ul);
      }

      if(new_weight != (*cc_it)->weight)
      {
        CreativeDef_var new_creative(new CreativeDef(**cc_it));
        new_creative->weight = new_weight;
        creatives.push_back(new_creative);
        creatives_changed = true;
      }
      else
      {
        creatives.push_back(*cc_it);
      }
    }

    return creatives_changed;
  }

  void
  CampaignConfigModifier::fill_campaign_delivery_check_params_(
    RevenueDecimal& prev_hour_factor,
    RevenueDecimal& cur_hour_factor,
    const Generics::Time& now)
    noexcept
  {
    static const Generics::Time DELIVERY_CALCULATE_PERIOD =
      Generics::Time::ONE_MINUTE * 30;

    Generics::ExtendedTime ex_now = now.get_gm_time();

    Generics::Time in_cur_hour_time = ex_now - Generics::ExtendedTime(
      ex_now.tm_year + 1900,
      ex_now.tm_mon + 1,
      ex_now.tm_mday,
      ex_now.tm_hour,
      0,
      0,
      0);

    Generics::Time in_prev_hour_time;

    if(in_cur_hour_time <= DELIVERY_CALCULATE_PERIOD)
    {
      in_prev_hour_time = DELIVERY_CALCULATE_PERIOD - in_cur_hour_time;

      cur_hour_factor = RevenueDecimal(false, 1, 0);
    }
    else if(in_cur_hour_time > DELIVERY_CALCULATE_PERIOD)
    {
      cur_hour_factor = RevenueDecimal::div(
        RevenueDecimal(false, DELIVERY_CALCULATE_PERIOD.tv_sec, 0),
        RevenueDecimal(false, in_cur_hour_time.tv_sec, 0));
    }

    prev_hour_factor = RevenueDecimal::div(
      RevenueDecimal(false, in_prev_hour_time.tv_sec, 0),
      RevenueDecimal(false, Generics::Time::ONE_HOUR.tv_sec, 0));
  }

  struct CampaignTagAmount
  {
    CampaignTagAmount(
      unsigned long tag_id_val,
      const RevenueDecimal& adv_amount_val)
      : tag_id(tag_id_val),
        adv_amount(adv_amount_val)
    {}

    const unsigned long tag_id;
    const RevenueDecimal adv_amount;
  };

  void
  CampaignConfigModifier::fill_ccg_delivery_tags_(
    RevenueDecimal& predictable_period_adv_amount,
    State& new_state,
    TagDeliveryMap& delivery_tags,
    StatSource::Stat::CCGStat& campaign_stat,
    const State* old_state,
    const RevenueDecimal& prev_hour_factor,
    const RevenueDecimal& cur_hour_factor,
    const TagMap::ActiveMap& active_tags,
    unsigned long ccg_id,
    const CampaignDef& campaign,
    const AccountDef* account,
    const RevenueDecimal& ccg_daily_budget,
    const Generics::Time& today_open_time,
    const Generics::Time& now)
    noexcept
  {
    static const char* FUN = "CampaignConfigModifier::fill_ccg_delivery_tags_()";

    typedef std::multimap<RevenueDecimal, CampaignTagAmount> TagPriorityMap;

    try
    {
      RevenueDecimal sum_amount = eval_period_amount_(
        account,
        campaign_stat.prev_hour_amount,
        campaign_stat.cur_hour_amount,
        campaign_stat.prev_hour_comm_amount,
        campaign_stat.cur_hour_comm_amount,
        prev_hour_factor,
        cur_hour_factor);

      RevenueDecimal budget_limit = eval_period_budget_limit_(
        ccg_daily_budget,
        today_open_time);

      State::CampaignState_var old_cmp_state;
      State::CampaignState_var new_cmp_state;

      if(old_state)
      {
        State::CampaignStateMap::const_iterator cmp_state_it =
          old_state->campaigns.find(ccg_id);
        if(cmp_state_it != old_state->campaigns.end())
        {
          old_cmp_state = cmp_state_it->second;
        }

        // reinit its counters for tags with changed tag pricings
        for(StatSource::Stat::CCGStat::TagStatMap::iterator ts_it =
            campaign_stat.tag_stats.begin();
            ts_it != campaign_stat.tag_stats.end(); )
        {
          TagMap::ActiveMap::const_iterator tag_it = active_tags.find(ts_it->first);
          if(tag_it != active_tags.end())
          {
            if(tag_it->second->tag_pricings_timestamp > old_state->timestamp)
            {
              campaign_stat.tag_stats.erase(ts_it++);
            }
            else
            {
              ++ts_it;
            }
          }
          else
          {
            ++ts_it;
          }
        }
      }

      if(sum_amount >= RevenueDecimal::mul(budget_limit,
           delivery_calc_params_.over_coef, Generics::DMR_FLOOR) ||
         // over delivering
         (!campaign.exclude_tags.empty() &&
         sum_amount < RevenueDecimal::mul(budget_limit,
           delivery_calc_params_.under_coef, Generics::DMR_FLOOR))
         // under delivering with not empty tag exclusion
         )
      {
        // all tag amount values already divided to its actual delivery_threshold value
        // e.m. this is amount values that can be reached if delivering will be 100%
        new_cmp_state = new State::CampaignState();

        TagPriorityMap tag_priorities;

        for(StatSource::Stat::CCGStat::TagStatMap::const_iterator ts_it =
              campaign_stat.tag_stats.begin();
            ts_it != campaign_stat.tag_stats.end(); ++ts_it)
        {
          TagDeliveryMap::const_iterator td_it =
            campaign.exclude_tags.find(ts_it->first);

          if(td_it == campaign.exclude_tags.end() || td_it->second != 0)
          {
            // fully excluded tags will be processed after
            const StatSource::Stat::CCGStat::TagHourStat& prev_hour_stat =
              ts_it->second.prev_hour_stat;
            const StatSource::Stat::CCGStat::TagHourStat& cur_hour_stat =
              ts_it->second.current_hour_stat;

            RevenueDecimal period_adv_amount = eval_period_amount_(
              account,
              prev_hour_stat.adv_amount,
              cur_hour_stat.adv_amount,
              prev_hour_stat.adv_comm_amount,
              cur_hour_stat.adv_comm_amount,
              prev_hour_factor,
              cur_hour_factor);

            if(period_adv_amount != RevenueDecimal::ZERO)
            {
              RevenueDecimal period_isp_pub_amount =
                RevenueDecimal::mul(prev_hour_stat.isp_pub_amount,
                  prev_hour_factor, Generics::DMR_FLOOR) +
                RevenueDecimal::mul(cur_hour_stat.isp_pub_amount,
                  cur_hour_factor, Generics::DMR_FLOOR);

              // tag priority is oix-profit / adv-amount =
              //  = (adv-amount - (pub-amount + isp-amount)) / adv-amount
              RevenueDecimal priority = period_adv_amount != RevenueDecimal::ZERO ?
                RevenueDecimal::div(
                  period_adv_amount - period_isp_pub_amount,
                  period_adv_amount) :
                RevenueDecimal::ZERO;

              tag_priorities.insert(std::make_pair(
                priority, CampaignTagAmount(ts_it->first, period_adv_amount)));
            }
          }
        }

        if(old_cmp_state)
        {
          for(State::CampaignState::TagExclusionMap::const_iterator ts_it =
                old_cmp_state->tag_exclusions.begin();
              ts_it != old_cmp_state->tag_exclusions.end(); ++ts_it)
          {
            TagDeliveryMap::const_iterator td_it =
              campaign.exclude_tags.find(ts_it->first);
            if(td_it != campaign.exclude_tags.end() && td_it->second == 0)
            {
              // fully excluded tags - we use old saved state
              // for its priority evaluate
              tag_priorities.insert(std::make_pair(
                ts_it->second.priority,
                CampaignTagAmount(ts_it->first, ts_it->second.adv_amount)));
            }
          }
        }

        // calculate tag set for 100% delivery,
        // without processing tags that not present in stats &
        // not present in exclusion
        RevenueDecimal sum_amount = RevenueDecimal::ZERO;
        TagPriorityMap::const_reverse_iterator tag_it =
          tag_priorities.rbegin();
        predictable_period_adv_amount = RevenueDecimal::ZERO;

        for(; tag_it != tag_priorities.rend(); ++tag_it)
        {
          sum_amount += tag_it->second.adv_amount;
          if(sum_amount > budget_limit)
          {
            if(tag_it->second.adv_amount != RevenueDecimal::ZERO)
            {
              RevenueDecimal target_amount = budget_limit - (
                sum_amount - tag_it->second.adv_amount);
              RevenueDecimal delivery_coef = RevenueDecimal::mul(
                RevenueDecimal::div(target_amount, tag_it->second.adv_amount),
                TAG_DELIVERY_MAX_DECIMAL,
                Generics::DMR_FLOOR);
              predictable_period_adv_amount += target_amount;

              if(delivery_coef.integer<unsigned long>() > 0)
              {
                delivery_tags.insert(std::make_pair(
                   tag_it->second.tag_id, delivery_coef.integer<unsigned long>()));
                ++tag_it;
              }
            }

            break;
          }
        }

        for(; tag_it != tag_priorities.rend(); ++tag_it)
        {
          delivery_tags.insert(std::make_pair(tag_it->second.tag_id, 0));
          // insert information into state
          State::CampaignState::TagExclusion tag_exclusion;
          tag_exclusion.time = now;
          tag_exclusion.priority = tag_it->first;
          tag_exclusion.adv_amount = tag_it->second.adv_amount;
          new_cmp_state->tag_exclusions[tag_it->second.tag_id] = tag_exclusion;
        }
      }
      else
      {
        predictable_period_adv_amount = sum_amount;

        // copy old tag exclusion, but remove tags with changed tag pricings
        for(TagDeliveryMap::const_iterator td_it =
              campaign.exclude_tags.begin();
            td_it != campaign.exclude_tags.end(); ++td_it)
        {
          TagMap::ActiveMap::const_iterator tag_it = active_tags.find(td_it->first);
          if(tag_it == active_tags.end() ||
             tag_it->second->tag_pricings_timestamp < old_state->timestamp)
          {
            delivery_tags.insert(*td_it);
          }
        }

        new_cmp_state = old_cmp_state;
      }

      new_state.campaigns[ccg_id] = new_cmp_state;
    }
    catch(const RevenueDecimal::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": can't evaluate delivery thresholds for ccg_id=" <<
        ccg_id << ": " << ex.what();

      logger_->log(
        ostr.str(),
        Logging::Logger::ERROR,
        Aspect::CAMPAIGN_CONFIG_MODIFIER);
    }
  }

  RevenueDecimal
  CampaignConfigModifier::eval_period_amount_(
    const AccountDef* account,
    const RevenueDecimal& prev_hour_amount,
    const RevenueDecimal& cur_hour_amount,
    const RevenueDecimal& prev_hour_comm_amount,
    const RevenueDecimal& cur_hour_comm_amount,
    const RevenueDecimal& prev_hour_factor,
    const RevenueDecimal& cur_hour_factor)
    noexcept
  {
    const RevenueDecimal res_prev_hour_amount = prev_hour_amount +
      (account->is_gross() ? prev_hour_comm_amount :
       RevenueDecimal::ZERO);
    const RevenueDecimal res_cur_hour_amount = cur_hour_amount +
      (account->is_gross() ? cur_hour_comm_amount :
       RevenueDecimal::ZERO);
    return RevenueDecimal::mul(
        res_prev_hour_amount, prev_hour_factor, Generics::DMR_FLOOR) +
      RevenueDecimal::mul(
        res_cur_hour_amount, cur_hour_factor, Generics::DMR_FLOOR);
  }

  RevenueDecimal
  CampaignConfigModifier::eval_period_budget_limit_(
    const RevenueDecimal& daily_budget,
    const Generics::Time& today_open_time)
    const noexcept
  {
    try
    {
      if(today_open_time != Generics::Time::ZERO)
      {
        return RevenueDecimal::div(
          RevenueDecimal::mul(
            daily_budget, delivery_calc_params_.day_part,
            Generics::DMR_FLOOR),
          RevenueDecimal(false, today_open_time.tv_sec, 0));
      }
    }
    catch(const RevenueDecimal::Overflow&)
    {
      // will be lost precision at great budget values
      return RevenueDecimal::mul(
        daily_budget,
        RevenueDecimal::div(delivery_calc_params_.day_part,
        REVENUEDECIMAL_ONE_DAY),
        Generics::DMR_FLOOR);
    }

    return RevenueDecimal::ZERO;
  }

  StatSource::Stat::CampaignStat*
  CampaignConfigModifier::find_campaign_stat_(
    StatSource::Stat& stats,
    CampaignDef* campaign)
    noexcept
  {
    const StatSource::Stat::CampaignStatMap::iterator it =
      stats.campaign_stats.find(campaign->campaign_group_id);
    return (it != stats.campaign_stats.end() ? &it->second : nullptr);
  }

  StatSource::Stat::CCGStat*
  CampaignConfigModifier::find_ccg_stat_(
    StatSource::Stat::CampaignStat& stats,
    unsigned long ccg_id)
    noexcept
  {
    const StatSource::Stat::CCGStatMap::iterator it =
      stats.ccgs.find(ccg_id);
    return (it != stats.ccgs.end() ? &it->second : nullptr);
  }
}
}
