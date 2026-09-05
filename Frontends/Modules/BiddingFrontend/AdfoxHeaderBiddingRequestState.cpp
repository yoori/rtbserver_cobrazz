#include "AdfoxHeaderBiddingRequestState.hpp"

#include <algorithm>
#include <charconv>
#include <stdexcept>
#include <string_view>

#include <String/AsciiStringManip.hpp>
#include <String/StringManip.hpp>

#include <Commons/GrpcAlgs.hpp>
#include <Commons/JsonFormatter.hpp>
#include <Frontends/FrontendCommons/HTTPUtils.hpp>

namespace AdServer::Bidding
{
  namespace
  {
    namespace Aspect
    {
      constexpr char BIDDING_FRONTEND[] = "BiddingFrontend";
    }

    const String::SubString JSON_CONTENT_TYPE("application/json");
    constexpr std::string_view ERROR_MESSAGE = "Invalid request";
    constexpr std::string_view INTERNAL_ERROR_MESSAGE = "Internal error";

    bool
    is_video(AdfoxRequestContext::CodeType code_type) noexcept
    {
      return code_type == AdfoxRequestContext::CodeType::InPage ||
        code_type == AdfoxRequestContext::CodeType::PreRoll ||
        code_type == AdfoxRequestContext::CodeType::MidRoll ||
        code_type == AdfoxRequestContext::CodeType::PostRoll;
    }

    std::string_view
    response_code_type(
      const AdfoxRequestContext::Place& place,
      const AdServer::Bidding::CampaignManager::AdSlotResult& slot) noexcept
    {
      switch (place.code_type)
      {
        case AdfoxRequestContext::CodeType::InPage:
          return "inpage";
        case AdfoxRequestContext::CodeType::PreRoll:
          return "preroll";
        case AdfoxRequestContext::CodeType::MidRoll:
          return "midroll";
        case AdfoxRequestContext::CodeType::PostRoll:
          return "postroll";
        case AdfoxRequestContext::CodeType::Banner:
        case AdfoxRequestContext::CodeType::Combo:
          return slot.mime_format.find("javascript") != std::string::npos ? "js" : "html";
      }

      return "html";
    }

    bool
    parse_size(std::string_view value, unsigned long& width, unsigned long& height) noexcept
    {
      const std::size_t separator = value.find('x');
      if (separator == std::string_view::npos)
      {
        return false;
      }

      const std::string_view width_value = value.substr(0, separator);
      const std::string_view height_value = value.substr(separator + 1);
      const auto width_result = std::from_chars(
        width_value.data(), width_value.data() + width_value.size(), width);
      const auto height_result = std::from_chars(
        height_value.data(), height_value.data() + height_value.size(), height);
      return width_result.ec == std::errc() && width_result.ptr == width_value.end() &&
        height_result.ec == std::errc() && height_result.ptr == height_value.end();
    }

    std::string
    url_safe_base64(std::string_view value)
    {
      std::string result;
      String::StringManip::base64_encode(result, value.data(), value.size(), true);
      std::replace(result.begin(), result.end(), '+', '-');
      std::replace(result.begin(), result.end(), '/', '_');
      return result;
    }

    CampaignSvcs::RevenueDecimal
    bid_cpm(
      const AdServer::Bidding::CampaignManager::AdSlotResult& slot,
      BiddingFrontendCore* bid_frontend,
      const RequestInfo& request_info)
    {
      CampaignSvcs::RevenueDecimal result = CampaignSvcs::RevenueDecimal::ZERO;
      for (const auto& creative : slot.selected_creatives)
      {
        result += GrpcAlgs::unpack_decimal<CampaignSvcs::RevenueDecimal>(creative.pub_ecpm);
      }

      bid_frontend->limit_max_cpm_(result, request_info.publisher_account_ids);
      result = CampaignSvcs::RevenueDecimal::div(
        result, CampaignSvcs::RevenueDecimal(false, 100, 0));
      result.floor(3);
      return result;
    }
  }

