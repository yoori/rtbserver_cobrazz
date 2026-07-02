#pragma once

#include <memory>

#include "ProcessorArgs.hpp"

namespace AdServer::CampaignSvcs::InstantiateAd
{
  class InstantiateAdCreativeArgsManager;

  class InstantiateAdCreativeArgsProvider:
    public String::TextTemplate::ArgsCallback,
    public ProviderContext
  {
  public:
    InstantiateAdCreativeArgsProvider(
      std::shared_ptr<const InstantiateAdCreativeArgsManager> manager,
      std::shared_ptr<InstantiateAdContext> context,
      std::shared_ptr<String::TextTemplate::ArgsCallback> source_args);

  private:
    bool
    get_argument(
      const String::SubString& key,
      std::string& result,
      bool value = true) const override;

  private:
    std::shared_ptr<const InstantiateAdCreativeArgsManager> manager_;
  };

  class InstantiateAdCreativeArgsManager:
    public ProcessorArgsManager<InstantiateAdCreativeArgsProvider>,
    public std::enable_shared_from_this<InstantiateAdCreativeArgsManager>
  {
  public:
    InstantiateAdCreativeArgsManager();

    std::shared_ptr<InstantiateAdCreativeArgsProvider>
    create_provider(
      std::shared_ptr<InstantiateAdContext> context,
      std::shared_ptr<String::TextTemplate::ArgsCallback> source_args) const;
  };
}
