#include "Utils.hpp"

namespace AdServer
{
namespace Bidding
{
  void add_token(
    AdServer::Bidding::CampaignManager::TokenSeq& tokens,
    const char* token_name,
    const std::string& token_value)
  {
    tokens.resize(tokens.size() + 1);
    tokens[tokens.size() - 1].name = token_name;
    tokens[tokens.size() - 1].value = token_value;
  }
}
}
