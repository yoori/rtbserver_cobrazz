#include "TokenProcessorArgs.hpp"

#include <utility>

namespace AdServer::CampaignSvcs::InstantiateAd
{
  TokenProcessorArgs::TokenProcessorArgs(
    std::shared_ptr<const OptionTokenValueMap> token_values,
    const TokenProcessorMap& token_processors,
    const CreativeInstantiateRule& rule,
    CreativeInstantiateArgs creative_args,
    std::shared_ptr<String::TextTemplate::ArgsCallback> source_args)
    : token_values_(std::move(token_values)),
      token_processors_(token_processors),
      rule_(rule),
      creative_args_(std::move(creative_args)),
      source_args_(std::move(source_args))
  {}

  bool
  TokenProcessorArgs::get_argument(
    const String::SubString& key,
    std::string& result,
    bool value) const
  {
    if(!token_values_)
    {
      return false;
    }

    const std::string_view name(key.data(), key.size());
    const auto token_it = token_values_->find(name);
    if(token_it == token_values_->end())
    {
      return false;
    }

    if(!value)
    {
      result.assign(key.data(), key.size());
      return true;
    }

    BaseTokenProcessor_var token_processor;
    const auto processor_it = token_processors_.find(token_it->second.option_id);
    if(processor_it != token_processors_.end())
    {
      token_processor = processor_it->second;
    }
    else
    {
      token_processor = BaseTokenProcessor::default_token_processor(
        std::string(name).c_str());
    }

    const TokenOptionValueProvider token_value_provider(source_args_.get(), *token_values_);

    return token_processor->instantiate(
      token_value_provider,
      token_processors_,
      rule_,
      creative_args_,
      result);
  }
}
