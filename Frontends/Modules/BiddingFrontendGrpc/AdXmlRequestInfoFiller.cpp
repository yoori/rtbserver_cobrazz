// THIS
#include <Commons/GrpcAlgs.hpp>
#include <Frontends/FrontendCommons/HTTPUtils.hpp>
#include <Frontends/Modules/BiddingFrontend/KeywordFormatter.hpp>
#include <Frontends/Modules/BiddingFrontendGrpc/RequestInfoFiller.hpp>
#include <Frontends/Modules/BiddingFrontendGrpc/AdXmlRequestInfoFiller.hpp>


namespace AdServer::Bidding::Grpc
{
  namespace
  {
    namespace Params
    {
      const std::string USER_ID = "uid";
      const std::string USER_ID2 = "subsId";
      const std::string USER_IP = "ip";
      const std::string USER_AGENT = "ua";
      const std::string USER_AGENT2 = "userAgent";
      const std::string REFERER("ref");
      const std::string KEYWORDS("kw");
      const std::string CATEGORIES("cat");
    };

    namespace Tokens
    {
      const std::string TITLE = "ADTITLE";
      const std::string DESCRIPTION = "DESCRIPTION2";
      const std::string IMAGE = "ADIMAGE";
      const std::string ICON = "ADICON";
    }

    const std::string DEFAULT_BIDFLOORCUR_CURRENCY = "usd";
    const std::string NATIVE_APP_FORMAT = "native";
  };

  struct AdXmlRequestInfoFiller::Context
  {
    std::string external_user_id;
    std::string ip;
    std::string user_agent;
    HTTP::HTTPAddress referer;
    std::string keywords;
    std::string categories; // TODO: process
  };

  AdXmlRequestInfoFiller::AdXmlRequestInfoFiller(
    RequestInfoFiller* request_info_filler)
    : request_info_filler_(request_info_filler)
  {
    add_param_processor_(
      Params::USER_ID,
      new FrontendCommons::StringParamProcessor<Context>(
        &Context::external_user_id));

    add_param_processor_(
      Params::USER_ID2,
      new FrontendCommons::StringParamProcessor<Context>(
        &Context::external_user_id));

    add_param_processor_(
      Params::USER_IP,
      new FrontendCommons::StringParamProcessor<Context>(
        &Context::ip));

    add_param_processor_(
      Params::USER_AGENT,
      new FrontendCommons::StringParamProcessor<Context>(
        &Context::user_agent));

    add_param_processor_(
      Params::USER_AGENT2,
      new FrontendCommons::StringParamProcessor<Context>(
        &Context::user_agent));

    add_param_processor_(
      Params::REFERER,
      new FrontendCommons::HTTPAddressParamProcessor<Context>(
        &Context::referer));
  }

  void AdXmlRequestInfoFiller::fill_by_request(
    RequestParams& request_params,
    RequestInfo& request_info,
    std::string& keywords,
    const HttpRequest& request,
    bool require_icon,
    const String::SubString& client,
    const String::SubString& size)
  {
    Context context;

    const HTTP::ParamList& params = request.params();

    for (HTTP::ParamList::const_iterator it = params.begin();
      it != params.end(); ++it)
    {
      ParamProcessorMap::const_iterator param_it =
        param_processors_.find(it->name);

      if(param_it != param_processors_.end())
      {
        param_it->second->process(context, it->value);
      }
    }

    keywords.swap(context.keywords);

    request_params.context_info.client =
      std::string_view(client.data(), client.size());
    request_params.common_info.request_type = AdServer::CampaignSvcs::AR_OPENRTB;
    request_params.common_info.user_status = AdServer::CampaignSvcs::US_UNDEFINED;
    request_params.common_info.hpos = 0;

    request_info_filler_->init_request_param(
      request_params, request_info);

    request_params.context_info.enabled_notice = false;
    request_params.fill_track_pixel = false;

    if(!context.external_user_id.empty())
    {
      request_params.common_info.external_user_id =
        !request_info.source_id.empty() ?
          request_info.source_id + "/" + context.external_user_id :
          context.external_user_id;
    }

    KeywordFormatter kw_fmt(request_info.source_id);

    if(request_params.common_info.external_user_id.empty())
    {
      kw_fmt.add_keyword(MatchKeywords::FULL_NO_ID);

      if(!request_info.source_id.empty())
      {
        kw_fmt.add_dict_keyword(MatchKeywords::NO_ID, String::SubString());
      }
    }

    request_info_filler_->fill_by_ip(
      request_info,
      request_params,
      context.ip);

    if(!context.ip.empty())
    {
      kw_fmt.add_ip(String::SubString(context.ip));
    }

    request_info_filler_->fill_by_user_agent(
      request_params,
      request_info,
      context.user_agent,
      request_info.filter_request,
      false);

    request_params.common_info.creative_instantiate_type = std::string_view(
      FrontendCommons::SECURE_INSTANTIATE_TYPE.text().data(),
      FrontendCommons::SECURE_INSTANTIATE_TYPE.text().size());

    request_info_filler_->fill_by_referer(
      request_params,
      request_info.search_words,
      context.referer,
      true, // fill_search_words
      false // fill_instantiate_type
      );

    request_params.context_info.profile_referer = true;

    if(request_params.common_info.referer.empty())
    {
      kw_fmt.add_keyword(MatchKeywords::FULL_NOREF);
    }

    request_params.ad_slots.emplace_back();
    auto& ad_slot = request_params.ad_slots.back();

    RequestInfoFiller::init_adslot(ad_slot);
    ad_slot.format = NATIVE_APP_FORMAT;
    ad_slot.ad_slot_id = 0;
    ad_slot.tag_id = 0;
    ad_slot.fill_track_html = false;
    request_params.fill_track_pixel = true;
    ad_slot.native_ads_impression_tracker_type = AdServer::CampaignSvcs::NAITT_RESOURCES;
    ad_slot.sizes.emplace_back(size.data(), size.size());

    ad_slot.passback = request_info.filter_request;
    ad_slot.tag_visibility = 100;
    ad_slot.tag_predicted_viewability = 100;
    ad_slot.min_ecpm = AdServer::CampaignSvcs::RevenueDecimal::ZERO;
    ad_slot.currency_codes.emplace_back(DEFAULT_BIDFLOORCUR_CURRENCY);
    ad_slot.min_ecpm_currency_code = DEFAULT_BIDFLOORCUR_CURRENCY;

    ad_slot.up_expand_space = -1;
    ad_slot.right_expand_space = -1;
    ad_slot.down_expand_space = -1;
    ad_slot.left_expand_space = -1;
    ad_slot.debug_ccg = 0;

    // fill data asset
    ad_slot.native_data_tokens.emplace_back(Tokens::TITLE, false);
    ad_slot.native_data_tokens.emplace_back(Tokens::DESCRIPTION, false);

    // fill image asset
    ad_slot.native_image_tokens.reserve(require_icon ? 2 : 1);
    ad_slot.native_image_tokens.emplace_back(Tokens::IMAGE, true);
    if (require_icon)
    {
      ad_slot.native_image_tokens.emplace_back(Tokens::ICON, true);
    }

    kw_fmt.assign_to(keywords);
  }

  void AdXmlRequestInfoFiller::add_param_processor_(
    const String::SubString& name,
    RequestParamProcessor* processor) noexcept
  {
    param_processors_.insert(std::make_pair(
      name,
      RequestParamProcessor_var(processor)));
  }
} // namespace AdServer::Bidding::Grpc
