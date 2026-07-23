#pragma once

#include <memory>

#include "../CreativeTemplateArgs.hpp"

namespace AdServer::CampaignSvcs::InstantiateAd
{
  class TokenProcessorArgs: public String::TextTemplate::ArgsCallback
  {
  public:
    TokenProcessorArgs(
      std::shared_ptr<const OptionTokenValueMap> token_values,
      const TokenProcessorMap& token_processors,
      const CreativeInstantiateRule& rule,
      CreativeInstantiateArgs creative_args,
      std::shared_ptr<String::TextTemplate::ArgsCallback> source_args);

  private:
    bool
    get_argument(
      const String::SubString& key,
      std::string& result,
      bool value = true) const override;

  private:
    std::shared_ptr<const OptionTokenValueMap> token_values_;
    const TokenProcessorMap& token_processors_;
    const CreativeInstantiateRule& rule_;
    CreativeInstantiateArgs creative_args_;
    std::shared_ptr<String::TextTemplate::ArgsCallback> source_args_;
  };
}
