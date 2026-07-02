#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include <String/TextTemplate.hpp>

#include "InstantiateAdContext.hpp"

namespace AdServer::CampaignSvcs::InstantiateAd
{
  class ProviderContext
  {
  public:
    ProviderContext(
      std::shared_ptr<InstantiateAdContext> context,
      std::shared_ptr<String::TextTemplate::ArgsCallback> source_args);

    InstantiateAdContext&
    context() const noexcept;

    const std::shared_ptr<String::TextTemplate::ArgsCallback>&
    source_args() const noexcept;

  private:
    std::shared_ptr<InstantiateAdContext> context_;
    std::shared_ptr<String::TextTemplate::ArgsCallback> source_args_;
  };

  template<typename ProviderType>
  class ProcessorArgsManager
  {
  public:
    using ArgFun = std::function<std::optional<std::string>(
      const ProviderType&)>;

    void
    add_processor(std::string_view name, ArgFun&& fun)
    {
      processors_[std::string(name)] = std::move(fun);
    }

    bool
    has_processor(std::string_view name) const
    {
      return processors_.find(std::string(name)) != processors_.end();
    }

    bool
    get_argument(
      const ProviderType& provider,
      const String::SubString& key,
      std::string& result,
      bool value) const
    {
      const std::string name(key.data(), key.size());
      const auto it = processors_.find(name);
      if(it != processors_.end())
      {
        if(!value)
        {
          result.assign(key.data(), key.size());
          return true;
        }

        std::optional<std::string> value_result = it->second(provider);
        if(!value_result)
        {
          return false;
        }

        result = std::move(*value_result);
        return true;
      }

      return false;
    }

  private:
    std::unordered_map<std::string, ArgFun> processors_;
  };
}
