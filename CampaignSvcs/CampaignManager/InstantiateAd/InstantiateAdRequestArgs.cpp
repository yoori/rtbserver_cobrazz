#include "InstantiateAdRequestArgs.hpp"

#include "../CreativeInstantiator.hpp"
#include "../CreativeTextGenerator.hpp"

#include <HTTP/UrlAddress.hpp>
#include <LogCommons/AdRequestLogger.hpp>
#include <Commons/ExternalUserIdUtils.hpp>
#include <Generics/CRC.hpp>

#include <utility>

namespace AdServer::CampaignSvcs::InstantiateAd
{
  namespace
  {
    const size_t MAX_UA_TOKEN_SIZE = 500;
    const char ADRIVER_CLICKPIXEL[] =
      "https://![rhost]/cgi-bin/eclick.cgi?xpid=![xpid]";

    template <typename T>
    std::string
    to_string(T i)
    {
      return String::StringManip::IntToStr(i).str().str();
    }

    const char*
    instantiate_user_status(UserStatus user_status) noexcept
    {
      if(user_status == US_OPTIN ||
        user_status == US_TEMPORARY)
      {
        return "1";
      }

      if(user_status == US_OPTOUT ||
        user_status == US_EXTERNALPROBE ||
        user_status == US_NOEXTERNALID)
      {
        return "-1";
      }

      return "0";
    }

    std::optional<std::string>
    get_referer_domain(const InstantiateAdContext& data)
    {
      const auto& request_params = *data.request_params;
      const std::string& referer = request_params.full_referer.empty() ?
        request_params.referer : request_params.full_referer;
      if(referer.empty())
      {
        return std::nullopt;
      }

      try
      {
        HTTP::BrowserAddress referer_url =
          HTTP::BrowserAddress(std::string_view(referer));
        return referer_url.host().str();
      }
      catch(const eh::Exception&)
      {}

      return std::nullopt;
    }

    std::optional<std::string>
    get_pub_pixels_optin(InstantiateAdContext& data)
    {
      const AccountIdList& pubpixel_accounts =
        data.consider_pub_pixel_accounts();
      if(pubpixel_accounts.empty())
      {
        return std::optional<std::string>(std::string());
      }

      std::string result = data.instantiate_info->pub_pixels_optin;
      result += "&aid=";
      for(auto it = pubpixel_accounts.begin(); it != pubpixel_accounts.end(); ++it)
      {
        if(it != pubpixel_accounts.begin())
        {
          result += ',';
        }

        result += to_string(*it);
      }

      return std::optional<std::string>(std::move(result));
    }

    std::optional<std::string>
    get_dns_encoded_uids(const InstantiateAdContext& data)
    {
      const auto& request_params = *data.request_params;
      AdServer::Commons::ExternalUserIdArray user_ids;
      if(!request_params.external_user_id.empty())
      {
        user_ids.push_back(request_params.external_user_id);
      }

      if(!request_params.user_id.is_null())
      {
        user_ids.push_back(
          std::string("/") + request_params.user_id.to_string());
      }

      if(user_ids.empty())
      {
        return std::optional<std::string>(std::string());
      }

      std::string result;
      AdServer::Commons::dns_encode_external_user_ids(result, user_ids);
      return std::optional<std::string>(std::move(result));
    }
  }

  // InstantiateAdRequestArgsProvider impl.
  InstantiateAdRequestArgsProvider::InstantiateAdRequestArgsProvider(
    std::shared_ptr<const InstantiateAdRequestArgsManager> manager,
    std::shared_ptr<InstantiateAdContext> context)
    : manager_(std::move(manager)),
      context_(std::move(context))
  {}

  InstantiateAdContext&
  InstantiateAdRequestArgsProvider::context() const noexcept
  {
    return *context_;
  }

  bool
  InstantiateAdRequestArgsProvider::get_argument(
    const String::SubString& key,
    std::string& result,
    bool value) const
  {
    return manager_->get_argument(*this, key, result, value);
  }

