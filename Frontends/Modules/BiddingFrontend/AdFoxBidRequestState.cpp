#include <Frontends/FrontendCommons/HTTPUtils.hpp>

#include "KeywordFormatter.hpp"
#include "AdFoxBidRequestState.hpp"

namespace AdServer::Bidding
{
  namespace
  {
    namespace Aspect
    {
      const char BIDDING_FRONTEND[] = "BiddingFrontend";
    }

    namespace Response::Header
    {
        const String::SubString CONTENT_TYPE("Content-Type");
      }

    namespace Response::Type
      {
        const String::SubString TEXT_XML("text/xml");
      }

    namespace Response::AdJson
      {
        const String::SubString BIDS("bids");

        const String::SubString DISPLAY_URL("displayUrl");
        const String::SubString ID("id"); // fill from request
        const String::SubString CPM("cpm");
        const String::SubString CURRENCY("currency");
        const String::SubString CODE_TYPE("codeType"); // = js ?
        const String::SubString PLACEMENT_ID("placementId"); // fill from request
        const String::SubString SIZE("size");
        const String::SubString WIDTH("width");
        const String::SubString HEIGHT("height");
      }

    const String::SubString ADFOX_CLIENT("adfox");
  }

  AdFoxBidRequestState::AdFoxBidRequestState(
    Frontend* bid_frontend,
    FCGI::HttpRequestHolder_var request_holder,
    FCGI::BaseHttpResponseWriter_var response_writer,
    const Generics::Time& start_processing_time)
    /*throw(Invalid)*/
    : OpenRtbBidRequestState(
        bid_frontend,
        std::move(request_holder),
        std::move(response_writer),
        start_processing_time),
      uri_(request_holder_->request().uri().str())
  {}

  bool
  AdFoxBidRequestState::write_response(
    const AdServer::Bidding::CampaignManager::RequestCreativeResult&
      campaign_match_result)
    noexcept
  {
    //static const char* FUN = "AdFoxBidRequestState::write_response()";

    AdServer::Bidding::CampaignManager::RequestParams& request_params =
      *request_params_;

    std::ostringstream response_ostr;

    fill_response_(
      response_ostr,
      request_info(),
      request_params,
      campaign_match_result);

    // write response
    const std::string bid_response = response_ostr.str();

    if(!bid_response.empty())
    {
      FCGI::HttpResponse_var response(new FCGI::HttpResponse());
      response->set_content_type_nocopy(Response::Type::TEXT_XML);

      FCGI::OutputStream& output = response->get_output_stream();
      std::string bid_response = response_ostr.str();
      output.write(bid_response.data(), bid_response.size());

      write_response_(200, response);

      return true;
    }

    return false;
  }

  void
  AdFoxBidRequestState::write_empty_response(unsigned int code)
    noexcept
  {
    FCGI::HttpResponse_var response(new FCGI::HttpResponse());

    if(code < 300)
    {
      // no-bid is No content
      write_response_(204, response);
    }
    else
    {
      write_response_(code, response);
    }
  }

  void
  AdFoxBidRequestState::clear() noexcept
  {
    BidRequestState::clear();
    uri_.clear();
  }

  void
  AdFoxBidRequestState::print_request(std::ostream& /*out*/) const noexcept
  {
    //out << bid_request_;
  }

  void
  AdFoxBidRequestState::fill_response_(
    std::ostream& response_ostr,
    const RequestInfo& /*request_info*/,
    const AdServer::Bidding::CampaignManager::RequestParams& request_params,
    const AdServer::Bidding::CampaignManager::
      RequestCreativeResult& campaign_match_result)
    noexcept
  {
    static const char* FUN = "AdFoxBidRequestState::fill_response_()";

    try
    {
      //Generics::Time now = Generics::Time::get_time_of_day();

      AdServer::Commons::JsonFormatter root_json(response_ostr);

      assert(campaign_match_result.ad_slots.size() > 0);

      AdServer::Commons::JsonObject bid_array(root_json.add_array(Response::AdJson::BIDS));

      for(unsigned long ad_slot_i = 0; ad_slot_i < campaign_match_result.ad_slots.size(); ++ad_slot_i)
      {
        const AdServer::Bidding::CampaignManager::
          AdSlotResult& ad_slot_result = campaign_match_result.ad_slots[ad_slot_i];

        AdServer::Commons::JsonObject bid_object(bid_array.add_object());

        CampaignSvcs::RevenueDecimal sum_pub_ecpm = CampaignManager::unpack_decimal<CampaignSvcs::RevenueDecimal>(
          ad_slot_result.selected_creatives[0].pub_ecpm);

        bid_frontend_->limit_max_cpm_(
          sum_pub_ecpm, request_params.publisher_account_ids);

        // result price, ecpm is in 0.01/1000
        CampaignSvcs::RevenueDecimal adjson_price = CampaignSvcs::RevenueDecimal::div(
          sum_pub_ecpm,
          CampaignSvcs::RevenueDecimal(false, 100000, 0));

        bid_object.add_number(Response::AdJson::CPM, adjson_price);

      // link
      root_json.add_string(
        Response::AdJson::CLICK_URL,
        String::SubString(ad_slot_result.selected_creatives[0].click_url));

      // icon
      if(ad_slot_result.native_image_tokens.size() > 1)
      {
        const AdServer::Bidding::CampaignManager::TokenImageInfo& token =
          ad_slot_result.native_image_tokens[1];
        root_json.add_escaped_string(
          Response::AdJson::ICON,
          String::SubString(token.value));
      }

      // image
      if(ad_slot_result.native_image_tokens.size() > 0)
      {
        const AdServer::Bidding::CampaignManager::TokenImageInfo& token =
          ad_slot_result.native_image_tokens[0];
        root_json.add_escaped_string(
          Response::AdJson::IMAGE, String::SubString(token.value));
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": error on filling response: " << ex.what();
      bid_frontend_->logger()->log(
        ostr.str(), Logging::Logger::EMERGENCY, Aspect::BIDDING_FRONTEND);
    }
  }
}
