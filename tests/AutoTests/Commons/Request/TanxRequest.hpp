#ifndef __AUTOTESTS_COMMONS_REQUEST_BIDREQUEST_HPP
#define __AUTOTESTS_COMMONS_REQUEST_BIDREQUEST_HPP

#include "ProtoBuf.hpp"
#include <Frontends/Modules/BiddingFrontend/tanx-bidding.pb.h>
#include <tests/AutoTests/Commons/Logger.hpp>
#include "DebugSizeParam.hpp"

namespace AutoTest
{

  /**
   * @class TanxRequest
   * @brief Presentation of bid (Tanx) request.
   */
  class TanxRequest : public BaseRequest
  {
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    typedef ProtoBuf::Enum<Tanx::BidRequest_AdzInfo_Location> Adz_Location;
    typedef RequestParam<TanxRequest> DebugParam;
    typedef DebugSizeParam<TanxRequest> DebugSize;

    /// Base url for all bid requests
    static const char* BASE_URL;
    static const char* DEFAULT_IP;
    
    Tanx::BidRequest bid_request_;
    Tanx::BidRequest_AdzInfo* bid_adzinfo_;

    struct Decoder : ClientRequest::Decoder
    {
      virtual
      void
      decode(
        std::ostream& out,
        Types decode_type,
        const std::string& data) const;
    };
    
    Decoder decoder_;

  public:
    
    typedef RequestParamSetter<TanxRequest> Member;

    /**
     * @brief Constructor.
     *
     * Create the bid (Tanx) request
     * and sets default values for params.
     * @param set_defs tells bid request
     * whether or not to set default values for parameters.
     */
    explicit TanxRequest(bool set_defs = true);

    /**
     * @brief Copy constructor.
     *
     * @param other request
     */
    TanxRequest(const TanxRequest& other);

    /**
     * @brief Destructor.
     */
    virtual ~TanxRequest() noexcept;

    /**
     * @brief Get request body.
     *
     * @return request HTTP body
     */
    virtual std::string body() const;

    /**
     * @brief Set HTTP request decoders.
     *
     * @param Client request object.
     */
    virtual
    void
    set_decoder(
      ClientRequest* request) const;
    
    /**
     * @brief Represents 'Content-Type' HTTP header.
     */
    HeaderParam<TanxRequest> content_type;

    /**
     * @brief Represents 'Accept-Encoding' HTTP header.
     */
    HeaderParam<TanxRequest> accept_encoding;

    /**
     * @brief Represents 'debug.ccg' param.
     *
     * Frontend log XML-result of campaign selection.
     */
    DebugParam debug_ccg;

    /**
     * @brief Represents aid debug parameter.
     *
     * Publisher account id.
     */
    DebugParam aid;

    /**
     * @brief Represents src debug parameter.
     *
     * Name of RTB system.
     */
    DebugParam src;

    /**
     * @brief Represents random parameter.
     *
     * Random value.
     */
    DebugParam random;

    /**
     * @brief Represents 'debug.time' param.
     *
     * Determines time of request.
     * Parameter is for debugging/testing purposes only.
     */
    RequestParam<TanxRequest, NewTimeParam> debug_time;

    /**
     * @brief Represents 'debug.size' param.
     *
     * Redefines protocol banner size value for indicated/all
     * banners of request.
     * Usefull for testing - no need to create unique sizes
     * with WIDTHxHEIGHT protocol name.
     */
    DebugSize debug_size;

    /**
     * @brief Represents TanxRequest.version.
     *
     * The current version number (=3).
     */
    BidParam<TanxRequest, ProtoBuf::Int> version;

    /**
     * @brief Represents TanxRequest.bid.
     *
     * Unique id (32 bytes).
     */
    BidParam<TanxRequest, ProtoBuf::String> bid;

    /**
     * @brief Represents TanxRequest.tid.
     *
     * Tanx user ID (the mapping value of acookie, 12 bytes).
     * If the inserted DSP is the internal DSP of Alibaba Group, then the
     * tid will be acookie (24 bytes).
     */
    BidParam<TanxRequest, ProtoBuf::String> tid;

    /**
     * @brief Represents TanxRequest.ip.
     */
    BidParam<TanxRequest, ProtoBuf::String> ip;

    /**
     * @brief Represents TanxRequest.user_agent.
     */
    BidParam<TanxRequest, ProtoBuf::String> user_agent;

    /**
     * @brief Represents TanxRequest.excluded_click_through_url.
     *
     * Advertisement target address set by the publisher (or flow provider)
     * which is prohibited to be shown, and it only supports top-level domain validation.
     * It is in non-coding UTF-8 format. Its maximum length is 200 bytes.
     */
    BidParam<TanxRequest, ProtoBuf::StringSeq> excluded_click_through_url;

