#include "CacheArgs.hpp"

#include <utility>

namespace AdServer::CampaignSvcs::InstantiateAd
{
  CacheArgs::CacheArgs(
    std::shared_ptr<String::TextTemplate::ArgsCallback> source)
    : source_(std::move(source))
  {}

  bool
  CacheArgs::get_argument(
    const String::SubString& key,
    std::string& result,
    bool value) const
  {
    if(!source_)
    {
      return false;
    }

    if(!value)
    {
      return source_->get_argument(key, result, false);
    }

    const std::string name(key.data(), key.size());
    const auto it = cached_values_.find(name);
    if(it != cached_values_.end())
    {
      result = it->second;
      return true;
    }

    if(!source_->get_argument(key, result, true))
    {
      return false;
    }

    cached_values_.emplace(name, result);
    return true;
  }
}
