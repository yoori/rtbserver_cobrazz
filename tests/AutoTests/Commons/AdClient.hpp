#ifndef __AUTOTESTS_COMMONS_ADCLIENT_HPP
#define __AUTOTESTS_COMMONS_ADCLIENT_HPP

#include <Generics/Uuid.hpp>

#include <tests/AutoTests/Commons/BaseAdClient.hpp>
#include <tests/AutoTests/Commons/Logger.hpp>
#include <tests/AutoTests/Commons/Request/Common.hpp>
#include <tests/AutoTests/Commons/DebugInfo.hpp>
#include <tests/AutoTests/Commons/BaseUnit.hpp>
#include <tests/AutoTests/Commons/Checkers/Checker.hpp>

namespace AutoTest
{
  enum UuidUsingEnum
  {
    UUE_ADMIN_PARAMETER,   // using in admin as parameter
    UUE_ADMIN_PARAMVALUE,  // using in admin as parameter value
    UUE_EXPECTED_VALUE     // using as expected value
  };


  /**
   * @class UuidGenarator_
   * @brief Uuid generator
   *
   * Use for generate and verify Uuid.
   * Do not create this class directly, use Singleton logic:
   *  Generics::Singleton<UuidGenarator_>::instance().generate(is_temporary)
   */
  class UuidGenarator_
  {
  public:
    UuidGenarator_();

    /**
     * @brief Generate Uuid.
     *
     * @param temporary uid flag
     * @return Uuid string
     */
    std::string generate(bool is_temporary);

    /**
     * @brief Verify Uuid.
     *
     * @param string for verification
     * @param temporary uid flag
     * @return Uuid verified
     */
    Generics::SignedUuid verify(const char* uid, bool is_temporary);

  private:
    Generics::SignedUuidGenerator temporary_generator_;
    Generics::SignedUuidGenerator persistent_generator_;
    Generics::SignedUuidVerifier  temporary_verifier_;
    Generics::SignedUuidVerifier  persistent_verifier_;
  };

  /**
   * @brief Generate uid
   * @param temporary uid flag, if 'true' - create temporary uid, 'false' - persistent uid
   * @return STL string
   */
  std::string generate_uid(bool is_temporary = true);

  /**
   *  @brief Cut UUID signature
   * need for making correct request with admins or check uid value
   * @param source uid
   * @param true - uid will use in admin parameters, false - in regexp
   * @param temporary uid flag, if 'true' - create temporary uid, 'false' - persistent uid
   * @return cutted uid, prepared to use in admin or regexp
   */
  std::string prepare_uid(
    const std::string& uid,
    UuidUsingEnum uid_using = UUE_ADMIN_PARAMETER,
    bool is_temporary = false);

  typedef std::list<std::string> DebugInfoList;

  /// Exact name of 'debug-info' HTTP header.
  const char DEBUG_INFO_HEADER[]       = "Debug-Info";

  /// Exact names of fields in 'Debug-Info' HTTP-header.
  const char CONTEXT_CHANNEL_FIELDNAME[] = "trigger_context_channels";
  const char NONCONTEXT_CHANNEL_FIELDNAME[] = "trigger_noncontext_channels";
  const char HISTORY_CHANNEL_FIELDNAME[] = "history_channels";
  const char CONTEXT_WORDS_FIELDNAME[] = "context_words";
  const char CC_ID_FIELDNAME[] = "ccid";
  const char PROBE_UID[] = "PPPPPPPPPPPPPPPPPPPPPA..";

  /**
   * @brief Declare RequestProcessError exception.
   *
   * Exception raised when any error during request processing
   */
  DECLARE_EXCEPTION(RequestProcessError, eh::DescriptiveException);

  /**
   * @brief debug-info urls types
   */
  enum ConsequenceActionType
  {
    TRACK = 1,
    CLICK = 2,
    ACTION = 4,
    CLICK_FIRST = 8,
    WAIT = 16,
    NON_EMPTY_ACTION = 32
  };

