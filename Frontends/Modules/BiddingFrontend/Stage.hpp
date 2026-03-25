#pragma once

#include <String/SubString.hpp>

namespace AdServer::Bidding
{

enum class Stage
{
  Initial,
  UserResolving,
  TriggerMatching,
  HistoryMatching,
  CampaignSelection,
  RequestParsing,
  CampaignSelectionConsidering
};

constexpr String::SubString convert_stage_to_string(
  const Stage stage)
{
  switch (stage)
  {
    case Stage::Initial:
      return String::SubString("initial");
    case Stage::UserResolving:
      return String::SubString("user resolving");
    case Stage::TriggerMatching:
      return String::SubString("trigger matching");
    case Stage::HistoryMatching:
      return String::SubString("history matching");
    case Stage::CampaignSelection:
      return String::SubString("campaign selection");
    case Stage::RequestParsing:
      return String::SubString("request parsing");
    case Stage::CampaignSelectionConsidering:
      return String::SubString("campaign selection considering");
  }

  return String::SubString();
}

} // namespace AdServer::Bidding
