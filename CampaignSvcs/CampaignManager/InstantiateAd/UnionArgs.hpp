#pragma once

#include <memory>
#include <vector>

#include <String/TextTemplate.hpp>

namespace AdServer::CampaignSvcs::InstantiateAd
{
  class UnionArgs: public String::TextTemplate::ArgsCallback
  {
  public:
    void
    add_provider(std::shared_ptr<String::TextTemplate::ArgsCallback> provider);

  private:
    bool
    get_argument(
      const String::SubString& key,
      std::string& result,
      bool value = true) const override;

  private:
    std::vector<std::shared_ptr<String::TextTemplate::ArgsCallback>> providers_;
  };
}
