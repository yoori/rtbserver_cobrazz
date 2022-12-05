#ifndef CAMPAIGNSVCS_BILLINGCONTAINERSTATE_HPP
#define CAMPAIGNSVCS_BILLINGCONTAINERSTATE_HPP

#include <map>

#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/PtrHolder.hpp>
#include <Sync/SyncPolicy.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>

#include "CTROptimizer.hpp"

namespace AdServer
{
namespace CampaignSvcs
{
  // BillingContainerState
  struct BillingContainerState: public ReferenceCounting::AtomicImpl
  {
  public:
    struct AccountDayAmount
    {
      AccountDayAmount()
        : amount_before(RevenueDecimal::ZERO),
          amount(RevenueDecimal::ZERO)
      {}

      // amount_before + amount = total amount on this day
      RevenueDecimal amount_before;
      RevenueDecimal amount;
    };

    typedef std::map<Generics::Time, AccountDayAmount> AccountDayAmountMap;

    struct AccountAmountHolder: public ReferenceCounting::AtomicCopyImpl
    {
      AccountAmountHolder();

      // pack amounts before defined date
      void
      pack(const Generics::Time& date)
        noexcept;

      void
      add_prev_days_amount(
        const RevenueDecimal& amount)
        noexcept;

      bool
      add_amount(
        const Generics::Time& date,
        const RevenueDecimal& amount)
        noexcept;

      RevenueDecimal
      get_total_amount() const /*throw(RevenueDecimal::Overflow)*/;

      RevenueDecimal
      get_day_amount(const Generics::Time& date) const noexcept;

      void
      print(std::ostream& ostr, const char* prefix) const noexcept;

      AccountDayAmountMap days;

    protected:
      virtual ~AccountAmountHolder() noexcept = default;
    };

    typedef ReferenceCounting::SmartPtr<AccountAmountHolder>
      AccountAmountHolder_var;

    typedef std::map<unsigned long, AccountAmountHolder_var>
      AccountIdAmountMap;

    struct DayAmount
    {
      DayAmount()
        : amount_before(RevenueDecimal::ZERO),
          amount(RevenueDecimal::ZERO),
          imps_before(0),
          imps(0),
          clicks_before(0),
          clicks(0)
      {}

      // amount_before + amount = total amount on this day
      RevenueDecimal amount_before;
      RevenueDecimal amount;

      ImpRevenueDecimal imps_before;
      ImpRevenueDecimal imps;

      ImpRevenueDecimal clicks_before;
      ImpRevenueDecimal clicks;
    };

    typedef std::map<Generics::Time, DayAmount> DayAmountMap;

    struct AmountHolder: public ReferenceCounting::AtomicCopyImpl
    {
      AmountHolder();

      // pack amounts before defined date
      void
      pack(const Generics::Time& date)
        noexcept;

      void
      add_prev_days_amount(
        const RevenueDecimal& amount,
        const ImpRevenueDecimal& imps,
        const ImpRevenueDecimal& clicks)
        noexcept;

      bool
      add_amount(
        const Generics::Time& date,
        const RevenueDecimal& amount,
        const ImpRevenueDecimal& imps,
        const ImpRevenueDecimal& clicks)
        noexcept;

      RevenueDecimal
      get_total_amount() const /*throw(RevenueDecimal::Overflow)*/;

      ImpRevenueDecimal
      get_total_imps() const;

      ImpRevenueDecimal
      get_total_clicks() const;

      RevenueDecimal
      get_day_amount(const Generics::Time& date) const noexcept;

      ImpRevenueDecimal
      get_day_imps(const Generics::Time& date) const noexcept;

      ImpRevenueDecimal
      get_day_clicks(const Generics::Time& date) const noexcept;

      void
      print(std::ostream& ostr, const char* prefix) const noexcept;

      //RevenueDecimal prev_days_amount;
      DayAmountMap days;

    protected:
      virtual ~AmountHolder() noexcept = default;
    };

    typedef ReferenceCounting::SmartPtr<AmountHolder>
      AmountHolder_var;

    typedef std::map<unsigned long, AmountHolder_var>
      IdAmountMap;

    struct HourActivity
    {
      HourActivity() noexcept;

      ImpRevenueDecimal imps;
      ImpRevenueDecimal clicks;
      Generics::Time use_time;
    };

    struct HourActivityDistribution
    {
      HourActivityDistribution() noexcept;

      std::vector<HourActivity> hours;
    };

    struct RateDistributionHolder
    {
      // ctr -> imps/clicks distribution
      typedef std::map<RevenueDecimal, HourActivityDistribution>
        RateHourActivityDistributionMap;

