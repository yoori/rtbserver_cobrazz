#include <iostream>
#include "CreativeTextGenerator.hpp"

namespace AdServer
{
namespace CampaignSvcs
{
  void
  CreativeTextGenerator::init_creative_tokens(
    const CreativeInstantiateRule& rule,
    const CreativeInstantiateArgs& creative_instantiate_args,
    const TokenProcessorMap& token_processors,
    const TokenValueMap& request_args,
    const OptionTokenValueMap& creative_args,
    TokenValueMap& result_creative_args)
    /*throw(eh::Exception)*/
  {
    init_creative_tokens(
      rule,
      creative_instantiate_args,
      token_processors,
      static_cast<const String::TextTemplate::ArgsCallback&>(request_args),
      creative_args,
      result_creative_args);
  }

  void
  CreativeTextGenerator::init_creative_tokens(
    const CreativeInstantiateRule& rule,
    const CreativeInstantiateArgs& creative_instantiate_args,
    const TokenProcessorMap& token_processors,
    const String::TextTemplate::ArgsCallback& request_args,
    const OptionTokenValueMap& creative_args,
    TokenValueMap& result_creative_args)
    /*throw(eh::Exception)*/
  {
    const TokenOptionValueProvider token_values(&request_args, creative_args);

    for (OptionTokenValueMap::const_iterator it = creative_args.begin();
      it != creative_args.end(); ++it)
    {
      BaseTokenProcessor_var token_processor;
      TokenProcessorMap::const_iterator proc_it = token_processors.find(
        it->second.option_id);
      if (proc_it == token_processors.end())
      {
        token_processor =
          BaseTokenProcessor::default_token_processor(it->first.c_str());
      }
      else
      {
        token_processor = proc_it->second;
      }

      std::string res;
      token_processor->instantiate(
        token_values,
        token_processors,
        rule,
        creative_instantiate_args,
        res);
      result_creative_args.set_value(it->first, std::move(res));
    }
  }
}
}
