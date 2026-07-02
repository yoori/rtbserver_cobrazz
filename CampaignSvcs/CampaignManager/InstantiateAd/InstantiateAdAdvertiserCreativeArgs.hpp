#pragma once

#include <memory>

#include "ProcessorArgs.hpp"

namespace AdServer::CampaignSvcs::InstantiateAd
{
  class InstantiateAdAdvertiserCreativeArgsManager;

  class InstantiateAdAdvertiserCreativeArgsProvider:
    public String::TextTemplate::ArgsCallback,
    public ProviderContext
  {
  public:
    InstantiateAdAdvertiserCreativeArgsProvider(
      std::shared_ptr<const InstantiateAdAdvertiserCreativeArgsManager> manager,
      std::shared_ptr<InstantiateAdContext> context,
      std::shared_ptr<String::TextTemplate::ArgsCallback> source_args);

  private:
    bool
    get_argument(
      const String::SubString& key,
      std::string& result,
      bool value = true) const override;

  private:
    std::shared_ptr<const InstantiateAdAdvertiserCreativeArgsManager> manager_;
  };

  class InstantiateAdAdvertiserCreativeArgsManager:
    public ProcessorArgsManager<InstantiateAdAdvertiserCreativeArgsProvider>,
    public std::enable_shared_from_this<
      InstantiateAdAdvertiserCreativeArgsManager>
  {
  public:
    std::shared_ptr<InstantiateAdAdvertiserCreativeArgsProvider>
    create_provider(
      std::shared_ptr<InstantiateAdContext> context,
      std::shared_ptr<String::TextTemplate::ArgsCallback> source_args) const;
  };
}
