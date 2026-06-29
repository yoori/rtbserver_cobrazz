#pragma once

#include <string>

namespace AdServer::Bidding
{
  template<typename TokenSeqType, typename StringType>
  inline void add_token(
    TokenSeqType& tokens,
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
