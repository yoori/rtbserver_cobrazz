#pragma once

#include <string>
#include <CORBACommons/CorbaAdapters.hpp>
#include "CampaignManagerTypes.hpp"
#include <CampaignSvcs/CampaignCommons/CampaignSvcsVersionAdapter.hpp>

namespace AdServer
{
namespace Bidding
{
  void add_token(
    AdServer::Bidding::CampaignManager::TokenSeq& tokens,
    const char* token_name,
    const std::string& token_value);
}
}
