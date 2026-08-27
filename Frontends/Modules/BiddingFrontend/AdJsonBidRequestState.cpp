#include <Frontends/FrontendCommons/HTTPUtils.hpp>
#include <Commons/GrpcAlgs.hpp>
#include <Commons/JsonFormatter.hpp>

#include "KeywordFormatter.hpp"
#include "AdJsonBidRequestState.hpp"

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
      const std::string TEXT_XML("text/xml");
    }

    namespace Response::AdJson
    {
      const std::string CRID("creative_id");
      const std::string TTL_CLICK("ttl_click");
      const std::string MODE("mode");
      const std::string COST("cost");
      const std::string TITLE("title");
      const std::string TEXT("text");
      const std::string NURL("nurl");
      const std::string CLICK_URL("link");
      const std::string ICON("icon");
      const std::string IMP_TRACKERS("pixels");
      const std::string IMAGE("image");
    }

    const std::string ADJSON_CLIENT("adjson");
    const std::string ADJSON_SIZE("492x328");
  }

  AdJsonBidRequestState::AdJsonBidRequestState(
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
  AdJsonBidRequestState::read_request() noexcept
  {
    static const char* FUN = "AdJsonBidRequestState::read_request()";

    std::string bid_request;

    try
    {
      const FCGI::HttpRequest& request = request_holder_->request();

      bid_frontend_->request_info_filler()->adxml_request_info_filler()->fill_by_request(
        request_info_,
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
        ", request: '" << bid_request << "'" << ", uri: '" << uri_ << "'";

      bid_frontend_->logger()->log(
        ostr.str(),
        Logging::Logger::ERROR,
        Aspect::BIDDING_FRONTEND,
        "ADS-IMPL-7601");
    }

    return false;
  }

  bool
  AdJsonBidRequestState::write_response(
    const AdServer::Bidding::CampaignManager::RequestCreativeResult&
      campaign_match_result)
    noexcept
  {
    //static const char* FUN = "AdJsonBidRequestState::write_response()";

    std::ostringstream response_ostr;

    fill_response_(response_ostr, request_info_, campaign_match_result);

    // write response
    const std::string bid_response = response_ostr.str();

    if (!bid_response.empty())
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
  AdJsonBidRequestState::write_empty_response(unsigned int code, bool response_claimed)
    noexcept
  {
    FCGI::HttpResponse_var response(new FCGI::HttpResponse());

    if (code < 300)
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
  AdJsonBidRequestState::clear() noexcept
  {
    BidRequestState::clear();
    uri_.clear();
  }

  void
  AdJsonBidRequestState::print_request(std::ostream& /*out*/) const noexcept
  {
    //out << bid_request_;
  }

  void
  AdJsonBidRequestState::fill_response_(
    std::ostream& response_ostr,
    const RequestInfo& request_info,
    const AdServer::Bidding::CampaignManager::
      RequestCreativeResult& campaign_match_result)
    noexcept
  {
    static const char* FUN = "AdJsonBidRequestState::fill_response_()";

    try
    {
      //Generics::Time now = Generics::Time::get_time_of_day();

      std::string response;
      {
        AdServer::Commons::JsonFormatter root_json(response);

      assert(campaign_match_result.ad_slots.size() > 0);

      const AdServer::Bidding::CampaignManager::
        AdSlotResult& ad_slot_result = campaign_match_result.ad_slots[0];

      CampaignSvcs::RevenueDecimal sum_pub_ecpm = GrpcAlgs::unpack_decimal<CampaignSvcs::RevenueDecimal>(
        ad_slot_result.selected_creatives[0].pub_ecpm);

        bid_frontend_->limit_max_cpm_(sum_pub_ecpm, request_info.publisher_account_ids);

      root_json.add_string(
        Response::AdJson::CRID,
        ad_slot_result.selected_creatives[0].creative_version_id);
      root_json.add_number(Response::AdJson::TTL_CLICK, 172800);
      root_json.add_string(Response::AdJson::MODE, "cpm");

      // result price in USD/1000, ecpm is in 0.01/1000
      CampaignSvcs::RevenueDecimal adjson_price = CampaignSvcs::RevenueDecimal::div(
        sum_pub_ecpm,
        CampaignSvcs::RevenueDecimal(false, 100, 0));
      root_json.add_number(Response::AdJson::COST, adjson_price);

      if (ad_slot_result.native_data_tokens.size() >= 1)
      {
        // NDTE_TITLE
        const AdServer::Bidding::CampaignManager::ResultTokenInfo& token =
          ad_slot_result.native_data_tokens[0];
        // title
        root_json.add_escaped_string(Response::AdJson::TITLE, token.value);
        // text
        root_json.add_escaped_string(Response::AdJson::TEXT, token.value);
      }
      // nurl
      if (!ad_slot_result.notice_url.empty())
      {
        root_json.add_escaped_string(Response::AdJson::NURL, ad_slot_result.notice_url);
      }

      // link
      root_json.add_string(
        Response::AdJson::CLICK_URL,
        ad_slot_result.selected_creatives[0].click_url);

      // icon
      if (ad_slot_result.native_image_tokens.size() > 1)
      {
        const AdServer::Bidding::CampaignManager::ResultTokenImageInfo& token =
          ad_slot_result.native_image_tokens[1];
        root_json.add_escaped_string(Response::AdJson::ICON, token.value);
      }

      // pixels
      {
        AdServer::Commons::JsonObject imp_trackers_obj(
          root_json.add_array(Response::AdJson::IMP_TRACKERS));
      }

      // image
      if (ad_slot_result.native_image_tokens.size() > 0)
      {
        const AdServer::Bidding::CampaignManager::ResultTokenImageInfo& token =
          ad_slot_result.native_image_tokens[0];
        root_json.add_escaped_string(Response::AdJson::IMAGE, token.value);
      }
      }
      response_ostr << response;
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
  AdJsonBidRequestState::fill_response_adslot_(
    std::ostream& response_ostr,
    const AdServer::Bidding::CampaignManager::AdSlotResult& ad_slot_result)
    noexcept
  {
    // find title & image
    response_ostr << "<ad>\n";
    response_ostr << "<title>";
    if (ad_slot_result.native_data_tokens.size() >= 1)
    {
      // NDTE_TITLE
      const AdServer::Bidding::CampaignManager::ResultTokenInfo& token =
        ad_slot_result.native_data_tokens[0];
      add_xml_escaped_string_(response_ostr, token.value.c_str());
    }
    response_ostr << "</title>\n";
    response_ostr << "<desc>";
    if (ad_slot_result.native_data_tokens.size() >= 2)
    {
      // NDTE_DESC
      const AdServer::Bidding::CampaignManager::ResultTokenInfo& token =
        ad_slot_result.native_data_tokens[1];
      add_xml_escaped_string_(response_ostr, token.value.c_str());
    }
    response_ostr << "</desc>\n";
    response_ostr << "<url/>\n";
    response_ostr << "<clickurl>";
    add_xml_escaped_string_(response_ostr, ad_slot_result.selected_creatives[0].click_url.c_str());
    response_ostr << "</clickurl>\n";

    // result price in USD/1000, ecpm is in 0.01/1000
    CampaignSvcs::RevenueDecimal adxml_price = CampaignSvcs::RevenueDecimal::div(
      GrpcAlgs::unpack_decimal<CampaignSvcs::RevenueDecimal>(
        ad_slot_result.selected_creatives[0].pub_ecpm),
      CampaignSvcs::RevenueDecimal(false, 100, 0));

    response_ostr << "<bid>" << adxml_price.str() << "</bid>\n";
    response_ostr << "<image>";
    if (ad_slot_result.native_image_tokens.size() > 0)
    {
      // NITE_MAIN
      const AdServer::Bidding::CampaignManager::ResultTokenImageInfo& token =
        ad_slot_result.native_image_tokens[0];
      add_xml_escaped_string_(response_ostr, token.value.c_str());
    }
    response_ostr << "</image>\n";
    response_ostr << "<crid>" << ad_slot_result.selected_creatives[0].creative_version_id <<
      "</crid>\n";
    response_ostr << "</ad>\n";
  }

  void
  AdJsonBidRequestState::add_xml_escaped_string_(std::ostream& response_ostr, const char* str)
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
