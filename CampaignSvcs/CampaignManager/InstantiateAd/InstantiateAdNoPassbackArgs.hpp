#pragma once

#include <string>

#include <String/TextTemplate.hpp>

namespace AdServer::CampaignSvcs::InstantiateAd
{
  class InstantiateAdNoPassbackArgs:
    public String::TextTemplate::ArgsCallback
  {
  private:
    bool
    get_argument(
      const String::SubString& key,
      std::string& result,
      bool value = true) const override;
  };
}
