#include "InstantiateAdCreativeArgs.hpp"

#include "../CreativeTextGenerator.hpp"

#include <String/StringManip.hpp>

#include <utility>

namespace AdServer::CampaignSvcs::InstantiateAd
{
  namespace
  {
    template<typename T>
    std::string
    to_string(T value)
    {
      return String::StringManip::IntToStr(value).str().str();
    }

    std::optional<std::string>
    optional_string(
      const std::optional<InstantiateAdContext::CreativeArgsData>& data,
      const std::optional<std::string> InstantiateAdContext::CreativeArgsData::*
        member)
    {
      return data ? (*data).*member : std::nullopt;
    }

    bool
    has_priority_over_ad_slot_tokens(std::string_view name)
    {
      namespace CreativeTokens = AdServer::CampaignSvcs::CreativeTokens;

      return name == CreativeTokens::REQUEST_ID ||
        name == CreativeTokens::CCID ||
        name == CreativeTokens::ADVERTISER_ID ||
        name == CreativeTokens::CGID ||
        name == CreativeTokens::CID ||
        name == CreativeTokens::CREATIVE_SIZE ||
        name == CreativeTokens::TEMPLATE_FORMAT ||
        name == CreativeTokens::ACTIONPIXEL;
    }
  }

  InstantiateAdCreativeArgsProvider::InstantiateAdCreativeArgsProvider(
    std::shared_ptr<const InstantiateAdCreativeArgsManager> manager,
    std::shared_ptr<InstantiateAdContext> context,
    std::shared_ptr<String::TextTemplate::ArgsCallback> source_args)
    : ProviderContext(std::move(context), std::move(source_args)),
      manager_(std::move(manager))
  {}

  bool
  InstantiateAdCreativeArgsProvider::get_argument(
    const String::SubString& key,
    std::string& result,
    bool value) const
  {
    const std::string_view name(key.data(), key.size());
    const auto& creative_data = context().creative_args_data;

    if(has_priority_over_ad_slot_tokens(name) &&
      manager_->get_argument(*this, key, result, value))
    {
      return true;
    }

    if(creative_data &&
      creative_data->creative &&
      context().request_params &&
      context().request_params->log_as_test &&
      name.compare(
        0,
        CreativeTokens::ADV_TRACK_PIXEL.size(),
        CreativeTokens::ADV_TRACK_PIXEL) == 0)
    {
      const auto token_it = creative_data->creative->tokens.find(name);
      if(token_it != creative_data->creative->tokens.end())
      {
        if(!value)
        {
          result.assign(key.data(), key.size());
        }
        else
        {
          result = context().instantiate_info ?
            context().instantiate_info->track_pixel_url : std::string();
        }

        return true;
      }
    }

    if(context().ad_slot_context &&
      context().ad_slot_context->tokens.get_argument(key, result, value))
    {
      return true;
    }

    return manager_->get_argument(*this, key, result, value);
  }