  /**
   * @brief Consequence action descriptor
   */
  struct ConsequenceAction
  {
    ConsequenceAction(
      ConsequenceActionType act,
      // don't set request debug_time if time == Generics::Time::ZERO
      const Generics::Time& time = Generics::Time::ZERO
      );

    ConsequenceActionType action;
    Generics::Time time;
  };
  typedef std::vector<ConsequenceAction> ConsequenceActionList;

  /**
   * @brief list of creatives
   * ! it is list of strings now
   */
  typedef StrList CreativeList;

  enum UserFlags
  {
    UF_FRONTEND_MINOR  = 1,
    UF_AD_FRONTEND = 2, // not profiling frontend
    UF_CENTRAL_FRONTEND = 4,
    UF_NULL_LOGGER = 8
  };

  /**
   * @brief Declare CookieNotFound exception.
   *
   * Exception raised when required cookie not found.
   */
  DECLARE_EXCEPTION(CookieNotFound, eh::DescriptiveException);

  /**
   * @brief Declare InvalidAdClientConf exception.
   *
   * Exception raised when configuration for AdClient is invalid
   */
  DECLARE_EXCEPTION(InvalidAdClientConf, eh::DescriptiveException);

  class TemporaryAdClient;

  /**
   * @class AdClient
   * @brief AutoTests HTTP client emulator
   * specializing on requests to AdServer.
   *
   * This client is directly used by AutoTest modules to implement
   * their functionality. Sends different requests to AdServer
   * and stores responses. Keeps cookies, headers and body
   * and has ability to analyze it. Contains tools related to unit's
   * constructing and provides debug information from AdServer.
   * It inherits AutoTest::BaseAdClient class.
   */
  class AdClient : public BaseAdClient
  {
    /**
     * @brief Declare TestFailed exception.
     *
     * Exception raised when some checking is failed.
     */
    DECLARE_EXCEPTION(TestFailed, eh::DescriptiveException);

  public:
    /**
     * @brief Create persistent user.
     * Create user with valid persistent UID (make request for probe UID)
     *
     * @param test.
     * @param frontend type.
     */
    static AdClient create_user(
      BaseUnit* test,
      unsigned short flags = 0)
      /*throw(eh::Exception)*/;

    static AdClient create_user(
      BaseUnit* test,
      const AutoTest::Time& debug_time,
      unsigned short flags = 0)
      /*throw(eh::Exception)*/;

    /**
     * @brief Create undefined user.
     * Create user with "unknown" UID.
     */
    static AdClient create_undef_user(
      BaseUnit* test,
      unsigned short flags = 0)
      /*throw(eh::Exception)*/;

    /**
     * @brief Create probe user.
     * Create user with probe UID.
     */
    static AdClient create_probe_user(
      BaseUnit* test,
      unsigned short flags = 0)
      /*throw(eh::Exception)*/;

    /**
     * @brief Create optout user.
     * Create user without UID.
     */
    static  AdClient create_nonoptin_user(
      BaseUnit* test,
      unsigned short flags = 0)
      /*throw(eh::Exception)*/;

    /**
     * @brief Create optout user.
     * Create user optout.
     */
    static  AdClient create_optout_user(
      BaseUnit* test,
      unsigned short flags = 0)
      /*throw(eh::Exception)*/;

    static AdClient create_optout_user(
      BaseUnit* test,
      const AutoTest::Time& debug_time,
      unsigned short flags = 0)
      /*throw(eh::Exception)*/;

    bool uses_profiling_cluster() /*throw(eh::Exception)*/;

  protected:
    /**
     * @brief Constructor.
     *
     * Creates AdClient object and initializes it with base_url and logger.
     * @param base_url initial base url for AdClient.
     * For more info see BaseAdClient::base_url_.
     * @param logger logger for created AdClient object.
     * For more info see BaseAdClient::logger_
     */
    AdClient(
      const char* base_url,
      LoggerType log_type,
      BaseUnit* unit)
      /*throw(Exception, eh::Exception)*/
      : BaseAdClient(base_url, log_type),
        request_count_(0),
        unit_(unit),
        client_index_(unit->get_client_index())
    {}

