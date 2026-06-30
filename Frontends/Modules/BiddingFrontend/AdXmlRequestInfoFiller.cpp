#include <Frontends/FrontendCommons/HTTPUtils.hpp>

#include "KeywordFormatter.hpp"
#include "AdXmlRequestInfoFiller.hpp"
#include "RequestInfoFiller.hpp"

namespace AdServer::Bidding
{
  namespace
  {
    namespace Params
    {
      const std::string USER_ID("uid");
      const std::string USER_ID2("subsId");

      const std::string USER_IP("ip");

      const std::string USER_AGENT("ua");
      const std::string USER_AGENT2("userAgent");

      const std::string REFERER("ref");
      const std::string KEYWORDS("kw");
      const std::string CATEGORIES("cat");
    };

    namespace Tokens
    {
      const std::string TITLE("ADTITLE");
      const std::string DESCRIPTION("DESCRIPTION2");
      const std::string IMAGE("ADIMAGE");
      const std::string ICON("ADICON");
    }

    const std::string DEFAULT_BIDFLOORCUR_CURRENCY("usd");
    const std::string NATIVE_APP_FORMAT("native");
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

  void
  AdXmlRequestInfoFiller::fill_by_request(
    RequestInfo& request_info,
    const FCGI::HttpRequest& request,
    bool require_icon,
    const String::SubString& client,
    const String::SubString& size)
  {
    Context context;

    const HTTP::ParamList& params = request.params();

    for(HTTP::ParamList::const_iterator it = params.begin();
      it != params.end(); ++it)
    {
      ParamProcessorMap::const_iterator param_it =
        param_processors_.find(it->name);

      if(param_it != param_processors_.end())
      {
        param_it->second->process(context, it->value);
      }
    }

    //
    request_info.keywords.assign(
      context.keywords.data(),
      context.keywords.size());

    request_info.client = request_info.hold_string(client.str());
    //request_info.client_version;
    request_info.request_type = AdServer::CampaignSvcs::AR_OPENRTB;
    request_info.user_status = static_cast<std::size_t>(
      AdServer::CampaignSvcs::US_UNDEFINED);
    request_info.hpos = 0;

    request_info_filler_->init_request_param(request_info);

    request_info.enabled_notice = false;
    request_info.fill_track_pixel = false;

    if(!context.external_user_id.empty())
    {
      request_info.external_user_id = (
        !request_info.source_id.empty() ?
          std::string(request_info.source_id.data(), request_info.source_id.size()) +
            "/" + context.external_user_id :
          context.external_user_id);
    }

    KeywordFormatter kw_fmt(request_info.source_id);

    if(request_info.external_user_id.empty())
    {
      kw_fmt.add_keyword(MatchKeywords::FULL_NO_ID);

      if(!request_info.source_id.empty())
      {
        kw_fmt.add_dict_keyword(MatchKeywords::NO_ID, std::string_view());
      }
    }

    if(!context.ip.empty())
    {
      const std::string_view ip = request_info.hold_string(
        std::move(context.ip));
      request_info_filler_->fill_by_ip(
        request_info,
        ip);
      kw_fmt.add_ip(ip);
    }
    else
    {
      request_info_filler_->fill_by_ip(
        request_info,
        std::string_view());
    }

    request_info_filler_->fill_by_user_agent(
      request_info,
      request_info.hold_string(std::move(context.user_agent)),
      request_info.filter_request,
      false);

    request_info.creative_instantiate_type = FrontendCommons::SECURE_INSTANTIATE_TYPE;

    request_info_filler_->fill_by_referer(
      request_info,
      request_info.search_words,
      context.referer,
      true, // fill_search_words
      false // fill_instantiate_type
      );

    request_info.profile_referer = true;

    if(request_info.referer.empty())
    {
      kw_fmt.add_keyword(MatchKeywords::FULL_NOREF);
    }

    request_info.ad_slots.clear();
    request_info.ad_slots.reserve(1);
    request_info.ad_slots.emplace_back(request_info.resource());

    AdServer::Bidding::RequestInfo::AdSlotInfo& ad_slot_request =
      request_info.ad_slots[0];

    RequestInfoFiller::init_adslot(ad_slot_request);
    ad_slot_request.format = NATIVE_APP_FORMAT;
    ad_slot_request.ad_slot_id = 0;
    ad_slot_request.tag_id = 0;
    ad_slot_request.fill_track_html = false;
    request_info.fill_track_pixel = true;
    ad_slot_request.native_ads_impression_tracker_type = AdServer::CampaignSvcs::NAITT_RESOURCES;
    ad_slot_request.sizes.resize(1);
    ad_slot_request.sizes[0] = ad_slot_request.hold_string(RequestInfo::PmrString(
      size.data(),
      size.size(),
      ad_slot_request.resource()));
    ad_slot_request.passback = request_info.filter_request;
    ad_slot_request.tag_visibility = 100;
    ad_slot_request.tag_predicted_viewability = 100;
    ad_slot_request.min_ecpm = AdServer::CampaignSvcs::RevenueDecimal::ZERO;
    ad_slot_request.currency_codes.resize(1);
    ad_slot_request.currency_codes[0] = DEFAULT_BIDFLOORCUR_CURRENCY;
    ad_slot_request.min_ecpm_currency_code =
      DEFAULT_BIDFLOORCUR_CURRENCY;

    ad_slot_request.up_expand_space = -1;
    ad_slot_request.right_expand_space = -1;
    ad_slot_request.down_expand_space = -1;
    ad_slot_request.left_expand_space = -1;
    ad_slot_request.debug_ccg = 0;
    //ad_slot_request.ext_tag_id = "";

    // fill data asset
    ad_slot_request.native_data_tokens.resize(2);
    ad_slot_request.native_data_tokens[0].name = Tokens::TITLE;
    ad_slot_request.native_data_tokens[0].required = false;
    ad_slot_request.native_data_tokens[1].name = Tokens::DESCRIPTION;
    ad_slot_request.native_data_tokens[1].required = false;

    // fill image asset
    ad_slot_request.native_image_tokens.resize(require_icon ? 2 : 1);
    ad_slot_request.native_image_tokens[0].name = Tokens::IMAGE;
    ad_slot_request.native_image_tokens[0].required = true;
    if(require_icon)
    {
      ad_slot_request.native_image_tokens[1].name = Tokens::ICON;
      ad_slot_request.native_image_tokens[1].required = true;
    }

    //
    kw_fmt.assign_to(request_info.keywords);
  }

  void
  AdXmlRequestInfoFiller::add_param_processor_(
    const String::SubString& name,
    RequestParamProcessor* processor)
    noexcept
  {
    param_processors_.insert(std::make_pair(
      name,
      RequestParamProcessor_var(processor)));
  }
}
