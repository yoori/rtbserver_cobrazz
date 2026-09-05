#include "AdfoxRequestInfoFiller.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string>

#include <HTTP/HTTPCookie.hpp>
#include <HTTP/UrlAddress.hpp>
#include <String/AsciiStringManip.hpp>
#include <String/StringManip.hpp>

#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <Frontends/FrontendCommons/Cookies.hpp>
#include <Frontends/FrontendCommons/HTTPExceptions.hpp>
#include <Frontends/FrontendCommons/HTTPUtils.hpp>

#include "KeywordFormatter.hpp"
#include "RequestInfoFiller.hpp"

namespace AdServer::Bidding
{
  namespace
  {
    constexpr std::string_view ADFOX_CLIENT = "adfox";
    constexpr std::string_view ADFOX_CLIENT_VERSION = "header-bidding";
    constexpr std::string_view BANNER_FORMAT = "html";
    constexpr std::string_view VAST_FORMAT = "vast";
    constexpr std::string_view VAST_SIZE = "vast";

    using FastJsonParser = AdServer::Commons::FastJsonParser<Generics::MonoString>;

    struct ParseState
    {
      RequestInfo* request_info = nullptr;
      AdfoxRequestContext* context = nullptr;
      AdfoxRequestContext::Place* place = nullptr;
      std::optional<unsigned long> size_width;
      bool invalid_size = false;
    };

    ParseState&
    state(void* context)
    {
      return *static_cast<ParseState*>(context);
    }

    class IgnoringValueProcessor: public FastJsonParser::ValueProcessor
    {
    public:
      void object_started(std::string_view, void*) const override
      {}

      void array_started(std::string_view, void*) const override
      {}

      void process_integer(int64_t, std::string_view, void*) const override
      {}

      void process_float(double, std::string_view, void*) const override
      {}

      void process_string(std::string_view, std::string_view, void*) const override
      {}

      void process_string(Generics::MonoString&&, std::string_view, void*) const override
      {}

      void process_bool(bool, std::string_view, void*) const override
      {}

      void process_null(std::string_view, void*) const override
      {}
    };

    class PlacesProcessor final: public IgnoringValueProcessor
    {
    public:
      void
      array_started(std::string_view, void* context) const override
      {
        state(context).context->places_present = true;
      }

      void
      object_started(std::string_view, void* context) const override
      {
        ParseState& parse_state = state(context);
        if (parse_state.size_width)
        {
          parse_state.invalid_size = true;
        }

        parse_state.context->places.emplace_back(parse_state.context->arena_);
        parse_state.place = &parse_state.context->places.back();
        parse_state.size_width.reset();
      }
    };

    class ObjectPresenceProcessor final: public IgnoringValueProcessor
    {
    public:
      explicit ObjectPresenceProcessor(bool AdfoxRequestContext::* field)
        : field_(field)
      {}

      void
      object_started(std::string_view, void* context) const override
      {
        ParseState& parse_state = state(context);
        parse_state.context->*field_ = true;
      }

    private:
      bool AdfoxRequestContext::* field_;
    };

    enum class PlaceStringField
    {
      Id,
      PlacementId,
      CodeType,
      TargetRef
    };

    class PlaceStringProcessor final: public IgnoringValueProcessor
    {
    public:
      explicit PlaceStringProcessor(PlaceStringField field)
        : field_(field)
      {}

      void
      process_string(std::string_view value, std::string_view, void* context) const override
      {
        set_(value, context);
      }

      void
      process_string(Generics::MonoString&& value, std::string_view, void* context) const override
      {
        ParseState& parse_state = state(context);
        set_(parse_state.request_info->hold_string(std::move(value)), context);
      }

    private:
      void
      set_(std::string_view value, void* context) const
      {
        AdfoxRequestContext::Place* place = state(context).place;
        if (!place)
        {
          return;
        }

        switch (field_)
        {
          case PlaceStringField::Id:
            place->id = value;
            break;
          case PlaceStringField::PlacementId:
            place->placement_id = value;
            break;
          case PlaceStringField::CodeType:
            place->code_type_name = value;
            break;
          case PlaceStringField::TargetRef:
            place->target_ref = value;
            break;
        }
      }

      PlaceStringField field_;
    };

    class CurrencyProcessor final: public IgnoringValueProcessor
    {
    public:
      void
      process_string(std::string_view value, std::string_view, void* context) const override
      {
        state(context).context->currency = value;
      }

