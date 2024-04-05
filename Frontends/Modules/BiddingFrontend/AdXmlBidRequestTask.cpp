#include <Frontends/FrontendCommons/HTTPUtils.hpp>

#include "KeywordFormatter.hpp"
#include "AdXmlBidRequestTask.hpp"

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
    } // namespace Response

    const String::SubString ADXML_CLIENT("adxml");
    const String::SubString ADXML_SIZE("300x300");
  }

  AdXmlBidRequestTask::AdXmlBidRequestTask(
    Frontend* bid_frontend,
    FrontendCommons::HttpRequestHolder_var request_holder,
    FrontendCommons::HttpResponseWriter_var response_writer,
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
  AdXmlBidRequestTask::read_request() noexcept
  {
    static const char* FUN = "AdXmlBidRequestTask::read_request()";

    std::string bid_request;

    try
    {
      const FrontendCommons::HttpRequest& request = request_holder_->request();

      bid_frontend_->request_info_filler()->adxml_request_info_filler()->fill_by_request(
        *request_params_,
        request_info_,
        keywords_,
        request,
        false,
        ADXML_CLIENT,
        ADXML_SIZE);

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
  AdXmlBidRequestTask::write_response(
    const AdServer::CampaignSvcs::CampaignManager::RequestCreativeResult&
      campaign_match_result)
    noexcept
  {
    //static const char* FUN = "AdXmlBidRequestTask::write_response()";

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
  AdXmlBidRequestTask::write_empty_response(unsigned int code)
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
  AdXmlBidRequestTask::clear() noexcept
  {
    BidRequestTask::clear();
    uri_.clear();
  }

  void
  AdXmlBidRequestTask::print_request(std::ostream& /*out*/) const noexcept
  {
    //out << bid_request_;
  }

  void
  AdXmlBidRequestTask::fill_response_(
    std::ostream& response_ostr,
    const RequestInfo& /*request_info*/,
    const AdServer::CampaignSvcs::CampaignManager::RequestParams& /*request_params*/,
    const AdServer::CampaignSvcs::CampaignManager::
      RequestCreativeResult& campaign_match_result)
    noexcept
  {
    static const char* FUN = "AdXmlBidRequestTask::fill_response_()";

    try
    {
      Generics::Time now = Generics::Time::get_time_of_day();

      response_ostr << "<xml version=\"11.5.2.8\">\n"
        "<result requesttime=\"" <<
        (now - start_processing_time()).get_gm_time().format("%s.%q") << "\">\n";

      for(CORBA::ULong slot_i = 0;
        slot_i < campaign_match_result.ad_slots.length(); ++slot_i)
      {
        fill_response_adslot_(
          response_ostr,
          campaign_match_result.ad_slots[slot_i]);
      }

      response_ostr << "</result>\n"
        "</xml>";
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
  AdXmlBidRequestTask::fill_response_adslot_(
    std::ostream& response_ostr,
    const AdServer::CampaignSvcs::CampaignManager::AdSlotResult& ad_slot_result)
    noexcept
  {
    // find title & image
    response_ostr << "<ad>\n";
    response_ostr << "<title>";
    if(ad_slot_result.native_data_tokens.length() >= 1)
    {
      // NDTE_TITLE
      const AdServer::CampaignSvcs::CampaignManager::TokenInfo& token =
        ad_slot_result.native_data_tokens[0];
      add_xml_escaped_string_(response_ostr, token.value);
    }
    response_ostr << "</title>\n";
    response_ostr << "<desc>";
    if(ad_slot_result.native_data_tokens.length() >= 2)
    {
      // NDTE_DESC
      const AdServer::CampaignSvcs::CampaignManager::TokenInfo& token =
        ad_slot_result.native_data_tokens[1];
      add_xml_escaped_string_(response_ostr, token.value);
    }
    response_ostr << "</desc>\n";
    response_ostr << "<url/>\n";
    response_ostr << "<clickurl>";
    add_xml_escaped_string_(response_ostr, ad_slot_result.selected_creatives[0].click_url);
    response_ostr << "</clickurl>\n";

    // result price in USD/1000, ecpm is in 0.01/1000
    CampaignSvcs::RevenueDecimal adxml_price = CampaignSvcs::RevenueDecimal::div(
      CorbaAlgs::unpack_decimal<CampaignSvcs::RevenueDecimal>(
        ad_slot_result.selected_creatives[0].pub_ecpm),
      CampaignSvcs::RevenueDecimal(false, 100, 0));

    response_ostr << "<bid>" << adxml_price.str() << "</bid>\n";
    response_ostr << "<image>";
    if(ad_slot_result.native_image_tokens.length() > 0)
    {
      // NITE_MAIN
      const AdServer::CampaignSvcs::CampaignManager::TokenImageInfo& token =
        ad_slot_result.native_image_tokens[0];
      add_xml_escaped_string_(response_ostr, token.value);
    }
    response_ostr << "</image>\n";
    response_ostr << "<crid>" << ad_slot_result.selected_creatives[0].creative_version_id <<
      "</crid>\n";
    response_ostr << "</ad>\n";
  }

  void
  AdXmlBidRequestTask::add_xml_escaped_string_(
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
