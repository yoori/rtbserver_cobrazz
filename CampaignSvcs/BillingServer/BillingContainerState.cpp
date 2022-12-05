#include "BillingContainerState.hpp"

namespace AdServer
{
namespace CampaignSvcs
{
  // BillingContainerState::AccountAmountHolder impl
  BillingContainerState::AccountAmountHolder::AccountAmountHolder()
  {}

  // pack amounts before defined date
  void
  BillingContainerState::AccountAmountHolder::pack(
    const Generics::Time& date)
    noexcept
  {
    auto start_it = days.lower_bound(date);
    RevenueDecimal sum_amount = RevenueDecimal::ZERO;

    for(auto it = days.begin(); it != days.end(); ++it)
    {
      sum_amount += it->second.amount;
    }

    days.erase(days.begin(), start_it);

    for(auto it = days.begin(); it != days.end(); ++it)
    {
      it->second.amount_before += sum_amount;
    }
  }

  void
  BillingContainerState::AccountAmountHolder::add_prev_days_amount(
    const RevenueDecimal& amount)
    noexcept
  {
    for(auto it = days.begin(); it != days.end(); ++it)
    {
      it->second.amount_before += amount;
    }
  }

  bool
  BillingContainerState::AccountAmountHolder::add_amount(
    const Generics::Time& date,
    const RevenueDecimal& amount)
    noexcept
  {
    auto ins = days.insert(std::make_pair(
      date, BillingContainerState::AccountDayAmount()));

    auto it = ins.first;

    try
    {
      ins.first->second.amount += amount;

      if(ins.second && ins.first != days.begin())
      {
        auto before_it = ins.first;
        --before_it;

        ins.first->second.amount_before +=
          before_it->second.amount_before + before_it->second.amount;
      }

      for(++it; it != days.end(); ++it)
      {
        it->second.amount_before += amount;
      }
    }
    catch(const RevenueDecimal::Overflow&)
    {
      // revert all changes
      ins.first->second.amount -= amount;

      for(; it != ins.first; --it)
      {
        it->second.amount_before -= amount;
      }

      if(ins.second)
      {
        // erase new inserted element (overflow can be on amount_before init)
        days.erase(ins.first);
      }

      return false;
    }

    return true;
  }

  RevenueDecimal
  BillingContainerState::AccountAmountHolder::get_total_amount()
    const
    /*throw(RevenueDecimal::Overflow)*/
  {
    return !days.empty() ?
      days.rbegin()->second.amount_before + days.rbegin()->second.amount :
      RevenueDecimal::ZERO;
  }

  RevenueDecimal
  BillingContainerState::AccountAmountHolder::get_day_amount(
    const Generics::Time& date)
    const
    noexcept
  {
    auto it = days.find(date);
    if(it != days.end())
    {
      return it->second.amount;
    }

    return RevenueDecimal::ZERO;
  }

  void
  BillingContainerState::AccountAmountHolder::print(
    std::ostream& out,
    const char* offset)
    const noexcept
  {
    for(auto day_it = days.begin(); day_it != days.end(); ++day_it)
    {
      out << offset << day_it->first.get_gm_time().format("%F") <<
        ": amount_before = " << day_it->second.amount_before <<
        ", amount = " << day_it->second.amount <<
        std::endl;
    }
  }

  // BillingContainerState::AmountHolder impl
  BillingContainerState::AmountHolder::AmountHolder()
  {}

  // pack amounts before defined date
  void
  BillingContainerState::AmountHolder::pack(
    const Generics::Time& date)
    noexcept
  {
    auto start_it = days.lower_bound(date);
    RevenueDecimal sum_amount = RevenueDecimal::ZERO;
    ImpRevenueDecimal sum_imps = ImpRevenueDecimal::ZERO;
    ImpRevenueDecimal sum_clicks = ImpRevenueDecimal::ZERO;

    for(auto it = days.begin(); it != days.end(); ++it)
    {
      sum_amount += it->second.amount;
      sum_imps += it->second.imps;
      sum_clicks += it->second.clicks;
    }

    days.erase(days.begin(), start_it);

    for(auto it = days.begin(); it != days.end(); ++it)
    {
      it->second.amount_before += sum_amount;
      it->second.imps_before += sum_imps;
      it->second.clicks_before += sum_clicks;
    }
  }

