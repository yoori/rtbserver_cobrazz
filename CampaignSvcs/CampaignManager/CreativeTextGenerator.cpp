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
    OptionTokenValueMap union_args;
    for(TokenValueMap::const_iterator req_arg_it =
          request_args.begin();
        req_arg_it != request_args.end(); ++req_arg_it)
    {
      union_args.insert(std::make_pair(
        req_arg_it->first,
        OptionValue(0, req_arg_it->second.c_str())));
    }

    for(OptionTokenValueMap::const_iterator cr_arg_it =
          creative_args.begin();
        cr_arg_it != creative_args.end(); ++cr_arg_it)
    {
      union_args[cr_arg_it->first] = cr_arg_it->second;
    }

    for(OptionTokenValueMap::const_iterator it = creative_args.begin();
        it != creative_args.end();
        ++it)
    {
      BaseTokenProcessor_var token_processor;
      TokenProcessorMap::const_iterator proc_it = token_processors.find(
        it->second.option_id);
      if(proc_it == token_processors.end())
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
        union_args,
        token_processors,
        rule,
        creative_instantiate_args,
        res);
      result_creative_args[it->first].swap(res);
    }
  }
}
}
