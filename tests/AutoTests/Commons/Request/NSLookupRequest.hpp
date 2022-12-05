#ifndef __AUTOTESTS_COMMONS_REQUEST_NSLOOKUPREQUEST_HPP
#define __AUTOTESTS_COMMONS_REQUEST_NSLOOKUPREQUEST_HPP

#include "BaseRequest.hpp"

namespace AutoTest
{
  /**
   * @class NSLookupRequest
   * @brief Presentation of nslookup AdServer request.
   * 
   * This class is used to build url of AdServer nslookup request,
   * which is base profiling/advertising request.
   * You can use default parameters for request or set owns.
   */  
  class NSLookupRequest : public BaseRequest
  {
    
    /// Base url for nslookup request
    static const char*          BASE_URL;

    /// Profiling url (for profiling cluster)
    static const char*          PROFILING_URL;

    /// Default value for 'app' param
    static const char*          APP_DEFAULT;

    /// Default value for 'v' (version) param
    static const char*          VERSION_DEFAULT;

    /// Default value for 'rnd' param
    static const unsigned long  RANDOM_DEFAULT;

    /// Default value for 'xinfopsid' param
    static const unsigned short XINFOPSID_DEFAULT;

    /// Default value for 'format' param
    static const char*          FORMAT_DEFAULT;

    /// Default value for 'require-debug-info' param
    static const char*          DEBUG_INFO_DEFAULT;

    /// Default value for 'country' param
    static const char*          DEFAULT_COUNTRY;

  public:

    typedef RequestParam <NSLookupRequest> NSLookupParam; //!< Params for NSLookupRequest
    typedef RequestParamSetter<NSLookupRequest> Member;   //!< NSLookupRequest member

  public:
    /**
     * @brief Constructor.
     *
     * Creates the NSLookupRequest and sets default values for parameters. 
     * @param set_defs this flag tells NSLookupRequest
     * whether or not to set default values for parameters.
     */
    explicit NSLookupRequest(bool set_defs = true);

    /**
     * @brief Copy constructor.
     *
     * @param other request
     */
    NSLookupRequest(const NSLookupRequest& other);
 
    /**
     * @brief Get profiling version of nslookup request
     */
    std::string profiling_url() const;
  
    // request parameters

    /**
     * @brief Represents 'require-debug-info' param.
     *
     * If value equal "header" ad server will put debug information
     * into Debug-Info HTTP response header,
     * if "body" than debug info will be returned as an HTTP body,
     * if "none" or parameter not present - no debug info provided.
     */
    NSLookupParam debug_info;

    /**
     * @brief Represents 'app' param.
     *
     * Application name, can be CP (ContextPlus client)
     * or PS (PageSense client). 
     */
    NSLookupParam app;

    /**
     * @brief Represents 'v' (version) param.
     *
     * Application version string. 
     */
    NSLookupParam version;

    /**
     * @brief Represents 'rnd' param.
     *
     * While processing ad request server produce a random number
     * or takes it from 'random' parameter.
     * This number is used while substituting %%RANDOM%% token.
     * Parameter is for debugging/testing purposes only. 
     */
    NSLookupParam random;

    /**
     * @brief Represents 'xinfopsid' param.
     *
     * Value of any parameter (header) which name begins with "xinfo"
     * prefix is used as a value of corresponding %%XINFO*%% token
     * used in creative instantiation from template.
     * XINFOPSID (PageSense instance ID) and XINFOPSBASE (PageSense base URL)
     * are currently used in templates.
     */
    NSLookupParam xinfopsid;

    /**
     * @brief Represents 'format' param.
     *
     * Creative format accepted by client. 
     */
    NSLookupParam format;

    /**
     * @brief Represents 'referer-kw' param.
     *
     * Comma separated list of keywords grabbed from a page
     * which have triggered an ad request. 
     */
    NSLookupParam referer_kw;

    /**
     * @brief Represents 'Referer' HTTP header.
     *
     * URL of a page which have trigered ad request. 
     */
    HeaderParam<NSLookupRequest> referer;

    /**
     * @brief Represents 'ft' param.
     *
     * full text parameter. 
     */
    NSLookupParam ft;

    /**
     * @brief Represents search 'referer' param.
     *
     * URL of a search which have trigered ad request. 
     */
    HeaderParam <NSLookupRequest, SearchParam> search;


    /**
     * @brief Represents 'muid' param.
     *
     * User id that will be merged to user with 'uid'. 
     */
    NSLookupParam muid;

