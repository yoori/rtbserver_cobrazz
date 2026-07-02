#pragma once

#include <memory>

#include "ProcessorArgs.hpp"

namespace AdServer::CampaignSvcs::InstantiateAd
{
  class InstantiateAdCommonCreativeArgsManager;

  class InstantiateAdCommonCreativeArgsProvider:
    public String::TextTemplate::ArgsCallback
  {
  public:
    InstantiateAdCommonCreativeArgsProvider(
      std::shared_ptr<const InstantiateAdCommonCreativeArgsManager> manager,
      std::shared_ptr<InstantiateAdContext> context);

    InstantiateAdContext&
    context() const noexcept;

  private:
    bool
    get_argument(
      const String::SubString& key,
      std::string& result,
      bool value = true) const override;

  private:
    std::shared_ptr<const InstantiateAdCommonCreativeArgsManager> manager_;
    std::shared_ptr<InstantiateAdContext> context_;
  };

  class InstantiateAdCommonCreativeArgsManager:
    public ProcessorArgsManager<InstantiateAdCommonCreativeArgsProvider>,
    public std::enable_shared_from_this<InstantiateAdCommonCreativeArgsManager>
  {
  public:
    InstantiateAdCommonCreativeArgsManager();

    std::shared_ptr<InstantiateAdCommonCreativeArgsProvider>
    create_provider(std::shared_ptr<InstantiateAdContext> context) const;
  };
}
