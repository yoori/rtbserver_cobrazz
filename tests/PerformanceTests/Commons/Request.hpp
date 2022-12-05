
#ifndef __REQUEST_HPP
#define __REQUEST_HPP

#include <tests/AutoTests/Commons/DebugInfo.hpp>
#include <tests/AutoTests/Commons/Request/OpenRTBRequest.hpp>

#include <ace/Message_Block.h>

#include "ConfigCommons.hpp"
#include "HttpPoolPolicies.hpp"
#include "StatCommons.hpp"
#include "QuerySenderBase.hpp"

/**
 * @class BaseRequest
 * @brief Base class for request objects.
 */
class BaseRequest :
  public ResponseCallback,
  public ReferenceCounting::AtomicImpl,
  public ACE_Message_Block // FIXME Get rid of ACE
{
public:

  /**
   * @brief Constructor.
   *
   * @param owner pointer for main test class - QuerySenderBase.
   * @param client (user) ID.
   * @param optout flag, if true request send in 'opted out' mode.
   * @param request configuration
   */
  BaseRequest(
    QuerySenderBase* owner,
    unsigned long client_id,
    bool optout,
    const RequestConfig_var& config,
    const char* url = "");

  /**
   * @brief Destructor.
   */
  virtual ~BaseRequest() noexcept;

  /**
   * @brief Get request URL.
   *
   * Build request url base on configuration parameters
   * and output it into string.
   * @return request url string.
   */
  const std::string& url();

  /**
   * @brief Get request Body (POST request).
   *
   * @return request body string.
   */
  const std::string& body();

  /**
   * @brief Get request HTTP headers.
   *
   * @param headers [out] list of HTTP headers.
   */
  virtual void
  headers(
    HeaderList& headers) const;

  /**
   * @brief response callback
   *
   * Called when request succeeded (overrided ResponseCallback method)
   * @param data response
   */
  virtual void
  on_response(
    const ResponseInformation& data) noexcept;

  /**
   * @brief error callback
   *
   * Called when request failed (overrided ResponseCallback method)
   * @param description error message
   * @param data request
   */
  virtual void
  on_error(
    const String::SubString& description,
    const RequestInformation& data) noexcept;

  /**
   * @brief Is HTTP method GET.
   *
   * @return true - if need use GET HTTP method, false - use POST.
   */
  bool isGet() const;

  /**
   * @brief Get client (user) ID.
   *
   * @return client (user) ID.
   */
  unsigned long client_id() const;

  /**
   * @brief Get optout flag.
   *
   * @return optout flag.
   */
  bool optout() const;

  /**
   * @brief dump request
   * @param output stream
   */
  virtual std::ostream&
  dump(
    std::ostream& out) const;

protected:
  /**
   * @brief Get request URL.
   *
   * Build request url base on configuration parameters
   * and output it into string.
   * @param need generate URL
   * @return request url string.
   */
  virtual const std::string& _url(bool generate = true) = 0;

  /**
   * @brief Get request Body.
   *
   * Build request body base on configuration parameters
   * and output it into string.
   * @return request BODY string.
   */
  virtual const std::string& _body();

  /**
   * @brief response processing
   *
   * @param response data
   */
  virtual void
  _on_response(
    const ResponseInformation& data) = 0;

  /**
   * @brief Check response code
   *
   * Using to validate response
   * @return true - got valid response, false - invalid response
   */
  virtual bool
  _check_response_code(
    unsigned long response_code);

private:

  volatile sig_atomic_t request_handled_; // request handling flag

protected:
  QuerySenderBase* owner_;           // request owner (sender)
  unsigned long client_id_;          // client (user) ID
  bool optout_;                      // optout flag
  const RequestConfig_var& config_;  // request configuration
  std::string url_;                  // request URL
  std::string body_;                 // request Body
};


/**
 * @class ParamsRequest
 * @brief Base class for requests with parameters.
 */
class ParamsRequest : public BaseRequest
{
public:

  /**
   * @brief Constructor.
   *
   * @param owner pointer for main test class - QuerySenderBase.
   * @param client (user) ID.
   * @param optout flag, if true request send in 'opted out' mode.
   * @param request configuration
   * @param adserver address
   * @param add advertising params to all opted out requests (flag)
   */
  ParamsRequest(
    QuerySenderBase* owner,
    unsigned long client_id,
    bool optout,
    const RequestConfig_var& config,
    const char* server,
    bool cfg_ad_all_optout);

  /**
   * @brief Destructor.
   */
  virtual ~ParamsRequest() noexcept;

protected:

  /**
   * @brief Get request URL (BaseRequest overridden method).
   *
   * Build request url base on configuration parameters
   * and output it into string.
   * @param need generate URL
   * @return request url string.
   */
  const std::string& _url(bool generate = true);

  /**
   * @brief response processing (BaseRequest overridden method).
   *
   * Called when request succeeded
   * @param response data
   */
  void _on_response(const ResponseInformation& data);

  /**
   * @brief Process response
   *
   * @param response data
   */
  virtual void _process_response(const ResponseInformation& data) = 0;

  // Parsed 'Debug-Info' HTTP header from response
  AutoTest::DebugInfo::DebugInfo debug_info_;

  // AdServer address
  std::string server_;

  // add advertising params to all opted out requests (flag)
  bool cfg_ad_all_optout_;

};

/**
 * @class NSLookupRequest
 * @brief NSLookup (advertising) request
 */
class NSLookupRequest: public ParamsRequest
{
public:
  /**
   * @brief Constructor.
   *
   * @param owner pointer for main test class - QuerySenderBase.
   * @param client (user) ID.
   * @param optout flag, if true request send in 'opted out' mode.
   * @param request configuration
   * @param adserver address
   * @param add advertising params to all opted out requests (flag)
   */
  NSLookupRequest(
    QuerySenderBase* owner,
    unsigned long client_id,
    bool optout,
    const RequestConfig_var& config,
    const char* server,
    bool cfg_ad_all_optout);

  /**
   * @brief Destructor.
   */
  virtual ~NSLookupRequest() noexcept;
protected:
  /**
   * @brief process response (ParamsRequest overridden method)
   *
   * @param response data
   */
  virtual void _process_response(const ResponseInformation& data);
};

/**
 * @class DiscoverRequest
 * @brief Discover request
 */
class DiscoverRequest: public ParamsRequest
{
public:

  /**
   * @brief Constructor.
   *
   * @param owner pointer for main test class - QuerySenderBase.
   * @param client (user) ID.
   * @param optout flag, if true request send in 'opted out' mode.
   * @param request configuration
   * @param adserver address
   */
  DiscoverRequest(
    QuerySenderBase* owner,
    unsigned long client_id,
    bool optout,
    const RequestConfig_var& config,
    const char* server);

  /**
   * @brief Destructor.
   */
  virtual ~DiscoverRequest() noexcept;

protected:

  /**
   * @brief process response (ParamsRequest overridden method)
   *
   * @param response data
   */
  virtual void _process_response(const ResponseInformation& data);
};


/**
 * @class SimpleRequest
 * @brief Base class for simple (generated by server) requests
 */
class SimpleRequest : public BaseRequest
{

public:
  /**
   * @brief Constructor.
   *
   * @param owner pointer for main test class - QuerySenderBase.
   * @param client (user) ID.
   * @param optout flag, if true request send in 'opted out' mode.
   * @param request configuration
   * @param request URL
   */
  SimpleRequest(
    QuerySenderBase* owner,
    unsigned long client_id,
    bool optout,
    const RequestConfig_var& config,
    const char* url);

  /**
   * @brief Destructor.
   */
  virtual ~SimpleRequest() noexcept;

protected:
  /**
   * @brief Get request URL (BaseRequest overridden method).
   *
   * Build request url base on configuration parameters
   * and output it into string.
   * @param need generate URL
   * @return request url string.
   */
  virtual const std::string& _url(bool generate = true);

  /**
   * @brief response processing (BaseRequest overridden method).
   *
   * Called when request succeeded
   * @param response data
   */
  virtual void _on_response(const ResponseInformation& data);

};

/**
 * @class ClickRequest
 * @brief Click request
 */
class ClickRequest : public SimpleRequest
{
public:
  /**
   * @brief Constructor.
   *
   * @param owner pointer for main test class - QuerySenderBase.
   * @param client (user) ID.
   * @param optout flag, if true request send in 'opted out' mode.
   * @param request configuration
   * @param request URL
   */
  ClickRequest(
    QuerySenderBase* owner,
    unsigned long client_id,
    bool optout,
    const RequestConfig_var& config,
    const char* url);

  /**
   * @brief Destructor.
   */
  virtual ~ClickRequest() noexcept;

protected:
  /**
   * @brief Check response code (overrided BaseRequest method)
   *
   * Using to validate response
   * @return true - got valid response, false - invalid response
   */
  virtual bool
  _check_response_code(
    unsigned long response_code);

};

/**
 * @class ActionRequest
 * @brief Action request
 */