  InstantiateAdCreativeArgsManager::InstantiateAdCreativeArgsManager()
  {
    namespace CreativeTokens = AdServer::CampaignSvcs::CreativeTokens;

    add_processor(
      CreativeTokens::KEYWORD,
      [](const InstantiateAdCreativeArgsProvider& provider)
        -> std::optional<std::string> {
        const auto& data = provider.context().creative_args_data;
        if(!data || data->keyword.empty())
        {
          return std::nullopt;
        }

        return data->keyword;
      });

    add_processor(
      CreativeTokens::CLICKURL,
      [](const InstantiateAdCreativeArgsProvider& provider) {
        return optional_string(
          provider.context().creative_args_data,
          &InstantiateAdContext::CreativeArgsData::click_url);
      });

    add_processor(
      CreativeTokens::CLICKF,
      [](const InstantiateAdCreativeArgsProvider& provider) {
        return optional_string(
          provider.context().creative_args_data,
          &InstantiateAdContext::CreativeArgsData::click_url_f);
      });

    add_processor(
      CreativeTokens::CLICK0,
      [](const InstantiateAdCreativeArgsProvider& provider) {
        return optional_string(
          provider.context().creative_args_data,
          &InstantiateAdContext::CreativeArgsData::click0_url);
      });

    add_processor(
      CreativeTokens::CLICKF0,
      [](const InstantiateAdCreativeArgsProvider& provider) {
        return optional_string(
          provider.context().creative_args_data,
          &InstantiateAdContext::CreativeArgsData::click0_url_f);
      });

    add_processor(
      CreativeTokens::PRECLICKURL,
      [](const InstantiateAdCreativeArgsProvider& provider) {
        return optional_string(
          provider.context().creative_args_data,
          &InstantiateAdContext::CreativeArgsData::preclick_url);
      });

    add_processor(
      CreativeTokens::PRECLICKF,
      [](const InstantiateAdCreativeArgsProvider& provider) {
        return optional_string(
          provider.context().creative_args_data,
          &InstantiateAdContext::CreativeArgsData::preclick_url_f);
      });

    add_processor(
      CreativeTokens::PRECLICK0,
      [](const InstantiateAdCreativeArgsProvider& provider) {
        return optional_string(
          provider.context().creative_args_data,
          &InstantiateAdContext::CreativeArgsData::preclick0_url);
      });

    add_processor(
      CreativeTokens::PRECLICKF0,
      [](const InstantiateAdCreativeArgsProvider& provider) {
        return optional_string(
          provider.context().creative_args_data,
          &InstantiateAdContext::CreativeArgsData::preclick0_url_f);
      });

    add_processor(
      CreativeTokens::REQUEST_ID,
      [](const InstantiateAdCreativeArgsProvider& provider)
        -> std::optional<std::string> {
        const auto& data = provider.context().creative_args_data;
        if(!data || !data->select_params)
        {
          return std::nullopt;
        }

        return data->select_params->request_id.to_string();
      });

    add_processor(
      CreativeTokens::CCID,
      [](const InstantiateAdCreativeArgsProvider& provider)
        -> std::optional<std::string> {
        const auto& data = provider.context().creative_args_data;
        return data && data->creative ?
          std::optional<std::string>(to_string(data->creative->ccid)) :
          std::nullopt;
      });

    add_processor(
      CreativeTokens::ADVERTISER_ID,
      [](const InstantiateAdCreativeArgsProvider& provider)
        -> std::optional<std::string> {
        const auto& data = provider.context().creative_args_data;
        return data && data->creative ?
          std::optional<std::string>(
            to_string(data->creative->campaign->advertiser->account_id)) :
          std::nullopt;
      });

    add_processor(
      CreativeTokens::CGID,
      [](const InstantiateAdCreativeArgsProvider& provider)
        -> std::optional<std::string> {
        const auto& data = provider.context().creative_args_data;
        return data && data->creative ?
          std::optional<std::string>(
            to_string(data->creative->campaign->campaign_id)) :
          std::nullopt;
      });

    add_processor(
      CreativeTokens::CID,
      [](const InstantiateAdCreativeArgsProvider& provider)
        -> std::optional<std::string> {
        const auto& data = provider.context().creative_args_data;
        return data && data->creative ?
          std::optional<std::string>(
            to_string(data->creative->campaign->campaign_group_id)) :
          std::nullopt;
      });

    add_processor(
      CreativeTokens::CREATIVE_SIZE,
      [](const InstantiateAdCreativeArgsProvider& provider)
        -> std::optional<std::string> {
        const auto* tag_size = provider.context().tag_size;
        return tag_size ?
          std::optional<std::string>(tag_size->size->protocol_name) :
          std::nullopt;
      });

    add_processor(
      CreativeTokens::TEMPLATE_FORMAT,
      [](const InstantiateAdCreativeArgsProvider& provider)
        -> std::optional<std::string> {
        const auto& data = provider.context().creative_args_data;
        return data && data->creative ?
          std::optional<std::string>(data->creative->creative_format) :
          std::nullopt;
      });

    add_processor(
      CreativeTokens::ACTIONPIXEL,
      [](const InstantiateAdCreativeArgsProvider&) {
        return std::optional<std::string>(std::string());
      });
  }

  std::shared_ptr<InstantiateAdCreativeArgsProvider>
  InstantiateAdCreativeArgsManager::create_provider(
    std::shared_ptr<InstantiateAdContext> context,
    std::shared_ptr<String::TextTemplate::ArgsCallback> source_args) const
  {
    return std::make_shared<InstantiateAdCreativeArgsProvider>(
      shared_from_this(),
      std::move(context),
      std::move(source_args));
  }
}
