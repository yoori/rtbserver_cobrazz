#pragma once

#include <map>
#include <string>

#include <Commons/Containers.hpp>

#include "CreativeTemplateArgs.hpp"

namespace AdServer::CampaignSvcs
{
  class CreativeInstantiatorTypes
  {
  public:
    struct CreativeInstantiate
    {
      struct SourceRule
      {
        Commons::Optional<std::string> click_prefix;
        std::string mime_encoded_click_prefix;

        std::string preclick;
        std::string mime_encoded_preclick;

        std::string vast_preclick;
        std::string mime_encoded_vast_preclick;
      };

      using SourceRuleMap = std::map<std::string, SourceRule>;

      CreativeInstantiateRuleMap creative_rules;
      SourceRuleMap source_rules;
      std::string template_local_prefix;
      std::string creative_template_dir;
    };
  };
}
