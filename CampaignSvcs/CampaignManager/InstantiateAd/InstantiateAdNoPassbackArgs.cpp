#include "InstantiateAdNoPassbackArgs.hpp"

#include "../CreativeTextGenerator.hpp"

#include <string_view>

namespace AdServer::CampaignSvcs::InstantiateAd
{
  bool
  InstantiateAdNoPassbackArgs::get_argument(
    const String::SubString& key,
    std::string& result,
    bool value) const
  {
    const std::string_view name(key.data(), key.size());
    const std::string_view passback_code(
      CreativeTokens::PASSBACK_CODE.data(),
      CreativeTokens::PASSBACK_CODE.size());
    if(name != passback_code)
    {
      return false;
    }

    if(value)
    {
      result.clear();
    }
    else
    {
      result.assign(key.data(), key.size());
    }

    return true;
  }
}
