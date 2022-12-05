
#ifndef _AUTOTESTS_COMMONS_REQUEST_CONVERSATIONREQUEST_HPP
#define _AUTOTESTS_COMMONS_REQUEST_CONVERSATIONREQUEST_HPP

#include "BaseRequest.hpp"

namespace AutoTest
{
  /**
   * @class ConversationRequest
   * @brief Presentation of Conversation (action) AdServer request.
   *
   * This request is sent when end user
   * complete an action (purchasing smth. for example). 
   */
  class ConversationRequest : public BaseRequest
  {

    /// Base url for action request
    static const char*          BASE_URL;

    typedef RequestParam<ConversationRequest> ConversationParam;

  public:

    typedef RequestParamSetter<ConversationRequest> Member;   //!< ConversationRequest member

    /**
     * @brief Constructor.
     *
     * Create ConversationRequest and set default values for params.
     * @param set_defs this flag tells ActionRequest
     * whether or not to set default values for parameters.
     */
    explicit ConversationRequest(bool set_defs = true);

    ConversationRequest(const ConversationRequest& other);

    //request params

    /**
     * @brief Represent 'convid' param.
     *
     * Conversion ID, required
     */
    ConversationParam convid;

    /**
     * @brief Represents 'value' param.
     *
     * Conversion value in float format (dot separated,
     *  for example, 5.99) in currency of advertiser account. 
     */
    ConversationParam value;

    /**
     * @brief Represents 'orderid' param.
     *
     * URL encoded order ID. Maximum decoded length
     * is 100 characters, anything longer is truncated.
     * Only stored on confirmed conversion requests.
     */
    ConversationParam orderid;

    /**
     * @brief Represents 'test' param.
     *
     * Test request flag - (0,1).
     */
    ConversationParam test;

    /**
     * @brief Represents 'debug.time' param
     *
     * Debug parameter used to emulate specific
     * request time (normally in future)
     */
    RequestParam <ConversationRequest, TimeParam> debug_time;


    /**
     * @brief Represents 'Referer' HTTP header.
     *
     * URL of a page which have trigered ad request. 
     */
    HeaderParam<ConversationRequest> referer;
  };
}//namespace AutoTest


#endif  // _AUTOTESTS_COMMONS_REQUEST_CONVERSATIONREQUEST_HPP
