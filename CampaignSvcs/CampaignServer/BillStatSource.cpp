#include "BillStatSource.hpp"

namespace AdServer
{
namespace CampaignSvcs
{
  void
  BillStatSource::Stat::AmountDistribution::print(
    std::ostream& out,
    const char* offset)
    const noexcept
  {
    out << offset <<
      "before " << prev_day.get_gm_time().format("%F") <<
      ": " << prev_days_amount << std::endl;

    for(auto day_it = day_amounts.begin();
      day_it != day_amounts.end(); ++day_it)
    {
      out << offset <<
        "at " << day_it->first.get_gm_time().format("%F") <<
        ": " << day_it->second << std::endl;
    }
  }

  void
  BillStatSource::Stat::AmountCountDistribution::print(
    std::ostream& out,
    const char* offset)
    const noexcept
  {
    out << offset <<
      "before " << prev_day.get_gm_time().format("%F") <<
      ": amount = " << prev_days_amount <<
      ", imps = " << prev_days_imps.str() <<
      ", clicks = " << prev_days_clicks.str() <<
      std::endl;

    for(auto day_it = day_amount_counts.begin();
      day_it != day_amount_counts.end(); ++day_it)
    {
      out << offset <<
        "at " << day_it->first.get_gm_time().format("%F") <<
        ": amount = " << day_it->second.amount <<
        ", imps = " << day_it->second.imps.str() <<
        ", clicks = " << day_it->second.clicks.str() <<
        std::endl;
    }
  }

  void
  BillStatSource::Stat::print(std::ostream& out, const char* offset) const
    noexcept
  {
    const std::string sub_offset = std::string(offset) + "  ";

    out << offset << "timestamp: " << timestamp.get_gm_time() << std::endl <<
      offset << "accounts:" << std::endl;
    for(auto acc_it = accounts.begin();
        acc_it != accounts.end(); ++acc_it)
    {
      out << offset << acc_it->first << ":" << std::endl;
      acc_it->second.print(out, sub_offset.c_str());
    }

    out << offset << "campaigns: " << std::endl;
    for(auto cmp_it = campaigns.begin();
      cmp_it != campaigns.end(); ++cmp_it)
    {
      out << offset << cmp_it->first << ":" << std::endl;
      cmp_it->second.print(out, sub_offset.c_str());
    }

    out << offset << "ccgs: " << std::endl;
    for(auto ccg_it = ccgs.begin();
      ccg_it != ccgs.end(); ++ccg_it)
    {
      out << offset << ccg_it->first << ":" << std::endl;
      ccg_it->second.print(out, sub_offset.c_str());
    }
  }
}
}
