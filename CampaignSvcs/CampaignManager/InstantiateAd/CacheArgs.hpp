#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include <String/TextTemplate.hpp>

namespace AdServer::CampaignSvcs::InstantiateAd
{
  class CacheArgs: public String::TextTemplate::ArgsCallback
  {
  public:
    explicit
    CacheArgs(std::shared_ptr<String::TextTemplate::ArgsCallback> source);

  private:
    bool
    get_argument(
      const String::SubString& key,
      std::string& result,
      bool value = true) const override;

  private:
    std::shared_ptr<String::TextTemplate::ArgsCallback> source_;
    mutable std::unordered_map<std::string, std::string> cached_values_;
  };
}
