#include "ProcessorArgs.hpp"

#include <utility>

namespace AdServer::CampaignSvcs::InstantiateAd
{
  ProviderContext::ProviderContext(
    std::shared_ptr<InstantiateAdContext> context,
    std::shared_ptr<String::TextTemplate::ArgsCallback> source_args)
    : context_(std::move(context)),
      source_args_(std::move(source_args))
  {}

  InstantiateAdContext&
  ProviderContext::context() const noexcept
  {
    return *context_;
  }

  const std::shared_ptr<String::TextTemplate::ArgsCallback>&
  ProviderContext::source_args() const noexcept
  {
    return source_args_;
  }
}