  public:
    /**
     * @brief Copy constructor.
     *
     * Creates a copy of existing AdClient.
     * @param client copied AdClient object.
     */
    AdClient(const AdClient& client)
      /*throw(Exception, eh::Exception)*/
      : BaseAdClient(client),
        request_count_(client.request_count_),
        unit_(client.unit_),
        client_index_(client.client_index_)
    {}

    /**
     * @brief Destructor.
     */
    virtual ~AdClient() noexcept
    {}

    /**
     * @brief Set PROBE UID for user.
     */
    virtual void set_probe_uid()
        /*throw(eh::Exception)*/;


    /**
     * @brief Merge persistent user with temporary (ns-lookup).
     *
     * Sends ns-lookup request with tuid.
     * @param temporary user.
     * @param merging request.
     */
    virtual void merge(
      const TemporaryAdClient& client_temp,
      const BaseRequest& request)
      /*throw(eh::Exception)*/;

    virtual void merge(
      const TemporaryAdClient& client_temp,
      const NSLookupRequest& request = NSLookupRequest())
      /*throw(eh::Exception)*/;

    /**
     * @brief Send request to the server.
     *
     * Sends GET request to the server, stores response,
     * parses Debug-Ingo header and saves results in field-member debug_info.
     * Log this action to logger.
     * @param request sending request.
     * @param detail_info description for this request,
     * which is reflected in logger.
     */
    void process_request(const BaseRequest& request, const char* detail_info = 0)
      /*throw(eh::Exception)*/;

    void process_request(const NSLookupRequest& request, const char* detail_info = 0)
      /*throw(eh::Exception)*/;

    /**
     * @brief Send request to the server and suppress all exceptions.
     *
     * This function is like previous one,
     * but suppress any exception thrown during execution.
     * @sa process_request().
     * @param request sending request.
     * @param suppress_exceptions whether it is needed
     * to suppress all exceptions.
     * @return response status code on sent request.
     */
    unsigned int process(
      const BaseRequest& request,
      bool suppress_exceptions = false)
      /*throw(eh::Exception)*/;

    unsigned int process(
      const NSLookupRequest& request,
      bool suppress_exceptions = false)
      /*throw(eh::Exception)*/;

    unsigned int process(
      const std::string& request,
      bool suppress_exceptions = false)
      /*throw(eh::Exception)*/;

    /**
     * @brief Repeat last request.
     *
     * Sends last sent request again.
     * @param detail_info description for this request,
     * which is reflected in logger.
     */
    virtual void repeat_request(const char* detail_info = 0)
      /*throw(eh::Exception)*/;

    /**
     * @brief Send request to the server.
     *
     * Sends request with indicated url.
     * Actually, url parameter is only part of real url
     * and appends to the base url,
     * or is whole request url if base url is empty.
     * @param url URL address of destination.
     * @param detail_info description for the request.
     */
    void process_request(const char* url, const char* detail_info = 0)
      /*throw(eh::Exception)*/;

    void process_request(const std::string url, const char* detail_info = 0)
      /*throw(eh::Exception)*/;

    /**
     * @brief Send POST request with indicated url and body.
     * @param url URL address of destination.
     * @param body HTTP message body sent to the server.
     * @param detail_info description for the request.
     */
    void process_post_request(
      const std::string& url,
      const std::string& body = std::string(),
      const char* detail_info = 0)
      /*throw(eh::Exception)*/;

    unsigned int
    process_post(
      const BaseRequest& request,
      bool suppress_exceptions = false)
      /*throw(eh::Exception)*/;

