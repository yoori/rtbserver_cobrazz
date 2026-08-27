#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>

#include "../CampaignConfig.hpp"
#include "../CampaignManagerCore.hpp"

namespace AdServer::CampaignSvcs
{
  class CreativeInstantiator;

  namespace InstantiateAd
  {
    struct InstantiateAdContext
    {
      using SignedRequestIdProvider = std::function<std::string()>;

      AccountIdList&
      consider_pub_pixel_accounts();

      bool
      pub_pixels_optout();

      struct CreativeArgsData
      {
        using ClickUrlInitializer = std::function<void(CreativeArgsData&)>;

        void
        init_click_urls();

        const CampaignSelectionData* select_params = nullptr;
        const Creative* creative = nullptr;
        std::string keyword;
        std::optional<CampaignManagerCore::ClickParams> click_params;
        std::string click_url_prefix;
        ClickUrlInitializer click_url_initializer;
        bool encode_click_urls = false;
        bool click_urls_initialized = false;
      };

      const CampaignManagerCore::CommonAdRequest* request_params = nullptr;
      const CampaignManagerCore::RequestResultParams* request_result_params = nullptr;
      const CampaignManagerCore::InstantiateParams* inst_params = nullptr;
      CampaignManagerCore::CreativeParamsList* creative_params_list = nullptr;
      CreativeInstantiator* creative_instantiator = nullptr;
      const CampaignConfig* campaign_config = nullptr;
      const Colocation* colocation = nullptr;
      const Tag* tag = nullptr;
      const Tag::Size* tag_size = nullptr;
      const AdSelectionResult* ad_selection_result = nullptr;
      const CreativeInstantiateRule* instantiate_info = nullptr;
      const CampaignManagerCore::AdSlotContext* ad_slot_context = nullptr;
      const Generics::MonoVector<unsigned long>* exclude_pubpixel_accounts = nullptr;
      const char* app_format = "";
      bool force_generate_pubpixel_accounts = false;
      SignedRequestIdProvider signed_request_id_provider;
      std::optional<CreativeArgsData> creative_args_data;
      std::optional<AdInstances::AccountIdList>
        consider_pub_pixel_accounts_cache;
      std::optional<bool> pub_pixels_optout_cache;
    };
  }
}
