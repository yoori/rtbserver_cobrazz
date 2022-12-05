#ifndef __AUTOTESTS_COMMONS_REQUEST_ACTIONREQUEST_HPP
#define __AUTOTESTS_COMMONS_REQUEST_ACTIONREQUEST_HPP

#include "BaseRequest.hpp"

namespace AutoTest
{
  /**
   * @class ActionRequest
   * @brief Presentation of action AdServer request.
   *
   * This request is sent when end user
   * complete an action (purchasing smth. for example). 
   */
  class ActionRequest : public BaseRequest
  {

    /// Base url for action request
    static const char*          BASE_URL;

    /// Default value for 'country' param
    static const char*          COUNTRY_CODE;
    typedef RequestParam<ActionRequest> ActionParam;

  public:

    typedef RequestParamSetter<ActionRequest> Member;   //!< ActionRequest member

    /**
     * @brief Constructor.
     *
     * Create ActionRequest and set default values for params.
     * @param set_defs this flag tells ActionRequest
     * whether or not to set default values for parameters.
     */
    explicit ActionRequest(bool set_defs = true);

    ActionRequest(const ActionRequest& other);

    //request params

    /**
     * @brief Represents 'cid' param.
     *
     * Campaign id 
     */
    ActionParam cid;

    /**
     * @brief Represents 'actionid' param.
     *
     * Action id unique identifier. 
     */
    ActionParam actionid;

    /**
     * @brief Represents 'country' param.
     *
     * Country code of action request. 
     */
    ActionParam country;

    /**
     * @brief Represents 'testrequest' param.
     *
     * Test request flag.
     * @sa NSLookupRequest::testrequest
     */
    ActionParam testrequest;

    /**
     * @brief Represents 'debug-time
     *
     * Determines time of action request.
     * Parameter is for debugging/testing purposes only.
     */
    RequestParam <ActionRequest, TimeParam> debug_time;


    /**
     * @brief Represents 'Referer' HTTP header.
     *
     * URL of a page which have trigered ad request. 
     */
    HeaderParam<ActionRequest> referer;
  };
}//namespace AutoTest

#endif  // __AUTOTESTS_COMMONS_REQUEST_ACTIONREQUEST_HPP
