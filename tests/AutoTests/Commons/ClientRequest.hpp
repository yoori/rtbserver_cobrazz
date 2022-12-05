/** $Id$
 * @file ClientRequest.hpp
 * http request wrapper
 */
#ifndef __AUTOTESTS_COMMONS_BASEREQUEST_HPP
#define __AUTOTESTS_COMMONS_BASEREQUEST_HPP

#include <string>

#include <ReferenceCounting/ReferenceCounting.hpp>

#include <eh/Exception.hpp>
#include <Sync/SyncPolicy.hpp>
#include <HTTP/HttpConnection.hpp>
#include <Generics/Time.hpp>
#include <HTTP/UrlAddress.hpp>

namespace AutoTest
{
  /**
   * @class ClientRequest
   *
   * @brief Class for sending HTTP request.
   *
   * Encapsulates data and functionality related to HTTP request.
   * Provides functionality of making request,
   * sending it to the server and receiving server response;
   */
  class ClientRequest : public virtual ReferenceCounting::Interface,
                  public virtual ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(InvalidArgument, Exception);

    struct Decoder
    {
      enum Types { T_REQUEST, T_RESPONSE };
      
      virtual
      void
      decode(
        std::ostream& out,
        Types decode_type,
        const std::string& data) const = 0;
    };

  public:

     /**
     * @brief Default constructor.
     */
    ClientRequest() 
      /*throw(Exception, eh::Exception)*/;

    /**
     * @brief Copy constructor.
     */
    ClientRequest(const ClientRequest& src)
      /*throw(Exception, eh::Exception)*/;

    /**
     * @brief Get request data.
     * @return request data string.
     */
    std::string& request_data() noexcept;

    /**
     * @brief Get response data.
     * @return response data string.
     */
    std::string& response_data() noexcept;

    /**
     * @brief Print request/response body (.
     * @param output stream.
     * @param body type (request or response).
     */
    void
    print(
      std::ostream& out,
      Decoder::Types data_type) const;

    /**
     * @brief Reset decoder.
     */
    void
    reset_decoder();
    
    /**
     * @brief Set decoder.
     * @param decoder.
     */
    template <class CustomDecoder>
    void
    set_decoder(const CustomDecoder& decoder);

    /**
     * @brief Get request timeout.
     * @return request timeout value.
     */
    Generics::Time& timeout() noexcept;

    /**
     * @brief Get request start time.
     * @return time of beginning of request.
     */
    const Generics::Time& start_time() const noexcept;

    /**
     * @brief Get request finish time.
     * @return time of ending of request.
     */
    const Generics::Time& finish_time() const noexcept;

    /**
     * @brief Get request headers.
     * @return list of request headers sent to the server.
     */
    HTTP::HeaderList& request_headers() noexcept;

    /**
     * @brief Get response headers.
     * @return list of response headers received from the server.
     */
    HTTP::HeaderList& response_headers() noexcept;

    /**
     * @brief Get request url.
     * @return http-address (url) of request.
     */
    HTTP::HTTPAddress address() const /*throw(eh::Exception)*/;

    /**
     * @brief Get size of http-request sent ot the server.
     * @return number of bytes sent to the server.
     */
    unsigned long bytes_out() noexcept;

    /**
     * @brief Get size of server response on http-request.
     * @return number of bytes received from the server.
     */
    unsigned long bytes_in() noexcept;

    /**
     * Set http address for the request.
     *
     * Initialize request with address of destination (URL).
     * Also initialize http connection for the request.
     * @param addr http address to set.
     */
    void address(const HTTP::HTTPAddress& addr)
      /*throw(Exception, eh::Exception)*/;

    /**
     * @brief Send request.
     *
     * Sends prepared and constructed request to the server
     * using indicated method and url (server http address).
     * @param method http method of the request (GET or POST).
     * @param need_response whether the response from the server is expected.
     * @param time output parameter, request duration.
     * @return true if there is no exceptions during sending the request.
     */
    virtual bool send(HTTP::HTTP_Connection::HTTP_Method method,
                      bool need_response,
                      Generics::Time& time)
      /*throw(Exception, eh::Exception)*/;

