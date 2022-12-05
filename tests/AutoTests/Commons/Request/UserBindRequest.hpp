#ifndef __AUTOTESTS_COMMONS_REQUEST_USERBINDREQUEST_HPP
#define __AUTOTESTS_COMMONS_REQUEST_USERBINDREQUEST_HPP

#include "BaseRequest.hpp"

namespace AutoTest
{
  /**
   * @class UserBindRequest
   * @brief Presentation of userbind AdServer request.
   * 
   * This class is used to build url of AdServer userbind request,
   * also known as Cookie Mapping or Cookie Sync.
   * You can use default parameters for request or set owns.
   */  
  class UserBindRequest : public BaseRequest
  {
    
    /// Base url for nslookup request
    static const char*          BASE_URL;

  public:

    typedef RequestParam <UserBindRequest> UserBindParam; //!< Params for UserBindRequest
    typedef RequestParamSetter<UserBindRequest> Member;   //!< UserBindRequest member
    
  public:
    /**
     * @brief Constructor.
     *
     * Creates the UserBindRequest.
     */
    UserBindRequest();

    /**
     * @brief Copy constructor.
     *
     * @param other request
     */
    UserBindRequest(const UserBindRequest& other);
    
    // request parameters

    /**
     * @brief Represents 'ssp_user_id' param.
     *
     * SSP (TanX, RBC etc) user identifier
     */
    UserBindParam ssp_user_id;

    // Aliases for ssp_user_id
    UserBindParam id;
    UserBindParam tid;

    /**
     * @brief Represents 'src' param.
     *
     * Source of the request (short name of the SSP)
     */
    UserBindParam src;


    /**
     * @brief Represents 'pbf' param.
     *
     * Passback flag, if set to 1, then redirect the
     * request to the URL set in CMS config (see REQ-3358)
     */
    UserBindParam pbf;

    /**
     * @brief Represents 'gi' param.
     *
     * generate external id, if SSP user identifier isn't
     * defined it will be generated (for use in redirect template)
     */
    UserBindParam gi;


    /**
     * @brief Represents 'uid' param
     *
     * AdServer user identifier
     */
    UserBindParam uid;

    /**
     * @brief Represents 'User-Agent' HTTP header.
     */
    HeaderParam<UserBindRequest> user_agent;

    /**
     * @brief Represents 'Referer' HTTP header.
     */
    HeaderParam<UserBindRequest> referer;

    /**
     * @brief Represents 'x-Forwarded-For' HTTP header.
     */
    HeaderParam<UserBindRequest> x_forwarded_for;

  };
}//namespace AutoTest

#endif  // __AUTOTESTS_COMMONS_REQUEST_USERBINDREQUEST_HPP
