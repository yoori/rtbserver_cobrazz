// THIS
#include <Frontends/FrontendCommons/HTTPUtils.hpp>
#include <Frontends/Modules/BiddingFrontend/KeywordFormatter.hpp>
#include <Frontends/Modules/BiddingFrontendGrpc/AdJsonBidRequestTask.hpp>

namespace AdServer::Bidding::Grpc
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
        const std::string CONTENT_TYPE = "Content-Type";
      }

      namespace Type
      {
        const std::string TEXT_XML = "text/xml";
      }

      namespace AdJson
      {
        const std::string CRID = "creative_id";
        const std::string TTL_CLICK = "ttl_click";
        const std::string MODE = "mode";
        const std::string COST = "cost";
        const std::string TITLE = "title";
        const std::string TEXT = "text";
        const std::string NURL = "nurl";
        const std::string CLICK_URL = "link";
        const std::string ICON = "icon";
        const std::string IMP_TRACKERS = "pixels";
        const std::string IMAGE = "image";
      }
    } // namespace Response

    const std::string ADJSON_CLIENT = "adjson";
    const std::string ADJSON_SIZE = "492x328";
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
    const AdServer::CampaignSvcs::Proto::RequestCreativeResult&
      campaign_match_result) noexcept
  {
    auto& request_params = *request_params_;

    std::string bid_response;

    fill_response_(
      bid_response,
      request_info(),
      request_params,
      campaign_match_result);

    if(!bid_response.empty())
    {
      FrontendCommons::HttpResponse_var response = bid_frontend_->create_response();
      response->set_content_type(Response::Type::TEXT_XML);

      auto& output = response->get_output_stream();
      output.write(bid_response.data(), bid_response.size());

      write_response_(200, response);

      return true;
    }

    return false;
  }

  void AdJsonBidRequestTask::write_empty_response(unsigned int code) noexcept
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
    std::string& response,
    const RequestInfo& /*request_info*/,
    const FrontendCommons::GrpcCampaignManagerPool::RequestParams& request_params,
    const AdServer::CampaignSvcs::Proto::RequestCreativeResult& campaign_match_result) noexcept
  {
    static const char* FUN = "AdJsonBidRequestTask::fill_response_()";

    try
    {
      AdServer::Commons::JsonFormatter root_json(response, true);

      assert(!campaign_match_result.ad_slots().empty());

      const auto& ad_slot_result = campaign_match_result.ad_slots()[0];

      const auto& selected_creatives = ad_slot_result.selected_creatives();
      assert(!selected_creatives.empty());

      CampaignSvcs::RevenueDecimal sum_pub_ecpm = GrpcAlgs::unpack_decimal<CampaignSvcs::RevenueDecimal>(
        selected_creatives[0].pub_ecpm());

      bid_frontend_->limit_max_cpm_(
        sum_pub_ecpm, request_params.publisher_account_ids);

      root_json.add_string(
        Response::AdJson::CRID,
        String::SubString(selected_creatives[0].creative_version_id()));
      root_json.add_number(Response::AdJson::TTL_CLICK, 172800);
      root_json.add_string(Response::AdJson::MODE, String::SubString("cpm"));

      // result price in USD/1000, ecpm is in 0.01/1000
      CampaignSvcs::RevenueDecimal adjson_price = CampaignSvcs::RevenueDecimal::div(
        sum_pub_ecpm,
        CampaignSvcs::RevenueDecimal(false, 100, 0));
      root_json.add_number(Response::AdJson::COST, adjson_price);

      if(ad_slot_result.native_data_tokens().size() >= 1)
      {
        // NDTE_TITLE
        const auto& token = ad_slot_result.native_data_tokens()[0];
        // title
        root_json.add_escaped_string(
          Response::AdJson::TITLE,
          String::SubString(token.value()));
        // text
        root_json.add_escaped_string(
          Response::AdJson::TEXT,
          String::SubString(token.value()));
      }

      // nurl
      if(!ad_slot_result.notice_url().empty())
      {
        root_json.add_escaped_string(
          Response::AdJson::NURL,
          String::SubString(ad_slot_result.notice_url()));
      }

      // link
      root_json.add_string(
        Response::AdJson::CLICK_URL,
        String::SubString(selected_creatives[0].click_url()));

      // icon
      if(ad_slot_result.native_image_tokens().size() > 1)
      {
        const auto& token = ad_slot_result.native_image_tokens()[1];
        root_json.add_escaped_string(
          Response::AdJson::ICON,
          String::SubString(token.value()));
      }

      // pixels
      {
        AdServer::Commons::JsonObject imp_trackers_obj(
          root_json.add_array(Response::AdJson::IMP_TRACKERS));
      }

      // image
      if(!ad_slot_result.native_image_tokens().empty())
      {
        const auto& token = ad_slot_result.native_image_tokens()[0];
        root_json.add_escaped_string(
          Response::AdJson::IMAGE, String::SubString(token.value()));
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

  void AdJsonBidRequestTask::fill_response_adslot_(
    std::ostream& response_ostr,
    const AdServer::CampaignSvcs::Proto::AdSlotResult& ad_slot_result) noexcept
  {
    // find title & image
    response_ostr << "<ad>\n";
    response_ostr << "<title>";
    if(ad_slot_result.native_data_tokens().size() >= 1)
    {
      // NDTE_TITLE
      const auto& token = ad_slot_result.native_data_tokens()[0];
      add_xml_escaped_string_(response_ostr, token.value());
    }
    response_ostr << "</title>\n";
    response_ostr << "<desc>";
    if(ad_slot_result.native_data_tokens().size() >= 2)
    {
      // NDTE_DESC
      const auto& token = ad_slot_result.native_data_tokens()[1];
      add_xml_escaped_string_(response_ostr, token.value());
    }
    response_ostr << "</desc>\n";
    response_ostr << "<url/>\n";
    response_ostr << "<clickurl>";
    add_xml_escaped_string_(response_ostr, ad_slot_result.selected_creatives()[0].click_url());
    response_ostr << "</clickurl>\n";

    // result price in USD/1000, ecpm is in 0.01/1000
    CampaignSvcs::RevenueDecimal adxml_price = CampaignSvcs::RevenueDecimal::div(
      GrpcAlgs::unpack_decimal<CampaignSvcs::RevenueDecimal>(
        ad_slot_result.selected_creatives()[0].pub_ecpm()),
      CampaignSvcs::RevenueDecimal(false, 100, 0));

    response_ostr << "<bid>" << adxml_price.str() << "</bid>\n";
    response_ostr << "<image>";
    if(ad_slot_result.native_image_tokens().size() > 0)
    {
      // NITE_MAIN
      const auto& token = ad_slot_result.native_image_tokens()[0];
      add_xml_escaped_string_(response_ostr, token.value());
    }
    response_ostr << "</image>\n";
    response_ostr << "<crid>" << ad_slot_result.selected_creatives()[0].creative_version_id() <<
      "</crid>\n";
    response_ostr << "</ad>\n";
  }

  void AdJsonBidRequestTask::add_xml_escaped_string_(
    std::ostream& response_ostr,
    const std::string& str) noexcept
  {
    std::string escaped_str;
    String::StringManip::xml_encode(
      str.c_str(),
      escaped_str,
      String::StringManip::XU_TEXT | String::StringManip::XU_PRESERVE_UTF8);
    response_ostr << escaped_str;
  }
} // namespace AdServer::Bidding::Grpc