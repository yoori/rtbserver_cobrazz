#include "AppNexusBidRequestTask.hpp"

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
      namespace AppNexus
      {
        const String::SubString BID_RESPONSE("bid_response");
        const String::SubString RESPONSES("responses");
        const String::SubString AUCTION_ID_64("auction_id_64");
        const String::SubString MEMBER_ID("member_id");
        const String::SubString PRICE("price");
        const String::SubString CREATIVE_CODE("creative_code");
        const String::SubString CUSTOM_MACROS("custom_macros");
        const String::SubString CM_NAME("name");
        const String::SubString CM_VALUE("value");
      }

      namespace Type
      {
        const String::SubString JSON("application/json");
      }
    }
  }

  AppNexusBidRequestTask::AppNexusBidRequestTask(
    Frontend* bid_frontend,
    FCGI::HttpRequestHolder_var request_holder,
    FCGI::HttpResponseWriter_var response_writer,
    const Generics::Time& start_processing_time)
    /*throw(Invalid)*/
    : BidRequestTask(
        bid_frontend,
        std::move(request_holder),
        std::move(response_writer),
        start_processing_time)
  {}

  bool
  AppNexusBidRequestTask::read_request() noexcept
  {
    //static const char* FUN = "AppNexusBidRequestTask::read_request()";

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

  bool
  AppNexusBidRequestTask::write_response(
    const AdServer::CampaignSvcs::CampaignManager::RequestCreativeResult&
      campaign_match_result)
    noexcept
  {
    static const char* FUN = "AppNexusBidRequestTask::write_response()";

    AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params =
      *request_params_;

    bool fill_response = true;
    std::ostringstream response_ostr;

    try
    {
      AdServer::Commons::JsonFormatter root_response(response_ostr);
      AdServer::Commons::JsonObject bid_response(
        root_response.add_object(Response::AppNexus::BID_RESPONSE));
      AdServer::Commons::JsonObject responses(
        bid_response.add_array(Response::AppNexus::RESPONSES));
      // std::string escaped_request_id =
      //  String::StringManip::json_escape(context_.request_id);

      assert(campaign_match_result.ad_slots.length() ==
             context_.ad_slots.size());
      JsonAdSlotProcessingContextList::const_iterator slot_it =
        context_.ad_slots.begin();

      for(CORBA::ULong ad_slot_i = 0;
          ad_slot_i < campaign_match_result.ad_slots.length();
          ++ad_slot_i, ++slot_it)
      {
        const AdServer::CampaignSvcs::CampaignManager::
          AdSlotResult& ad_slot_result = campaign_match_result.ad_slots[ad_slot_i];

        if(ad_slot_result.selected_creatives.length() > 0)
        {
          AdServer::Commons::JsonObject bid_response(
            responses.add_object());
          // campaigns selected
          CampaignSvcs::RevenueDecimal sum_pub_ecpm = CampaignSvcs::RevenueDecimal::ZERO;
          const AdServer::CampaignSvcs::CampaignManager::CreativeSelectResult& creative =
            ad_slot_result.selected_creatives[0];

          sum_pub_ecpm += CorbaAlgs::unpack_decimal<CampaignSvcs::RevenueDecimal>(
            creative.pub_ecpm);

          bid_frontend_->limit_max_cpm_(sum_pub_ecpm, request_params.publisher_account_ids);
          // result price in RUB/1000, ecpm is in 0.01/1000
          CampaignSvcs::RevenueDecimal appnexus_price = CampaignSvcs::RevenueDecimal::div(
            sum_pub_ecpm,
            CampaignSvcs::RevenueDecimal(false, 100, 0));

          std::string escaped_creative_body =
            String::StringManip::json_escape(
              String::SubString(ad_slot_result.creative_body));

          unsigned long member_id = 0;
          if(request_info_.appnexus_member_id.present())
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
          bid_response.add_as_string(Response::AppNexus::CREATIVE_CODE, creative.creative_id);

          AdServer::Commons::JsonObject custom_macros(bid_response.add_array(
                                                        Response::AppNexus::CUSTOM_MACROS));
          AdServer::Commons::JsonObject custom_macros_elem(custom_macros.add_object());
          custom_macros_elem.add_string(
            Response::AppNexus::CM_NAME, String::SubString("EXT_DATA"));
          custom_macros_elem.add_escaped_string(
            Response::AppNexus::CM_VALUE, String::SubString(ad_slot_result.creative_body));
        } // if(ad_slot_result.selected_creatives.length() > 0)
      } // for(CORBA::ULong ad_slot_i = 0, ...

      fill_response = true;
    } // try JsonObject root
    catch(const AdServer::Commons::JsonObject::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << " Error on formatting Json response: '" << ex.what() << "'";
      bid_frontend_->logger()->log(ostr.str(), Logging::Logger::EMERGENCY, Aspect::BIDDING_FRONTEND);
    }

    if(fill_response)
    {
      const std::string bid_response = response_ostr.str();

      FCGI::HttpResponse_var response(new FCGI::HttpResponse());
      response->set_content_type(Response::Type::JSON);
      FCGI::OutputStream& output = response->get_output_stream();
      output.write(bid_response.data(), bid_response.size());

      write_response_(200, response);

      return true;
    }

    return false;
  }

  void
  AppNexusBidRequestTask::write_empty_response(unsigned int code)
    noexcept
  {
    FCGI::HttpResponse_var response(new FCGI::HttpResponse());

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

      FCGI::OutputStream& output = response->get_output_stream();
      output.write(no_bid, sizeof(no_bid) - 1);

      write_response_(200, response);
    }
  }

  void
  AppNexusBidRequestTask::print_request(std::ostream& /*out*/) const noexcept
  {
    //out << bid_request_;
  }

  void
  AppNexusBidRequestTask::clear() noexcept
  {
    BidRequestTask::clear();
    context_ = JsonProcessingContext();
  }
}
}