      void
      add(unsigned long hour,
        const RevenueDecimal& rate,
        bool free_budget,
        const ImpRevenueDecimal& imps,
        const ImpRevenueDecimal& clicks,
        const Generics::Time& use_time)
        noexcept;

      bool
      empty() const noexcept;

      void
      save(std::ostream& out) const
        noexcept;

      void
      load(const String::SubString& str);

      // ctr division
      RateHourActivityDistributionMap rates;
      RateHourActivityDistributionMap free_rates;

    protected:
      static void save_(
        std::ostream& out,
        bool& first_rec,
        const RateHourActivityDistributionMap& save_rates,
        char type)
        noexcept;
    };

    typedef std::map<Generics::Time, RateDistributionHolder>
      DateRateDistributionHolderMap;

    struct CampaignCCGId
    {
      CampaignCCGId() noexcept;

      CampaignCCGId(
        unsigned long campaign_id_val,
        unsigned long ccg_id_val)
        noexcept;

      bool
      operator<(const CampaignCCGId& right) const
        noexcept;

      unsigned long campaign_id;
      unsigned long ccg_id;
    };

    struct RateGoalHolder
    {
      RateGoalHolder() noexcept;

      // hour optimization fields
      Generics::Time last_eval_min_rate_goal_time;
      RevenueDecimal min_rate_goal;

      // free budget, hour division
      CTROptimizer::HourBudgetDistribution free_budget_distribution;
      CTROptimizer::HourAmountDistribution free_amount_distribution;
    };

    typedef std::map<unsigned long, RateGoalHolder>
      RateGoalMap;

    // RateAmountHolder
    struct RateAmountHolder
    {
      RateAmountHolder() noexcept;

      // confirm fill fields
      // free_amount_distribution : aggregate of free_rates
      DateRateDistributionHolderMap dates;
    };

    typedef std::map<CampaignCCGId, RateAmountHolder>
      RateAmountMap;

    // RateOptimizationHolder
    struct RateOptimizationHolder
    {
    public:
      struct DateHolder
      {
        bool
        empty() const noexcept;

        void
        save(std::ostream& out) const
          noexcept;

        void
        load(const String::SubString& str);

        CTROptimizer::HourRateDistribution min_rates;
      };

      typedef std::map<Generics::Time, DateHolder>
        DateMap;

    public:
      DateMap dates;
    };

    typedef std::map<unsigned long, RateOptimizationHolder>
      RateOptimizationMap;

    typedef Sync::Policy::PosixThreadRW SyncPolicy;

  public:
    mutable SyncPolicy::Mutex accounts_lock;
    AccountIdAmountMap accounts;

    mutable SyncPolicy::Mutex campaigns_lock;
    IdAmountMap campaigns;

    mutable SyncPolicy::Mutex ccgs_lock;
    IdAmountMap ccgs;

    // evaluated goals holder (read on check, modify on optimization)
    mutable SyncPolicy::Mutex campaign_rate_goals_lock;
    RateGoalMap campaign_rate_goals;

    // optimization additional info by date holder (modify on optimization)
    mutable SyncPolicy::Mutex campaign_rate_opts_lock;
    RateOptimizationMap campaign_rate_opts;

    // amount by rate holder (read on check, modify on confirm)
    mutable SyncPolicy::Mutex ccg_rate_amounts_lock;
    RateAmountMap ccg_rate_amounts;

  public:
    static void
    convert_rate_amount(
      CTROptimizer::RateAmountDistribution& result,
      const RateDistributionHolder::RateHourActivityDistributionMap& rates,
      const RevenueDecimal& imp_amount,
      const RevenueDecimal& click_amount,
      const RevenueDecimal& factual_coef, // weight of actual amount
      const RevenueDecimal& click_rate_coef, // weight of click rate
      const RevenueDecimal& noise_ignore_part,
      const RevenueDecimal& rate_multiplier
      )
      noexcept;

    static void
    convert_rate_amount_distribution(
      CTROptimizer::RateAmountDistribution& free_result,
      CTROptimizer::RateAmountDistribution& result,
      const RateDistributionHolder& source,
      const RevenueDecimal& imp_amount,
      const RevenueDecimal& click_amount,
      const RevenueDecimal& factual_coef, // weight of actual amount
      const RevenueDecimal& click_rate_coef, // weight of click rate
      const RevenueDecimal& noise_ignore_part,
      const RevenueDecimal& rate_multiplier
      )
      noexcept;

  protected:
    virtual ~BillingContainerState() noexcept = default;
  };
}
}

#endif /*CAMPAIGNSVCS_BILLINGCONTAINERSTATE_HPP*/
