#pragma once

#include <memory>
#include <string>
#include <Generics/Time.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <ChannelSvcs/ChannelCommons/ChannelCommons.hpp>

namespace AdServer::ChannelSvcs::ChannelUpdate
{
  struct TriggerInfo
  {
    std::uint32_t channel_trigger_id = 0;
    std::string trigger;
  };

  struct ChannelById
  {
    std::uint32_t channel_id = 0;
    std::vector<TriggerInfo> words;
    Generics::Time stamp = Generics::Time::ZERO;
  };

  struct CCGKeyword
  {
    std::uint32_t ccg_keyword_id = 0;
    std::uint32_t ccg_id = 0;
    std::uint32_t channel_id = 0;
    CampaignSvcs::RevenueDecimal max_cpc = CampaignSvcs::RevenueDecimal::ZERO;
    CampaignSvcs::CTRDecimal ctr = CampaignSvcs::CTRDecimal::ZERO;
    std::string click_url;
    std::string original_keyword;
  };

  struct TriggerVersion
  {
    std::uint32_t id = 0;
    Generics::Time stamp = Generics::Time::ZERO;
  };

  struct ChannelVersion
  {
    std::uint32_t id = 0;
    std::uint32_t size = 0;
    Generics::Time stamp = Generics::Time::ZERO;
  };

  struct CCGQuery
  {
    Generics::Time master_stamp = Generics::Time::ZERO;
    std::uint32_t start = 0;
    std::uint32_t limit = 0;
    ChannelIdList channel_ids;
    bool use_only_list = false;
  };

  struct PosCCGResult
  {
    std::uint32_t start_id = 0;
    std::vector<CCGKeyword> keywords;
    std::vector<TriggerVersion> deleted;
    std::int32_t source_id = -1;
  };

  struct CheckQuery
  {
    std::uint32_t colo_id = 0;
    std::string version;
    Generics::Time master_stamp = Generics::Time::ZERO;
    ChannelIdList new_ids;
    bool use_only_list = false;
  };

  struct CheckData
  {
    Generics::Time first_stamp = Generics::Time::ZERO;
    Generics::Time master_stamp = Generics::Time::ZERO;
    std::vector<ChannelVersion> versions;
    bool special_track = false;
    bool special_adv = false;
    std::int32_t source_id = -1;
    Generics::Time max_time = Generics::Time::ZERO;
  };

  struct UpdateData
  {
    std::int32_t source_id = -1;
    std::vector<ChannelById> channels;
  };

  using CheckDataPtr = std::unique_ptr<CheckData>;
  using UpdateDataPtr = std::unique_ptr<UpdateData>;
  using PosCCGResultPtr = std::unique_ptr<PosCCGResult>;
}