      void
      process_string(Generics::MonoString&& value, std::string_view, void* context) const override
      {
        ParseState& parse_state = state(context);
        parse_state.context->currency = parse_state.request_info->hold_string(std::move(value));
      }
    };

    class WindowDimensionProcessor final: public IgnoringValueProcessor
    {
    public:
      explicit WindowDimensionProcessor(unsigned long AdfoxRequestContext::* field)
        : field_(field)
      {}

      void
      process_integer(int64_t value, std::string_view, void* context) const override
      {
        if (value > 0 && static_cast<uint64_t>(value) <= std::numeric_limits<unsigned long>::max())
        {
          state(context).context->*field_ = static_cast<unsigned long>(value);
        }
      }

    private:
      unsigned long AdfoxRequestContext::* field_;
    };

    class SizesProcessor final: public IgnoringValueProcessor
    {
    public:
      void
      process_integer(int64_t value, std::string_view, void* context) const override
      {
        ParseState& parse_state = state(context);
        if (!parse_state.place || value <= 0 ||
          static_cast<uint64_t>(value) > std::numeric_limits<unsigned long>::max())
        {
          parse_state.invalid_size = true;
          return;
        }

        if (!parse_state.size_width)
        {
          parse_state.size_width = static_cast<unsigned long>(value);
        }
        else
        {
          parse_state.place->sizes.push_back({*parse_state.size_width,
            static_cast<unsigned long>(value)});
          parse_state.size_width.reset();
        }
      }

      void
      process_float(double, std::string_view, void* context) const override
      {
        state(context).invalid_size = true;
      }
    };

    std::string_view
    first_forwarded_ip(std::string_view value)
    {
      const std::size_t comma = value.find(',');
      value = value.substr(0, comma);
      while (!value.empty() && String::AsciiStringManip::SPACE(value.front()))
      {
        value.remove_prefix(1);
      }

      while (!value.empty() && String::AsciiStringManip::SPACE(value.back()))
      {
        value.remove_suffix(1);
      }

      return value;
    }

    bool
    parse_code_type(AdfoxRequestContext::Place& place)
    {
      std::string code_type(place.code_type_name);
      String::AsciiStringManip::to_lower(code_type);
      if (code_type.empty() || code_type == "banner")
      {
        place.code_type = AdfoxRequestContext::CodeType::Banner;
      }
      else if (code_type == "combo")
      {
        place.code_type = AdfoxRequestContext::CodeType::Combo;
      }
      else if (code_type == "inpage")
      {
        place.code_type = AdfoxRequestContext::CodeType::InPage;
      }
      else if (code_type == "preroll")
      {
        place.code_type = AdfoxRequestContext::CodeType::PreRoll;
      }
      else if (code_type == "midroll")
      {
        place.code_type = AdfoxRequestContext::CodeType::MidRoll;
      }
      else if (code_type == "postroll")
      {
        place.code_type = AdfoxRequestContext::CodeType::PostRoll;
      }
      else
      {
        return false;
      }

      return true;
    }

    bool
    is_video(AdfoxRequestContext::CodeType code_type) noexcept
    {
      return code_type == AdfoxRequestContext::CodeType::InPage ||
        code_type == AdfoxRequestContext::CodeType::PreRoll ||
        code_type == AdfoxRequestContext::CodeType::MidRoll ||
        code_type == AdfoxRequestContext::CodeType::PostRoll;
    }

    Generics::MonoString
    size_string(unsigned long width, unsigned long height, Generics::MonoAllocatorArena* arena)
    {
      Generics::MonoString result(arena);
      result += std::to_string(width);
      result += 'x';
      result += std::to_string(height);
      return result;
    }

    template<typename Description>
    [[noreturn]] void
    invalid_request(const Description& description)
    {
      Stream::Error error;
      error << "AdfoxRequestInfoFiller::fill(): " << description;
      throw FrontendCommons::HTTPExceptions::InvalidParamException(error);
    }
  }

  void
  AdfoxRequestContext::clear() noexcept
  {
    places.clear();
    currency = {};
    window_width = 0;
    window_height = 0;
    places_present = false;
    settings_present = false;
    window_size_present = false;
  }

