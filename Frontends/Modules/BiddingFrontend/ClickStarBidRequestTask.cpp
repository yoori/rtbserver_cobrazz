#include <Frontends/FrontendCommons/HTTPUtils.hpp>

#include "KeywordFormatter.hpp"
#include "ClickStarBidRequestTask.hpp"

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
        const String::SubString JSON("application/json");
      }

      namespace Json
      {
        const String::SubString CPC_PRICE("bid");
        const String::SubString TITLE("title");
        const String::SubString DESCRIPTION("description");
        const String::SubString IMAGE("image");
        const String::SubString ICON("icon");
        const String::SubString CLICK_URL("click_url");
        const String::SubString TTL("ttl");
        const unsigned long TTL_VALUE = 86400;
      }
    } // namespace Response

    const String::SubString CLICKSTAR_CLIENT("directnative");
    const String::SubString CLICKSTAR_SIZE("492x328");
    const CampaignSvcs::RevenueDecimal EXPECTED_CTR("0.002");
  }

  ClickStarBidRequestTask::ClickStarBidRequestTask(
    Frontend* bid_frontend,
    FCGI::HttpRequestHolder_var request_holder,
    FCGI::HttpResponseWriter_var response_writer,
    const Generics::Time& start_processing_time)
    /*throw(Invalid)*/
    : BidRequestTask(
        bid_frontend,
        std::move(request_holder),
        std::move(response_writer),
        start_processing_time),
      uri_(request_holder_->request().uri().str())
  {}

  bool
  ClickStarBidRequestTask::read_request() noexcept
  {
    static const char* FUN = "ClickStarBidRequestTask::read_request()";

    std::string bid_request;

    try
    {
      const FCGI::HttpRequest& request = request_holder_->request();

      bid_frontend_->request_info_filler()->adxml_request_info_filler()->fill_by_request(
        *request_params_,
        request_info_,
        keywords_,
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
  ClickStarBidRequestTask::write_response(
    const AdServer::CampaignSvcs::CampaignManager::RequestCreativeResult&
      campaign_match_result)
    noexcept
  {
    //static const char* FUN = "ClickStarBidRequestTask::write_response()";

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
      FCGI::HttpResponse_var response(new FCGI::HttpResponse());
      response->set_content_type(Response::Type::JSON);

      FCGI::OutputStream& output = response->get_output_stream();
      std::string bid_response = response_ostr.str();
      output.write(bid_response.data(), bid_response.size());

      write_response_(200, response);

      return true;
    }

    return false;
  }

  void
  ClickStarBidRequestTask::write_empty_response(unsigned int code)
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
  ClickStarBidRequestTask::clear() noexcept
  {
    BidRequestTask::clear();
    uri_.clear();
  }

  void
  ClickStarBidRequestTask::print_request(std::ostream& /*out*/) const noexcept
  {
    //out << bid_request_;
  }

  void
  ClickStarBidRequestTask::fill_response_(
    std::ostream& response_ostr,
    const RequestInfo& /*request_info*/,
    const AdServer::CampaignSvcs::CampaignManager::RequestParams& /*request_params*/,
    const AdServer::CampaignSvcs::CampaignManager::
      RequestCreativeResult& campaign_match_result)
    noexcept
  {
    static const char* FUN = "ClickStarBidRequestTask::fill_response_()";

    try
    {
      response_ostr << "[";

      for(CORBA::ULong slot_i = 0;
        slot_i < campaign_match_result.ad_slots.length(); ++slot_i)
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
  ClickStarBidRequestTask::fill_response_adslot_(
    std::ostream& response_ostr,
    const AdServer::CampaignSvcs::CampaignManager::AdSlotResult& ad_slot_result)
    noexcept
  {
    AdServer::Commons::JsonFormatter root_json(response_ostr);

    // result cpc price, ecpm is in 0.01/1000
    CampaignSvcs::RevenueDecimal cpc_price =
      CampaignSvcs::RevenueDecimal::div(
        CampaignSvcs::RevenueDecimal::div(
          CorbaAlgs::unpack_decimal<CampaignSvcs::RevenueDecimal>(
            ad_slot_result.selected_creatives[0].pub_ecpm),
          EXPECTED_CTR),
        CampaignSvcs::RevenueDecimal(false, 100000, 0));

    root_json.add_number(Response::Json::CPC_PRICE, cpc_price);

    if(ad_slot_result.native_data_tokens.length() >= 1)
    {
      const AdServer::CampaignSvcs::CampaignManager::TokenInfo& token =
        ad_slot_result.native_data_tokens[0];
      root_json.add_string(
        Response::Json::TITLE,
        String::SubString(token.value));
    }

    if(ad_slot_result.native_data_tokens.length() >= 2)
    {
      const AdServer::CampaignSvcs::CampaignManager::TokenInfo& token =
        ad_slot_result.native_data_tokens[1];
      root_json.add_string(
        Response::Json::DESCRIPTION,
        String::SubString(token.value));
    }

    if(ad_slot_result.native_image_tokens.length() >= 1)
    {
      // NITE_MAIN
      const AdServer::CampaignSvcs::CampaignManager::TokenImageInfo& token =
        ad_slot_result.native_image_tokens[0];
      root_json.add_string(
        Response::Json::IMAGE,
        String::SubString(token.value));
    }

    if(ad_slot_result.native_image_tokens.length() >= 2)
    {
      // NITE_ICON
      const AdServer::CampaignSvcs::CampaignManager::TokenImageInfo& token =
        ad_slot_result.native_image_tokens[1];
      root_json.add_string(
        Response::Json::ICON,
        String::SubString(token.value));
    }

    root_json.add_string(
      Response::Json::CLICK_URL,
      String::SubString(ad_slot_result.selected_creatives[0].click_url));

    root_json.add_number(Response::Json::TTL, Response::Json::TTL_VALUE);
  }

  void
  ClickStarBidRequestTask::add_xml_escaped_string_(
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
}
