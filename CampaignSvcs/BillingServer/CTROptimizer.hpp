#ifndef CAMPAIGNSVCS_CTROPTIMIZER_HPP
#define CAMPAIGNSVCS_CTROPTIMIZER_HPP

#include <vector>

#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/PtrHolder.hpp>

#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>

namespace AdServer
{
namespace CampaignSvcs
{
  class CTROptimizer
  {
  public:
    static const RevenueDecimal SCAN_BUDGET_SHARE;

    // fill scan budget "timeline"
    // approximate scan budget to full
    // approximate goaled budget
    // get today exp ctr budget distribution by
    //   union if approximated scan & goaled


    // how to reconstruct exp amount by scan budget
    // 1) proportionaly to sent amount
    
    // keep imps, clicks for convert to amount with actual cost's
    // keep hour distribution for adapt between hours

    // in daily budget
    // yesterday ctr,amount distribution
    // today ctr,amount distribution
    // current time

    struct RevenueDecimalHolder: public RevenueDecimal
    {
      RevenueDecimalHolder() noexcept;

      RevenueDecimalHolder&
      operator=(const RevenueDecimal& right) noexcept;
    };

    // budget
    typedef RevenueDecimalHolder HourBudget;

    struct HourBudgetDistribution: std::vector<HourBudget>
    {
      HourBudgetDistribution() noexcept;

      RevenueDecimal
      sum_amount(unsigned long hour = 24) const noexcept;

      void
      print(std::ostream& out, const char* prefix = "") const noexcept;
    };

    struct HourRateDistribution: std::vector<RevenueDecimal>
    {
      HourRateDistribution() noexcept;

      void
      print(std::ostream& out, const char* prefix = "") const noexcept;
    };

    // amount struct's
    struct HourAmount
    {
      // rollback don't increase use_time, but rollback amount
      HourAmount() noexcept;

      HourAmount(const RevenueDecimal& amount_val, const Generics::Time& use_time_val)
        noexcept;

      // used amount
      RevenueDecimal amount;
      // part of hour used for sent amount
      Generics::Time use_time;
    };

    struct HourAmountDistribution
    {
      HourAmountDistribution() noexcept;

      void
      add(unsigned long hour_i,
        const RevenueDecimal& amount,
        const Generics::Time* time_in_hour)
        noexcept;

      HourAmountDistribution&
      operator+=(const HourAmountDistribution& right) noexcept;

      RevenueDecimal
      sum_amount() const noexcept;

      void
      print(std::ostream& out, const char* prefix = "") const noexcept;

      // hour amounts
      std::vector<HourAmount> amounts;
    };

    struct RateAmountDistribution: public HourAmountDistribution
    {
      typedef std::map<RevenueDecimal, RevenueDecimalHolder>
        RateAmountMap;

      typedef std::map<RevenueDecimal, HourAmountDistribution>
        RateAmountTimeDistributionMap;

      void
      swap(RateAmountDistribution& right) noexcept;

      void
      add(unsigned long hour_i,
        const RevenueDecimal& rate,
        const RevenueDecimal& amount,
        const Generics::Time* time_in_hour)
        noexcept;

      RevenueDecimal
      restricted_amount(const RevenueDecimal& min_rate)
        const noexcept;

      void
      print(std::ostream& out, const char* prefix = "") const noexcept;

      // ctr => hour amount distribution
      RateAmountTimeDistributionMap rate_distributions;
      // ctr => amount
      RateAmountMap rates;
    };

  public:
    CTROptimizer(
      const RevenueDecimal& max_underdelivery_coef,
      const RevenueDecimal& max_goal_correct_coef,
      const RevenueDecimal& default_safe_goal_daily_budget_multiplier)
      noexcept;

    //
    void
    recalculate_rate_goal(
      RevenueDecimal& goal_rate,
      RevenueDecimal& goal_budget,
      HourBudgetDistribution& free_budget_distribution,
      const RateAmountDistribution& past_goaled_amount_distribution,
      const RateAmountDistribution& past_free_amount_distribution,
      const RateAmountDistribution& actual_goaled_amount_distribution,
      const RateAmountDistribution& actual_free_amount_distribution,
      const HourRateDistribution& actual_rate_distribution,
      const RevenueDecimal& today_budget,
      const Generics::Time& now)
      noexcept;

  protected:
    typedef std::vector<RevenueDecimal> RevenueDecimalArray;

  protected:
    static void
    approximate_hour_amount_distribution_(
      HourAmountDistribution& full_amount_distribution,
      const HourAmountDistribution& amount_distribution)
      noexcept;

    static void
    approximate_rate_amount_distribution_(
      RateAmountDistribution& full_amount_distribution,
      const RateAmountDistribution& amount_distribution)
      noexcept;

    /*
    void
    plan_goaled_budget_distribution_(
      RevenueDecimal& today_planned_goal_rate,
      HourBudgetDistribution& today_planned_goaled_budget_distribution,
      const RateAmountDistribution& combined_amount_distribution)
      noexcept;
    */

    void
    plan_free_budget_distribution_(
      HourBudgetDistribution& full_amount_distribution,
      const HourAmountDistribution& prev_amount_distribution,
      const RevenueDecimal& free_budget)
      noexcept;

    void
    eval_actual_free_budget_distribution_(
      HourBudgetDistribution& free_budget_distribution,
      const HourBudgetDistribution& today_planned_free_budget_distribution,
      const RateAmountDistribution& actual_free_amount_distribution,
      unsigned long now_hour)
      noexcept;

    void
    eval_goal_rate_(
      RevenueDecimal& goal_rate,
      HourBudgetDistribution& free_budget_distribution,
      const RateAmountDistribution& approx_amount_distribution,
      const RateAmountDistribution& actual_goaled_amount_distribution,
      const HourRateDistribution& actual_rate_distribution,
      const RevenueDecimal& daily_budget,
      unsigned long cur_hour)
      noexcept;

    // help methods
    void
    add_amount_distribution_(
      RateAmountDistribution& combined_amount_distribution,
      const RateAmountDistribution& add_amount_distribution)
      noexcept;

    static HourAmount
    approximate_hour_lineary_(
      const HourAmount& left,
      const HourAmount& right)
      noexcept;

    template<typename ResultIteratorType, typename IteratorType>
    static void
    distribute_decimal_sum_(
      ResultIteratorType res_begin_it,
      ResultIteratorType res_end_it,
      IteratorType begin_it,
      IteratorType end_it,
      const RevenueDecimal& sum)
      noexcept;

  protected:
    const RevenueDecimal MAX_UNDERDELIVERY_COEF_;
    const RevenueDecimal MAX_GOAL_CORRECT_COEF_;
    const RevenueDecimal DEFAULT_SAFE_GOAL_DAILY_BUDGET_MULTIPLIER_;

    RevenueDecimalArray basic_load_distribution_;
  };
}
}

#endif /*CAMPAIGNSVCS_CTROPTIMIZER_HPP*/
