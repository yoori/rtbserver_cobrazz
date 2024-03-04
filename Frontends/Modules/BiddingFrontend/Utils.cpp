#include "Utils.hpp"

namespace AdServer
{
namespace Bidding
{
  void add_token(
    AdServer::CampaignSvcs::CampaignManager::TokenSeq& tokens,
    const char* token_name,
    const std::string& token_value)
  {
    tokens.length(tokens.length() + 1);
    tokens[tokens.length() - 1].name = token_name;
    tokens[tokens.length() - 1].value << token_value;
  }
}
}