    /**
     * @brief Get status code of response.
     *
     * On every http request server responds with status code
     * in the first line of response.
     * Status code is a number that shows result of request.
     * @return status code of last response.
     */
    unsigned int status() const noexcept;
  
  protected:

    /**
     * @brief Default destructor.
     */
    virtual ~ClientRequest() noexcept;
    
  private:

    
    typedef std::unique_ptr<HTTP::HTTP_Connection> HTTP_Connection_var;

    /**
     * @brief ClientRequest headers list.
     *
     * List of headers which are sent to the server.
     * Header is a string containing name-value pair divided by a colon.
     */
    HTTP::HeaderList request_headers_;

    /**
     * @brief Response headers list.
     *
     * List of headers which are extracted from the server response.
     */
    HTTP::HeaderList response_headers_;

    /**
     * @brief ClientRequest data.
     *
     * This data will be sent to the server as http message body.
     */
    std::string       request_data_;

    /**
     * @brief Response data.
     *
     * HTTP message body that is received from the server.
     */
    std::string       response_data_;

    /**
     * @brief ClientRequest URL.
     */
    HTTP::HTTPAddress address_;

    /**
     * @brief ClientRequest timeout.
     */
    Generics::Time    timeout_;

    /**
     * @brief Time of sending request.
     */
    Generics::Time start_time_;

    /**
     * @brief Time of receiving response on request.
     */
    Generics::Time finish_time_;

    /**
     * @brief Number of bytes sent to the server.
     */
    unsigned int  bytes_out_;

    /**
     * @brief Number of bytes received from the server.
     */
    unsigned int  bytes_in_;

    /**
     * @brief Http connection object.
     *
     * Represents socket stream which is used
     * by two sides to communicate via HTTP.
     */
    HTTP_Connection_var http_connection_;

    /**
     * @brief HTTP response status code.
     */
    unsigned int status_;

    /**
     * @brief HTTP response & request body decoder.
     */
    std::unique_ptr<Decoder> decoder_;
  };
    
  typedef ReferenceCounting::SmartPtr<ClientRequest> ClientRequest_var;
  
  ///////////////////////////////////////////////////////////////////////////////
  // Inlines
  ///////////////////////////////////////////////////////////////////////////////
  
  inline
  unsigned int
  ClientRequest::status() const noexcept
  {
    return status_;
  }
  
  inline
  std::string&
  ClientRequest::request_data() noexcept
  {
    return request_data_;
  }
  
  inline
  std::string&
  ClientRequest::response_data() noexcept
  {
    return response_data_;
  }

  inline
  void
  ClientRequest::reset_decoder()
  {
    decoder_.reset();
  }

  template <class CustomDecoder>
  void
  ClientRequest::set_decoder(
    const CustomDecoder& decoder)
  {
    decoder_ = std::unique_ptr<Decoder>(new CustomDecoder(decoder));
  }
  
  inline
  HTTP::HTTPAddress
  ClientRequest::address() const /*throw(eh::Exception)*/
  {
    return address_;
  }
  
  inline
  Generics::Time&
  ClientRequest::timeout() noexcept
  {
    return timeout_;
  }
  
  inline
  const Generics::Time&
  ClientRequest::start_time() const noexcept
  {
    return start_time_;
  }
  
  inline
  const Generics::Time&
  ClientRequest::finish_time() const noexcept
  {
    return finish_time_;
  }
  
  inline
  HTTP::HeaderList&
  ClientRequest::request_headers() noexcept
  {
    return request_headers_;
  }
  
  inline
  HTTP::HeaderList&
  ClientRequest::response_headers() noexcept
  {
    return response_headers_;
  }
  
  inline
  unsigned long
  ClientRequest::bytes_out() noexcept
  {
    return bytes_out_;
  }
  
  inline
  unsigned long
  ClientRequest::bytes_in() noexcept
  {
    return bytes_in_;
  }
  
} // namespace AutoTest

#endif // __AUTOTESTS_COMMONS_BASEREQUEST_HPP

