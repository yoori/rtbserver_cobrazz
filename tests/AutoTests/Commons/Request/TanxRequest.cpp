#include "TanxRequest.hpp"

namespace AutoTest
{

  namespace ProtoBuf
  {
    template<> const EnumDescriptor*
    Enum<Tanx::BidRequest_AdzInfo_Location>::descriptor_ =
      Tanx::BidRequest_AdzInfo_Location_descriptor();
  }

  void
  TanxRequest::Decoder::decode(
    std::ostream& out,
    Types decode_type,
    const std::string& data) const
  {
    if (decode_type == T_RESPONSE)
    {
      Tanx::BidResponse r;
      r.ParseFromString(data);
      out << r.DebugString();
    }
    else
    {
      Tanx::BidRequest r;
      r.ParseFromString(data);
      out << r.DebugString();
    }
  }

  const char* TanxRequest::BASE_URL = "/bid";
  const char* TanxRequest::DEFAULT_IP = "5.10.16.20";

  TanxRequest::TanxRequest(bool set_defs) :
    BaseRequest(BASE_URL, BaseRequest::RT_ENCODED),
    bid_adzinfo_(bid_request_.add_adzinfo()),
    content_type(
      this,
      "Content-Type",
      "application/octet-stream",
      set_defs),
    accept_encoding(
      this,
      "Accept-Encoding",
      "identity",
      set_defs),
    debug_ccg(this, "debug.ccg"),
    aid(this, "aid"),
    src(this, "src"),
    random(this, "random"),
    debug_time(this, "debug.time"),
    debug_size(this, "debug.size"),
    version(this, &bid_request_, "version", 3, set_defs),
    bid(this, &bid_request_, "bid", "c9487a53a202fd915c7b1fca50a1e474", set_defs),
    tid(this, &bid_request_, "tid", "1234", set_defs),
    ip(this, &bid_request_, "ip", DEFAULT_IP, set_defs),
    user_agent(this, &bid_request_, "user_agent", DEFAULT_USER_AGENT, set_defs),
    excluded_click_through_url(this, &bid_request_, "excluded_click_through_url"),
    url(this, &bid_request_, "url"),
    category(this, &bid_request_, "category", 61804),
    adx_type(this, &bid_request_, "adx_type"),
    is_test(this, &bid_request_, "is_test", 0, set_defs),
    timezone_offset(this, &bid_request_, "timezone_offset", 480, set_defs),
    detected_language(this, &bid_request_, "detected_language", "en", set_defs),
    excluded_sensitive_category(this, &bid_request_, "excluded_sensitive_category"),
    excluded_ad_category(this, &bid_request_, "excluded_ad_category"),
    category_version(this, &bid_request_, "category_version", 1, set_defs),
    tid_version(this, &bid_request_, "tid_version", 1, set_defs),
    id(this, bid_adzinfo_, "id", 0, set_defs),
    pid(this, bid_adzinfo_, "pid", "mm_123_456_789", set_defs),
    size(this, bid_adzinfo_, "size"),
    ad_bid_count(this, bid_adzinfo_, "ad_bid_count", 1, set_defs),
    view_type(this, bid_adzinfo_, "view_type", 1, set_defs),
    excluded_filter(this, bid_adzinfo_, "excluded_filter"),
    min_cpm_price(this, bid_adzinfo_, "min_cpm_price"),
    adz_location(this, bid_adzinfo_, "adz_location", Tanx::BidRequest_AdzInfo_Location_NA)
  { }

  TanxRequest::~TanxRequest() noexcept
  { }

  TanxRequest::TanxRequest(const TanxRequest& other) :
    BaseRequest(BASE_URL, BaseRequest::RT_ENCODED),
    bid_request_(other.bid_request_),
    bid_adzinfo_(bid_request_.mutable_adzinfo(0)),
    content_type(this, other.content_type),
    accept_encoding(this, other.accept_encoding),
    debug_ccg(this, other.debug_ccg),
    aid(this, other.aid),
    src(this, other.src),
    random(this, other.random),
    debug_time(this, other.debug_time),
    debug_size(this, other.debug_size),
    version(this, &bid_request_, other.version),
    bid(this, &bid_request_, other.bid),
    tid(this, &bid_request_, other.tid),
    ip(this, &bid_request_, other.ip),
    user_agent(this, &bid_request_, other.user_agent),
    excluded_click_through_url(this, &bid_request_, other.excluded_click_through_url),
    url(this, &bid_request_, other.url),
    category(this, &bid_request_, other.category),
    adx_type(this, &bid_request_, other.adx_type),
    is_test(this, &bid_request_, other.is_test),
    timezone_offset(this, &bid_request_, other.timezone_offset),
    detected_language(this, &bid_request_, other.detected_language),
    excluded_sensitive_category(this, &bid_request_, other.excluded_sensitive_category),
    excluded_ad_category(this, &bid_request_, other.excluded_ad_category),
    category_version(this, &bid_request_, other.category_version),
    tid_version(this, &bid_request_, other.tid_version),
    id(this, bid_adzinfo_, other.id),
    pid(this, bid_adzinfo_, other.pid),
    size(this, bid_adzinfo_, other.size),
    ad_bid_count(this, bid_adzinfo_, other.ad_bid_count),
    view_type(this, bid_adzinfo_, other.view_type),
    excluded_filter(this, bid_adzinfo_, other.excluded_filter),
    min_cpm_price(this, bid_adzinfo_, other.min_cpm_price),
    adz_location(this, bid_adzinfo_, other.adz_location)
  { }

  std::string
  TanxRequest::body() const
  {
    std::string body;
    if (bid_request_.SerializeToString(&body))
    {
      return body;
    }
    throw Exception("Can't serialize Tanx::TanxRequest");
  }

  void
  TanxRequest::set_decoder(
    ClientRequest* request) const
  {
    request->set_decoder(decoder_);
  }
}



