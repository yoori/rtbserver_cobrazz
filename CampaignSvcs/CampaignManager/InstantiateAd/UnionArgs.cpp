#include "UnionArgs.hpp"

#include <utility>

namespace AdServer::CampaignSvcs::InstantiateAd
{
  void
  UnionArgs::add_provider(
    std::shared_ptr<String::TextTemplate::ArgsCallback> provider)
  {
    providers_.emplace_back(std::move(provider));
  }

  bool
  UnionArgs::get_argument(
    const String::SubString& key,
    std::string& result,
    bool value) const
  {
    for(const auto& provider : providers_)
    {
      if(provider && provider->get_argument(key, result, value))
      {
        return true;
      }
    }

    return false;
  }
}
