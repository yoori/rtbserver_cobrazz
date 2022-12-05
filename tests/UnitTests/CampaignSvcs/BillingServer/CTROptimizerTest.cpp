#include <fstream>
#include <Logger/StreamLogger.hpp>
#include <Generics/AppUtils.hpp>

#include <CampaignSvcs/BillingServer/CTROptimizer.hpp>

using namespace AdServer::CampaignSvcs;

// CTROptimizerTest --goal-hours=5:0.001:21.6:3600,5:0.005:21.6:3600 --free-hours=5:0.001:21.6:1800,5:0.005:21.6:1800 --budget=24.0

int units()
{
  return 0;
}

int
parse_rate_amount_distribution(
  CTROptimizer::RateAmountDistribution& rate_amount_distribution,
  const String::SubString& str)
{
  typedef const String::AsciiStringManip::Char2Category<',', '|'>
    ListSepType;

  String::StringManip::Splitter<ListSepType> tokenizer(str);
  String::SubString token;
  while(tokenizer.get_token(token))
  {
    String::StringManip::SplitColon sub_tokenizer(token);

    String::SubString hour_str;
    String::SubString ctr_str;
    String::SubString amount_str;
    String::SubString use_time_str;
    if(!sub_tokenizer.get_token(hour_str) ||
      !sub_tokenizer.get_token(ctr_str) ||
      !sub_tokenizer.get_token(amount_str) ||
      !sub_tokenizer.get_token(use_time_str))
    {
      std::cerr << "invalid hour part '" << token << "'" << std::endl;
      return 1;
    }

    unsigned long hour;
    if(!String::StringManip::str_to_int(hour_str, hour) || hour >= 24)
    {
      std::cerr << "invalid hour value '" << hour_str << "'" << std::endl;
      return 1;
    }

    RevenueDecimal ctr;
    try
    {
      ctr = RevenueDecimal(ctr_str);
    }
    catch(const eh::Exception& ex)
    {
      std::cerr << "invalid ctr value '" << ctr_str << "'" << std::endl;
      return 1;
    }

    RevenueDecimal amount;
    try
    {
      amount = RevenueDecimal(amount_str);
    }
    catch(const eh::Exception& ex)
    {
      std::cerr << "invalid amount value '" << amount_str << "'" << std::endl;
      return 1;
    }

    unsigned long use_time_sec;
    if(!String::StringManip::str_to_int(use_time_str, use_time_sec))
    {
      std::cerr << "invalid use time value '" << use_time_str << "'" << std::endl;
      return 1;
    }

    Generics::Time use_time(use_time_sec);
    rate_amount_distribution.add(hour, ctr, amount, &use_time);
  }

  return 0;
}

int
parse_rate_hour_distribution(
  CTROptimizer::HourRateDistribution& rate_distribution,
  const String::SubString& str)
{
  typedef const String::AsciiStringManip::Char2Category<',', '|'>
    ListSepType;

  String::StringManip::Splitter<ListSepType> tokenizer(str);
  String::SubString token;
  while(tokenizer.get_token(token))
  {
    String::StringManip::SplitColon sub_tokenizer(token);

    String::SubString hour_str;
    String::SubString ctr_str;
    if(!sub_tokenizer.get_token(hour_str) ||
      !sub_tokenizer.get_token(ctr_str))
    {
      std::cerr << "invalid hour part '" << token << "'" << std::endl;
      return 1;
    }

    unsigned long hour;
    if(!String::StringManip::str_to_int(hour_str, hour) || hour >= 24)
    {
      std::cerr << "invalid hour value '" << hour_str << "'" << std::endl;
      return 1;
    }

    RevenueDecimal ctr;
    try
    {
      ctr = RevenueDecimal(ctr_str);
    }
    catch(const eh::Exception& ex)
    {
      std::cerr << "invalid ctr value '" << ctr_str << "'" << std::endl;
      return 1;
    }

    rate_distribution[hour] = ctr;
  }

  return 0;
}

