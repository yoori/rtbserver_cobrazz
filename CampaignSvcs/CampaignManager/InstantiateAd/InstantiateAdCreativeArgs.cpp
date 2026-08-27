#include "InstantiateAdCreativeArgs.hpp"

#include "../CreativeTextGenerator.hpp"

#include <String/StringManip.hpp>

#include <algorithm>
#include <string_view>
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

    constexpr std::string_view CLICK_F_SUFFIX = "*amp*m*eql*f";
    constexpr std::string_view PRECLICK_SUFFIX = "*amp*relocate*eql*";
    constexpr std::string_view PRECLICK_F_SUFFIX = "*amp*m*eql*f*amp*relocate*eql*";

    std::string
    format_click_url(
      const InstantiateAdContext::CreativeArgsData& data,
      const std::string& base_url,
      std::string_view suffix)
    {
      std::string url;
      url.reserve(base_url.size() + suffix.size());
      url += base_url;
      url.append(suffix.data(), suffix.size());

      if (!data.encode_click_urls || data.click_url_prefix.empty())
      {
        return url;
      }

      std::string encoded_url;
      String::StringManip::mime_url_encode(url, encoded_url);
      return data.click_url_prefix + encoded_url;
    }

    std::optional<std::string>
    optional_creative_click_string(
      std::optional<InstantiateAdContext::CreativeArgsData>& data,
      const std::string CampaignManagerCore::ClickParams::* member,
      std::string_view suffix)
    {
      if (!data || !data->click_params || !data->click_url_initializer)
      {
        return std::nullopt;
      }

      data->init_click_urls();
      const std::string& base_url = (*data->click_params).*member;
      if (base_url.empty())
      {
        return std::nullopt;
      }

      return format_click_url(*data, base_url, suffix);
    }

    std::optional<std::string>
    optional_preclick_string(
      std::optional<InstantiateAdContext::CreativeArgsData>& data,
      const std::string CampaignManagerCore::ClickParams::* member,
      std::string_view suffix)
    {
      if (!data || !data->click_params)
      {
        return std::nullopt;
      }

      const std::string& base_url = (*data->click_params).*member;
      if (base_url.empty())
      {
        return std::nullopt;
      }

      return format_click_url(*data, base_url, suffix);
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

    // Override track pixels for log_as_test mode.
    if (context().request_params &&
      context().request_params->log_as_test &&
      creative_data &&
      creative_data->creative &&
      name.compare(0, CreativeTokens::ADV_TRACK_PIXEL.size(), CreativeTokens::ADV_TRACK_PIXEL) == 0)
    {
      const auto& adv_track_pixel_tokens = creative_data->creative->adv_track_pixel_tokens;
      const auto token_it = std::find_if (
        adv_track_pixel_tokens.begin(),
        adv_track_pixel_tokens.end(),
        [name](const auto& token)
        {
          return std::string_view(token.name) == name;
        });

      if (token_it != adv_track_pixel_tokens.end())
      {
        if (!value)
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

    if (context().ad_slot_context &&
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
        if (!data || data->keyword.empty())
        {
          return std::nullopt;
        }

        return data->keyword;
      });

    add_processor(
      CreativeTokens::CLICKURL,
      [](const InstantiateAdCreativeArgsProvider& provider) {
        return optional_creative_click_string(
          provider.context().creative_args_data,
          &CampaignManagerCore::ClickParams::click_url,
          {});
      });

    add_processor(
      CreativeTokens::CLICKF,
      [](const InstantiateAdCreativeArgsProvider& provider) {
        return optional_creative_click_string(
          provider.context().creative_args_data,
          &CampaignManagerCore::ClickParams::click_url,
          CLICK_F_SUFFIX);
      });

    add_processor(
      CreativeTokens::CLICK0,
      [](const InstantiateAdCreativeArgsProvider& provider) {
        return optional_creative_click_string(
          provider.context().creative_args_data,
          &CampaignManagerCore::ClickParams::click0_url,
          {});
      });

    add_processor(
      CreativeTokens::CLICKF0,
      [](const InstantiateAdCreativeArgsProvider& provider) {
        return optional_creative_click_string(
          provider.context().creative_args_data,
          &CampaignManagerCore::ClickParams::click0_url,
          CLICK_F_SUFFIX);
      });

    add_processor(
      CreativeTokens::PRECLICKURL,
      [](const InstantiateAdCreativeArgsProvider& provider) {
        return optional_preclick_string(
          provider.context().creative_args_data,
          &CampaignManagerCore::ClickParams::click_url,
          PRECLICK_SUFFIX);
      });

    add_processor(
      CreativeTokens::PRECLICKF,
      [](const InstantiateAdCreativeArgsProvider& provider) {
        return optional_preclick_string(
          provider.context().creative_args_data,
          &CampaignManagerCore::ClickParams::click_url,
          PRECLICK_F_SUFFIX);
      });

    add_processor(
      CreativeTokens::PRECLICK0,
      [](const InstantiateAdCreativeArgsProvider& provider) {
        return optional_preclick_string(
          provider.context().creative_args_data,
          &CampaignManagerCore::ClickParams::click0_url,
          PRECLICK_SUFFIX);
      });

    add_processor(
      CreativeTokens::PRECLICKF0,
      [](const InstantiateAdCreativeArgsProvider& provider) {
        return optional_preclick_string(
          provider.context().creative_args_data,
          &CampaignManagerCore::ClickParams::click0_url,
          PRECLICK_F_SUFFIX);
      });

    add_processor(
      CreativeTokens::REQUEST_ID,
      [](const InstantiateAdCreativeArgsProvider& provider)
        -> std::optional<std::string> {
        const auto& data = provider.context().creative_args_data;
        if (!data || !data->select_params)
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
          std::optional<std::string>(to_string(data->creative->campaign->advertiser->account_id)) :
          std::nullopt;
      });

    add_processor(
      CreativeTokens::CGID,
      [](const InstantiateAdCreativeArgsProvider& provider)
        -> std::optional<std::string> {
        const auto& data = provider.context().creative_args_data;
        return data && data->creative ?
          std::optional<std::string>(to_string(data->creative->campaign->campaign_id)) :
          std::nullopt;
      });

    add_processor(
      CreativeTokens::CID,
      [](const InstantiateAdCreativeArgsProvider& provider)
        -> std::optional<std::string> {
        const auto& data = provider.context().creative_args_data;
        return data && data->creative ?
          std::optional<std::string>(to_string(data->creative->campaign->campaign_group_id)) :
          std::nullopt;
      });

    add_processor(
      CreativeTokens::CREATIVE_SIZE,
      [](const InstantiateAdCreativeArgsProvider& provider)
        -> std::optional<std::string> {
        const auto* tag_size = provider.context().tag_size;
        return tag_size ? std::optional<std::string>(tag_size->size->protocol_name) : std::nullopt;
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
