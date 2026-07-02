#include "InstantiateAdAdvertiserCreativeArgs.hpp"

#include <utility>

namespace AdServer::CampaignSvcs::InstantiateAd
{
  InstantiateAdAdvertiserCreativeArgsProvider::
  InstantiateAdAdvertiserCreativeArgsProvider(
    std::shared_ptr<const InstantiateAdAdvertiserCreativeArgsManager> manager,
    std::shared_ptr<InstantiateAdContext> context,
    std::shared_ptr<String::TextTemplate::ArgsCallback> source_args)
    : ProviderContext(std::move(context), std::move(source_args)),
      manager_(std::move(manager))
  {}

  bool
  InstantiateAdAdvertiserCreativeArgsProvider::get_argument(
    const String::SubString& key,
    std::string& result,
    bool value) const
  {
    return manager_->get_argument(*this, key, result, value);
  }

  std::shared_ptr<InstantiateAdAdvertiserCreativeArgsProvider>
  InstantiateAdAdvertiserCreativeArgsManager::create_provider(
    std::shared_ptr<InstantiateAdContext> context,
    std::shared_ptr<String::TextTemplate::ArgsCallback> source_args) const
  {
    return std::make_shared<InstantiateAdAdvertiserCreativeArgsProvider>(
      shared_from_this(),
      std::move(context),
      std::move(source_args));
  }
}
