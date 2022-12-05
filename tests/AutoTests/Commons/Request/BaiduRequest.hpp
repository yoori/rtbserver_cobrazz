
#ifndef _AUTOTESTS_COMMONS_REQUEST_BAIDUREQUEST_HPP
#define _AUTOTESTS_COMMONS_REQUEST_BAIDUREQUEST_HPP

#include "ProtoBuf.hpp"
#include <Frontends/Modules/BiddingFrontend/baidu-bidding.pb.h>
#include <tests/AutoTests/Commons/Logger.hpp>
#include "DebugSizeParam.hpp"

namespace AutoTest
{

  /**
   * @class BaiduRequest
   * @brief Presentation of bid (Baidu) request.
   */
  class BaiduRequest : public BaseRequest
  {
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    typedef RequestParam<BaiduRequest> DebugParam;
    typedef DebugSizeParam<BaiduRequest> DebugSize;

    /// Base url for all bid requests
    static const char* BASE_URL;
    static const char* DEFAULT_IP;

    /// Baidu default user ID
    static const char* USER_ID;

    Baidu::BidRequest bid_request_;
    Baidu::BidRequest_AdSlot* bid_adslot_;
    Baidu::BidRequest_Mobile_MobileApp* bid_mobile_app_;

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

    /**
     * @brief Constructor.
     *
     * Create the bid (Baidu) request
     * and sets default values for params.
     * @param set_defs tells bid request
     * whether or not to set default values for parameters.
     */
    explicit BaiduRequest(bool set_defs = true);

    /**
     * @brief Copy constructor.
     *
     * @param other request
     */
    BaiduRequest(const BaiduRequest& other);

    /**
     * @brief Destructor.
     */
    virtual ~BaiduRequest() noexcept;

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
    HeaderParam<BaiduRequest> content_type;

    /**
     * @brief Represents 'Accept-Encoding' HTTP header.
     */
    HeaderParam<BaiduRequest> accept_encoding;

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
     * @brief Represents 'debug.time' param.
     *
     * Determines time of request.
     * Parameter is for debugging/testing purposes only.
     */
    RequestParam<BaiduRequest, NewTimeParam> debug_time;

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
     * @brief Represents BaiduRequest.id.
     * Request ID, uniquely identifies this request, the plaintext string
     */
    BidParam<BaiduRequest, ProtoBuf::String> id;


    /**
     * @brief Represents BaiduRequest.ip.
     * User IP address, in dotted decimal string
     */
    BidParam<BaiduRequest, ProtoBuf::String> ip;


    /**
     * @brief Represents BaiduRequest.user_agent.
     * User-Agent
     */
    BidParam<BaiduRequest, ProtoBuf::String> user_agent;

    /**
     * @brief Represents BaiduRequest.baidu_user_id.
     * User ID
     */
    BidParam<BaiduRequest, ProtoBuf::String> user_id;

    /**
     * @brief Represents BaiduRequest.url.
     * The current page URL
     */
    BidParam<BaiduRequest, ProtoBuf::String> url;

    /**
     * @brief Represents BaiduRequest.excluded_product_category.
     * Publishers do not allow advertising industry
     */
    BidParam<BaiduRequest, ProtoBuf::IntSeq> excluded_product_category;

    /**
     * @brief Represents BaiduRequest.is_test.
     */
    BidParam<BaiduRequest, ProtoBuf::Bool> is_test;

    /**
     * @brief Represents BaiduRequest.mobile_app.app_id.
     * App id Baidu Mobile Alliance allocated for App
     */
    BidParam<BaiduRequest, ProtoBuf::IntSeq> app_id;

    /**
     * @brief Represents BaiduRequest.mobile_app.app_bundle_id.
     * If you come from the Apple store, it is app-store id directly
     * If you come from Android devices, it is the package's full name
     */
    BidParam<BaiduRequest, ProtoBuf::String> app_bundle_id;
    
    /**
     * @brief Represents BaiduRequest.adslot.slot_visibility.
     * Placement
     */
    BidParam<BaiduRequest, ProtoBuf::Int> slot_visibility;

    /**
     * @brief Represents BaiduRequest.adslot.width.
     * Width
     */
    BidParam<BaiduRequest, ProtoBuf::Int> width;

    /**
     * @brief Represents BaiduRequest.adslot.height.
     * Height
     */
    BidParam<BaiduRequest, ProtoBuf::Int> height;

    /**
     * @brief Represents BaiduRequest.adslot.minimum_cpm.
     * Publishers set the reserve price, the unit divided
     */
    BidParam<BaiduRequest, ProtoBuf::Int> minimum_cpm;
    
    /**
     * @brief Represents BaiduRequest.adslot.max_video_duration.
     * The maximum length of video ads
     */
    BidParam<BaiduRequest, ProtoBuf::Int> max_video_duration;

    /**
     * @brief Represents BaiduRequest.adslot.min_video_duration.
     * The most hour-long video ads
     */
    BidParam<BaiduRequest, ProtoBuf::Int> min_video_duration;
  };

}
#endif  // _AUTOTESTS_COMMONS_REQUEST_BAIDUREQUEST_HPP