  void
  BillingContainerState::AmountHolder::add_prev_days_amount(
    const RevenueDecimal& amount,
    const ImpRevenueDecimal& imps,
    const ImpRevenueDecimal& clicks)
    noexcept
  {
    for(auto it = days.begin(); it != days.end(); ++it)
    {
      it->second.amount_before += amount;
      it->second.imps_before += imps;
      it->second.clicks_before += clicks;
    }
  }

  bool
  BillingContainerState::AmountHolder::add_amount(
    const Generics::Time& date,
    const RevenueDecimal& amount,
    const ImpRevenueDecimal& imps,
    const ImpRevenueDecimal& clicks)
    noexcept
  {
    auto ins = days.insert(std::make_pair(
      date, BillingContainerState::DayAmount()));

    auto it = ins.first;

    try
    {
      ins.first->second.amount += amount;
      ins.first->second.imps += imps;
      ins.first->second.clicks += clicks;

      if(ins.second && ins.first != days.begin())
      {
        auto before_it = ins.first;
        --before_it;

        ins.first->second.amount_before +=
          before_it->second.amount_before + before_it->second.amount;
        ins.first->second.imps_before +=
          before_it->second.imps_before + before_it->second.imps;
        ins.first->second.clicks_before +=
          before_it->second.clicks_before + before_it->second.clicks;
      }

      for(++it; it != days.end(); ++it)
      {
        it->second.amount_before += amount;
        it->second.imps_before += imps;
        it->second.clicks_before += clicks;
      }
    }
    catch(const RevenueDecimal::Overflow&)
    {
      // revert all changes
      ins.first->second.amount -= amount;
      ins.first->second.imps -= imps;
      ins.first->second.clicks -= clicks;

      for(; it != ins.first; --it)
      {
        it->second.amount_before -= amount;
        it->second.imps_before -= imps;
        it->second.clicks_before -= clicks;
      }

      if(ins.second)
      {
        // erase new inserted element (overflow can be on amount_before init)
        days.erase(ins.first);
      }

      return false;
    }

    return true;
  }

  RevenueDecimal
  BillingContainerState::AmountHolder::get_total_amount()
    const
    /*throw(RevenueDecimal::Overflow)*/
  {
    return !days.empty() ?
      days.rbegin()->second.amount_before + days.rbegin()->second.amount :
      RevenueDecimal::ZERO;
  }

  ImpRevenueDecimal
  BillingContainerState::AmountHolder::get_total_imps()
    const
  {
    return !days.empty() ?
      days.rbegin()->second.imps_before + days.rbegin()->second.imps :
      ImpRevenueDecimal::ZERO;
  }

  ImpRevenueDecimal
  BillingContainerState::AmountHolder::get_total_clicks()
    const
  {
    return !days.empty() ?
      days.rbegin()->second.clicks_before + days.rbegin()->second.clicks :
      ImpRevenueDecimal::ZERO;
  }

  RevenueDecimal
  BillingContainerState::AmountHolder::get_day_amount(
    const Generics::Time& date)
    const
    noexcept
  {
    auto it = days.find(date);
    if(it != days.end())
    {
      return it->second.amount;
    }

    return RevenueDecimal::ZERO;
  }

  ImpRevenueDecimal
  BillingContainerState::AmountHolder::get_day_imps(
    const Generics::Time& date)
    const
    noexcept
  {
    auto it = days.find(date);
    if(it != days.end())
    {
      return it->second.imps;
    }

    return ImpRevenueDecimal::ZERO;
  }

  ImpRevenueDecimal
  BillingContainerState::AmountHolder::get_day_clicks(
    const Generics::Time& date)
    const
    noexcept
  {
    auto it = days.find(date);
    if(it != days.end())
    {
      return it->second.clicks;
    }

    return ImpRevenueDecimal::ZERO;
  }

  void
  BillingContainerState::AmountHolder::print(
    std::ostream& out,
    const char* offset)
    const noexcept
  {
    for(auto day_it = days.begin(); day_it != days.end(); ++day_it)
    {
      out << offset << day_it->first.get_gm_time().format("%F") <<
        ": amount_before = " << day_it->second.amount_before <<
        ", amount = " << day_it->second.amount <<
        ", imps_before = " << day_it->second.imps_before.str() <<
        ", imps = " << day_it->second.imps.str() <<
        ", clicks_before = " << day_it->second.clicks_before.str() <<
        ", clicks = " << day_it->second.clicks.str() <<
        std::endl;
    }
  }

