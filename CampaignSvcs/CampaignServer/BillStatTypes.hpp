#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include <Commons/WireTypes.hpp>

namespace AdServer::CampaignSvcs
{
  struct DayAmount
  {
    AdServer::Commons::TimestampInfo day{};
    AdServer::Commons::DecimalInfo amount{};
  };

  using DayAmountSeq = std::vector<DayAmount>;

  struct AmountDistributionInfo
  {
    DayAmount prev_days_amount{};
    DayAmountSeq day_amounts{};
  };

  using AmountDistributionSeq = std::vector<AmountDistributionInfo>;

  struct AccountAmountDistributionInfo
  {
    std::uint32_t account_id{};
    AmountDistributionInfo amount_distribution{};
  };

  using AccountAmountDistributionSeq = std::vector<AccountAmountDistributionInfo>;

  struct DayAmountCount
  {
    AdServer::Commons::TimestampInfo day{};
    AdServer::Commons::DecimalInfo amount{};
    AdServer::Commons::DecimalInfo imps{};
    AdServer::Commons::DecimalInfo clicks{};
  };

  using DayAmountCountSeq = std::vector<DayAmountCount>;

  struct AmountCountDistributionInfo
  {
    DayAmountCount prev_days_amount_count{};
    DayAmountCountSeq day_amount_counts{};
  };

  struct CampaignAmountDistributionInfo
  {
    std::uint32_t campaign_id{};
    AmountCountDistributionInfo amount_count_distribution{};
  };

  using CampaignAmountDistributionSeq = std::vector<CampaignAmountDistributionInfo>;

  struct CCGAmountDistributionInfo
  {
    std::uint32_t ccg_id{};
    AmountCountDistributionInfo amount_count_distribution{};
  };

  using CCGAmountDistributionSeq = std::vector<CCGAmountDistributionInfo>;

  struct BillStatInfo
  {
    AccountAmountDistributionSeq accounts{};
    CampaignAmountDistributionSeq campaigns{};
    CCGAmountDistributionSeq ccgs{};
  };

}
