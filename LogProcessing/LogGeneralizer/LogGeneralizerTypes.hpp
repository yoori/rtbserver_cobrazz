#pragma once

#include <cstdint>
#include <vector>
#include <eh/Exception.hpp>
#include <Generics/SimpleDecimal.hpp>
#include <Generics/Time.hpp>

namespace AdServer::LogProcessing
{
  using StatDecimal = Generics::SimpleDecimal<std::uint64_t, 18, 8>;

  DECLARE_EXCEPTION(LogGeneralizerError, eh::DescriptiveException);
  DECLARE_EXCEPTION(NotSupported, LogGeneralizerError);
  DECLARE_EXCEPTION(CollectionNotStarted, LogGeneralizerError);

  struct CreativeStatInfo
  {
    std::uint32_t cc_id = 0;
    std::int64_t requests = 0;
    std::int64_t impressions = 0;
    std::int64_t clicks = 0;
    std::int64_t actions = 0;
  };

  using CreativeStatSeq = std::vector<CreativeStatInfo>;

  struct PublisherAmountInfo
  {
    std::uint32_t publisher_account_id = 0;
    StatDecimal adv_amount = StatDecimal::ZERO;
  };

  using PublisherAmountSeq = std::vector<PublisherAmountInfo>;

  struct TagAmountInfo
  {
    std::uint32_t tag_id = 0;
    StatDecimal pub_isp_amount = StatDecimal::ZERO;
    StatDecimal adv_amount = StatDecimal::ZERO;
    StatDecimal adv_comm_amount = StatDecimal::ZERO;
  };

  using TagAmountSeq = std::vector<TagAmountInfo>;

  struct CtrResetStatInfo
  {
    std::uint32_t ctr_reset_id = 0;
    std::uint64_t impressions = 0;
  };

  using CtrResetStatSeq = std::vector<CtrResetStatInfo>;

  struct CampaignStatInfo
  {
    Generics::Time sdate;
    Generics::Time adv_sdate;
    std::uint32_t adv_account_id = 0;
    std::uint32_t campaign_id = 0;
    std::uint32_t ccg_id = 0;
    StatDecimal adv_account_amount = StatDecimal::ZERO;
    StatDecimal adv_amount = StatDecimal::ZERO;
    StatDecimal adv_comm_amount = StatDecimal::ZERO;
    StatDecimal adv_payable_comm_amount = StatDecimal::ZERO;
    CreativeStatSeq creative_stats;
    PublisherAmountSeq publisher_amounts;
    TagAmountSeq tag_amounts;
    CtrResetStatSeq ctr_reset_stats;
  };

  using CampaignStatSeq = std::vector<CampaignStatInfo>;

  struct StatInfo
  {
    CampaignStatSeq campaign_stats;
  };

}