class ActionRequest : public SimpleRequest
{
public:
  /**
   * @brief Constructor.
   *
   * @param owner pointer for main test class - QuerySenderBase.
   * @param client (user) ID.
   * @param optout flag, if true request send in 'opted out' mode.
   * @param request configuration
   * @param request URL
   */
  ActionRequest(
    QuerySenderBase* owner,
    unsigned long client_id,
    bool optout,
    const RequestConfig_var& config,
    const char* url);

  /**
   * @brief Destructor.
   */
  virtual ~ActionRequest() noexcept;

protected:
  /**
   * @brief Check response code (overrided BaseRequest method)
   *
   * Using to validate response
   * @return true - got valid response, false - invalid response
   */
  virtual bool
  _check_response_code(
    unsigned long response_code);

};

/**
 * @class PassbackRequest
 * @brief Passback request
 */
class PassbackRequest : public SimpleRequest
{
public:
  /**
   * @brief Constructor.
   *
   * @param owner pointer for main test class - QuerySenderBase.
   * @param client (user) ID.
   * @param optout flag, if true request send in 'opted out' mode.
   * @param request configuration
   * @param request URL
   */
  PassbackRequest(
    QuerySenderBase* owner,
    unsigned long client_id,
    bool optout,
    const RequestConfig_var& config,
    const char* url);

  /**
   * @brief Destructor.
   */
  virtual ~PassbackRequest() noexcept;

protected:
  /**
   * @brief Check response code (overrided BaseRequest method)
   *
   * Using to validate response
   * @return true - got valid response, false - invalid response
   */
  virtual bool
  _check_response_code(
    unsigned long response_code);

};

/**
 * @class UserBindRequest
 * @brief UserBind request
 */
class UserBindRequest: public BaseRequest
{
public:

  /**
   * @brief Constructor.
   *
   * @param owner pointer for main test class - QuerySenderBase.
   * @param client (user) ID.
   * @param optout flag, if true request send in 'opted out' mode.
   * @param request configuration
   * @param adserver address
   */
  UserBindRequest(
    QuerySenderBase* owner,
    unsigned long client_id,
    const std::string& ssp_user_id,
    const RequestConfig_var& config,
    const char* server);

  /**
   * @brief Destructor.
   */
  virtual ~UserBindRequest() noexcept;

protected:
  /**
   * @brief Get request URL.
   *
   * Build request url base on configuration parameters
   * and output it into string.
   * @param need generate URL
   * @return request url string.
   */
  virtual const std::string&
  _url(
    bool generate = true);

  /**
   * @brief response processing
   *
   * @param response data
   */
  virtual void
  _on_response(
    const ResponseInformation& data);

private:
  // AdServer address
  std::string server_;

  // SSP user id
  std::string ssp_user_id_;
};

/**
 * @class OpenRTBRequest
 * @brief OpenRTB request
 */
class OpenRTBRequest: public BaseRequest
{

  struct OpenRTBParam
  {
    const char* name;
    AutoTest::OpenRTBRequest::Member param;
  };

  static const size_t PARAM_COUNT = 15;
  static const OpenRTBParam PARAMS[PARAM_COUNT];

public:

  /**
   * @brief Constructor.
   *
   * @param owner pointer for main test class - QuerySenderBase.
   * @param client (user) ID.
   * @param optout flag, if true request send in 'opted out' mode.
   * @param request configuration
   * @param adserver address
   */
  OpenRTBRequest(
    QuerySenderBase* owner,
    unsigned long client_id,
    const std::string& ssp_user_id,
    const RequestConfig_var& config,
    const char* server);

  /**
   * @brief Destructor.
   */
  virtual ~OpenRTBRequest() noexcept;

protected:
  /**
   * @brief Get request URL.
   *
   * Build request url base on configuration parameters
   * and output it into string.
   * @param need generate URL
   * @return request url string.
   */
  virtual const std::string&
  _url(
    bool generate = true);

  /**
   * @brief Get request BODY.
   *
   * Build request BODY based on configuration parameters
   * and output it into string.
   * @return request BODY string.
   */
  virtual const std::string&
  _body();

  /**
   * @brief response processing
   *
   * @param response data
   */
  virtual void
  _on_response(
    const ResponseInformation& data);

private:
  void _init();

  int
  _get_param_idx(
    const std::string& name);

  // AdServer address
  std::string server_;

  // SSP user id
  std::string ssp_user_id_;
};


typedef ReferenceCounting::SmartPtr<BaseRequest> Request_var;

inline
std::ostream&
operator<< (std::ostream& out, const BaseRequest* request)
{
  return request->dump(out);
}

#endif  // __REQUEST_HPP