    /**
     * @brief Process block of request on client
     * throws RequestProcessError exception if ccids
     *  or action_list urls not found
     * @param advertising request to process
     * @param expected ccids
     * @param list of required actions to process
     * @param count of requests
     * @param HTTP request headers
     */
    void do_ad_requests(
      const NSLookupRequest& request,
      const CreativeList& ccids,
      const ConsequenceActionList& action_list,
      unsigned long count = 1)
      /*throw(eh::Exception)*/;

    /**
     * @brief Process block of request on client
     * throws RequestProcessError exception if ccids
     *  or action_list urls not found
     * @param advertising request to process
     * @param expected ccids
     * @param action flags for making action list
     * @param count of requests
     * @param HTTP request headers
     */
    void do_ad_requests(
      const NSLookupRequest& request,
      const CreativeList& ccids,
      unsigned long action_flags = 0,
      unsigned long count = 1)
      /*throw(eh::Exception)*/;

    /**
     * @brief Process block of request on client
     * process required debug-info urls
     * initial request to get debug-info urls ! must ! be made
     * throws RequestProcessError exception if ccids
     *  or action_list urls not found
     * @param expected ccids
     * @param list of required actions to process
     * @param HTTP request headers
     */
    void do_ad_requests(
      const CreativeList& ccids,
      const ConsequenceActionList& action_list,
      const HTTP::HeaderList& headers = HTTP::HeaderList())
      /*throw(eh::Exception)*/;

    //Checkers

    /**
     * @brief Check if client has specific cookie.
     * @param cookie_name cookie name to check.
     * @return whether client has indicated cookie.
     */
    bool has_cookie (const std::string& cookie_name);

    /**
     * @brief Check if client has specific cookie
     * and compare its value with expected.
     * @param cookie_name cookie name to check.
     * @param cookie_value expected cookie value.
     * @return whether client has indicated cookie
     * and its value equal to expected one.
     */
    bool has_cookie (
      const std::string& cookie_name,
      const std::string& cookie_value);

    /**
     * @brief Check if client has cookies for specific host.
     * @return whether client has any cookies for specific host,
     * defined in base url.
     */
    bool has_host_cookies ();

    /**
     * @brief Check if client has specific cookie for specific host.
     * @param cookie_name cookie name to check.
     * @return whether client has cookie with indicated name for specific host,
     * defined in base url.
     */
    bool has_host_cookie (const std::string& cookie_name);

    /**
     * @brief Check if client has specific cookie
     * for specific host and compare its value with expected.
     * @param cookie_name cookie name to check.
     * @param cookie_value expected cookie value.
     * @param check_type type of comparison - on equality or on inequality.
     * @return whether client has indicated cookie for specific host
     * and its value is successfully compared with expected value.
     */
    bool has_host_cookie (
      const std::string& cookie_name,
      const std::string& cookie_value,
      bool present = true);

    /**
     * @brief Check if client has cookies for specific domain.
     * @return whether client has any cookies for specific domain,
     * defined in base url.
     */
    bool has_domain_cookies ();

    /**
     * @brief Check if client has specific cookie for specific domain.
     * @param cookie_name cookie name to check.
     * @return whether client has cookie with indicated name for specific domain,
     * defined in base url.
     */
    bool has_domain_cookie(const std::string& cookie_name);

    /**
     * @brief Check if client has specific cookie
     * for specific domain and compare its value with expected.
     * @param cookie_name cookie name to check.
     * @param cookie_value expected cookie value.
     * @param check_type type of comparison - on equality or on inequality.
     * @return whether client has indicated cookie for specific domain
     * and its value is successfully compared with expected value.
     */
    bool has_domain_cookie(
      const std::string& cookie_name,
      const std::string& cookie_value,
      bool present = true);

    /**
     * @brief Get cookie value.
     *
     * Finds cookie with indicated name and returns its value.
     * In case of cookie is not found it throws CookieNotFound exception.
     * @param cookie_name cookie name.
     * @param cookie_value outpur parameter. Found cookie value.
     */
    void get_cookie_value(
      const char *cookie_name,
      std::string& cookie_value) const
      /*throw(CookieNotFound)*/;

