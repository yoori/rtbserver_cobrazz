#include <Commons/CorbaAlgs.hpp>
#include <Frontends/FrontendCommons/HTTPUtils.hpp>

#include "KeywordFormatter.hpp"
#include "AdXmlRequestInfoFiller.hpp"
#include "RequestInfoFiller.hpp"

namespace AdServer
{
namespace Bidding
{
  namespace
  {
    namespace Params
    {
      const String::SubString USER_ID("uid");
      const String::SubString USER_ID2("subsId");

      const String::SubString USER_IP("ip");

      const String::SubString USER_AGENT("ua");
      const String::SubString USER_AGENT2("userAgent");

      const String::SubString REFERER("ref");
      const String::SubString KEYWORDS("kw");
      const String::SubString CATEGORIES("cat");
    }

    namespace Tokens
    {
      const String::SubString TITLE("ADTITLE");
      const String::SubString DESCRIPTION("DESCRIPTION2");
      const String::SubString IMAGE("ADIMAGE");
      const String::SubString ICON("ADICON");
    }

    const String::SubString DEFAULT_BIDFLOORCUR_CURRENCY("usd");
    const String::SubString NATIVE_APP_FORMAT("native");
  }

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
    AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
    RequestInfo& request_info,
    std::string& keywords,
    const FrontendCommons::HttpRequest& request,
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
    keywords.swap(context.keywords);

    request_params.context_info.client << client;
    //request_params.context_info.client_version;
    request_params.common_info.request_type = AdServer::CampaignSvcs::AR_OPENRTB;
    request_params.common_info.user_status = static_cast<CORBA::ULong>(
      AdServer::CampaignSvcs::US_UNDEFINED);
    request_params.common_info.hpos = 0;

    request_info_filler_->init_request_param(
      request_params, request_info);

    request_params.context_info.enabled_notice = false;
    request_params.fill_track_pixel = false;

    if(!context.external_user_id.empty())
    {
      request_params.common_info.external_user_id << (
        !request_info.source_id.empty() ?
          request_info.source_id + "/" + context.external_user_id :
          context.external_user_id);
    }

    KeywordFormatter kw_fmt(request_info.source_id);

    if(request_params.common_info.external_user_id[0] == 0)
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

    request_params.common_info.creative_instantiate_type <<
      FrontendCommons::SECURE_INSTANTIATE_TYPE;

    request_info_filler_->fill_by_referer(
      request_params,
      request_info.search_words,
      context.referer,
      true, // fill_search_words
      false // fill_instantiate_type
      );

    request_params.context_info.profile_referer = true;

    if(request_params.common_info.referer[0] == 0)
    {
      kw_fmt.add_keyword(MatchKeywords::FULL_NOREF);
    }

    request_params.ad_slots.length(1);

    AdServer::CampaignSvcs::CampaignManager::AdSlotInfo& ad_slot_request =
      request_params.ad_slots[0];

    RequestInfoFiller::init_adslot(ad_slot_request);
    ad_slot_request.format << NATIVE_APP_FORMAT;
    ad_slot_request.ad_slot_id = 0;
    ad_slot_request.tag_id = 0;
    ad_slot_request.fill_track_html = false;
    request_params.fill_track_pixel = true;
    ad_slot_request.native_ads_impression_tracker_type = AdServer::CampaignSvcs::NAITT_RESOURCES;
    ad_slot_request.sizes.length(1);
    ad_slot_request.sizes[0] << size;
    ad_slot_request.passback = request_info.filter_request;
    ad_slot_request.tag_visibility = 100;
    ad_slot_request.tag_predicted_viewability = 100;
    ad_slot_request.min_ecpm = CorbaAlgs::pack_decimal(AdServer::CampaignSvcs::RevenueDecimal::ZERO);
    ad_slot_request.currency_codes.length(1);
    ad_slot_request.currency_codes[0] << DEFAULT_BIDFLOORCUR_CURRENCY;
    ad_slot_request.min_ecpm_currency_code << DEFAULT_BIDFLOORCUR_CURRENCY;

    ad_slot_request.up_expand_space = -1;
    ad_slot_request.right_expand_space = -1;
    ad_slot_request.down_expand_space = -1;
    ad_slot_request.left_expand_space = -1;
    ad_slot_request.debug_ccg = 0;
    //ad_slot_request.ext_tag_id = "";

    // fill data asset
    ad_slot_request.native_data_tokens.length(2);
    ad_slot_request.native_data_tokens[0].name << Tokens::TITLE;
    ad_slot_request.native_data_tokens[0].required = false;
    ad_slot_request.native_data_tokens[1].name << Tokens::DESCRIPTION;
    ad_slot_request.native_data_tokens[1].required = false;

    // fill image asset
    ad_slot_request.native_image_tokens.length(require_icon ? 2 : 1);
    ad_slot_request.native_image_tokens[0].name << Tokens::IMAGE;
    ad_slot_request.native_image_tokens[0].required = true;
    if(require_icon)
    {
      ad_slot_request.native_image_tokens[1].name << Tokens::ICON;
      ad_slot_request.native_image_tokens[1].required = true;
    }

    //
    kw_fmt.assign_to(keywords);
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
}
