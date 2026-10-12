#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include <Commons/WireTypes.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignConfigTypes.hpp>

namespace AdServer::CampaignSvcs
{
  struct CreativeStatInfo
  {
    std::uint32_t cc_id{};
    AdServer::Commons::DecimalInfo impressions{};
    AdServer::Commons::DecimalInfo clicks{};
    AdServer::Commons::DecimalInfo actions{};
  };

  using CreativeStatSeq = std::vector<CreativeStatInfo>;

  struct PublisherCCGStatInfo
  {
    std::uint32_t pub_account_id{};
    AdServer::Commons::DecimalInfo amount{};
  };

  using PublisherCCGStatSeq = std::vector<PublisherCCGStatInfo>;

  struct TagCCGStatInfo
  {
    std::uint32_t tag_id{};
    AdServer::Commons::DecimalInfo cur_pub_isp_amount{};
    AdServer::Commons::DecimalInfo cur_adv_amount{};
    AdServer::Commons::DecimalInfo cur_adv_comm_amount{};
    AdServer::Commons::DecimalInfo prev_pub_isp_amount{};
    AdServer::Commons::DecimalInfo prev_adv_amount{};
    AdServer::Commons::DecimalInfo prev_adv_comm_amount{};
  };

  using TagCCGStatSeq = std::vector<TagCCGStatInfo>;

  struct CtrResetStatInfo
  {
    std::uint32_t ctr_reset_id{};
    AdServer::Commons::DecimalInfo impressions{};
  };

  using CtrResetStatSeq = std::vector<CtrResetStatInfo>;

  struct CCGStatInfo
  {
    std::uint32_t ccg_id{};
    AdServer::Commons::DecimalInfo impressions{};
    AdServer::Commons::DecimalInfo clicks{};
    AdServer::Commons::DecimalInfo actions{};
    AdServer::Commons::DecimalInfo amount{};
    AdServer::Commons::DecimalInfo comm_amount{};
    AdServer::Commons::DecimalInfo daily_amount{};
    AdServer::Commons::DecimalInfo daily_comm_amount{};
    AdServer::Commons::DecimalInfo prev_hour_amount{};
    AdServer::Commons::DecimalInfo prev_hour_comm_amount{};
    AdServer::Commons::DecimalInfo current_hour_amount{};
    AdServer::Commons::DecimalInfo current_hour_comm_amount{};
    CreativeStatSeq creatives{};
    PublisherCCGStatSeq publishers{};
    TagCCGStatSeq tags{};
    CtrResetStatSeq ctr_resets{};
  };

  using CCGStatSeq = std::vector<CCGStatInfo>;

  struct CampaignStatInfo
  {
    std::uint32_t campaign_id{};
    AdServer::Commons::DecimalInfo amount{};
    AdServer::Commons::DecimalInfo comm_amount{};
    AdServer::Commons::DecimalInfo daily_amount{};
    AdServer::Commons::DecimalInfo daily_comm_amount{};
    CCGStatSeq ccgs{};
  };

  struct AmountStatInfo
  {
    std::uint32_t id{};
    AdServer::Commons::DecimalInfo amount{};
    AdServer::Commons::DecimalInfo comm_amount{};
    AdServer::Commons::DecimalInfo daily_amount{};
    AdServer::Commons::DecimalInfo daily_comm_amount{};
  };

  using AmountStatSeq = std::vector<AmountStatInfo>;

  using AccountStatSeq = AmountStatSeq;

  using CampaignStatSeq = std::vector<CampaignStatInfo>;

  struct StatInfo
  {
    AccountStatSeq accounts{};
    CampaignStatSeq campaigns{};
  };

}
