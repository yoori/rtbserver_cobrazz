#include <Frontends/FrontendCommons/HTTPUtils.hpp>

#include "KeywordFormatter.hpp"
#include "AdFoxBidRequestTask.hpp"

namespace AdServer
{
namespace Bidding
{
  namespace
  {
    namespace Aspect
    {
      const char BIDDING_FRONTEND[] = "BiddingFrontend";
    }

    namespace Response
    {
      namespace Header
      {
        const String::SubString CONTENT_TYPE("Content-Type");
      }

      namespace Type
      {
        const String::SubString TEXT_XML("text/xml");
      }

      namespace AdJson
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
    } // namespace Response

    const String::SubString ADFOX_CLIENT("adfox");
  }

  AdFoxBidRequestTask::AdFoxBidRequestTask(
    Frontend* bid_frontend,
    FrontendCommons::HttpRequestHolder_var request_holder,
    FrontendCommons::HttpResponseWriter_var response_writer,
    const Generics::Time& start_processing_time)
    /*throw(Invalid)*/
    : OpenRtbBidRequestTask(
        bid_frontend,
        std::move(request_holder),
        std::move(response_writer),
        start_processing_time),
      uri_(request_holder_->request().uri().str())
  {}

  bool
  AdFoxBidRequestTask::write_response(
    const AdServer::CampaignSvcs::CampaignManager::RequestCreativeResult&
      campaign_match_result)
    noexcept
  {
    //static const char* FUN = "AdFoxBidRequestTask::write_response()";

    AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params =
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
      FrontendCommons::HttpResponse_var response = bid_frontend_->create_response();
      response->set_content_type(Response::Type::TEXT_XML);

      FrontendCommons::OutputStream& output = response->get_output_stream();
      std::string bid_response = response_ostr.str();
      output.write(bid_response.data(), bid_response.size());

      write_response_(200, response);

      return true;
    }

    return false;
  }

  void
  AdFoxBidRequestTask::write_empty_response(unsigned int code)
    noexcept
  {
    FrontendCommons::HttpResponse_var response = bid_frontend_->create_response();

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
  AdFoxBidRequestTask::clear() noexcept
  {
    BidRequestTask::clear();
    uri_.clear();
  }

  void
  AdFoxBidRequestTask::print_request(std::ostream& /*out*/) const noexcept
  {
    //out << bid_request_;
  }

  void
  AdFoxBidRequestTask::fill_response_(
    std::ostream& response_ostr,
    const RequestInfo& /*request_info*/,
    const AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
    const AdServer::CampaignSvcs::CampaignManager::
      RequestCreativeResult& campaign_match_result)
    noexcept
  {
    static const char* FUN = "AdFoxBidRequestTask::fill_response_()";

    try
    {
      //Generics::Time now = Generics::Time::get_time_of_day();

      AdServer::Commons::JsonFormatter root_json(response_ostr);

      assert(campaign_match_result.ad_slots.length() > 0);

      AdServer::Commons::JsonObject bid_array(root_json.add_array(Response::AdJson::BIDS));

      for(unsigned long ad_slot_i = 0; ad_slot_i < campaign_match_result.ad_slots.length(); ++ad_slot_i)
      {
        const AdServer::CampaignSvcs::CampaignManager::
          AdSlotResult& ad_slot_result = campaign_match_result.ad_slots[ad_slot_i];

        AdServer::Commons::JsonObject bid_object(bid_array.add_object());
      
        CampaignSvcs::RevenueDecimal sum_pub_ecpm = CorbaAlgs::unpack_decimal<CampaignSvcs::RevenueDecimal>(
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
      if(ad_slot_result.native_image_tokens.length() > 1)
      {
        const AdServer::CampaignSvcs::CampaignManager::TokenImageInfo& token =
          ad_slot_result.native_image_tokens[1];
        root_json.add_escaped_string(
          Response::AdJson::ICON,
          String::SubString(token.value));
      }

      // image
      if(ad_slot_result.native_image_tokens.length() > 0)
      {
        const AdServer::CampaignSvcs::CampaignManager::TokenImageInfo& token =
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
}
