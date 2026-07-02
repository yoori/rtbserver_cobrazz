#pragma once

#include <memory>

#include "ProcessorArgs.hpp"

namespace AdServer::CampaignSvcs::InstantiateAd
{
  class InstantiateAdRequestArgsManager;

  class InstantiateAdRequestArgsProvider:
    public String::TextTemplate::ArgsCallback
  {
  public:
    InstantiateAdRequestArgsProvider(
      std::shared_ptr<const InstantiateAdRequestArgsManager> manager,
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
    std::shared_ptr<const InstantiateAdRequestArgsManager> manager_;
    std::shared_ptr<InstantiateAdContext> context_;
  };

  class InstantiateAdRequestArgsManager:
    public ProcessorArgsManager<InstantiateAdRequestArgsProvider>,
    public std::enable_shared_from_this<InstantiateAdRequestArgsManager>
  {
  public:
    InstantiateAdRequestArgsManager();

    std::shared_ptr<InstantiateAdRequestArgsProvider>
    create_provider(std::shared_ptr<InstantiateAdContext> context) const;
  };
}
