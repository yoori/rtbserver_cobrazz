#ifndef CAMPAIGNCONFIGMODIFIER_HPP
#define CAMPAIGNCONFIGMODIFIER_HPP

#include <eh/Exception.hpp>
#include <Sync/SyncPolicy.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/PtrHolder.hpp>
#include <Logger/Logger.hpp>
#include <Generics/Time.hpp>

#include "CampaignConfig.hpp"
#include "StatSource.hpp"
#include "ModifyConfigSource.hpp"

namespace AdServer
{
namespace CampaignSvcs
{
  /**/
  class CampaignConfigModifier:
    public ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(NotReady, Exception);

    struct State: public ReferenceCounting::AtomicImpl
    {
      struct CampaignState: public ReferenceCounting::AtomicImpl
      {
        struct TagExclusion
        {
          Generics::Time time; // time when tag excluded
          RevenueDecimal priority;
          RevenueDecimal adv_amount;
        };

        typedef std::map<unsigned long, TagExclusion> TagExclusionMap;

        TagExclusionMap tag_exclusions;

      protected:
        virtual ~CampaignState() noexcept
        {}
      };

      typedef ReferenceCounting::SmartPtr<CampaignState> CampaignState_var;
      // ccg_id => CampaignState
      typedef std::map<unsigned long, CampaignState_var> CampaignStateMap;

      Generics::Time timestamp;
      CampaignStateMap campaigns;

    protected:
      virtual ~State() noexcept
      {}
    };

    typedef ReferenceCounting::ConstPtr<State> CState_var;

    CampaignConfigModifier(
      Logging::Logger* logger,
      StatSource* primary_stat_source,
      const Generics::Time& stamp_sync_period,
      StatSource* secondary_stat_source,
      ModifyConfigSource* modify_config_source,
      bool enable_delivery_thresholds)
      noexcept;

    void update(
      CampaignConfig* campaign_config,
      const Generics::Time& now)
      /*throw(Exception)*/;

    void update_stat(
      CampaignConfig* campaign_config,
      const Generics::Time& now)
      /*throw(Exception)*/;

    StatSource::CStat_var stat() const /*throw(NotReady)*/;

    CState_var state() const /*throw(NotReady)*/;

  protected:
    virtual ~CampaignConfigModifier() noexcept {}
      
  private:
    typedef Sync::Policy::PosixThread SyncPolicy;

    struct DeliveryCalcParams
    {
      RevenueDecimal under_coef;
      RevenueDecimal over_coef;
      Generics::Time calc_period;
      RevenueDecimal day_part; // time in seconds
    };

    typedef ReferenceCounting::QualPtr<State> State_var;

    struct DeliveryKey;
    struct DeliveryValue;
    typedef std::map<DeliveryKey, DeliveryValue> DeliveryMap;
    typedef std::map<RevenueDecimal, DeliveryMap> PriorityDeliveryMap;

  private:
    StatSource::Stat_var
    update_stat_i_(
      StatSource::Stat* campaign_stat,
      const Generics::Time& now,
      bool full_update)
      /*throw(Exception)*/;

    void update_config_(
      State& new_state,
      CampaignConfig& campaign_config,
      StatSource::Stat& stats,
      const ModifyConfig& modify_config,
      const State* old_state,
      const Generics::Time& sysdate)
      /*throw(Exception)*/;

    /* move daily values to total if day changed (in account timezone)
     * clear campaigns -> tag stats for tags with changed tag pricings */
    static
    void adapt_stat_(
      StatSource::Stat& stat,
      const CampaignConfig* campaign_config,
      const Generics::Time& now)
      noexcept;

    template<typename OStream>
    static bool
    campaign_is_active_(
      OStream* deactivate_trace_str,
      bool& campaign_daily_budget_defined,
      RevenueDecimal& campaign_daily_budget,
      const Generics::Time& now,
      const AccountDef* account,
      unsigned long id,
      const CampaignDef* campaign,
      const StatSource::Stat& stat)
      noexcept;

    template<typename OStream>
    static bool
    ccg_is_active_(
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
      noexcept;

    static Generics::Time
    campaign_open_time_(
      const CampaignDef* campaign,
      const Generics::Time& now)
      noexcept;

    static bool
    get_daily_budget_(
      RevenueDecimal& daily_budget,
      const Generics::Time& now,
      const CampaignDeliveryLimits& delivery_limits,
      const Generics::Time& time_offset,
      const RevenueDecimal& amount,
      const RevenueDecimal& daily_amount)
      noexcept;

    static void
    fill_campaign_exclude_pub_accounts_(
      AccountIdSet& exclude_pub_accounts,
      const CampaignDef* campaign,
      const StatSource::Stat::CCGStat::PublisherStatMap& publisher_amounts)
      noexcept;

    static bool
    fill_campaign_creative_weights_(
      CreativeList& creatives,
      const CampaignDef* campaign,
      const StatSource::Stat::CCGStat::CreativeStatMap* creative_stats)
      noexcept;

    static void
    fill_campaign_delivery_check_params_(
      RevenueDecimal& prev_hour_factor,
      RevenueDecimal& cur_hour_factor,
      const Generics::Time& now)
      noexcept;

    void
    fill_ccg_delivery_tags_(
      RevenueDecimal& period_adv_amount,
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
      noexcept;

    static RevenueDecimal
    eval_period_amount_(
      const AccountDef* account,
      const RevenueDecimal& prev_hour_amount,
      const RevenueDecimal& cur_hour_amount,
      const RevenueDecimal& prev_hour_comm_amount,
      const RevenueDecimal& cur_hour_comm_amount,
      const RevenueDecimal& prev_hour_factor,
      const RevenueDecimal& cur_hour_factor)
      noexcept;

    static unsigned long
    time_to_week_offset_(const Generics::Time& time)
      noexcept;

    RevenueDecimal
    eval_period_budget_limit_(
      const RevenueDecimal& daily_budget,
      const Generics::Time& today_open_time)
      const noexcept;

    static StatSource::Stat::CampaignStat*
    find_campaign_stat_(
      StatSource::Stat& stats,
      CampaignDef* campaign)
      noexcept;

    static StatSource::Stat::CCGStat*
    find_ccg_stat_(
      StatSource::Stat::CampaignStat& stats,
      unsigned long ccg_id)
      noexcept;

  private:
    Logging::Logger_var logger_;
    DeliveryCalcParams delivery_calc_params_;

    FStatSource_var stamp_stat_source_;
    FStatSource_var increment_stat_source_;
    FModifyConfigSource_var modify_config_source_;

    /* period when try to synchronize with DB,
     * if = 0 : try synchronize at each update call */
    const Generics::Time STAMP_SYNC_PERIOD_;
    const bool ENABLE_DELIVERY_THRESHOLDS_;

    mutable SyncPolicy::Mutex update_stat_lock_;
    mutable SyncPolicy::Mutex update_config_lock_;
    mutable SyncPolicy::Mutex modify_config_lock_;
    Generics::Time stamp_last_update_;
    bool full_synch_required_;

    ReferenceCounting::PtrHolder<StatSource::CStat_var> campaign_stat_;
    CModifyConfig_var modify_config_;

    ReferenceCounting::PtrHolder<State_var> state_;
  };

  typedef ReferenceCounting::SmartPtr<CampaignConfigModifier>
    CampaignConfigModifier_var;
}
}

#endif /*CAMPAIGNCONFIGMODIFIER_HPP*/
