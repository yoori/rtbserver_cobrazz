#include "InstantiateAdCommonCreativeArgs.hpp"

#include "InstantiateAdRequestArgs.hpp"
#include "../CreativeTextGenerator.hpp"

#include <utility>

namespace AdServer::CampaignSvcs::InstantiateAd
{
  namespace
  {
    namespace AdProtocol
    {
      const char USER_ID_DISTRIBUTION_HASH[] = "h";
      const char PUB_PIXEL_ACCOUNTS[] = "paid";
    }

    template <typename T>
    std::string
    to_string(T i)
    {
      return String::StringManip::IntToStr(i).str().str();
    }

  }

  InstantiateAdCommonCreativeArgsProvider::
  InstantiateAdCommonCreativeArgsProvider(
    std::shared_ptr<const InstantiateAdCommonCreativeArgsManager> manager,
    std::shared_ptr<InstantiateAdContext> context)
    : manager_(std::move(manager)),
      context_(std::move(context))
  {}

  InstantiateAdContext&
  InstantiateAdCommonCreativeArgsProvider::context() const noexcept
  {
    return *context_;
  }

  bool
  InstantiateAdCommonCreativeArgsProvider::get_argument(
    const String::SubString& key,
    std::string& result,
    bool value) const
  {
    return manager_->get_argument(*this, key, result, value);
  }

  InstantiateAdCommonCreativeArgsManager::
  InstantiateAdCommonCreativeArgsManager()
  {
    namespace CreativeTokens = AdServer::CampaignSvcs::CreativeTokens;

    add_processor(
      CreativeTokens::PASSBACK_URL,
      [](const InstantiateAdCommonCreativeArgsProvider& provider)
        -> std::optional<std::string> {
        auto& data = provider.context();
        return data.ad_slot_context->passback_url[0] ?
          std::optional<std::string>(data.ad_slot_context->passback_url) :
          std::nullopt;
      });

    add_processor(
      CreativeTokens::PASSBACK_TYPE,
      [](const InstantiateAdCommonCreativeArgsProvider& provider)
        -> std::optional<std::string> {
        const auto& data = provider.context();
        if(!data.request_params->passback_type.empty())
        {
          return data.request_params->passback_type;
        }

        return data.tag ? std::optional<std::string>(data.tag->passback_type) :
          std::nullopt;
      });

    add_processor(
      CreativeTokens::PASSBACK_PIXEL,
      [](const InstantiateAdCommonCreativeArgsProvider& provider)
        -> std::optional<std::string> {
        auto& data = provider.context();
        if(!data.tag)
        {
          return std::nullopt;
        }

        std::string passback_imp_url =
          data.instantiate_info->passback_pixel_url;
        passback_imp_url += "?requestid=";
        passback_imp_url += data.request_params->request_id.to_string();
        passback_imp_url += "&random=";
        passback_imp_url += to_string(data.request_params->random);

        if(data.ad_slot_context->test_request)
        {
          passback_imp_url += "&testrequest=1";
        }

        if(data.inst_params->user_id_hash_mod.present())
        {
          passback_imp_url += '&';
          passback_imp_url += AdProtocol::USER_ID_DISTRIBUTION_HASH;
          passback_imp_url += '=';
          passback_imp_url +=
            to_string(*data.inst_params->user_id_hash_mod);
        }

        const AccountIdList& consider_pub_pixel_accounts =
          data.consider_pub_pixel_accounts();
        if(!consider_pub_pixel_accounts.empty())
        {
          passback_imp_url += '&';
          passback_imp_url += AdProtocol::PUB_PIXEL_ACCOUNTS;
          passback_imp_url += '=';
          for(auto it = consider_pub_pixel_accounts.begin();
              it != consider_pub_pixel_accounts.end(); ++it)
          {
            if(it != consider_pub_pixel_accounts.begin())
            {
              passback_imp_url += ',';
            }

            passback_imp_url += to_string(*it);
          }
        }

        return std::optional<std::string>(std::move(passback_imp_url));
      });
  }

  std::shared_ptr<InstantiateAdCommonCreativeArgsProvider>
  InstantiateAdCommonCreativeArgsManager::create_provider(
    std::shared_ptr<InstantiateAdContext> context) const
  {
    return std::make_shared<InstantiateAdCommonCreativeArgsProvider>(
      shared_from_this(),
      std::move(context));
  }
}