  AdfoxRequestInfoFiller::AdfoxRequestInfoFiller(RequestInfoFiller* request_info_filler)
    : request_info_filler_(request_info_filler)
  {
    FastJsonParser::ProcessorSet processors;
    processors.add_processor("places", std::make_shared<PlacesProcessor>());
    processors.add_processor(
      "places.id", std::make_shared<PlaceStringProcessor>(PlaceStringField::Id));
    processors.add_processor(
      "places.placementId",
      std::make_shared<PlaceStringProcessor>(PlaceStringField::PlacementId));
    processors.add_processor(
      "places.codeType",
      std::make_shared<PlaceStringProcessor>(PlaceStringField::CodeType));
    processors.add_processor(
      "places.targetRef",
      std::make_shared<PlaceStringProcessor>(PlaceStringField::TargetRef));
    processors.add_processor("places.sizes", std::make_shared<SizesProcessor>());
    processors.add_processor(
      "settings",
      std::make_shared<ObjectPresenceProcessor>(&AdfoxRequestContext::settings_present));
    processors.add_processor("settings.currency", std::make_shared<CurrencyProcessor>());
    processors.add_processor(
      "settings.windowSize",
      std::make_shared<ObjectPresenceProcessor>(&AdfoxRequestContext::window_size_present));
    processors.add_processor(
      "settings.windowSize.width",
      std::make_shared<WindowDimensionProcessor>(&AdfoxRequestContext::window_width));
    processors.add_processor(
      "settings.windowSize.height",
      std::make_shared<WindowDimensionProcessor>(&AdfoxRequestContext::window_height));
    parser_ = std::make_unique<FastJsonParser>(std::move(processors), false);
  }

