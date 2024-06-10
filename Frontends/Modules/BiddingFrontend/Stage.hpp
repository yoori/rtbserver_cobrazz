#pragma once

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

constexpr std::string_view convert_stage_to_string(
  const Stage stage)
{
  switch (stage)
  {
    case Stage::Initial:
      return "initial";
    case Stage::UserResolving:
      return "user resolving";
    case Stage::TriggerMatching:
      return "trigger matching";
    case Stage::HistoryMatching:
      return "history matching";
    case Stage::CampaignSelection:
      return "campaign selection";
    case Stage::RequestParsing:
      return "request parsing";
    case Stage::CampaignSelectionConsidering:
      return "campaign selection considering";
  }

  return {};
}

} // namespace AdServer::Bidding
