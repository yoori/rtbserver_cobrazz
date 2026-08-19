#pragma once

#include <LogCommons/LogCommons.hpp>
#include <LogCommons/StatCollector.hpp>
#include <LogCommons/CsvUtils.hpp>

namespace AdServer
{
namespace LogProcessing
{
  struct BidCostData
  {
    DayTimestamp time;
    unsigned long tag_id = 0;
    std::string ext_tag_id;
    std::string url;
    FixedNumber cost;
    long unverified_imps = 0;
    long imps = 0;
    long clicks = 0;
  };

  using BidCostCollector = SeqCollector<BidCostData>;

  struct BidCostTraits:
    LogDefaultTraits<BidCostCollector, false, false>
  {
    static const char* csv_base_name() { return "BidCost"; }

    static const char* csv_header()
    {
      return "Timestamp,Tag ID,External Tag ID,URL,Cost,Unverified Imps,Imps,Clicks";
    }

    static std::ostream&
    write_data_as_csv(
      std::ostream& os,
      const BaseTraits::CollectorType::DataT& data)
    {
      write_date_as_csv(os, data.time) << ',';
      os << data.tag_id << ',';
      write_string_as_csv(os, data.ext_tag_id) << ',';
      write_string_as_csv(os, data.url) << ',';
      os << data.cost << ',';
      os << data.unverified_imps << ',';
      os << data.imps << ',';
      os << data.clicks;
      return os;
    }
  };
}
}