  // BillingContainerState::HourActivity
  BillingContainerState::
  HourActivity::HourActivity() noexcept
    : imps(RevenueDecimal::ZERO),
      clicks(RevenueDecimal::ZERO),
      use_time(Generics::Time::ZERO)
  {}

  // BillingContainerState::HourActivityDistribution impl
  BillingContainerState::
  HourActivityDistribution::HourActivityDistribution()
    noexcept
  {
    hours.resize(24);
  }

  // BillingContainerState::RateDistributionHolder impl
  void
  BillingContainerState::
  RateDistributionHolder::save(std::ostream& out)
    const noexcept
  {
    // hour:rate:type:imps:clicks:use_time,...
    bool first_rec = true;
    save_(out, first_rec, free_rates, 'F');
    save_(out, first_rec, rates, 'G');
    if(first_rec)
    {
      out << '-';
    }
  }

  bool
  BillingContainerState::
  RateDistributionHolder::empty()
    const noexcept
  {
    for(auto rate_it = free_rates.begin(); rate_it != free_rates.end(); ++rate_it)
    {
      for(auto hour_it = rate_it->second.hours.begin();
        hour_it != rate_it->second.hours.end(); ++hour_it)
      {
        if(hour_it->imps != ImpRevenueDecimal::ZERO || hour_it->clicks != ImpRevenueDecimal::ZERO)
        {
          return false;
        }
      }
    }

    for(auto rate_it = rates.begin(); rate_it != rates.end(); ++rate_it)
    {
      for(auto hour_it = rate_it->second.hours.begin();
        hour_it != rate_it->second.hours.end(); ++hour_it)
      {
        if(hour_it->imps != ImpRevenueDecimal::ZERO || hour_it->clicks != ImpRevenueDecimal::ZERO)
        {
          return false;
        }
      }
    }

    return true;
  }

  void
  BillingContainerState::
  RateDistributionHolder::save_(
    std::ostream& out,
    bool& first_rec,
    const RateHourActivityDistributionMap& save_rates,
    char type)
    noexcept
  {
    // hour:rate:type:imps:clicks:use_time,...
    for(auto rate_it = save_rates.begin(); rate_it != save_rates.end(); ++rate_it)
    {
      for(auto hour_it = rate_it->second.hours.begin();
        hour_it != rate_it->second.hours.end(); ++hour_it)
      {
        if(hour_it->imps != ImpRevenueDecimal::ZERO || hour_it->clicks != ImpRevenueDecimal::ZERO)
        {
          if(!first_rec)
          {
            out << ",";
          }
          else
          {
            first_rec = false;
          }

          out << (hour_it - rate_it->second.hours.begin()) << ':' << // hour
            rate_it->first << ':' << // rate
            type << ':' << // type ('F', 'G')
            hour_it->imps.str() << ':' << // imps
            hour_it->clicks.str() << ':' << // clicks
            hour_it->use_time.tv_sec // use_time seconds
            ;
        }
      }
    }
  }

