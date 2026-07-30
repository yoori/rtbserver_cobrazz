#pragma once

#include <memory>
#include <string>

#include <String/TextTemplate.hpp>

#include "../CreativeTemplateArgs.hpp"

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
    mutable StringFlatMap<std::string> cached_values_;
  };
}