    /**
     * @brief Represents 'tuid' param.
     *
     * Temporary user id. Identifies some navigations sequence
     * that can be merged into permanent user profile (uid). 
     */
    NSLookupParam tuid;

    /**
     * @brief Represents 'hid' param.
     *
     * Household ID. 
     */
    NSLookupParam hid;

    /**
     * @brief Represents 'uid' param.
     *
     * User id. Identifies an end-user having CP or PS client running.
     * For CP is assigned during installation.
     * For PS client UID is assigned by the Ad Server and preserver
     * using HTTP cookies mechanism under "UID" cookie name.
     */
    NSLookupParam uid;

    /**
     * @brief Represents 'tid' param.
     *
     * Comma-separated list of Tag ids(unsigned number).
     * In version 1.7.0 and after AdServer ignore all tags except first.
     */
    NSLookupParam tid;

    /**
     * @brief Represents 'tag.inv' param.
     *
     * Enabling/disabling publisher inventory for tag.
     * For more info see
     * https://confluence.ocslab.com/display/TDOCDRAFT/REQ-131+Inventory+Estimation+for+Publishers
     */

    NSLookupParam tag_inv;

    /**
     * @brief Represents 'colo' param.
     *
     * If colo defined AdServer log user impression with this value,
     * else AdServer use value defined in it's configuration. 
     */
    NSLookupParam colo;

    /**
     * @brief Represents 'debug-time' param.
     *
     * Determines time of nslookup request.
     * Parameter is for debugging/testing purposes only.
     */
    RequestParam <NSLookupRequest, TimeParam> debug_time;

    /**
     * @brief Represents 'setuid' param.
     *
     * If defined it override config option set_uid,
     * if equal 0, server don't set uid
     */
    NSLookupParam setuid;

    /**
     * @brief Represents 'orig' param.
     *
     * The page where user was intended to go
     * until was interfered by the "bridge ad".
     * Sent by PS client as a part of "bridge ad" request
     * to allow ad server to produce creative redirecting
     * to original page at some point.
     */
    NSLookupParam orig;

    /**
     * @brief Represents 'testrequest' param.
     *
     * If not equal 0 - server don't save any logs. 
     */
    NSLookupParam testrequest;

    /**
     * @brief Represents 'pb' param.
     *
     * Passback url for client redirection, if creative
     * cann't shown. Server send URL, linked with Passback frontend, 
     * in the Location HTTP-header with unique request id.
     * Client goto passback frontend and redirected to passback url.
     */
    NSLookupParam passback;

    /**
     * @brief Represents 'pt' param.
     *
     * Passback url type (html or js)
     */
    NSLookupParam pt;

     /**
     * @brief Represents 'debug.nofraud' param.
     *
     * Mark request as not fraud. Use to prevent fraud protection
     */
    NSLookupParam debug_nofraud;

    /**
     * @brief Represents 'loc.coord' param.
     *
     * Coordinates in format '<Latitude>/<Longitude>'
     */
    NSLookupParam loc_coord;

    /**
     * @brief Represents 'loc.name' param.
     *
     * Location in format '<country code (2-chars)>[/[<state>][/<city>]]'
     */
    NSLookupParam loc_name;

    /**
     * @brief Represents 'pl' param.
     *
     * page load id: random number identificate page loading
     */
    NSLookupParam pl;

    /**
     * @brief Represents 'kn' param.
     *
     * referer-kw keyword normalization
     * (1 - normalized referer-kw, 0 - native referer-kw)
     */
    NSLookupParam kn;

    /**
     * @brief Represents 'vis' param.
     *
     * visibility, refers to country min_tag_visibility
     */
    NSLookupParam vis;

    /**
     * @brief Represents 'User-Agent' HTTP header.
     */
    HeaderParam<NSLookupRequest> user_agent;

    /**
     * @brief Publisher preclick URL from adrequest (mime-decoded).
     */    
    NSLookupParam preclick;
    
    /**
     * @brief Represents 'debug.ip' param
     * 
     * Emulate client ip address, used for country detect.
     * Is for debugging/testing purposes only.
     */    
    NSLookupParam debug_ip;

    /**
     * @brief Represents 'rm-tuid' param
     *
     * If not equal to 0 then server will remove
     * user profile with tuid after merging
     */    
    NSLookupParam rm_tuid;
  };
}//namespace AutoTest

#endif  // __AUTOTESTS_COMMONS_REQUEST_NSLOOKUPREQUEST_HPP