  void
  BillingContainerState::
  RateDistributionHolder::load(const String::SubString& str)
  {
    typedef const String::AsciiStringManip::Char2Category<',', '|'>
      ListSepType;

    if(str != "-")
    {
      String::StringManip::Splitter<ListSepType> tokenizer(str);
      String::SubString token;
      while(tokenizer.get_token(token))
      {
        String::StringManip::SplitColon sub_tokenizer(token);

        String::SubString hour_str;
        String::SubString rate_str;
        String::SubString type_str;
        String::SubString imps_str;
        String::SubString clicks_str;
        String::SubString use_time_str;

        if(!sub_tokenizer.get_token(hour_str) ||
          !sub_tokenizer.get_token(rate_str) ||
          !sub_tokenizer.get_token(type_str) ||
          !sub_tokenizer.get_token(imps_str) ||
          !sub_tokenizer.get_token(clicks_str) ||
          !sub_tokenizer.get_token(use_time_str))
        {
          Stream::Error ostr;
          ostr << "invalid hour part '" << token << "'";
          throw Exception(ostr);
        }

        unsigned long hour;
        if(!String::StringManip::str_to_int(hour_str, hour) || hour >= 24)
        {
          Stream::Error ostr;
          ostr << "invalid hour value '" << hour_str << "'";
          throw Exception(ostr);
        }

        RevenueDecimal rate;
        try
        {
          rate = RevenueDecimal(rate_str);
        }
        catch(const eh::Exception& ex)
        {
          Stream::Error ostr;
          ostr << "invalid rate value '" << rate_str << "'";
          throw Exception(ostr);
        }

        if(type_str.length() != 1 || (
             type_str[0] != 'F' && type_str[0] != 'G'))
        {
          Stream::Error ostr;
          ostr << "invalid type '" << type_str << "'(len=" << type_str.length() << ")";
          throw Exception(ostr);
        }

        ImpRevenueDecimal imps;
        try
        {
          imps = ImpRevenueDecimal(imps_str);
        }
        catch(const eh::Exception& ex)
        {
          Stream::Error ostr;
          ostr << "invalid imps value '" << imps_str << "'";
          throw Exception(ostr);
        }

        ImpRevenueDecimal clicks;
        try
        {
          clicks = ImpRevenueDecimal(clicks_str);
        }
        catch(const eh::Exception& ex)
        {
          Stream::Error ostr;
          ostr << "invalid clicks value '" << clicks_str << "'";
          throw Exception(ostr);
        }

        unsigned long use_time_sec;
        if(!String::StringManip::str_to_int(use_time_str, use_time_sec) ||
          use_time_sec > 3600)
        {
          Stream::Error ostr;
          ostr << "invalid use time value '" << use_time_str << "'";
          throw Exception(ostr);
        }

        Generics::Time use_time(use_time_sec);
        add(hour, rate, type_str[0] == 'F', imps, clicks, use_time);
      }
    }
  }

  void
  BillingContainerState::
  RateDistributionHolder::add(
    unsigned long hour,
    const RevenueDecimal& rate,
    bool free_budget,
    const ImpRevenueDecimal& imps,
    const ImpRevenueDecimal& clicks,
    const Generics::Time& use_time)
    noexcept
  {
    HourActivity& hour_act = free_budget ?
      free_rates[rate].hours[hour] :
      rates[rate].hours[hour];

    hour_act.imps += imps;
    hour_act.clicks += clicks;
    hour_act.use_time = std::max(hour_act.use_time, use_time);
  }

  // BillingContainerState::RateOptimizationHolder impl
  bool
  BillingContainerState::
  RateOptimizationHolder::DateHolder::empty() const noexcept
  {
    for(auto min_rate_it = min_rates.begin(); min_rate_it != min_rates.end(); ++min_rate_it)
    {
      if(*min_rate_it != RevenueDecimal::ZERO)
      {
        return false;
      }
    }

    return true;
  }

  void
  BillingContainerState::
  RateOptimizationHolder::DateHolder::save(std::ostream& out) const
    noexcept
  {
    for(auto min_rate_it = min_rates.begin(); min_rate_it != min_rates.end(); ++min_rate_it)
    {
      if(min_rate_it != min_rates.begin())
      {
        out << ',';
      }

      out << *min_rate_it;
    }
  }

  void
  BillingContainerState::
  RateOptimizationHolder::DateHolder::load(const String::SubString& str)
  {
    typedef const String::AsciiStringManip::Char2Category<',', '|'>
      ListSepType;

    unsigned long hour_i = 0;
    String::StringManip::Splitter<ListSepType> tokenizer(str);
    String::SubString token;
    while(tokenizer.get_token(token))
    {
      RevenueDecimal min_rate;
      try
      {
        min_rates[hour_i] = RevenueDecimal(token);
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << "invalid min rate value '" << token << "'";
        throw Exception(ostr);
      }

      ++hour_i;
    }
  }

  // BillingContainerState::CampaignCCGId impl
  BillingContainerState::
  CampaignCCGId::CampaignCCGId()
    noexcept
    : campaign_id(0),
      ccg_id(0)
  {}

  BillingContainerState::
  CampaignCCGId::CampaignCCGId(
    unsigned long campaign_id_val,
    unsigned long ccg_id_val)
    noexcept
    : campaign_id(campaign_id_val),
      ccg_id(ccg_id_val)
  {}

