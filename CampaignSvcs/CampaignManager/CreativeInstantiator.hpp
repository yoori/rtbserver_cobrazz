#pragma once

#include <memory>

#include "CampaignManagerCore.hpp"
#include "CreativeInstantiatorTypes.hpp"

namespace AdServer::CampaignSvcs::InstantiateAd
{
  class InstantiateAdCommonCreativeArgsManager;
  class InstantiateAdCreativeArgsManager;
  class InstantiateAdRequestArgsManager;
}

namespace AdServer::CampaignSvcs
{
  class CreativeInstantiator:
    public CreativeInstantiatorTypes
  {
  public:
    using Exception = CampaignManagerCore::Exception;
    using CreativeInstantiateProblem = CampaignManagerCore::CreativeInstantiateProblem;
    using CreativeTemplateProblem = CampaignManagerCore::CreativeTemplateProblem;
    using CreativeOptionsProblem = CampaignManagerCore::CreativeOptionsProblem;

    using ClickParams = CampaignManagerCore::ClickParams;
    using AdSlotContext = CampaignManagerCore::AdSlotContext;
    using CommonAdRequest = CampaignManagerCore::CommonAdRequest;
    using CountryList = CampaignManagerCore::CountryList;
    using CreativeParams = CampaignManagerCore::CreativeParams;
    using CreativeParamsList = CampaignManagerCore::CreativeParamsList;
    using GetAdRequest = CampaignManagerCore::GetAdRequest;
    using IdArray = CampaignManagerCore::IdArray;
    using ImageToken = CampaignManagerCore::ImageToken;
    using InstantiateParams = CampaignManagerCore::InstantiateParams;
    using NativeDataTokenInfo = CampaignManagerCore::NativeDataTokenInfo;
    using NativeImageTokenInfo = CampaignManagerCore::NativeImageTokenInfo;
    using PreviewCreativeParams = CampaignManagerCore::PreviewCreativeParams;
    using RequestResultParams = CampaignManagerCore::RequestResultParams;
    using TokenToParamMap = CampaignManagerCore::TokenToParamMap;
    using AdSlotRequest = CampaignManagerCore::AdSlotRequest;

    struct Config
    {
      std::string service_index;
      std::string post_instantiate_script_mime_format;
      std::string post_instantiate_iframe_mime_format;
      std::string post_instantiate_script_template_file;
      std::string post_instantiate_iframe_template_file;
      std::string instantiate_track_html_file;
    };

    CreativeInstantiator(
      const Config& config,
      const CreativeInstantiate& creative_instantiate,
      PassbackTemplateMap& passback_templates,
      const TokenToParamMap& token_to_parameters,
      Commons::IPCrypter* ip_crypter,
      const Generics::SignedUuidGenerator& rid_signer,
      Logging::Logger* logger,
      const CountryList& country_whitelist) noexcept;

    void
    instantiate_creative(
      const CommonAdRequest& request_params,
      const CampaignConfig* campaign_config,
      const Colocation* colocation,
      const InstantiateParams& inst_params,
      const char* format,
      AdSelectionResult& ad_selection_result,
      RequestResultParams& request_result_params,
      CreativeParamsList& creative_instantiate_info,
      std::string& creative_body,
      const AdSlotContext& ad_slot_context,
      const Generics::MonoVector<unsigned long>* exclude_pubpixel_accounts,
      std::shared_ptr<String::TextTemplate::ArgsCallback>* instantiate_args_out =
        nullptr,
      bool fill_click_url_result = false)
      /*throw(CreativeTemplateProblem, CreativeOptionsProblem, eh::Exception)*/;

    void
    instantiate_creative_body(
      AdInstantiateType ad_instantiate_type,
      const GetAdRequest& request_params,
      const CampaignConfig* config,
      const Colocation* colocation,
      const char* cr_size,
      const AdSlotRequest& ad_slot,
      AdSelectionResult& ad_selection_result,
      RequestResultParams& request_result_params,
      CreativeParamsList& creative_params_list,
      std::string& creative_body,
      std::string& creative_url,
      const AdSlotContext& ad_slot_context,
      const String::SubString& ext_tag_id,
      bool fill_click_url_result = false)
      /*throw(CreativeTemplateProblem, CreativeOptionsProblem, eh::Exception)*/;

    bool
    instantiate_creative_preview(
      const PreviewCreativeParams& params,
      const CampaignConfig& campaign_config,
      const Campaign* campaign,
      const Creative* creative,
      const Tag* tag,
      const Tag::Size& tag_size,
      std::string& creative_body)
      /*throw(eh::Exception)*/;

    bool
    instantiate_passback(
      std::string& mime_format,
      std::string& passback_body,
      const CampaignConfig& campaign_config,
      const Colocation* colocation,
      const Tag* tag,
      const char* app_format,
      const GetAdRequest& request_params,
      const AdSlotContext& ad_slot_context,
      const String::SubString& ext_tag_id)
      /*throw(eh::Exception)*/;

    void
    instantiate_click_url(
      const CampaignConfig& campaign_config,
      const OptionValue& click_url,
      std::string& result_click_url,
      const unsigned long* colo_id,
      const Tag* tag,
      const Tag::Size* tag_size,
      const Creative* creative,
      const CampaignKeywordBase* campaign_keyword,
      const TokenValueMap& tokens)
      /*throw(CreativeOptionsProblem, eh::Exception)*/;

    void
    fill_instantiate_url_(
      std::string& instantiate_url,
      AdInstantiateType ad_instantiate_type,
      CreativeParamsList& creative_params_list,
      const RequestResultParams& request_result_params,
      const InstantiateParams& inst_params,
      const CreativeInstantiateRule& instantiate_info,
      const CampaignConfig& campaign_config,
      const CommonAdRequest& request_params,
      const AdSelectionResult& ad_selection_result,
      const AdSlotContext& ad_slot_context,
      const char* app_format,
      const AccountIdList& pub_pixel_accounts,
      bool fill_auction_price,
      bool fill_creative_params)
      /*throw(CreativeTemplateProblem, CreativeOptionsProblem, eh::Exception)*/;

    void
    get_inst_optin_pub_pixel_account_ids_(
      AccountIdList& result_account_ids,
      const CampaignConfig* campaign_config,
      const Tag* tag,
      const CommonAdRequest& request_params,
      const Generics::MonoVector<unsigned long>& exclude_pubpixel_accounts)
      noexcept;

    PubPixelAccountMap::const_iterator
    find_pub_pixel_accounts_(
      const CampaignConfig* campaign_config,
      const char* country,
      UserStatus user_status) const
      noexcept;

  private:
    void
    fill_passback_request_params_(
      TokenValueMap& request_args,
      AccountIdList* consider_pub_pixel_accounts,
      bool fill_pub_pixels,
      const CampaignConfig& campaign_config,
      const Colocation* colocation,
      const Tag* tag,
      const Tag::Size* tag_size,
      const char* app_format,
      const CommonAdRequest& request_params,
      const AccountIdList* pubpixel_accounts,
      const Generics::MonoVector<unsigned long>* exclude_pubpixel_accounts,
      const CreativeInstantiateRule& instantiate_info,
      const AdSlotContext& ad_slot_context,
      const String::SubString& ext_tag_id)
      /*throw(eh::Exception)*/;

    std::shared_ptr<String::TextTemplate::ArgsCallback>
    fill_instantiate_creative_args_(
      const CommonAdRequest& request_params,
      const CampaignConfig* campaign_config,
      const Colocation* colocation,
      const CreativeTemplate& template_descr,
      const Template* creative_template,
      const InstantiateParams& inst_params,
      const CreativeInstantiateRule& instantiate_info,
      const char* app_format,
      AdSelectionResult& ad_selection_result,
      RequestResultParams& request_result_params,
      CreativeParamsList& creative_params_list,
      const AdSlotContext& ad_slot_context,
      const Generics::MonoVector<unsigned long>* exclude_pubpixel_accounts,
      bool fill_click_url_result)
      /*throw(eh::Exception)*/;

    static void
    fill_iurl_(
      std::string& iurl,
      const CampaignConfig* campaign_config,
      const CreativeInstantiateRule& instantiate_info,
      const CommonAdRequest& request_params,
      const Creative* creative,
      const Size* size)
      noexcept;

    void
    fill_track_urls_(
      const AdSelectionResult& ad_selection_result,
      RequestResultParams& request_result_params,
      const CommonAdRequest& request_params,
      bool track_impressions,
      const InstantiateParams& inst_params,
      const CreativeInstantiateRule& instantiate_info,
      const AccountIdList* consider_pub_pixel_accounts)
      noexcept;

    TemplateParams_var
    fill_creatives_json_(const TemplateParamsList& creative_template_params) const;

    void
    instantiate_url_creative_(
      std::string& creative_body,
      RequestResultParams& request_result_params,
      const AdSelectionResult& ad_selection_result,
      const String::SubString& instantiate_url,
      AdInstantiateType ad_instantiate_type)
      /*throw(CreativeTemplateProblem)*/;

    void
    init_instantiate_url_(
      std::string& instantiate_url,
      AdInstantiateType ad_instantiate_type,
      CreativeParamsList& creative_params_list,
      RequestResultParams& request_result_params,
      const CampaignConfig& campaign_config,
      const Tag* tag,
      const InstantiateParams& inst_params,
      const CreativeInstantiateRule& instantiate_info,
      const CommonAdRequest& request_params,
      const AdSelectionResult& ad_selection_result,
      const AdSlotContext& ad_slot_context,
      const char* app_format,
      const Generics::MonoVector<unsigned long>& exclude_pubpixel_accounts,
      bool fill_auction_price = true)
      /*throw(CreativeTemplateProblem, CreativeOptionsProblem, eh::Exception)*/;

    void
    init_yandex_tokens_(
      const CampaignConfig* campaign_config,
      const CreativeInstantiateRule& instantiate_info,
      RequestResultParams& request_result_params,
      const CommonAdRequest& request_params,
      const AdSlotContext& ad_slot_context,
      const Creative* creative)
      /*throw(CreativeInstantiateProblem)*/;

    void
    init_track_pixels_(
      const CampaignConfig* campaign_config,
      RequestResultParams& request_result_params,
      const CommonAdRequest& request_params,
      const AdSlotContext& ad_slot_context,
      const Creative* creative,
      const CreativeInstantiateRule& instantiate_info,
      bool need_absolute_urls);

    void
    init_native_tokens_(
      const CampaignConfig* campaign_config,
      const CreativeInstantiateRule& instantiate_info,
      RequestResultParams& request_result_params,
      const CommonAdRequest& request_params,
      const AdSlotRequest& ad_slot,
      const AdSlotContext& ad_slot_context,
      const Creative* creative,
      const String::TextTemplate::ArgsCallback& instantiate_args)
      /*throw(CreativeInstantiateProblem)*/;

    void
    fill_yandex_track_params_(
      std::string& yandex_track_params,
      const CommonAdRequest& request_params,
      const AdSlotContext& ad_slot_context)
      noexcept;

    void
    init_vast_tokens_(RequestResultParams& request_result_params, const Creative* creative)
      /*throw(CreativeInstantiateProblem)*/;

    std::string
    init_click_params0_(
      const AdServer::Commons::RequestId& request_id,
      const Colocation* colocation,
      const Creative* creative,
      const Tag* tag,
      const Tag::Size* tag_size,
      const CampaignKeyword* campaign_keyword,
      const RevenueDecimal& ctr,
      const InstantiateParams& inst_params,
      const CommonAdRequest& request_params,
      const AdSlotContext& ad_slot_context)
      noexcept;

    void
    init_click_url_(
      ClickParams& click_params,
      const Colocation* colocation,
      const Tag* tag,
      const Tag::Size* tag_size,
      const InstantiateParams& inst_params,
      const CommonAdRequest& request_params,
      const AdSlotContext& ad_slot_context,
      const CampaignSelectionData& select_params,
      const std::string& base_click_url);

    void
    fill_creative_instantiate_args_(
      CreativeInstantiateArgs& creative_instantiate_args,
      const CreativeInstantiateRule& creative_instantiate_rule,
      const Creative* creative,
      const ClickParams& click_params,
      unsigned long random)
      noexcept;

    void
    get_pub_pixel_account_ids_(
      AccountArray& result_account_ids,
      const CampaignConfig* campaign_config,
      const char* country,
      UserStatus user_status,
      const AccountIdSet& exclude_publisher_account_ids,
      unsigned long limit)
      noexcept;

  private:
    const Config config_;
    const CreativeInstantiate& creative_instantiate_;
    PassbackTemplateMap& passback_templates_;
    const TokenToParamMap& token_to_parameters_;
    Commons::IPCrypter_var ip_crypter_;
    const Generics::SignedUuidGenerator& rid_signer_;
    Logging::Logger_var logger_;
    const CountryList& country_whitelist_;
    const std::shared_ptr<
      const InstantiateAd::InstantiateAdCommonCreativeArgsManager>
      common_creative_args_manager_;
    const std::shared_ptr<const InstantiateAd::InstantiateAdCreativeArgsManager>
      creative_args_manager_;
    const std::shared_ptr<const InstantiateAd::InstantiateAdRequestArgsManager>
      request_args_manager_;
  };
}
