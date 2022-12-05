/** $Id$
 * @file BaseAdClient.hpp
 * Ad server client 
 */
#ifndef __AUTOTESTS_COMMONS_BASEADCLIENT_HPP
#define __AUTOTESTS_COMMONS_BASEADCLIENT_HPP

#if !defined(__AUTOTESTS_COMMONS_ADCLIENT_HPP)
#error system header, must be included only from AdClient
#endif //!__AUTOTESTS_COMMONS_ADCLIENT_HPP

#include <vector>
#include <list>
#include <String/RegEx.hpp>
#include <String/StringManip.hpp>
#include <eh/Exception.hpp>
#include <HTTP/HttpConnection.hpp>

#include <tests/AutoTests/Commons/Logger.hpp>
#include <tests/AutoTests/Commons/ClientRequest.hpp>
#include <tests/AutoTests/Commons/Cookie.hpp>

typedef std::vector<std::string> StrVector;
typedef std::list<std::string> StrList;

namespace AutoTest
{
  /**
   * @class BaseAdClient
   *
   * @brief Base AutoTest AdClient for interaction with AdServer.
   *
   * Class for quick constructing of unit's functionality
   * for AdServer testing. Sends requests and stores server's responses,
   * so you can analyze it and expect known results.
   * Contains tools related to unit's constructing
   * and provides debug information from AdServer.
   */
  class BaseAdClient
  {

    static Logging::Logger_var NULL_LOGGER_;
    
    /**
     * @brief Base url.
     *
     * Default url which is sent to the server without additional constructions.
     * Additional constructions means setting path and params for the request.
     * Generally represents address of AdFrontend service.
     */
    std::string base_url_;

    /**
     * @brief AdClient cookies
     *
     * Cookie is a small string of text stored on a client.
     * A cookie consists of one or more name-value pairs
     * containing bits of information such as user id, session info
     * or other data used by AdServer.
     */
    Cookie::UnitCookieList cookies_;


    /**
     * @brief Duration of the last request.
     */
    Generics::Time time_;


  protected:


    /**
     * @brief HTTP Request object.
     *
     * It responsible for sending requests to the server
     * and receiving responses from it.
     */
    ClientRequest_var request_;
    
    /**
     * @brief Stored last request.
     */
    ClientRequest_var stored_request_;
      
  public:

    enum LoggerType
    {
      LT_BASE,
      LT_NULL
    };

    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(InvalidArgument, Exception);

    /**
     * @brief Find http header value.
     *
     * Searches header with indicated name in server response
     * and fetches its value if it's found.
     * @param name header name.
     * @param value output parameter, found header value.
     * @return whether header with indicated name found.
     */
    bool find_header_value(const char* name, std::string& value) const
      /*throw(Exception)*/;

    /**
     * @brief Extract domain name from url.
     * @param url source url for extracting domain.
     * @return extracted domain name.
     */
    std::string get_domain_from_url(const std::string& url)
      /*throw(Exception)*/;

    /**
     * @brief Extract domain name from base url.
     * @return extracted domain name.
     */
    std::string get_domain ()
      /*throw(Exception)*/;

    /**
     * @brief Extract host name from url.
     * @param url source url.
     * @return extracted host name.
     */
    const std::string get_host_from_url(const std::string& url)
      /*throw(Exception)*/;

    /**
     * @brief Extract host name from base url.
     * @return extracted host name.
     */
    const std::string get_host ()
      /*throw(Exception)*/;

    /**
     * @brief Get base url for client requests.
     * @return base url.
     */
    const std::string& get_base_url () const
      /*throw(Exception)*/;

    /**
     * @brief Add header to request headers list.
     *
     * Added header will be sent to the server with next request.
     * @param header_name name of added header.
     * @param header_val value of added header.
     */
    void
    add_http_header(
      const std::string& header_name,
      const std::string& header_val)
      /*throw(Exception)*/;

    /**
     * @brief Send request to the server.
     *
     * Processes http request with client's cookies,
     * receives server response and stores it on client side.
     * Extracts cookies from response headers.
     * @param request_url part of url, appended to the base url
     * or whole request url if base url is empty.
     * @param method specifies http method of the request (GET or POST).
     * @return whether processing request is successful.
     */
    bool
    process_request(
      const char* request_url, 
      HTTP::HTTP_Connection::HTTP_Method method =
        HTTP::HTTP_Connection::HM_Get)
      /*throw(Exception, InvalidArgument)*/;