    /**
     * @brief Set cookie value.
     *
     * Sets new value for cookie with indicated name.
     * If there is no cookie with such name, throws CookieNotFound exception.
     * If flag fail_if_not_exists is set to false, then creates new cookie with
     * indicated name and sets value for it instead of throwing the exception.
     * @param cookie_name cookie name to set.
     * @param cookie_value cookie value to set.
     * @param fail_if_not_exists flag to determine function behaviour
     * in case of cookie with indicated name was not found.
     */
    virtual void set_cookie_value(
      const char *cookie_name,
      const char *cookie_value,
      bool fail_if_not_exists = true)
      /*throw(eh::Exception)*/;

    /**
     * @brief Get uid value.
     *
     * Returns value of 'uid' cookie. If client hasn't got 'uid' cookie,
     * it throws CookieNotFound exception.
     * @return 'uid' cookie value.
     */
    std::string get_uid () const /*throw(CookieNotFound)*/;

    /**
     * @brief Set uid value.
     *
     * Sets new value for 'uid' cookie. If client hasn't got this cookie,
     * it throws CookieNotFound exception. But you can suppress this exception
     * using fail_if_not_exists flag - if it's set to false,
     * client creates new uid cookie and sets indicated value for it.
     * @param value new uid value to set.
     * @param fail_if_not_exists flag to determine function behavior
     * in case of uid cookie was not found.
     */
    virtual void set_uid (
      const std::string& value,
      bool fail_if_not_exists = true)
      /*throw(eh::Exception)*/;


    /**
     * @brief Get global config.
     * @return global config object.
     */
    Params get_global_params() noexcept;


    /**
     * @brief Get local test config.
     * @return local test config object.
     */
    Locals get_local_params() noexcept;

    /**
     * @brief Assignment operator.
     */
    AdClient& operator= (const AdClient& client);

    /**
     * @brief To keep parsed AdSever debug-info.
     */
    DebugInfo::DebugInfo debug_info;

  protected:

    /**
     * @brief To keep descriptive information about request.
     */
    std::string stored_request_info_;

    /**
     * @brief Stores number of requests processed by this client.
     */
    unsigned long request_count_;

    /**
    * @brief Stores parent unit .
    */
    BaseUnit* unit_;

    /**
    * @brief Stores number of requests processed by this client.
    */
    unsigned long client_index_;

    /**
     * @brief Checker wrapper.
     *
     * Checks some conditions using indicated checker
     * and catches all exceptions.
     * @param checker class for check some conditions.
     * @note calls check() function of the checker.
     */
    void checker_wrapper(Checker& checker) /*throw(TestFailed)*/;

    virtual void process_request_(const char* url, const char* detail_info)
      /*throw(eh::Exception)*/;

    void set_headers_(const HTTP::HeaderList& headers)
      /*throw(eh::Exception)*/;


    virtual void merge_(
      const TemporaryAdClient& client_temp,
      const std::string& url)
      /*throw(eh::Exception)*/;
  };

    /**
   * @class TemporaryAdClient
   * @brief Temporary adserver user
   *
   * This client emulate temporary adserver client.
   * There are some rules for temporary users:
   *   - no 'uid' cookie, request with setuid parameter is invalid;
   *   - no merging possibility, request with tuid parameter, which
         isn't equal to own temporary uid, is invalid;
   *   - all nslookup and discover request have tuid parameter
         equal to own temporary uid parameter
   * It inherits AutoTest::AdClient class.
   */
  class TemporaryAdClient : public AdClient
  {
    /**
     * @brief Declare InvalidRequest exception.
     *
     * Exception raised when request is invalid for temporary user.
     */
    DECLARE_EXCEPTION(InvalidRequest, eh::DescriptiveException);

    /**
     * @brief Declare InvalidCall exception.
     *
     * Exception raised when try to call unexpected method.
     */
    DECLARE_EXCEPTION(InvalidCall, eh::DescriptiveException);

