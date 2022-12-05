#include "CTROptimizer.hpp"

//#define DEBUG_OUT_

namespace AdServer
{
namespace CampaignSvcs
{
  const RevenueDecimal
  DEFAULT_SAFE_GOAL_DAILY_BUDGET_MULTIPLIER = RevenueDecimal(1.1);

  // CTROptimizer::RevenueDecimalHolder impl
  CTROptimizer::RevenueDecimalHolder::RevenueDecimalHolder() noexcept
    : RevenueDecimal(RevenueDecimal::ZERO)
  {}

  CTROptimizer::RevenueDecimalHolder&
  CTROptimizer::RevenueDecimalHolder::operator=(const RevenueDecimal& right) noexcept
  {
    static_cast<RevenueDecimal&>(*this) = right;
    return *this;
  }

  // CTROptimizer::HourBudgetDistribution impl
  CTROptimizer::HourBudgetDistribution::HourBudgetDistribution() noexcept
  {
    resize(24);
    std::fill(begin(), end(), RevenueDecimal::ZERO);
  }

  RevenueDecimal
  CTROptimizer::HourBudgetDistribution::sum_amount(unsigned long hour)
    const noexcept
  {
    RevenueDecimal res = RevenueDecimal::ZERO;
    unsigned long hour_i = 0;
    for(auto hour_it = begin(); hour_it != end() && hour_i < hour; ++hour_it, ++hour_i)
    {
      res += *hour_it;
    }
    return res;
  }

  void
  CTROptimizer::HourBudgetDistribution::print(
    std::ostream& out,
    const char* /*prefix*/)
    const noexcept
  {
    unsigned long hour_i = 0;
    bool first_rec = true;
    out << "[ ";
    for(auto it = begin(); it != end(); ++it, ++hour_i)
    {
      if(*it != RevenueDecimal::ZERO)
      {
        if(!first_rec)
        {
          out << ", ";
        }
        out << hour_i << ":" << *it;
        first_rec = false;
      }
    }
    out << " ]";
  }

  // CTROptimizer::HourRateDistribution impl
  CTROptimizer::HourRateDistribution::HourRateDistribution() noexcept
  {
    resize(24);
    std::fill(begin(), end(), RevenueDecimal::ZERO);
  }

  void
  CTROptimizer::HourRateDistribution::print(
    std::ostream& out,
    const char* /*prefix*/)
    const noexcept
  {
    unsigned long hour_i = 0;
    out << "[ ";
    for(auto it = begin(); it != end(); ++it, ++hour_i)
    {
      if(it != begin())
      {
        out << ", ";
      }
      out << hour_i << ":" << *it;
    }
    out << " ]";
  }

  // CTROptimizer::HourAmount impl
  CTROptimizer::HourAmount::HourAmount() noexcept
    : amount(RevenueDecimal::ZERO),
      use_time(Generics::Time::ZERO)
  {}

  CTROptimizer::HourAmount::HourAmount(
    const RevenueDecimal& amount_val,
    const Generics::Time& use_time_val)
    noexcept
    : amount(amount_val),
      use_time(use_time_val)
  {}

  // CTROptimizer::HourAmountDistribution impl
  CTROptimizer::HourAmountDistribution::HourAmountDistribution()
    noexcept
  {
    amounts.resize(24);
  }

  void
  CTROptimizer::HourAmountDistribution::add(
    unsigned long hour_i,
    const RevenueDecimal& amount,
    const Generics::Time* time_in_hour)
    noexcept
  {
    HourAmount& hour_amount = amounts[hour_i];
    hour_amount.amount += amount;
    if(time_in_hour)
    {
      //assert(*time_in_hour <= Generics::Time::ONE_HOUR + Generics::Time::ONE_SECOND);
      hour_amount.use_time = std::max(hour_amount.use_time, *time_in_hour);
    }
  }