    /**
     * @brief Send http request with not null body.
     *
     * Set request data (http message body) and process request.
     * @param request_url part of url, appended to the base url
     * or whole request url if base url is empty.
     * @param body http message body.
     * @param method specifies http method of the request (GET or POST).
     * @return whether processing request is successful.
     */
    bool
    process_request(
      const std::string& request_url, 
      const std::string& body,
      HTTP::HTTP_Connection::HTTP_Method method =
        HTTP::HTTP_Connection::HM_Get)
      /*throw(Exception, InvalidArgument)*/;

    /**
     * @brief Get client's cookies.
     * @return cookies list stored by AdClient.
     */
    Cookie::UnitCookieList& get_cookies() noexcept;

    const Cookie::UnitCookieList& get_cookies() const noexcept;

    /**
     * @brief Get response headers.
     * @return response headers list.
     */
    HTTP::HeaderList& get_response_headers() noexcept;

    /**
     * @brief Get request headers.
     * @return request header list.
     */
    HTTP::HeaderList& get_request_headers() noexcept;

    /**
     * @brief Get response data (http message body).
     * @return received from server message body.
     */
    const std::string& req_response_data() const noexcept;

    /**
     * @brief Get response status code.
     * @return status code of the last request.
     */
    unsigned int req_status() const noexcept;

    /**
     * @brief Get request duration.
     * @return duration of the last request.
     */
    const Generics::Time req_time() const noexcept;

    /**
     * @brief Print response data of the last request.
     *
     * Dumps status code, response headers,
     * set cookies and response message body into logger.
     * @param url last request url.
     */
    void print_request_data(const std::string url);

    /**
     * @brief Get last request url.
     *
     * AdClient stores last request url in internal structures.
     * @return last request url.
     */
    std::string get_stored_url() const;

    /**
     * @brief Clear all data related with the last request.
     *
     * Make request parameters, headers and body empty.
     */
    void clear_request_data() /*throw(eh::Exception)*/;


    /** @brief Change base URL
     *
     *  Set new base URL
     */
    void change_base_url(const char* base_url);


    /** @brief Get client logger
     *
     *  @return logger
     */
    Logging::Logger& logger() const;

  protected:

    /**
     * @brief Constructor.
     *
     * Creates BaseAdClient object and initializes it with
     * base_url and logger.
     * @param base_url base url for created object.
     * @param logger logger for created object.
     */
    BaseAdClient(
      const char* base_url, 
      LoggerType log_type) 
      /*throw(Exception, eh::Exception)*/;

    /**
     * @brief Copy constructor.
     *
     * Creates copy of existing object.
     * @param src copied object.
     */
    BaseAdClient(const BaseAdClient& src) 
      /*throw(Exception, eh::Exception)*/;

    /**
     * @brief Default destructor.
     */
    ~BaseAdClient() noexcept;

    /**
     * @brief Assignment operator.
     */
    BaseAdClient& operator= (const BaseAdClient& client);


    /**
     * @brief AdClient logger.
     *
     * Dumps debug information about AdClient's actions
     * in a chronological order into log files.
     * Also dumps any another information related with AdClient.
     */
    LoggerType log_type_;
  };
    
  //
  //   AdClient inlines
  //
  inline
  bool
  BaseAdClient::process_request(
    const std::string& request_url, 
    const std::string& body,
    HTTP::HTTP_Connection::HTTP_Method method)
    /*throw(Exception, InvalidArgument)*/
  {
    if(!body.empty())
    {
      request_->request_data().assign(body);
    }
    return process_request(request_url.c_str(), method);
  }

    
  inline  
  unsigned int 
  BaseAdClient::req_status() const
    noexcept
  {
    return request_->status();
  }
    
  inline
  const Generics::Time 
  BaseAdClient::req_time() const 
    noexcept
  {
    return time_;
  }
    
  inline
  const std::string& 
  BaseAdClient::req_response_data() const
    noexcept
  {
      return request_->response_data();
  }
    
  inline
  Cookie::UnitCookieList& 
  BaseAdClient::get_cookies()
    noexcept
  {
    return cookies_;
  }

  inline
  const Cookie::UnitCookieList&
  BaseAdClient::get_cookies() const
    noexcept
  {
    return cookies_;
  }

  inline
  HTTP::HeaderList&
  BaseAdClient::get_response_headers() noexcept
  {
    return request_->response_headers();
  }

  inline
  HTTP::HeaderList&
  BaseAdClient::get_request_headers() noexcept
  {
    return request_->request_headers();
  }

  inline std::string BaseAdClient::get_stored_url() const
  {
    return
      stored_request_->
        address().url();
  }

} //namespace AutoTest

#endif // __AUTOTESTS_COMMONS_BASEADCLIENT_HPP
