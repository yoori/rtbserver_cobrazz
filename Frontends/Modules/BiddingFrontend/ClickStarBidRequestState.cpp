#include <Frontends/FrontendCommons/HTTPUtils.hpp>
#include <Commons/JsonFormatter.hpp>

#include "KeywordFormatter.hpp"
#include "ClickStarBidRequestState.hpp"

#include <Commons/GrpcAlgs.hpp>

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
      const std::string CONTENT_TYPE("Content-Type");
    }

    namespace Response::Type
    {
      const std::string JSON("application/json");
    }

    namespace Response::Json
    {
      const std::string CPC_PRICE("bid");
      const std::string TITLE("title");
      const std::string DESCRIPTION("description");
      const std::string IMAGE("image");
      const std::string ICON("icon");
      const std::string CLICK_URL("click_url");
      const std::string TTL("ttl");
      const unsigned long TTL_VALUE = 86400;
    }

    const std::string CLICKSTAR_CLIENT("directnative");
    const std::string CLICKSTAR_SIZE("492x328");
    const CampaignSvcs::RevenueDecimal EXPECTED_CTR("0.002");
  }

  ClickStarBidRequestState::ClickStarBidRequestState(
    BiddingFrontendCore* bid_frontend,
    FCGI::HttpRequestHolder_var request_holder,
    FCGI::BaseHttpResponseWriter_var response_writer,
    const Generics::Time& start_processing_time)
    /*throw(Invalid)*/
    : BidRequestState(
        bid_frontend,
        std::move(request_holder),
        std::move(response_writer),
        start_processing_time),
      uri_(request_holder_->request().uri().str())
  {}

  bool
  ClickStarBidRequestState::read_request() noexcept
  {
    static const char* FUN = "ClickStarBidRequestState::read_request()";

    std::string bid_request;

    try
    {
      const FCGI::HttpRequest& request = request_holder_->request();

      bid_frontend_->request_info_filler()->adxml_request_info_filler()->fill_by_request(
        request_info_,
        request,
        true, // require icon
        CLICKSTAR_CLIENT,
        CLICKSTAR_SIZE);

      return true;
    }
    catch(const FrontendCommons::HTTPExceptions::InvalidParamException& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": bad request, " << ex.what() <<
        ", request: '" << bid_request << "'" <<
        ", uri: '" << uri_ << "'";

      bid_frontend_->logger()->log(
        ostr.str(),
        Logging::Logger::ERROR,
        Aspect::BIDDING_FRONTEND,
        "ADS-IMPL-7601");
    }

    return false;
  }

  bool
  ClickStarBidRequestState::write_response(
    const AdServer::Bidding::CampaignManager::RequestCreativeResult&
      campaign_match_result)
    noexcept
  {
    //static const char* FUN = "ClickStarBidRequestState::write_response()";

    std::ostringstream response_ostr;

    fill_response_(
      response_ostr,
      request_info_,
      campaign_match_result);

    // write response
    const std::string bid_response = response_ostr.str();

    if(!bid_response.empty())
    {
      FCGI::HttpResponse_var response(new FCGI::HttpResponse());
      response->set_content_type_nocopy(Response::Type::JSON);

      FCGI::OutputStream& output = response->get_output_stream();
      std::string bid_response = response_ostr.str();
      output.write(bid_response.data(), bid_response.size());

      write_response_(200, response);

      return true;
    }

    return false;
  }

  void
  ClickStarBidRequestState::write_empty_response(unsigned int code, bool response_claimed)
    noexcept
  {
    FCGI::HttpResponse_var response(new FCGI::HttpResponse());

    if(code < 300)
    {
      // no-bid is No content
      write_response_(204, response, response_claimed);
    }
    else
    {
      write_response_(code, response, response_claimed);
    }
  }

  void
  ClickStarBidRequestState::clear() noexcept
  {
    BidRequestState::clear();
    uri_.clear();
  }

  void
  ClickStarBidRequestState::print_request(std::ostream& /*out*/) const noexcept
  {
    //out << bid_request_;
  }

  void
  ClickStarBidRequestState::fill_response_(
    std::ostream& response_ostr,
    const RequestInfo& /*request_info*/,
    const AdServer::Bidding::CampaignManager::
      RequestCreativeResult& campaign_match_result)
    noexcept
  {
    static const char* FUN = "ClickStarBidRequestState::fill_response_()";

    try
    {
      response_ostr << "[";

      for(std::size_t slot_i = 0;
        slot_i < campaign_match_result.ad_slots.size(); ++slot_i)
      {
        if(slot_i > 0)
        {
          response_ostr << ",";
        }

        fill_response_adslot_(
          response_ostr,
          campaign_match_result.ad_slots[slot_i]);
      }

      response_ostr << "]";
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": error on filling response: " << ex.what();
      bid_frontend_->logger()->log(
        ostr.str(), Logging::Logger::EMERGENCY, Aspect::BIDDING_FRONTEND);
    }
  }

  void
  ClickStarBidRequestState::fill_response_adslot_(
    std::ostream& response_ostr,
    const AdServer::Bidding::CampaignManager::AdSlotResult& ad_slot_result)
    noexcept
  {
    std::string response;
    {
      AdServer::Commons::JsonFormatter root_json(response);

    // result cpc price, ecpm is in 0.01/1000
    CampaignSvcs::RevenueDecimal cpc_price =
      CampaignSvcs::RevenueDecimal::div(
        CampaignSvcs::RevenueDecimal::div(
          GrpcAlgs::unpack_decimal<CampaignSvcs::RevenueDecimal>(
            ad_slot_result.selected_creatives[0].pub_ecpm),
          EXPECTED_CTR),
        CampaignSvcs::RevenueDecimal(false, 100000, 0));

    root_json.add_number(Response::Json::CPC_PRICE, cpc_price);

    if(ad_slot_result.native_data_tokens.size() >= 1)
    {
      const AdServer::Bidding::CampaignManager::ResultTokenInfo& token =
        ad_slot_result.native_data_tokens[0];
      root_json.add_string(
        Response::Json::TITLE,
        token.value);
    }

    if(ad_slot_result.native_data_tokens.size() >= 2)
    {
      const AdServer::Bidding::CampaignManager::ResultTokenInfo& token =
        ad_slot_result.native_data_tokens[1];
      root_json.add_string(
        Response::Json::DESCRIPTION,
        token.value);
    }

    if(ad_slot_result.native_image_tokens.size() >= 1)
    {
      // NITE_MAIN
      const AdServer::Bidding::CampaignManager::ResultTokenImageInfo& token =
        ad_slot_result.native_image_tokens[0];
      root_json.add_string(
        Response::Json::IMAGE,
        token.value);
    }

    if(ad_slot_result.native_image_tokens.size() >= 2)
    {
      // NITE_ICON
      const AdServer::Bidding::CampaignManager::ResultTokenImageInfo& token =
        ad_slot_result.native_image_tokens[1];
      root_json.add_string(
        Response::Json::ICON,
        token.value);
    }

    root_json.add_string(
      Response::Json::CLICK_URL,
      ad_slot_result.selected_creatives[0].click_url);

    root_json.add_number(Response::Json::TTL, Response::Json::TTL_VALUE);
    }
    response_ostr << response;
  }

  void
  ClickStarBidRequestState::add_xml_escaped_string_(
    std::ostream& response_ostr,
    const char* str)
    noexcept
  {
    std::string escaped_str;
    String::StringManip::xml_encode(
      str,
      escaped_str,
      String::StringManip::XU_TEXT | String::StringManip::XU_PRESERVE_UTF8);
    response_ostr << escaped_str;
  }
}
