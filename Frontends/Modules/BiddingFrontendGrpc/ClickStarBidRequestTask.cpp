// THIS
#include <Frontends/FrontendCommons/HTTPUtils.hpp>
#include <Frontends/Modules/BiddingFrontend/KeywordFormatter.hpp>
#include <Frontends/Modules/BiddingFrontendGrpc/ClickStarBidRequestTask.hpp>

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
        const std::string JSON = "application/json";
      }

      namespace Json
      {
        const std::string CPC_PRICE = "bid";
        const std::string TITLE = "title";
        const std::string DESCRIPTION = "description";
        const std::string IMAGE = "image";
        const std::string ICON = "icon";
        const std::string CLICK_URL = "click_url";
        const std::string TTL = "ttl";
        const unsigned long TTL_VALUE = 86400;
      }
    } // namespace Response

    const std::string CLICKSTAR_CLIENT = "directnative";
    const std::string CLICKSTAR_SIZE = "492x328";
    const CampaignSvcs::RevenueDecimal EXPECTED_CTR("0.002");
  }

  ClickStarBidRequestTask::ClickStarBidRequestTask(
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
  ClickStarBidRequestTask::read_request() noexcept
  {
    static const char* FUN = "ClickStarBidRequestTask::read_request()";

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

  bool ClickStarBidRequestTask::write_response(
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
      response->set_content_type(Response::Type::JSON);

      auto& output = response->get_output_stream();
      output.write(bid_response.data(), bid_response.size());
      write_response_(200, response);

      return true;
    }

    return false;
  }

  void ClickStarBidRequestTask::write_empty_response(unsigned int code) noexcept
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
    std::string& response,
    const RequestInfo& /*request_info*/,
    const FrontendCommons::GrpcCampaignManagerPool::RequestParams& /*request_params*/,
    const AdServer::CampaignSvcs::Proto::RequestCreativeResult& campaign_match_result)
    noexcept
  {
    static const char* FUN = "ClickStarBidRequestTask::fill_response_()";

    try
    {
      AdServer::Commons::JsonFormatter json_formatter(response, false);

      for(const auto& ad_slot : campaign_match_result.ad_slots())
      {
        fill_response_adslot_(
          &json_formatter,
          ad_slot);
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
  ClickStarBidRequestTask::fill_response_adslot_(
    AdServer::Commons::JsonFormatter* json_formatter,
    const AdServer::CampaignSvcs::Proto::AdSlotResult& ad_slot_result) noexcept
  {
    AdServer::Commons::JsonObject root_json(
      json_formatter->add_object());

    // result cpc price, ecpm is in 0.01/1000
    CampaignSvcs::RevenueDecimal cpc_price =
      CampaignSvcs::RevenueDecimal::div(
        CampaignSvcs::RevenueDecimal::div(
          GrpcAlgs::unpack_decimal<CampaignSvcs::RevenueDecimal>(
            ad_slot_result.selected_creatives()[0].pub_ecpm()),
          EXPECTED_CTR),
        CampaignSvcs::RevenueDecimal(false, 100000, 0));

    root_json.add_number(Response::Json::CPC_PRICE, cpc_price);

    if(ad_slot_result.native_data_tokens().size() >= 1)
    {
      const auto& token = ad_slot_result.native_data_tokens()[0];
      root_json.add_string(
        Response::Json::TITLE,
        String::SubString(token.value()));
    }

    if(ad_slot_result.native_data_tokens().size() >= 2)
    {
      const auto& token = ad_slot_result.native_data_tokens()[1];
      root_json.add_string(
        Response::Json::DESCRIPTION,
        String::SubString(token.value()));
    }

    if(ad_slot_result.native_image_tokens().size() >= 1)
    {
      // NITE_MAIN
      const auto& token = ad_slot_result.native_image_tokens()[0];
      root_json.add_string(
        Response::Json::IMAGE,
        String::SubString(token.value()));
    }

    if(ad_slot_result.native_image_tokens().size() >= 2)
    {
      // NITE_ICON
      const auto& token = ad_slot_result.native_image_tokens()[1];
      root_json.add_string(
        Response::Json::ICON,
        String::SubString(token.value()));
    }

    root_json.add_string(
      Response::Json::CLICK_URL,
      String::SubString(ad_slot_result.selected_creatives()[0].click_url()));

    root_json.add_number(Response::Json::TTL, Response::Json::TTL_VALUE);
  }
} // namespace AdServer::Bidding::Grpc