  void
  AdfoxRequestInfoFiller::fill(
    RequestInfo& request_info,
    AdfoxRequestContext& context,
    const FCGI::HttpRequest& request,
    std::string&& body) const
  {
    if (request.method() != FCGI::HttpRequest::RM_POST || body.empty())
    {
      invalid_request("POST request with a non-empty body is required");
    }

    request_info.client = ADFOX_CLIENT;
    request_info.client_version = ADFOX_CLIENT_VERSION;
    request_info.request_type = AdServer::CampaignSvcs::AR_OPENRTB;
    request_info.user_status = static_cast<std::size_t>(AdServer::CampaignSvcs::US_UNDEFINED);
    request_info_filler_->init_request_param(request_info);

    if (request_info.publisher_account_ids.empty())
    {
      invalid_request("publisher account is not configured");
    }

    const std::string_view held_body = request_info.hold_string(std::move(body));
    ParseState parse_state{&request_info, &context, nullptr, std::nullopt, false};

    try
    {
      parser_->parse(
        held_body,
        &parse_state,
        [&request_info]()
        {
          return Generics::MonoString(request_info.resource());
        });
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error error;
      error << "invalid JSON: " << ex.what();
      invalid_request(error.str());
    }

    if (parse_state.invalid_size || parse_state.size_width)
    {
      invalid_request("sizes must contain positive width-height pairs");
    }

    if (!context.places_present || context.places.empty())
    {
      invalid_request("places must be a non-empty array");
    }

    if (!context.settings_present || !context.window_size_present ||
      !context.window_width || !context.window_height)
    {
      invalid_request("settings.windowSize with positive width and height is required");
    }

    std::string currency(context.currency);
    String::AsciiStringManip::to_upper(currency);
    if (currency.empty())
    {
      invalid_request("settings.currency is required");
    }
    context.currency = request_info.hold_string(std::move(currency));

    std::string campaign_currency(context.currency);
    String::AsciiStringManip::to_lower(campaign_currency);
    const std::string_view campaign_currency_view =
      request_info.hold_string(std::move(campaign_currency));

    std::string_view target_ref;
    bool has_video = false;
    for (AdfoxRequestContext::Place& place : context.places)
    {
      if (place.id.empty() || place.placement_id.empty() || place.target_ref.empty() ||
        !parse_code_type(place))
      {
        invalid_request("each place must contain valid id, placementId and targetRef");
      }

      if (target_ref.empty())
      {
        target_ref = place.target_ref;
      }
      else if (target_ref != place.target_ref)
      {
        invalid_request("all places must have the same targetRef");
      }

      has_video = has_video || is_video(place.code_type);
    }

    const HTTP::SubHeaderList& headers = request.headers();
    std::string_view peer_ip;
    std::string_view forwarded_ip;
    std::string_view user_agent;
    for (const auto& header : headers)
    {
      std::string name = header.name.str();
      String::AsciiStringManip::to_lower(name);
      const std::string_view value(header.value.data(), header.value.size());
      if (name == ".remotehost")
      {
        peer_ip = value;
      }
      else if (name == "x-forwarded-for" || name == "x_forwarded_for")
      {
        forwarded_ip = first_forwarded_ip(value);
      }
      else if (name == "user-agent" || name == "user_agent")
      {
        user_agent = value;
      }
    }

    request_info_filler_->fill_by_ip(request_info, forwarded_ip.empty() ? peer_ip : forwarded_ip);
    request_info_filler_->fill_by_user_agent(
      request_info, user_agent, request_info.filter_request, false);

    try
    {
      HTTP::CookieList cookies;
      cookies.load_from_headers(headers);
      for (const auto& cookie : cookies)
      {
        if (cookie.name == FrontendCommons::Cookies::OPTOUT &&
          cookie.value == FrontendCommons::Cookies::OPTOUT_TRUE_VALUE)
        {
          request_info.filter_request = true;
          request_info.user_status = static_cast<std::size_t>(AdServer::CampaignSvcs::US_OPTOUT);
          request_info.user_id.reset();
          request_info.track_user_id.reset();
          break;
        }

        if (cookie.name == FrontendCommons::Cookies::CLIENT_ID ||
          cookie.name == FrontendCommons::Cookies::CLIENT_ID2)
        {
          request_info_filler_->fill_by_user_id_cookie(
            request_info, std::string_view(cookie.value.data(), cookie.value.size()));
        }
      }
    }
    catch (const eh::Exception&)
    {}

    HTTP::BrowserAddress referer(target_ref);
    request_info_filler_->fill_by_referer(
      request_info,
      request_info.search_words,
      referer,
      true,
      false);
    request_info.creative_instantiate_type = request.secure() || referer.secure() ?
      FrontendCommons::SECURE_INSTANTIATE_TYPE : FrontendCommons::UNSECURE_INSTANTIATE_TYPE;
    request_info.enabled_notice = false;
    request_info.fill_track_pixel = false;

    BasicKeywordFormatter<Generics::MonoString> keywords(
      request_info.source_id, request_info.arena());
    keywords.add_keyword(MatchKeywords::FULL_REQ);
    if (!request_info.source_id.empty())
    {
      keywords.add_dict_keyword(MatchKeywords::REQ, std::string_view());
    }

    if (!request_info.peer_ip.empty())
    {
      keywords.add_ip(request_info.peer_ip);
    }

    if (!request_info.user_id && request_info.external_user_id.empty())
    {
      keywords.add_keyword(MatchKeywords::FULL_NO_ID);
      if (!request_info.source_id.empty())
      {
        keywords.add_dict_keyword(MatchKeywords::NO_ID, std::string_view());
      }
    }

    if (request_info.referer.empty())
    {
      keywords.add_keyword(MatchKeywords::FULL_NOREF);
    }
    keywords.assign_to(request_info.keywords);
    request_info_filler_->add_special_keywords_(request_info.keywords, request_info);

    if (has_video)
    {
      request_info_filler_->fill_vast_instantiate_type_(request_info, request_info.source_id);
      request_info.enabled_notice = false;
    }

    request_info.ad_slots.clear();
    request_info.ad_slots.reserve(context.places.size());
    for (std::size_t i = 0; i < context.places.size(); ++i)
    {
      const AdfoxRequestContext::Place& place = context.places[i];
      request_info.ad_slots.emplace_back(request_info.arena());
      RequestInfo::AdSlotInfo& slot = request_info.ad_slots.back();
      RequestInfoFiller::init_adslot(slot);
      slot.ad_slot_id = i;
      slot.ext_tag_id.assign(place.placement_id.data(), place.placement_id.size());
      slot.fill_track_html = false;
      slot.passback = request_info.filter_request;
      slot.tag_visibility = 100;
      slot.tag_predicted_viewability = 100;
      slot.min_ecpm = AdServer::CampaignSvcs::RevenueDecimal::ZERO;
      slot.min_ecpm_currency_code = campaign_currency_view;
      slot.currency_codes.push_back(campaign_currency_view);

      if (is_video(place.code_type))
      {
        slot.format = VAST_FORMAT;
        slot.sizes.push_back(VAST_SIZE);
        if (!place.sizes.empty())
        {
          slot.video_width = place.sizes.front().width;
          slot.video_height = place.sizes.front().height;
        }
        else
        {
          slot.video_width = context.window_width;
          slot.video_height = context.window_height;
        }
      }
      else
      {
        slot.format = BANNER_FORMAT;
        slot.sizes.reserve(place.sizes.size());
        for (const AdfoxRequestContext::Size& size : place.sizes)
        {
          slot.sizes.push_back(slot.hold_string(
            size_string(size.width, size.height, slot.resource())));
        }
      }
    }
  }
}