  bool
  BillingContainerState::
  CampaignCCGId::operator<(const CampaignCCGId& right) const
    noexcept
  {
    return campaign_id < right.campaign_id ||
      (campaign_id == right.campaign_id && ccg_id < right.ccg_id);
  }

  // BillingContainerState::RateGoalHolder impl
  BillingContainerState::
  RateGoalHolder::RateGoalHolder() noexcept
    : last_eval_min_rate_goal_time(Generics::Time::ZERO),
      min_rate_goal(RevenueDecimal::ZERO)
  {}

  // BillingContainerState::RateAmountHolder impl
  BillingContainerState::
  RateAmountHolder::RateAmountHolder() noexcept
  {}

  // BillingContainerState impl
  void
  BillingContainerState::convert_rate_amount(
    CTROptimizer::RateAmountDistribution& result,
    const RateDistributionHolder::RateHourActivityDistributionMap& rates,
    const RevenueDecimal& imp_amount,
    const RevenueDecimal& click_amount,
    const RevenueDecimal& factual_coef, // weight of actual amount
    const RevenueDecimal& click_rate_coef, // weight of click rate
    const RevenueDecimal& noise_ignore_part,
    const RevenueDecimal& rate_multiplier
    )
    noexcept
  {
    RevenueDecimal max_noise_click_rate = RevenueDecimal(true, 1, 0); // -1

    if(click_rate_coef > RevenueDecimal::ZERO)
    {
      // eval rate after that we have prediction noise
      ImpRevenueDecimal all_imps = ImpRevenueDecimal::ZERO;

      for(auto rate_it = rates.begin(); rate_it != rates.end(); ++rate_it)
      {
        for(auto hour_it = rate_it->second.hours.begin();
          hour_it != rate_it->second.hours.end(); ++hour_it)
        {
          all_imps += hour_it->imps;
        }
      }

      ImpRevenueDecimal max_imps = ImpRevenueDecimal::mul(
        all_imps,
        ImpRevenueDecimal(noise_ignore_part),
        Generics::DMR_FLOOR);

      all_imps = ImpRevenueDecimal::ZERO;

      for(auto rate_it = rates.begin(); rate_it != rates.end(); ++rate_it)
      {
        for(auto hour_it = rate_it->second.hours.begin();
          hour_it != rate_it->second.hours.end(); ++hour_it)
        {
          all_imps += hour_it->imps;
          if(all_imps >= max_imps)
          {
            max_noise_click_rate = rate_it->first;
            break;
          }
        }
      }
    }

    for(auto rate_it = rates.begin(); rate_it != rates.end(); ++rate_it)
    {
      for(auto hour_it = rate_it->second.hours.begin();
          hour_it != rate_it->second.hours.end(); ++hour_it)
      {
        RevenueDecimal amount((
          ImpRevenueDecimal::mul(
            hour_it->imps, ImpRevenueDecimal(imp_amount), Generics::DMR_FLOOR) +
          ImpRevenueDecimal::mul(
            hour_it->clicks, ImpRevenueDecimal(click_amount), Generics::DMR_FLOOR)).str());

        if(rate_it->first <= max_noise_click_rate)
        {
          // mix
          amount = RevenueDecimal((
            ImpRevenueDecimal(RevenueDecimal::mul(amount, factual_coef, Generics::DMR_FLOOR)) +
            ImpRevenueDecimal::mul(
              ImpRevenueDecimal::mul(
                ImpRevenueDecimal(RevenueDecimal::mul(rate_it->first, rate_multiplier, Generics::DMR_FLOOR)),
                hour_it->imps,
                Generics::DMR_FLOOR),
              ImpRevenueDecimal(click_amount),
              Generics::DMR_FLOOR)).str());
        }

        result.add(
          hour_it - rate_it->second.hours.begin(),
          rate_it->first,
          amount,
          &hour_it->use_time);
      }
    }
  }

  void
  BillingContainerState::convert_rate_amount_distribution(
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
    noexcept
  {
    convert_rate_amount(
      free_result,
      source.free_rates,
      imp_amount,
      click_amount,
      factual_coef,
      click_rate_coef,
      noise_ignore_part,
      rate_multiplier);

    convert_rate_amount(
      result,
      source.rates,
      imp_amount,
      click_amount,
      factual_coef,
      click_rate_coef,
      noise_ignore_part,
      rate_multiplier);
  }  
}
}
