// THIS
#include <Frontends/Modules/BiddingFrontendGrpc/AppNexusBidRequestTask.hpp>

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
      namespace AppNexus
      {
        const std::string BID_RESPONSE = "bid_response";
        const std::string RESPONSES = "responses";
        const std::string AUCTION_ID_64 = "auction_id_64";
        const std::string MEMBER_ID = "member_id";
        const std::string PRICE = "price";
        const std::string CREATIVE_CODE = "creative_code";
        const std::string CUSTOM_MACROS = "custom_macros";
        const std::string CM_NAME = "name";
        const std::string CM_VALUE = "value";
      }

      namespace Type
      {
        const std::string JSON = "application/json";
      }
    }
  }

  AppNexusBidRequestTask::AppNexusBidRequestTask(
    Frontend* bid_frontend,
    FrontendCommons::HttpRequestHolder_var request_holder,
    FrontendCommons::HttpResponseWriter_var response_writer,
    const Generics::Time& start_processing_time) /*throw(Invalid)*/
    : BidRequestTask(
        bid_frontend,
        std::move(request_holder),
        std::move(response_writer),
        start_processing_time)
  {}

  bool AppNexusBidRequestTask::read_request() noexcept
  {
    try
    {
      std::string bid_request;

      Stream::BinaryStreamReader request_reader(
        &request_holder_->request().get_input_stream());

      char buf[16 * 1024];

      while(!request_reader.eof() && !request_reader.bad())
      {
        request_reader.read(buf, sizeof(buf));
        bid_request.append(buf, request_reader.gcount());
      }

      bid_frontend_->request_info_filler_->fill_by_appnexus_request(
        *request_params_,
        request_info_,
        keywords_,
        context_,
        bid_request.c_str());

      return true;
    }
    catch(const eh::Exception&)
    {}

    return false;
  }

  bool AppNexusBidRequestTask::write_response(
    const AdServer::CampaignSvcs::Proto::RequestCreativeResult&
      campaign_match_result) noexcept
  {
    static const char* FUN = "AppNexusBidRequestTask::write_response()";

    auto& request_params = *request_params_;
    bool fill_response = true;
    std::string bid_response_string;

    try
    {
      AdServer::Commons::JsonFormatter root_response(bid_response_string);
      AdServer::Commons::JsonObject bid_response(
        root_response.add_object(Response::AppNexus::BID_RESPONSE));
      AdServer::Commons::JsonObject responses(
        bid_response.add_array(Response::AppNexus::RESPONSES));

      assert(campaign_match_result.ad_slots().size() ==
        static_cast<int>(context_.ad_slots.size()));
      JsonAdSlotProcessingContextList::const_iterator slot_it =
        context_.ad_slots.begin();

      const std::size_t size = campaign_match_result.ad_slots().size();
      for(std::size_t ad_slot_i = 0;
          ad_slot_i < size;
          ++ad_slot_i, ++slot_it)
      {
        const auto& ad_slot_result = campaign_match_result.ad_slots()[ad_slot_i];

        const auto& selected_creatives = ad_slot_result.selected_creatives();
        if(!selected_creatives.empty())
        {
          AdServer::Commons::JsonObject bid_response(
            responses.add_object());
          CampaignSvcs::RevenueDecimal sum_pub_ecpm = CampaignSvcs::RevenueDecimal::ZERO;
          const auto& creative = selected_creatives[0];

          sum_pub_ecpm += GrpcAlgs::unpack_decimal<CampaignSvcs::RevenueDecimal>(
            creative.pub_ecpm());

          bid_frontend_->limit_max_cpm_(sum_pub_ecpm, request_params.publisher_account_ids);
          // result price in RUB/1000, ecpm is in 0.01/1000
          CampaignSvcs::RevenueDecimal appnexus_price = CampaignSvcs::RevenueDecimal::div(
            sum_pub_ecpm,
            CampaignSvcs::RevenueDecimal(false, 100, 0));

          std::string escaped_creative_body = String::StringManip::json_escape(
            String::SubString(ad_slot_result.creative_body()));

          unsigned long member_id = 0;
          if(request_info_.appnexus_member_id)
          {
            member_id = *request_info_.appnexus_member_id;
          }
          else
          {
            member_id = (
              !context_.member_ids.empty() ? *context_.member_ids.begin() : 0);
          }

          bid_response.add_number(Response::AppNexus::AUCTION_ID_64, slot_it->id);
          bid_response.add_number(Response::AppNexus::MEMBER_ID, member_id);
          bid_response.add_number(Response::AppNexus::PRICE, appnexus_price);
          bid_response.add_as_string(Response::AppNexus::CREATIVE_CODE, creative.creative_id());

          AdServer::Commons::JsonObject custom_macros(
            bid_response.add_array(Response::AppNexus::CUSTOM_MACROS));
          AdServer::Commons::JsonObject custom_macros_elem(custom_macros.add_object());
          custom_macros_elem.add_string(
            Response::AppNexus::CM_NAME, String::SubString("EXT_DATA"));
          custom_macros_elem.add_escaped_string(
            Response::AppNexus::CM_VALUE, String::SubString(ad_slot_result.creative_body()));
        }
      }

      fill_response = true;
    }
    catch(const AdServer::Commons::JsonObject::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << " Error on formatting Json response: '" << ex.what() << "'";
      bid_frontend_->logger()->log(ostr.str(), Logging::Logger::EMERGENCY, Aspect::BIDDING_FRONTEND);
    }

    if(fill_response)
    {
      FrontendCommons::HttpResponse_var response = bid_frontend_->create_response();
      response->set_content_type(Response::Type::JSON);
      auto& output = response->get_output_stream();
      output.write(bid_response_string.data(), bid_response_string.size());

      write_response_(200, response);

      return true;
    }

    return false;
  }

  void AppNexusBidRequestTask::write_empty_response(unsigned int code) noexcept
  {
    FrontendCommons::HttpResponse_var response = bid_frontend_->create_response();
    response->set_content_type(Response::Type::JSON);
    if (code >= 300)
    {
      write_response_(code, response);
    }
    else
    {
      const char no_bid[] =
        "{\n"
        "  \"bid_response\":{\n"
        "    \"no_bid\":true\n"
        "  }\n"
        "}";

      FrontendCommons::OutputStream& output = response->get_output_stream();
      output.write(no_bid, sizeof(no_bid) - 1);
      write_response_(200, response);
    }
  }

  void AppNexusBidRequestTask::print_request(std::ostream& /*out*/) const noexcept
  {
    //out << bid_request_;
  }

  void AppNexusBidRequestTask::clear() noexcept
  {
    BidRequestTask::clear();
    context_ = JsonProcessingContext();
  }
} // namespace AdServer::Bidding::Grpc