  CTROptimizer::HourAmountDistribution&
  CTROptimizer::HourAmountDistribution::operator+=(
    const CTROptimizer::HourAmountDistribution& right)
    noexcept
  {
    auto left_it = amounts.begin();
    for(auto right_it = right.amounts.begin();
      right_it != right.amounts.end(); ++right_it, ++left_it)
    {
      left_it->amount += right_it->amount;
      left_it->use_time = std::max(left_it->use_time, right_it->use_time);
    }
    return *this;
  }

  RevenueDecimal
  CTROptimizer::HourAmountDistribution::sum_amount() const noexcept
  {
    RevenueDecimal res = RevenueDecimal::ZERO;
    for(auto hour_it = amounts.begin(); hour_it != amounts.end(); ++hour_it)
    {
      res += hour_it->amount;
    }
    return res;
  }

  void
  CTROptimizer::HourAmountDistribution::print(
    std::ostream& out,
    const char* /*prefix*/)
    const noexcept
  {
    unsigned long hour_i = 0;
    bool first_rec = true;
    out << "[ ";
    for(auto it = amounts.begin(); it != amounts.end(); ++it, ++hour_i)
    {
      if(it->amount != RevenueDecimal::ZERO)
      {
        if(!first_rec)
        {
          out << ", ";
        }
        out << hour_i << ":" << it->amount << "(" << it->use_time.tv_sec << ")";
        first_rec = false;
      }
    }
    out << " ]";
  }

  // CTROptimizer::RateAmountDistribution impl
  void
  CTROptimizer::RateAmountDistribution::swap(
    RateAmountDistribution& right) noexcept
  {
    amounts.swap(right.amounts);
    rate_distributions.swap(right.rate_distributions);
    rates.swap(right.rates);
  }

  void
  CTROptimizer::RateAmountDistribution::add(
    unsigned long hour_i,
    const RevenueDecimal& rate,
    const RevenueDecimal& amount,
    const Generics::Time* time_in_hour)
    noexcept
  {
    HourAmountDistribution::add(hour_i, amount, time_in_hour);
    rate_distributions[rate].add(hour_i, amount, time_in_hour);
    rates[rate] += amount;
  }

  RevenueDecimal
  CTROptimizer::RateAmountDistribution::restricted_amount(
    const RevenueDecimal& min_rate)
    const noexcept
  {
    RevenueDecimal res = RevenueDecimal::ZERO;

    auto begin_rate_it = rate_distributions.lower_bound(min_rate);
    for(auto rate_it = begin_rate_it; rate_it != rate_distributions.end(); ++rate_it)
    {
      res += rate_it->second.sum_amount();
    }

    return res;
  }

  void
  CTROptimizer::RateAmountDistribution::print(
    std::ostream& out,
    const char* prefix)
    const noexcept
  {
    out << prefix << "{" << std::endl;
    for(auto rate_it = rate_distributions.begin(); rate_it != rate_distributions.end(); ++rate_it)
    {
      out << prefix << "  " << rate_it->first << "(sum=" << rate_it->second.sum_amount() << "): ";
      rate_it->second.print(out);
      out << std::endl;
    }
    out << prefix << "}" << std::endl;
  }

  // CTROptimizer impl
  const RevenueDecimal
  CTROptimizer::SCAN_BUDGET_SHARE = RevenueDecimal::div(
    RevenueDecimal(false, 1, 0),
    RevenueDecimal(false, 10, 0),
    Generics::DDR_FLOOR);