  InstantiateAdRequestArgsManager::InstantiateAdRequestArgsManager()
  {
    namespace CreativeTokens = AdServer::CampaignSvcs::CreativeTokens;

    add_processor(
      CreativeTokens::PUB_PIXELS_OPTIN,
      [](const InstantiateAdRequestArgsProvider& provider) {
        return get_pub_pixels_optin(provider.context());
      });

    add_processor(
      CreativeTokens::PUB_PIXELS_OPTOUT,
      [](const InstantiateAdRequestArgsProvider& provider) {
        InstantiateAdContext& data = provider.context();
        return std::optional<std::string>(
          data.pub_pixels_optout() ?
            data.instantiate_info->pub_pixels_optout : std::string());
      });

    add_processor(
      CreativeTokens::DNS_ENCODED_UIDS,
      [](const InstantiateAdRequestArgsProvider& provider) {
        return get_dns_encoded_uids(provider.context());
      });

    add_processor(
      CreativeTokens::CRVSERVER,
      [](const InstantiateAdRequestArgsProvider& provider) {
        return std::optional<std::string>(
          provider.context().instantiate_info->ad_image_server);
      });

    add_processor(
      CreativeTokens::ADIMAGE_SERVER,
      [](const InstantiateAdRequestArgsProvider& provider) {
        return std::optional<std::string>(
          provider.context().instantiate_info->ad_image_server);
      });

    add_processor(
      CreativeTokens::AD_SERVER,
      [](const InstantiateAdRequestArgsProvider& provider) {
        return std::optional<std::string>(
          provider.context().instantiate_info->ad_server);
      });

    add_processor(
      CreativeTokens::PP,
      [](const InstantiateAdRequestArgsProvider& provider) {
        return std::optional<std::string>(
          provider.context().request_params->pub_param);
      });

    add_processor(
      CreativeTokens::ORIGLINK,
      [](const InstantiateAdRequestArgsProvider& provider) {
        return std::optional<std::string>(
          provider.context().request_params->original_url);
      });

    add_processor(
      CreativeTokens::COHORT,
      [](const InstantiateAdRequestArgsProvider& provider) {
        return std::optional<std::string>(
          provider.context().request_params->cohort);
      });

    add_processor(
      CreativeTokens::TNS_COUNTER_DEVICE_TYPE,
      [](const InstantiateAdRequestArgsProvider& provider) {
        return std::optional<std::string>(
          provider.context().ad_slot_context->tns_counter_device_type);
      });

    add_processor(
      CreativeTokens::APP_FORMAT,
      [](const InstantiateAdRequestArgsProvider& provider) {
        return std::optional<std::string>(
          provider.context().app_format ? provider.context().app_format : "");
      });

    add_processor(
      CreativeTokens::GREQUESTID,
      [](const InstantiateAdRequestArgsProvider& provider) {
        const InstantiateAdContext& data = provider.context();
        return std::optional<std::string>(
          data.signed_request_id_provider ?
            data.signed_request_id_provider() : std::string());
      });

    add_processor(
      CreativeTokens::RANDOM,
      [](const InstantiateAdRequestArgsProvider& provider) {
        return std::optional<std::string>(
          to_string(provider.context().request_params->random));
      });

    add_processor(
      CreativeTokens::REFERER,
      [](const InstantiateAdRequestArgsProvider& provider) {
        const auto& request_params = *provider.context().request_params;
        return std::optional<std::string>(
          request_params.full_referer.empty() ?
            request_params.referer : request_params.full_referer);
      });

    add_processor(
      CreativeTokens::REFERER_DOMAIN,
      [](const InstantiateAdRequestArgsProvider& provider) {
        return get_referer_domain(provider.context());
      });

    add_processor(
      CreativeTokens::REFERER_DOMAIN_HASH,
      [](const InstantiateAdRequestArgsProvider& provider)
        -> std::optional<std::string> {
        auto domain = get_referer_domain(provider.context());
        if(!domain)
        {
          return std::nullopt;
        }

        return std::to_string(Generics::CRC::quick(
          0,
          domain->data(),
          domain->size()));
      });

    add_processor(
      CreativeTokens::ETID,
      [](const InstantiateAdRequestArgsProvider& provider) {
        std::string result;
        const InstantiateAdContext& data = provider.context();
        AdServer::LogProcessing::undisplayable_mime_encode(
          result,
          data.inst_params->ext_tag_id);
        return std::optional<std::string>(std::move(result));
      });

    add_processor(
      CreativeTokens::SOURCE_ID,
      [](const InstantiateAdRequestArgsProvider& provider)
        -> std::optional<std::string> {
        const auto& value = provider.context().request_params->source_id;
        return value.empty() ? std::nullopt :
          std::optional<std::string>(value);
      });

    add_processor(
      CreativeTokens::EXTERNAL_USER_ID,
      [](const InstantiateAdRequestArgsProvider& provider)
        -> std::optional<std::string> {
        const auto& value =
          provider.context().request_params->external_user_id;
        return value.empty() ? std::nullopt :
          std::optional<std::string>(value);
      });

    add_processor(
      CreativeTokens::UNSIGNEDUID,
      [](const InstantiateAdRequestArgsProvider& provider)
        -> std::optional<std::string> {
        const auto& request_params = *provider.context().request_params;
        if(request_params.track_user_id.is_null())
        {
          return std::nullopt;
        }

        return std::optional<std::string>(
          request_params.track_user_id.to_string());
      });

    add_processor(
      CreativeTokens::USER_STATUS,
      [](const InstantiateAdRequestArgsProvider& provider) {
        return std::optional<std::string>(
          instantiate_user_status(
            static_cast<UserStatus>(
              provider.context().request_params->user_status)));
      });

    add_processor(
      CreativeTokens::COLOCATION,
      [](const InstantiateAdRequestArgsProvider& provider) {
        const Colocation* colocation = provider.context().colocation;
        return std::optional<std::string>(
          to_string(colocation ? colocation->colo_id : 0));
      });

    add_processor(
      CreativeTokens::TEST_REQUEST,
      [](const InstantiateAdRequestArgsProvider& provider) {
        return std::optional<std::string>(
          to_string(
            provider.context().ad_slot_context->test_request ? 1 : 0));
      });

    add_processor(
      CreativeTokens::EXT_TRACK_PARAMS,
      [](const InstantiateAdRequestArgsProvider& provider)
        -> std::optional<std::string> {
        const auto& value =
          provider.context().request_params->ext_track_params;
        return value.empty() ? std::nullopt :
          std::optional<std::string>(value);
      });

    add_processor(
      CreativeTokens::REFERER_KW,
      [](const InstantiateAdRequestArgsProvider&) {
        return std::optional<std::string>(std::string());
      });

    add_processor(
      CreativeTokens::CONTEXT_KW,
      [](const InstantiateAdRequestArgsProvider&) {
        return std::optional<std::string>(std::string());
      });

    add_processor(
      CreativeTokens::SEARCH_TR,
      [](const InstantiateAdRequestArgsProvider&) {
        return std::optional<std::string>(std::string());
      });

    add_processor(
      CreativeTokens::TAGID,
      [](const InstantiateAdRequestArgsProvider& provider)
        -> std::optional<std::string> {
        const Tag* tag = provider.context().tag;
        return tag ? std::optional<std::string>(to_string(tag->tag_id)) :
          std::nullopt;
      });

    add_processor(
      CreativeTokens::SITE_ID,
      [](const InstantiateAdRequestArgsProvider& provider)
        -> std::optional<std::string> {
        const Tag* tag = provider.context().tag;
        return tag ? std::optional<std::string>(to_string(tag->site->site_id)) :
          std::nullopt;
      });

    add_processor(
      CreativeTokens::PUBLISHER_ID,
      [](const InstantiateAdRequestArgsProvider& provider)
        -> std::optional<std::string> {
        const Tag* tag = provider.context().tag;
        return tag ?
          std::optional<std::string>(to_string(tag->site->account->account_id)) :
          std::nullopt;
      });

    add_processor(
      CreativeTokens::PUBPIXELS,
      [](const InstantiateAdRequestArgsProvider& provider)
        -> std::optional<std::string> {
        const Tag* tag = provider.context().tag;
        return tag ? std::optional<std::string>(
          tag->site->account->use_pub_pixels ? "1" : "0") : std::nullopt;
      });

    add_processor(
      CreativeTokens::TAGWIDTH,
      [](const InstantiateAdRequestArgsProvider& provider)
        -> std::optional<std::string> {
        const Tag::Size* tag_size = provider.context().tag_size;
        return tag_size ?
          std::optional<std::string>(to_string(tag_size->size->width)) :
          std::nullopt;
      });

    add_processor(
      CreativeTokens::TAGHEIGHT,
      [](const InstantiateAdRequestArgsProvider& provider)
        -> std::optional<std::string> {
        const Tag::Size* tag_size = provider.context().tag_size;
        return tag_size ?
          std::optional<std::string>(to_string(tag_size->size->height)) :
          std::nullopt;
      });

    add_processor(
      CreativeTokens::TAGSIZE,
      [](const InstantiateAdRequestArgsProvider& provider)
        -> std::optional<std::string> {
        const Tag::Size* tag_size = provider.context().tag_size;
        return tag_size ?
          std::optional<std::string>(tag_size->size->protocol_name) :
          std::nullopt;
      });

    add_processor(
      CreativeTokens::PUBPRECLICK,
      [](const InstantiateAdRequestArgsProvider& provider) {
        const auto& request_params = *provider.context().request_params;
        return std::optional<std::string>(
          request_params.request_type == AR_ADRIVER ?
            ADRIVER_CLICKPIXEL : request_params.preclick_url);
      });

    add_processor(
      CreativeTokens::EXTDATA,
      [](const InstantiateAdRequestArgsProvider& provider) {
        auto& data = provider.context();
        if(!data.creative_instantiator ||
          !data.creative_params_list ||
          !data.campaign_config ||
          !data.ad_selection_result ||
          !data.request_result_params ||
          !data.inst_params ||
          !data.instantiate_info ||
          !data.request_params ||
          !data.ad_slot_context)
        {
          return std::optional<std::string>(std::string());
        }

        const AccountIdList& consider_pub_pixel_accounts =
          data.consider_pub_pixel_accounts();
        std::string ext_data;
        data.creative_instantiator->fill_instantiate_url_(
          ext_data,
          AIT_DATA_PARAM_VALUE,
          *data.creative_params_list,
          *data.request_result_params,
          *data.inst_params,
          *data.instantiate_info,
          *data.campaign_config,
          *data.request_params,
          *data.ad_selection_result,
          *data.ad_slot_context,
          data.app_format,
          consider_pub_pixel_accounts,
          false,
          false);

        return std::optional<std::string>(std::move(ext_data));
      });

    add_processor(
      CreativeTokens::TRACKPIXEL,
      [](const InstantiateAdRequestArgsProvider& provider) {
        return std::optional<std::string>(
          provider.context().request_result_params->track_pixel_url);
      });

    add_processor(
      CreativeTokens::TRACKHTMLURL,
      [](const InstantiateAdRequestArgsProvider& provider) {
        return std::optional<std::string>(
          provider.context().request_result_params->track_html_url);
      });

    add_processor(
      CreativeTokens::VIDEOW,
      [](const InstantiateAdRequestArgsProvider& provider)
        -> std::optional<std::string> {
        const unsigned long video_width =
          provider.context().inst_params->video_width;
        return video_width ?
          std::optional<std::string>(to_string(video_width)) : std::nullopt;
      });

    add_processor(
      CreativeTokens::VIDEOH,
      [](const InstantiateAdRequestArgsProvider& provider)
        -> std::optional<std::string> {
        const unsigned long video_height =
          provider.context().inst_params->video_height;
        return video_height ?
          std::optional<std::string>(to_string(video_height)) : std::nullopt;
      });

    add_processor(
      CreativeTokens::IP,
      [](const InstantiateAdRequestArgsProvider& provider) {
        return std::optional<std::string>(
          provider.context().request_params->peer_ip);
      });

    add_processor(
      CreativeTokens::UA,
      [](const InstantiateAdRequestArgsProvider& provider)
        -> std::optional<std::string> {
        const auto& user_agent =
          provider.context().request_params->user_agent;
        if(user_agent.empty())
        {
          return std::nullopt;
        }

        return std::optional<std::string>(
          user_agent.substr(0, MAX_UA_TOKEN_SIZE));
      });

    add_processor(
      CreativeTokens::TAG_TRACK_PIXEL,
      [](const InstantiateAdRequestArgsProvider& provider) {
        const InstantiateAdContext& data = provider.context();
        const auto& request_params = *data.request_params;
        if(!request_params.pub_impr_track_url.empty())
        {
          return std::optional<std::string>(
            request_params.pub_impr_track_url);
        }
        else if(request_params.log_as_test)
        {
          return std::optional<std::string>(
            data.instantiate_info->track_pixel_url);
        }

        return std::optional<std::string>(std::string());
      });

    add_processor(
      CreativeTokens::PUB_POSITION_BOTTOM,
      [](const InstantiateAdRequestArgsProvider& provider)
        -> std::optional<std::string> {
        const auto& request_params = *provider.context().request_params;
        if(request_params.hpos == UNDEFINED_PUB_POSITION_BOTTOM)
        {
          return std::nullopt;
        }

        return std::optional<std::string>(to_string(request_params.hpos));
      });
  }

  std::shared_ptr<InstantiateAdRequestArgsProvider>
  InstantiateAdRequestArgsManager::create_provider(
    std::shared_ptr<InstantiateAdContext> context) const
  {
    return std::make_shared<InstantiateAdRequestArgsProvider>(
      shared_from_this(),
      std::move(context));
  }
}
