#include <Frontends/FrontendCommons/HTTPUtils.hpp>

#include "KeywordFormatter.hpp"
#include "AdJsonBidRequestTask.hpp"

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
        const String::SubString CRID("creative_id");
        const String::SubString TTL_CLICK("ttl_click");
        const String::SubString MODE("mode");
        const String::SubString COST("cost");
        const String::SubString TITLE("title");
        const String::SubString TEXT("text");
        const String::SubString NURL("nurl");
        const String::SubString CLICK_URL("link");
        const String::SubString ICON("icon");
        const String::SubString IMP_TRACKERS("pixels");
        const String::SubString IMAGE("image");
      }
    } // namespace Response

    const String::SubString ADJSON_CLIENT("adjson");
    const String::SubString ADJSON_SIZE("492x328");
  }

  AdJsonBidRequestTask::AdJsonBidRequestTask(
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
  AdJsonBidRequestTask::read_request() noexcept
  {
    static const char* FUN = "AdJsonBidRequestTask::read_request()";

    std::string bid_request;

    try
    {
      const FrontendCommons::HttpRequest& request = request_holder_->request();

      bid_frontend_->request_info_filler()->adxml_request_info_filler()->fill_by_request(
        *request_params_,
        request_info_,
        keywords_,
        request,
        true, // require icon
        ADJSON_CLIENT,
        ADJSON_SIZE);

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
  AdJsonBidRequestTask::write_response(
    const AdServer::CampaignSvcs::CampaignManager::RequestCreativeResult&
      campaign_match_result)
    noexcept
  {
    //static const char* FUN = "AdJsonBidRequestTask::write_response()";

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
  AdJsonBidRequestTask::write_empty_response(unsigned int code)
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
  AdJsonBidRequestTask::clear() noexcept
  {
    BidRequestTask::clear();
    uri_.clear();
  }

  void
  AdJsonBidRequestTask::print_request(std::ostream& /*out*/) const noexcept
  {
    //out << bid_request_;
  }

  void
  AdJsonBidRequestTask::fill_response_(
    std::ostream& response_ostr,
    const RequestInfo& /*request_info*/,
    const AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
    const AdServer::CampaignSvcs::CampaignManager::
      RequestCreativeResult& campaign_match_result)
    noexcept
  {
    static const char* FUN = "AdJsonBidRequestTask::fill_response_()";

    try
    {
      //Generics::Time now = Generics::Time::get_time_of_day();

      AdServer::Commons::JsonFormatter root_json(response_ostr);

      assert(campaign_match_result.ad_slots.length() > 0);

      const AdServer::CampaignSvcs::CampaignManager::
        AdSlotResult& ad_slot_result = campaign_match_result.ad_slots[0];

      CampaignSvcs::RevenueDecimal sum_pub_ecpm = CorbaAlgs::unpack_decimal<CampaignSvcs::RevenueDecimal>(
        ad_slot_result.selected_creatives[0].pub_ecpm);

      bid_frontend_->limit_max_cpm_(
        sum_pub_ecpm, request_params.publisher_account_ids);

      root_json.add_string(
        Response::AdJson::CRID,
        String::SubString(ad_slot_result.selected_creatives[0].creative_version_id));
      root_json.add_number(Response::AdJson::TTL_CLICK, 172800);
      root_json.add_string(Response::AdJson::MODE, String::SubString("cpm"));

      // result price in USD/1000, ecpm is in 0.01/1000
      CampaignSvcs::RevenueDecimal adjson_price = CampaignSvcs::RevenueDecimal::div(
        sum_pub_ecpm,
        CampaignSvcs::RevenueDecimal(false, 100, 0));
      root_json.add_number(Response::AdJson::COST, adjson_price);

      if(ad_slot_result.native_data_tokens.length() >= 1)
      {
        // NDTE_TITLE
        const AdServer::CampaignSvcs::CampaignManager::TokenInfo& token =
          ad_slot_result.native_data_tokens[0];
        // title
        root_json.add_escaped_string(
          Response::AdJson::TITLE,
          String::SubString(token.value));
        // text
        root_json.add_escaped_string(
          Response::AdJson::TEXT,
          String::SubString(token.value));
      }

      // nurl
      if(ad_slot_result.notice_url[0])
      {
        root_json.add_escaped_string(
          Response::AdJson::NURL,
          String::SubString(ad_slot_result.notice_url));
      }

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

      // pixels
      {
        AdServer::Commons::JsonObject imp_trackers_obj(
          root_json.add_array(Response::AdJson::IMP_TRACKERS));
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

  void
  AdJsonBidRequestTask::fill_response_adslot_(
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
  AdJsonBidRequestTask::add_xml_escaped_string_(
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