    /**
     * @brief Represents TanxRequest.url.
     * Current page url, and it is in non-coding UTF-8 format.
     * This parameter is probably null. If the anonymous_id field has a value,
     * then the url will definitely be null.
     */
    BidParam<TanxRequest, ProtoBuf::String> url;

    /**
     * @brief Represents TanxRequest.category.
     *
     * Publisher's (or flow provider's) website classification. 
     */
    BidParam<TanxRequest, ProtoBuf::UInt> category;

    /**
     * @brief Represents TanxRequest.adx_type.
     *
     * Flow provider's flag.
     * For those bidding platforms other than tanx (which is only effective to
     * the DSP of the Alibaba System, and is fixed to be 0 for the third party's DSP),
     * it can set the % FOREIGN_FEEDBACK%% macro in the html_snippet to get the
     * feedback information of bidding success (if the platform of the other side supports it).     
     */
    BidParam<TanxRequest, ProtoBuf::Int> adx_type;

    /**
     * @brief Represents TanxRequest.is_test.
     *
     * Denotes that the current flow is test flow, which is 0 by default.
     * It means that, for the non-test flow, the DSP needs to return a normal BidResponse,
     * but the result will not be shown to the customer and the response will
     * not be sent to the DSP.
     */
    BidParam<TanxRequest, ProtoBuf::UInt> is_test;

    /**
     * @brief Represents TanxRequest.timezone_offset.
     *
     * Amount of minute offset for the user's time zone.
     */
    BidParam<TanxRequest, ProtoBuf::Int> timezone_offset;

    /**
     * @brief Represents TanxRequest.detected_language.
     *
     * Page language.
     * It uses ISO 639-1 coding standards (2 letter code). 
     */
    BidParam<TanxRequest, ProtoBuf::String> detected_language;

    /**
     * @brief Represents TanxRequest.excluded_sensitive_category.
     */
    BidParam<TanxRequest, ProtoBuf::IntSeq> excluded_sensitive_category;

    /**
     * @brief Represents TanxRequest.excluded_ad_category
     */
    BidParam<TanxRequest, ProtoBuf::IntSeq> excluded_ad_category;

    /**
     * @brief Represents TanxRequest.category_version.
     *
     * Version of the webpage/website classification.
     */
    BidParam<TanxRequest, ProtoBuf::Int> category_version;

    /**
     * @brief Represents TanxRequest.tid_version.
     */
    BidParam<TanxRequest, ProtoBuf::UInt> tid_version;
    
    /**
     * @brief Represents TanxRequest.AdzInfo.id
     *
     * Subscript of the current ads space.
     * Since one bid is supported for only one advertising space at present,
     * this id currently is a fixed value: 0
     */
    BidParam<TanxRequest, ProtoBuf::UInt> id;

    /**
     * @brief Represents TanxRequest.AdzInfo.pid
     *
     * Pid of the current ads space (inside the Alibaba Group).
     * If the flow provider doesn't belong to the Alibaba System,
     * then this pid will be the flow provider's virtual pid in the Alibaba System. 
     */
    BidParam<TanxRequest, ProtoBuf::String> pid;
    
    /**
     * @brief Represents TanxRequest.AdzInfo.size
     *
     * Banner size string (for example, 120x80)
     */
    BidParam<TanxRequest, ProtoBuf::String> size;

    /**
     * @brief Represents TanxRequest.AdzInfo.ad_bid_count
     *
     * Amount of the bidding advertisements that the current ads
     * space hopes to obtain, and it is 2 by default.
     * The DSP can provide advertisements with numbers, Tan(X), less than or equal to this
     * number, or obtain the corresponding amount of advertisement bids
     * (only one advertisement bid will win in the bidding result)
     */
    BidParam<TanxRequest, ProtoBuf::UInt> ad_bid_count;

    /**
     * @brief Represents TanxRequest.AdzInfo.view_type
     */
    BidParam<TanxRequest, ProtoBuf::UIntSeq> view_type;

    /**
     * @brief Represents TanxRequest.AdzInfo.excluded_filter
     */
    BidParam<TanxRequest, ProtoBuf::UIntSeq> excluded_filter;

    /**
     * @brief Represents TanxRequest.AdzInfo.min_cpm_price
     *
     * Lowest advertisement price (cent/ecpm).
     */
    BidParam<TanxRequest, ProtoBuf::UInt> min_cpm_price;

    /**
     * @brief Represents TanxRequest.AdzInfo.adz_location
     */
     BidParam<TanxRequest, Adz_Location> adz_location;
  };
  
}//namespace AutoTest

#endif  // __AUTOTESTS_COMMONS_REQUEST_BIDREQUEST_HPP
