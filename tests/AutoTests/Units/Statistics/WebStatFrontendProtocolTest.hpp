#ifndef _AUTOTEST__WEBSTATFRONTENDPROTOCOLTEST_
#define _AUTOTEST__WEBSTATFRONTENDPROTOCOLTEST_
  
#include <tests/AutoTests/Commons/Common.hpp>
  
namespace ORM = AutoTest::ORM;

class WebStatFrontendProtocolTest : public BaseDBUnit
{

  static const char* BROWSER;
  static const char* OS;

  typedef AutoTest::AdClient AdClient;
  typedef ORM::StatsList<ORM::WebStats> WebStatsList;
  typedef ORM::StatsList<ORM::OptOutStats> OOStatsList;

  /**
   * @class WebStatRequest
   * @brief Presentation of nslookup AdServer request.
   *
   * '/sl.gif' request implementation
   */
  class WebStatRequest : public AutoTest::BaseRequest
  {

    typedef AutoTest::RequestParam<WebStatRequest> WebStatParam;
    
    /// Base url for all webwise requests
    static const char*          BASE_URL;

 
  public:

    /**
     * @brief Constructor.
     *
     * Creates the WebStatRequest. 
     */
    explicit WebStatRequest();

    /**
     * @brief Copy constructor.
     *
     * @param other request
     */
    WebStatRequest(const WebStatRequest& other);

    /**
     * @brief Returns encode flag.
     *
     * @return true  - request's parameters need MIME encoding.
     */
    virtual bool need_encode() const;

    /**
     * @brief Represents 'res' param.
     *
     * operation status:
     *   -'S' = Success,
     *   -'F' = Failure,
     *   -'U' = Undefined
     */
    WebStatParam res;

    /**
     * @brief Represents 'app' param.
     *
     * application identifier
     */
    WebStatParam app;

    /**
     * @brief Represents 'src' param.
     *
     * source identifier
     */
    WebStatParam src;

    /**
     * @brief Represents 'ct' param.
     *
     * cohort
     */
    WebStatParam ct;

    /**
     * @brief Represents 'op' param.
     *
     * operation identifier
     */
    WebStatParam op;

    /**
     * @brief Represents 'testrequest' param.
     *
     * log stats as test
     */
    WebStatParam testrequest;

    /**
     * @brief Represents 'time' param.
     *
     * Request time, used for tests 
     */
    AutoTest::RequestParam <WebStatRequest,AutoTest::TimeParam> time;

    /**
     * @brief Represents 'User-Agent' HTTP header.
     */
    AutoTest::HeaderParam<WebStatRequest> user_agent;

    /**
     * @brief Represents 'Origin' HTTP header.
     */
    AutoTest::HeaderParam<WebStatRequest> origin;

    /**
     * @brief Represents 'ccid' HTTP header.
     */
    WebStatParam ccid;

    /**
     * @brief Represents 'ccid' HTTP header.
     */
    WebStatParam tid;
  };

  typedef AutoTest::RequestParamSetter<WebStatRequest> Member;

  enum CCTagCaseEnum
  {
    CCTCE_LOG_TID = 1,
    CCTCE_LOG_CCID_REMOTE = 2,
    CCTCE_LOG_CCID_CENTRAL = 4,
    CCTCE_NO_LOG_CT = 8,
    CCTCE_NO_LOG_CURCT = 16,
    CCTCE_NO_LOG_OS = 32,
    CCTCE_NO_LOG_BROWSER = 64,
    CCTCE_NO_LOG_HOUR = 128,
    
    CCTCE_LOG_CCID = CCTCE_LOG_CCID_REMOTE | CCTCE_LOG_CCID_CENTRAL,
    CCTCE_NO_LOG_ALL =
      CCTCE_NO_LOG_CT | CCTCE_NO_LOG_CURCT | CCTCE_NO_LOG_OS |
      CCTCE_NO_LOG_BROWSER | CCTCE_NO_LOG_HOUR
  };

public:

  struct TestCase
  {
    const char* value;
    const char* ct_cookie;
    unsigned long status;
    unsigned long flags;
  };
  
public:
  WebStatFrontendProtocolTest(
    UnitStat& stat_var,
    const char* task_name,
    XsdParams params_var) :
    BaseDBUnit(stat_var, task_name, params_var)
  { }

  virtual ~WebStatFrontendProtocolTest() noexcept
  { } 

private:

  AutoTest::Time now_;
  
  virtual void
  pre_condition();
  
  virtual bool
  run();
  
  virtual void
  post_condition();
  
  virtual void
  tear_down();

  template<size_t Count>
  void
  process_(
    AdClient& client,
    const TestCase(&tests)[Count],
    WebStatsList& web_stats,
    Member member,
    const char* user_status,
    const WebStatRequest& base_request = WebStatRequest(),
    const std::string& os = OS,
    const std::string& browser = BROWSER);

  // Cases
  void
  invalid_uid();

  void
  probe_uid();

  void
  res_param(
    AdClient& client);

  void
  testrequest_param(
    AdClient& client);

  void
  post_cors(
    AdClient& client);

  void
  app_param(
    AdClient& client);

  void
  app_webapp_deleted(
    AdClient& client);

  void
  op_param(
    AdClient& client);

  void
  src_missed(
    AdClient& client);

  void
  src_param(
    AdClient& client);

  void
  ct_param();

  void
  ua_case(
    AdClient& client);

  void
  not_exists_triplet(
    AdClient& client);

  void
  oo_cases_();

  void
  cc_tag_cases_(
    unsigned short client_flags,
    CCTagCaseEnum ccid_log_mask = CCTCE_LOG_CCID_CENTRAL);
};

#endif // _AUTOTEST__WEBSTATFRONTENDPROTOCOLTEST_
