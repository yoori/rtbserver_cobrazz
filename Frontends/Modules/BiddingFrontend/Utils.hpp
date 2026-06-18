#pragma once

#include <string>
#include "CampaignManagerTypes.hpp"
#include <CampaignSvcs/CampaignCommons/CampaignSvcsVersionAdapter.hpp>

namespace AdServer::Bidding
{
  template<typename StringType>
  inline void add_token(
    AdServer::Bidding::CampaignManager::TokenSeq& tokens,
    const char* token_name,
    const StringType& token_value)
  {
    tokens.resize(tokens.size() + 1);
    tokens[tokens.size() - 1].name = token_name;
    tokens[tokens.size() - 1].value.assign(
      token_value.data(),
      token_value.size());
  }
}