int main(int argc, char** argv) noexcept
{
  Generics::AppUtils::Args args(-1);

  Generics::AppUtils::StringOption opt_goal_hours; // <hour>:<ctr>:<imps>:<clicks>:<use seconds>,...
  Generics::AppUtils::StringOption opt_free_hours;
  Generics::AppUtils::StringOption opt_actual_goal_hours;
  Generics::AppUtils::StringOption opt_actual_free_hours;
  Generics::AppUtils::StringOption opt_actual_rate_distribution;
  Generics::AppUtils::Option<RevenueDecimal> opt_budget(RevenueDecimal(false, 24, 0));
  Generics::AppUtils::Option<unsigned long> opt_hour(0);

  //Generics::AppUtils::Option<RevenueDecimal> opt_imp_cost(RevenueDecimal::ZERO);
  //Generics::AppUtils::Option<RevenueDecimal> opt_click_cost(RevenueDecimal::ZERO);

  args.add(Generics::AppUtils::equal_name("goal-hours"), opt_goal_hours);
  args.add(Generics::AppUtils::equal_name("free-hours"), opt_free_hours);
  args.add(Generics::AppUtils::equal_name("act-goal-hours"), opt_actual_goal_hours);
  args.add(Generics::AppUtils::equal_name("act-free-hours"), opt_actual_free_hours);
  args.add(Generics::AppUtils::equal_name("act-hour-rates"), opt_actual_rate_distribution);
  args.add(Generics::AppUtils::equal_name("budget"), opt_budget);
  args.add(Generics::AppUtils::equal_name("hour"), opt_hour);

  //args.add(Generics::AppUtils::equal_name("imp_cost"), opt_imp_cost);
  //args.add(Generics::AppUtils::equal_name("click_cost"), opt_click_cost);

  args.parse(argc - 1, argv + 1);
  //const Generics::AppUtils::Args::CommandList& commands = args.commands();

  // parse hours
  CTROptimizer::RateAmountDistribution past_goaled_amount_distribution;
  CTROptimizer::RateAmountDistribution past_free_amount_distribution;
  CTROptimizer::RateAmountDistribution actual_goaled_amount_distribution;
  CTROptimizer::RateAmountDistribution actual_free_amount_distribution;
  CTROptimizer::HourRateDistribution actual_rate_distribution;

  if(parse_rate_amount_distribution(past_goaled_amount_distribution, *opt_goal_hours))
  {
    return 1;
  }
  
  if(parse_rate_amount_distribution(past_free_amount_distribution, *opt_free_hours))
  {
    return 1;
  }

  if(parse_rate_amount_distribution(actual_goaled_amount_distribution, *opt_actual_goal_hours))
  {
    return 1;
  }

  if(parse_rate_amount_distribution(actual_free_amount_distribution, *opt_actual_free_hours))
  {
    return 1;
  }

  if(parse_rate_hour_distribution(actual_rate_distribution, *opt_actual_rate_distribution))
  {
    return 1;
  }

  {
    std::unique_ptr<CTROptimizer> ctr_optimizer(
      new CTROptimizer(
        RevenueDecimal(0.1), // max_underdelivery_coef : 10 %
        RevenueDecimal(3), // max goal correct coef
        RevenueDecimal(1.1) // default safe goal daily budget multiplier
        ));

    CTROptimizer::HourBudgetDistribution free_budget_distribution;
    RevenueDecimal today_budget = RevenueDecimal(false, 24, 0);
    Generics::Time now = Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d") +
      Generics::Time::ONE_HOUR * (*opt_hour);

    RevenueDecimal goal_rate;
    RevenueDecimal goal_budget;

    /*
    // 5 : 30 minutes
    Generics::Time goal_use_time_5 = Generics::Time::ONE_HOUR;
    Generics::Time free_use_time_5 = Generics::Time::ONE_MINUTE * 30;
    //Generics::Time use_time_5 = Generics::Time::ONE_HOUR;

    past_goaled_amount_distribution.add(5, RevenueDecimal("0.001"), RevenueDecimal("21.6"), &goal_use_time_5);
    past_goaled_amount_distribution.add(5, RevenueDecimal("0.005"), RevenueDecimal("21.6"), &goal_use_time_5);

    past_free_amount_distribution.add(5, RevenueDecimal("0.001"), RevenueDecimal("2.4"), &free_use_time_5);
    past_free_amount_distribution.add(5, RevenueDecimal("0.005"), RevenueDecimal("2.4"), &free_use_time_5);
    */

    std::cout << "Input:" << std::endl <<
      "  hour: " << *opt_hour << std::endl <<
      "  past_goal_amount_distribution: " << std::endl;
    past_goaled_amount_distribution.print(std::cout, "  ");
    std::cout << std::endl <<
      "  past_free_amount_distribution: " << std::endl;
    past_free_amount_distribution.print(std::cout, "  ");
    std::cout << std::endl;

    ctr_optimizer->recalculate_rate_goal(
      goal_rate, // out : goal rate
      goal_budget, // out : goal budget
      free_budget_distribution, // out : free budget distribution
      past_goaled_amount_distribution, // in : past goaled amount distribution
      past_free_amount_distribution, // in : past free amount distribution
      actual_goaled_amount_distribution, // in : today goaled amount distribution
      actual_free_amount_distribution, // in : today free amount distribution
      actual_rate_distribution, // in : today rate distribution
      today_budget,
      now);

    std::cout << std::endl << "Result:" << std::endl <<
      "  goal_rate: " << goal_rate << std::endl <<
      "  goal_budget: " << goal_budget << std::endl <<
      "  free_budget_distribution: ";
    free_budget_distribution.print(std::cout);
    std::cout << std::endl <<
      "  free budget sum: " <<
        free_budget_distribution.sum_amount() << std::endl <<
      "  free budget before hour #" << *opt_hour << ": " <<
        free_budget_distribution.sum_amount(*opt_hour) << std::endl <<
      "  free budget after hour #" << *opt_hour << ": " <<
        (free_budget_distribution.sum_amount() - free_budget_distribution.sum_amount(*opt_hour)) << std::endl <<
      std::endl;
  }

  return 0;
}