  public:
    /**
     * @brief Create temporary user.
     * Create user with valid temporary UID
     *
     * @param test.
     * @param frontend type.
     */
    static TemporaryAdClient create_user(
      BaseUnit* test,
      unsigned short flags = 0)
      /*throw(eh::Exception)*/;

  protected:
    /**
     * @brief Constructor.
     *
     * Creates TemporaryAdClient object and initializes
     * it with base_url and logger.
     * @param base_url initial base url for AdClient.
     * For more info see AdClient::base_url_.
     * @param logger logger for created AdClient object.
     * For more info see AdClient::logger_
     */
    TemporaryAdClient(
      const char* base_url,
      LoggerType log_type,
      BaseUnit* test)
      /*throw(Exception, eh::Exception)*/;

  public:
    /**
     * @brief Copy constructor.
     *
     * Creates a copy of existing AdClient.
     * @param client copied AdClient object.
     */
    TemporaryAdClient(const TemporaryAdClient& client)
       /*throw(Exception, eh::Exception)*/;

    /**
     * @brief Destructor.
     */
    virtual ~TemporaryAdClient() noexcept;

    /**
     * @brief Get temorary uid value.
     *
     * Returns value of temporary UID.
     * @return temporary UID.
     */
    const std::string& get_tuid() const;

    void repeat_request(const char* detail_info = 0) /*throw(eh::Exception)*/;

  protected:
    // Protect some AdClient methods
    virtual void set_uid (
      const std::string& value,
      bool fail_if_not_exists = true)
      /*throw(eh::Exception)*/;

    virtual void set_cookie_value(
      const char *cookie_name,
      const char *cookie_value,
      bool fail_if_not_exists = true)
      /*throw(eh::Exception)*/;

    virtual void merge_(
      const TemporaryAdClient& client_temp,
      const std::string& url)
      /*throw(eh::Exception)*/;

    virtual void process_request_(const char* url, const char* detail_info)
      /*throw(eh::Exception)*/;

    bool is_valid_request(const char* url);

  private:
    std::string tuid_;
  };

  /**
   * @class ClientGenerator
   * @brief implement create user and send request use case
   *
   * May be useful in the case, when need 'create & send' many times
   */
  template <
      typename Client = AdClient,
      Client (*Creator)(BaseUnit*, unsigned short) = &Client::create_user>
  class ClientGenerator : private Generics::Uncopyable
  {
  public:
    /**
     * @brief Process request on clients
     * @param test
     * @param advertising request to process
     * @param expected ccids
     * @param action list
     * @param count of requests
     */
    static void do_ad_requests(
      BaseUnit* test,
      const NSLookupRequest& request,
      const CreativeList& ccids,
      const ConsequenceActionList& action_list,
      unsigned long count = 1)
        /*throw(eh::Exception)*/
    {
      for (unsigned long i = 0; i < count; ++i)
      {
        Client client = Creator(test, 0);
        client.do_ad_requests(
          request, ccids, action_list, 1);
      }
    }

    /**
     * @brief Process request on clients
     * @param test
     * @param advertising request to process
     * @param expected ccids
     * @param action flags for making action list
     * @param count of requests
     */
    static void do_ad_requests(
      BaseUnit* test,
      const NSLookupRequest& request,
      const CreativeList& ccids,
      unsigned long action_flags = 0,
      unsigned long count = 1)
        /*throw(eh::Exception)*/
    {
      for (unsigned long i = 0; i < count; ++i)
      {
        Client client = Creator(test, 0);
        client.do_ad_requests(
          request, ccids, action_flags, 1);
      }
    }

  private:
    /* User can't create this class */
    ClientGenerator() noexcept;
    ~ClientGenerator() noexcept;
  };

  inline
  ConsequenceAction::ConsequenceAction(
    ConsequenceActionType act,
    const Generics::Time& tm)
    : action(act),
      time(tm)
  {}

}


#endif  // __AUTOTESTS_COMMONS_ADCLIENT_HPP