  CTROptimizer::CTROptimizer(
    const RevenueDecimal& max_underdelivery_coef,
    const RevenueDecimal& max_goal_correct_coef,
    const RevenueDecimal& default_safe_goal_daily_budget_multiplier)
    noexcept
    : MAX_UNDERDELIVERY_COEF_(max_underdelivery_coef),
      MAX_GOAL_CORRECT_COEF_(max_goal_correct_coef),
      DEFAULT_SAFE_GOAL_DAILY_BUDGET_MULTIPLIER_(default_safe_goal_daily_budget_multiplier)
  {
    RevenueDecimalArray standard_load_distribution;
    standard_load_distribution.resize(24);

    standard_load_distribution[0] = RevenueDecimal(0.025249);
    standard_load_distribution[1] = RevenueDecimal(0.015127);
    standard_load_distribution[2] = RevenueDecimal(0.010366);
    standard_load_distribution[3] = RevenueDecimal(0.008811);
    standard_load_distribution[4] = RevenueDecimal(0.010119);
    standard_load_distribution[5] = RevenueDecimal(0.013658);
    standard_load_distribution[6] = RevenueDecimal(0.020215);
    standard_load_distribution[7] = RevenueDecimal(0.028129);
    standard_load_distribution[8] = RevenueDecimal(0.037594);
    standard_load_distribution[9] = RevenueDecimal(0.046183);
    standard_load_distribution[10] = RevenueDecimal(0.051759);
    standard_load_distribution[11] = RevenueDecimal(0.054745);
    standard_load_distribution[12] = RevenueDecimal(0.057458);
    standard_load_distribution[13] = RevenueDecimal(0.058372);
    standard_load_distribution[14] = RevenueDecimal(0.059052);
    standard_load_distribution[15] = RevenueDecimal(0.058802);
    standard_load_distribution[16] = RevenueDecimal(0.057743);
    standard_load_distribution[17] = RevenueDecimal(0.056700);
    standard_load_distribution[18] = RevenueDecimal(0.055877);
    standard_load_distribution[19] = RevenueDecimal(0.056823);
    standard_load_distribution[20] = RevenueDecimal(0.058827);
    standard_load_distribution[21] = RevenueDecimal(0.061080);
    standard_load_distribution[22] = RevenueDecimal(0.055481);
    standard_load_distribution[23] = RevenueDecimal(0.041820);

    basic_load_distribution_ = standard_load_distribution;

    {
      RevenueDecimal basic_load_distribution_sum = RevenueDecimal::ZERO;
      for(auto b_it = basic_load_distribution_.begin();
        b_it != basic_load_distribution_.end(); ++b_it)
      {
        basic_load_distribution_sum += *b_it;
      }
    }

    // redistribute last hours
    const size_t FIRST_CORRECT_HOUR = 21;
    for(size_t correct_hour_i = FIRST_CORRECT_HOUR; correct_hour_i < 24; ++correct_hour_i)
    {
      const RevenueDecimal post_hour_portion = RevenueDecimal::div(
        basic_load_distribution_[correct_hour_i],
        RevenueDecimal(false, 10, 0),
        Generics::DDR_FLOOR);

      distribute_decimal_sum_(
        basic_load_distribution_.begin(),
        basic_load_distribution_.begin() + FIRST_CORRECT_HOUR,
        standard_load_distribution.begin(),
        standard_load_distribution.begin() + FIRST_CORRECT_HOUR,
        standard_load_distribution[correct_hour_i] - post_hour_portion);

      basic_load_distribution_[correct_hour_i] = post_hour_portion;
    }

    {
      RevenueDecimal basic_load_distribution_sum = RevenueDecimal::ZERO;
      for(auto b_it = basic_load_distribution_.begin();
        b_it != basic_load_distribution_.end(); ++b_it)
      {
        basic_load_distribution_sum += *b_it;
      }
    }
  }