  AdfoxHeaderBiddingRequestState::AdfoxHeaderBiddingRequestState(
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
      uri_(request_holder_->request().uri().str()),
      context_(request_info_.resource())
  {}

  bool
  AdfoxHeaderBiddingRequestState::read_request() noexcept
  {
    static constexpr char FUN[] = "AdfoxHeaderBiddingRequestState::read_request()";
    std::string body;

    try
    {
      const FCGI::HttpRequest& request = request_holder_->request();
      if (!request.body().empty())
      {
        body.assign(request.body().data(), request.body().size());
      }

      bid_frontend_->request_info_filler()->adfox_request_info_filler()->fill(
        request_info_, context_, request, std::move(body));
      return true;
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error error;
      error << FUN << ": bad request, " << ex.what() << ", uri: '" << uri_ << "'";
      bid_frontend_->logger()->log(
        error.str(), Logging::Logger::ERROR, Aspect::BIDDING_FRONTEND, "ADS-IMPL-7601");
    }

    return false;
  }

  bool
  AdfoxHeaderBiddingRequestState::write_response(
    const AdServer::Bidding::CampaignManager::RequestCreativeResult& campaign_match_result)
    noexcept
  {
    try
    {
      write_json_response_(200, make_response_(campaign_match_result));
      return true;
    }
    catch (const std::exception& ex)
    {
      Stream::Error error;
      error << "AdfoxHeaderBiddingRequestState::write_response(): " << ex.what();
      bid_frontend_->logger()->log(
        error.str(), Logging::Logger::EMERGENCY, Aspect::BIDDING_FRONTEND);
      write_empty_response(500);
      return true;
    }

    catch (...)
    {
      write_empty_response(500);
      return true;
    }
  }

  void
  AdfoxHeaderBiddingRequestState::write_empty_response(
    unsigned int code,
    bool response_claimed) noexcept
  {
    try
    {
      std::string body;
      int response_code = 200;
      {
        AdServer::Commons::JsonFormatter root(body);
        if (code < 300)
        {
          AdServer::Commons::JsonObject bids(root.add_array("bids"));
        }
        else
        {
          response_code = code;
          AdServer::Commons::JsonObject error(root.add_object("error"));
          error.add_string("code", std::to_string(code));
          error.add_string("message", code < 500 ? ERROR_MESSAGE : INTERNAL_ERROR_MESSAGE);
        }
      }

      write_json_response_(response_code, std::move(body), response_claimed);
    }
    catch (...)
    {
      FCGI::HttpResponse_var response(new FCGI::HttpResponse());
      write_response_(code < 300 ? 200 : code, response, response_claimed);
    }
  }

  void
  AdfoxHeaderBiddingRequestState::clear() noexcept
  {
    uri_.clear();
    context_.clear();
    BidRequestState::clear();
  }

  void
  AdfoxHeaderBiddingRequestState::print_request(std::ostream&) const noexcept
  {}

  void
  AdfoxHeaderBiddingRequestState::write_json_response_(
    int code,
    std::string&& body,
    bool response_claimed) noexcept
  {
    FCGI::HttpResponse_var response(new FCGI::HttpResponse());
    response->set_content_type_nocopy(JSON_CONTENT_TYPE);
    try
    {
      FrontendCommons::CORS::set_headers(request_holder_->request(), *response);
    }
    catch (...)
    {}

    response->get_output_stream().write(body.data(), body.size());
    write_response_(code, response, response_claimed);
  }

  std::string
  AdfoxHeaderBiddingRequestState::make_response_(
    const AdServer::Bidding::CampaignManager::RequestCreativeResult& campaign_match_result)
    const
  {
    std::string response;
    {
      AdServer::Commons::JsonFormatter root(response);
      AdServer::Commons::JsonObject bids(root.add_array("bids"));
      if (context_.places.size() != campaign_match_result.ad_slots.size())
      {
        throw std::runtime_error("CampaignManager returned an unexpected ad slot count");
      }

      for (std::size_t i = 0; i < context_.places.size(); ++i)
      {
        const AdfoxRequestContext::Place& place = context_.places[i];
        const AdServer::Bidding::CampaignManager::AdSlotResult& slot =
          campaign_match_result.ad_slots[i];
        if (slot.selected_creatives.empty() ||
          (slot.creative_body.empty() && slot.creative_url.empty()))
        {
          continue;
        }

        const CampaignSvcs::RevenueDecimal cpm = bid_cpm(slot, bid_frontend_, request_info_);
        if (cpm <= CampaignSvcs::RevenueDecimal::ZERO)
        {
          continue;
        }

        AdServer::Commons::JsonObject bid(bids.add_object());
        if (!slot.creative_url.empty())
        {
          bid.add_escaped_string("displayUrl", slot.creative_url);
        }
        else if (is_video(place.code_type))
        {
          bid.add_escaped_string("displayCode", url_safe_base64(slot.creative_body));
        }
        else
        {
          bid.add_escaped_string("displayCode", slot.creative_body);
        }

        bid.add_escaped_string("id", place.id);
        bid.add_number("cpm", cpm);
        bid.add_string("currency", context_.currency);
        bid.add_string("codeType", response_code_type(place, slot));
        bid.add_escaped_string("placementId", place.placement_id);

        if (!is_video(place.code_type))
        {
          unsigned long parsed_width = 0;
          unsigned long parsed_height = 0;
          long width = -1;
          long height = -1;
          if (parse_size(
            slot.selected_creatives.front().creative_size,
            parsed_width,
            parsed_height))
          {
            width = static_cast<long>(parsed_width);
            height = static_cast<long>(parsed_height);
          }
          else if (!place.sizes.empty())
          {
            width = static_cast<long>(place.sizes.front().width);
            height = static_cast<long>(place.sizes.front().height);
          }

          AdServer::Commons::JsonObject size(bid.add_object("size"));
          size.add_number("width", width);
          size.add_number("height", height);
        }
      }
    }

    return response;
  }
}