  void
  CTROptimizer::recalculate_rate_goal(
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
    noexcept
  {
    goal_budget = RevenueDecimal::ZERO;

    // approximate scanning budget and under delivered budget (-> HourAmountDistribution)
    RateAmountDistribution approximated_free_amount_distribution;
    approximate_rate_amount_distribution_(
      approximated_free_amount_distribution,
      past_free_amount_distribution);

    /*
    std::cout << "approximated_free_amount_distribution = ";
    approximated_free_amount_distribution.print(std::cout);
    std::cout << std::endl;
    */

    // approximate goaled budget (-> HourAmountDistribution)
    RateAmountDistribution approximated_goaled_amount_distribution;
    approximate_rate_amount_distribution_(
      approximated_goaled_amount_distribution,
      past_goaled_amount_distribution);

    // combine past distributions
    RateAmountDistribution combined_amount_distribution;
    combined_amount_distribution.swap(approximated_free_amount_distribution);

    add_amount_distribution_(
      combined_amount_distribution,
      approximated_goaled_amount_distribution);

    // planned values at day start point (only by past info)
    HourBudgetDistribution today_planned_free_budget_distribution;
    const RevenueDecimal today_planned_scan_budget = RevenueDecimal::mul(
      today_budget,
      SCAN_BUDGET_SHARE,
      Generics::DMR_FLOOR);

    goal_budget = today_budget - today_planned_scan_budget;

#   ifdef DEBUG_OUT_
    std::cout << "today_planned_scan_budget = " << today_planned_scan_budget <<
      std::endl;
#   endif

    plan_free_budget_distribution_(
      today_planned_free_budget_distribution,
      past_free_amount_distribution,
      today_planned_scan_budget);

#   ifdef DEBUG_OUT_
    std::cout << "today_planned_free_budget_distribution(" <<
      today_planned_free_budget_distribution.sum_amount() << ") = ";
    today_planned_free_budget_distribution.print(std::cout);
    std::cout << std::endl << std::endl;
#   endif

    unsigned long now_hour = now.get_gm_time().tm_hour;

#   ifdef DEBUG_OUT_
    std::cout << "DEBUG: actual_free_amount_distribution(" <<
      actual_free_amount_distribution.sum_amount() << ") = ";
    actual_free_amount_distribution.print(std::cout);
    std::cout << std::endl << std::endl;
#   endif

    eval_actual_free_budget_distribution_(
      free_budget_distribution, // out: actual free budget distribution
      today_planned_free_budget_distribution, // planned free today budget distribution (scan budget)
      actual_free_amount_distribution, // actual free today amount distribution
      now_hour
      );

#   ifdef DEBUG_OUT_
    std::cout << "DEBUG: free_budget_distribution(" <<
      free_budget_distribution.sum_amount() << ") = ";
    free_budget_distribution.print(std::cout);
    std::cout << std::endl << std::endl;
#   endif

    // select goal ctr (HourAmountDistribution -> )
    //   input:
    //     approximated past amount distribution
    //     today amount distribution
    //   output:
    //     goal ctr, that garantee (try) delivering of budget
    //     free budget (scan) distribution for today
    //
    const RevenueDecimal goal_daily_budget = today_budget - today_planned_scan_budget;

#   ifdef DEBUG_OUT_
    std::cout << "combined_amount_distribution = ";
    combined_amount_distribution.print(std::cout);
    std::cout << std::endl;
#   endif

    /*
    std::cout << "actual_goaled_amount_distribution = ";
    actual_goaled_amount_distribution.print(std::cout);
    std::cout << std::endl;
    */

    eval_goal_rate_(
      goal_rate, // out: actual goal rate
      free_budget_distribution, // out: underdelivered budget can be here
      combined_amount_distribution, // past amount distribution (approx)
      actual_goaled_amount_distribution, // actual goaled today amount distribution
      actual_rate_distribution,
      goal_daily_budget,
      now_hour
      );

#   ifdef DEBUG_OUT_
    std::cout <<
      "free budget sum = " << free_budget_distribution.sum_amount() <<
      ", restricted amount = " <<
        combined_amount_distribution.restricted_amount(goal_rate) << std::endl;
#   endif
  }

  void
  CTROptimizer::approximate_hour_amount_distribution_(
    HourAmountDistribution& full_amount_distribution,
    const HourAmountDistribution& amount_distribution)
    noexcept
  {
    auto last_hour_it = --amount_distribution.amounts.end();
    auto res_hour_it = full_amount_distribution.amounts.begin();

    for(auto hour_it = amount_distribution.amounts.begin();
      hour_it != last_hour_it;
      ++hour_it, ++res_hour_it)
    {
      if(hour_it->use_time == Generics::Time::ZERO)
      {
        *res_hour_it = HourAmount();
      }
      else
      {
        // find next not empty hour
        auto next_hour_it = hour_it;
        ++next_hour_it;

        // if next hour use time is zero, approximate to zero (?)
        *res_hour_it = approximate_hour_lineary_(*hour_it, *next_hour_it);
      }
    }

    // approximate last hour by first hour
    *res_hour_it = last_hour_it->use_time != Generics::Time::ZERO ?
      approximate_hour_lineary_(
        *last_hour_it,
        *amount_distribution.amounts.begin()) :
      HourAmount();
  }

  void
  CTROptimizer::approximate_rate_amount_distribution_(
    RateAmountDistribution& full_amount_distribution,
    const RateAmountDistribution& amount_distribution)
    noexcept
  {
    for(auto rate_it = amount_distribution.rate_distributions.begin();
      rate_it != amount_distribution.rate_distributions.end(); ++rate_it)
    {
      HourAmountDistribution approximated_hour_amount_distribution;
      approximate_hour_amount_distribution_(
        approximated_hour_amount_distribution,
        rate_it->second);

      unsigned long hour_i = 0;
      for(auto hour_it = approximated_hour_amount_distribution.amounts.begin();
        hour_it != approximated_hour_amount_distribution.amounts.end();
          ++hour_it, ++hour_i)
      {
        full_amount_distribution.add(
          hour_i,
          rate_it->first,
          hour_it->amount,
          &Generics::Time::ONE_HOUR);
      }
    }
  }

  /*
  void
  CTROptimizer::plan_goaled_budget_distribution_(
    RevenueDecimal& today_planned_goal_rate,
    HourBudgetDistribution& today_planned_goaled_budget_distribution,
    const RateAmountDistribution& combined_amount_distribution)
    noexcept
  {
  }
  */

  void
  CTROptimizer::plan_free_budget_distribution_(
    HourBudgetDistribution& full_amount_distribution,
    const HourAmountDistribution& prev_amount_distribution,
    const RevenueDecimal& budget)
    noexcept
  {
    // redistribute 25% by standard distribution
    // allow to fill stopped hours ...
    //
    auto res_hour_it = full_amount_distribution.begin();

    RevenueDecimal prev_amount_sum = RevenueDecimal::ZERO;
    for(auto hour_it = prev_amount_distribution.amounts.begin();
      hour_it != prev_amount_distribution.amounts.end();
      ++hour_it)
    {
      prev_amount_sum += hour_it->amount;
    }

    RevenueDecimal redistribute_budget;

    if(prev_amount_sum != RevenueDecimal::ZERO)
    {
      redistribute_budget = RevenueDecimal::ZERO;

      for(auto hour_it = prev_amount_distribution.amounts.begin();
        hour_it != prev_amount_distribution.amounts.end();
        ++hour_it, ++res_hour_it)
      {
        const RevenueDecimal normalized_hour_amount = RevenueDecimal::mul(
          RevenueDecimal::div(
            hour_it->amount,
            prev_amount_sum,
            Generics::DDR_FLOOR),
          budget,
          Generics::DMR_FLOOR);

        const RevenueDecimal redistribute_hour_portion = RevenueDecimal::div(
          normalized_hour_amount,
          RevenueDecimal(false, 4, 0),
          Generics::DDR_FLOOR);

        redistribute_budget += redistribute_hour_portion;
        *res_hour_it += normalized_hour_amount - redistribute_hour_portion;
      }
    }
    else
    {
      redistribute_budget = budget;
    }

#   ifdef DEBUG_OUT_
    std::cout << "CTROptimizer::plan_free_budget_distribution_(): "
      "to redistribute (redistribute_budget = " << redistribute_budget << "): ";
    full_amount_distribution.print(std::cout);
    std::cout << std::endl;
#   endif

    distribute_decimal_sum_(
      full_amount_distribution.begin(),
      full_amount_distribution.end(),
      basic_load_distribution_.begin(),
      basic_load_distribution_.end(),
      redistribute_budget);

#   ifdef DEBUG_OUT_
    std::cout << "CTROptimizer::plan_free_budget_distribution_(): "
      "redistributed (redistribute_budget = " << redistribute_budget << "): ";
    full_amount_distribution.print(std::cout);
    std::cout << std::endl;
#   endif
  }

  void
  CTROptimizer::add_amount_distribution_(
    RateAmountDistribution& combined_amount_distribution,
    const RateAmountDistribution& add_amount_distribution)
    noexcept
  {
    for(auto rate_it = add_amount_distribution.rate_distributions.begin();
      rate_it != add_amount_distribution.rate_distributions.end(); ++rate_it)
    {
      auto res_rate_it = combined_amount_distribution.rate_distributions.find(rate_it->first);

      if(res_rate_it != combined_amount_distribution.rate_distributions.end())
      {
        HourAmountDistribution& res_hour_amount_distribution = res_rate_it->second;
        auto res_hour_it = res_hour_amount_distribution.amounts.begin();

        for(auto hour_it = rate_it->second.amounts.begin();
          hour_it != rate_it->second.amounts.end();
          ++hour_it, ++res_hour_it)
        {
          res_hour_it->amount += hour_it->amount;
          res_hour_it->use_time = std::max(res_hour_it->use_time, hour_it->use_time);
        }
      }
      else
      {
        combined_amount_distribution.rate_distributions.insert(*rate_it);
      }
    }
  }

  void
  CTROptimizer::eval_actual_free_budget_distribution_(
    HourBudgetDistribution& free_budget_distribution,
    const HourBudgetDistribution& today_planned_free_budget_distribution,
    const RateAmountDistribution& actual_free_amount_distribution,
    unsigned long now_hour)
    noexcept
  {
    // eval delivered free amount
    RevenueDecimal actual_delivered_free_amount = RevenueDecimal::ZERO;
    RevenueDecimal planned_free_amount = RevenueDecimal::ZERO;

    for(unsigned long hour_i = 0; hour_i < 24; ++hour_i)
    {
      if(hour_i < now_hour)
      {
        actual_delivered_free_amount +=
          actual_free_amount_distribution.amounts[hour_i].amount;
        planned_free_amount +=
          today_planned_free_budget_distribution[hour_i];
      }

      free_budget_distribution[hour_i] +=
        today_planned_free_budget_distribution[hour_i];
    }

#   ifdef DEBUG_OUT_
    std::cerr << "TT> actual_delivered_free_amount = " << actual_delivered_free_amount << std::endl <<
      "TT> planned_free_amount = " << planned_free_amount << std::endl;
#   endif

    if(actual_delivered_free_amount < planned_free_amount)
    {
      // add underdelivered amount to current hour
      free_budget_distribution[now_hour] +=
        planned_free_amount - actual_delivered_free_amount;
    }
    else if(planned_free_amount < actual_delivered_free_amount)
    {
      // decrease overdelivered 
      distribute_decimal_sum_(
        free_budget_distribution.begin() + now_hour,
        free_budget_distribution.end(),
        basic_load_distribution_.begin() + now_hour,
        basic_load_distribution_.end(),
        planned_free_amount - actual_delivered_free_amount // negative decrease
        );
    }
  }

  void
  CTROptimizer::eval_goal_rate_(
    RevenueDecimal& goal_rate,
    HourBudgetDistribution& free_budget_distribution,
    const RateAmountDistribution& past_amount_distribution,
    const RateAmountDistribution& actual_goaled_amount_distribution,
    const HourRateDistribution& actual_rate_distribution,
    const RevenueDecimal& goal_daily_budget,
    unsigned long now_hour)
    noexcept
  {
    if(goal_daily_budget == RevenueDecimal::ZERO)
    {
      goal_rate = RevenueDecimal::ZERO;
      return;
    }

    // eval actual delivered goaled amount
    RevenueDecimal actual_delivered_goaled_amount = RevenueDecimal::ZERO;

    for(unsigned long hour_i = 0; hour_i < now_hour; ++hour_i)
    {
      actual_delivered_goaled_amount +=
        actual_goaled_amount_distribution.amounts[hour_i].amount;
    }

    // eval planned amount
    RevenueDecimal planned_goaled_amount = RevenueDecimal::ZERO;

    for(auto rate_it = past_amount_distribution.rate_distributions.rbegin();
      rate_it != past_amount_distribution.rate_distributions.rend();
      ++rate_it)
    {
      unsigned long hour_i = 0;

      for(auto hour_it = rate_it->second.amounts.begin();
          hour_it != rate_it->second.amounts.end() &&
            hour_i < now_hour;
          ++hour_it, ++hour_i)
      {
        if(rate_it->first >= actual_rate_distribution[hour_i])
        {
          planned_goaled_amount += hour_it->amount;
        }
      }
    }

    planned_goaled_amount = std::min(planned_goaled_amount, goal_daily_budget);

    // check planned budget execution - under delivered part should considered with great coef
    // eval "start point" distribution - planned at day beginning
    RevenueDecimal free_budget_reminder = RevenueDecimal::ZERO;
    RevenueDecimal safe_goal_daily_budget_multiplier;

    if(planned_goaled_amount != RevenueDecimal::ZERO &&
        actual_delivered_goaled_amount < planned_goaled_amount)
    {
      try
      {
        // if underdelivered more then 10% of daily budget - correct multiplier and
        // push underdelivered part to free
        RevenueDecimal max_underdelivery_budget = RevenueDecimal::mul(
          goal_daily_budget,
          MAX_UNDERDELIVERY_COEF_,
          Generics::DMR_FLOOR);

        if(now_hour >= 20 || //
          planned_goaled_amount - actual_delivered_goaled_amount > max_underdelivery_budget)
        {
          // underdelivering coef = actual_delivered_goaled_amount / planned_goaled_amount
          // set safe_goal_daily_budget_multiplier = min(1 / (underdelivering coef), MAX_GOAL_CORRECT_COEF)
          // other underdelivered part push into free budget

          if(actual_delivered_goaled_amount == RevenueDecimal::ZERO)
          {
            safe_goal_daily_budget_multiplier = MAX_GOAL_CORRECT_COEF_;
          }
          else
          {
            safe_goal_daily_budget_multiplier = std::min(
              // (<planned amount> + <underdelivered amount>) / <actual delivered amount>
              RevenueDecimal::div(
                planned_goaled_amount + planned_goaled_amount - actual_delivered_goaled_amount,
                actual_delivered_goaled_amount,
                Generics::DDR_FLOOR),
              MAX_GOAL_CORRECT_COEF_);
          }

          free_budget_reminder = planned_goaled_amount - actual_delivered_goaled_amount;
        }
        else
        {
          safe_goal_daily_budget_multiplier = DEFAULT_SAFE_GOAL_DAILY_BUDGET_MULTIPLIER_;
        }
      }
      catch(const RevenueDecimal::Overflow&)
      {
        safe_goal_daily_budget_multiplier = MAX_GOAL_CORRECT_COEF_;
        free_budget_reminder = planned_goaled_amount - actual_delivered_goaled_amount;
      }
    }
    else
    {
      safe_goal_daily_budget_multiplier = DEFAULT_SAFE_GOAL_DAILY_BUDGET_MULTIPLIER_;
    }

    free_budget_reminder = std::max(free_budget_reminder, RevenueDecimal::ZERO);

    RevenueDecimal safe_goal_daily_budget = RevenueDecimal::mul(
      std::max(goal_daily_budget - actual_delivered_goaled_amount, RevenueDecimal::EPSILON),
      safe_goal_daily_budget_multiplier,
      Generics::DMR_CEIL);

#   ifdef DEBUG_OUT_
    std::cout << "DEBUG: eval_goal_rate_(): " << std::endl <<
      "  planned_goaled_amount = " << planned_goaled_amount << std::endl <<
      "  goal_daily_budget = " << goal_daily_budget << std::endl <<
      "  actual_delivered_goaled_amount = " << actual_delivered_goaled_amount << std::endl <<
      "  free_budget_reminder = " << free_budget_reminder << std::endl <<
      "  safe_goal_daily_budget_multiplier = " << safe_goal_daily_budget_multiplier << std::endl <<
      "  safe_goal_daily_budget = " << safe_goal_daily_budget << std::endl <<
      std::endl;
#   endif

    // select rate goal by safe_goal_daily_budget
    RevenueDecimal cur_goal_rate = RevenueDecimal::ZERO;
    HourAmountDistribution today_planned_goaled_budget_distribution;
    RevenueDecimal cur_goal_amount = RevenueDecimal::ZERO;

    for(auto rate_it = past_amount_distribution.rate_distributions.rbegin();
      rate_it != past_amount_distribution.rate_distributions.rend();
      ++rate_it)
    {
      today_planned_goaled_budget_distribution += rate_it->second;

      auto hour_it = rate_it->second.amounts.begin();
      std::advance(hour_it, now_hour);
      for(; hour_it != rate_it->second.amounts.end(); ++hour_it)
      {
        cur_goal_amount += hour_it->amount;
      }

      if(cur_goal_amount >= safe_goal_daily_budget)
      {
        cur_goal_rate = rate_it->first;
        break;
      }
    }

    goal_rate = cur_goal_rate;
    free_budget_distribution[now_hour] += free_budget_reminder;

#   ifdef DEBUG_OUT_
    std::cout << "DEBUG: eval_goal_rate_(): " << std::endl <<
      "  cur_goal_amount = " << cur_goal_amount << std::endl <<
      "  free_budget_reminder = " << free_budget_reminder <<
      std::endl;
#   endif
  }

  CTROptimizer::HourAmount
  CTROptimizer::approximate_hour_lineary_(
    const HourAmount& left,
    const HourAmount& right)
    noexcept
  {
    const RevenueDecimal cur_sec_amount = RevenueDecimal::div(
      left.amount,
      RevenueDecimal(false, left.use_time.tv_sec, 0),
      Generics::DDR_FLOOR);

    const RevenueDecimal next_sec_amount =
      right.use_time != Generics::Time::ZERO ?
      RevenueDecimal::div(
        right.amount,
        RevenueDecimal(false, right.use_time.tv_sec, 0),
        Generics::DDR_FLOOR) :
      RevenueDecimal::ZERO;

    const RevenueDecimal hour_add_amount = RevenueDecimal::mul(
      RevenueDecimal::div(
        cur_sec_amount + next_sec_amount,
        RevenueDecimal(false, 2, 0),
        Generics::DDR_FLOOR),
      RevenueDecimal(
        false,
        (Generics::Time::ONE_HOUR - left.use_time).tv_sec,
        0),
      Generics::DMR_FLOOR);

    return HourAmount(left.amount + hour_add_amount, Generics::Time::ONE_HOUR);
  }

  template<typename ResultIteratorType, typename IteratorType>
  void
  CTROptimizer::distribute_decimal_sum_(
    ResultIteratorType res_begin_it,
    ResultIteratorType res_end_it,
    IteratorType begin_it,
    IteratorType end_it,
    const RevenueDecimal& sum)
    noexcept
  {
    RevenueDecimal coef_sum = RevenueDecimal::ZERO;

    for(auto it = begin_it; it != end_it; ++it)
    {
      coef_sum += *it;
    }

    if(coef_sum != RevenueDecimal::ZERO)
    {
      const RevenueDecimal normalizer = RevenueDecimal::div(
        RevenueDecimal(false, 1, 0),
        coef_sum,
        Generics::DDR_FLOOR);

      for(; res_begin_it != res_end_it && begin_it != end_it; ++res_begin_it, ++begin_it)
      {
        *res_begin_it += RevenueDecimal::mul(
          RevenueDecimal::mul(sum, *begin_it, Generics::DMR_FLOOR),
          normalizer,
          Generics::DMR_FLOOR);

        if(*res_begin_it <= RevenueDecimal::ZERO)
        {
          *res_begin_it = RevenueDecimal::ZERO;
        }
      }
    }
  }
}
}

