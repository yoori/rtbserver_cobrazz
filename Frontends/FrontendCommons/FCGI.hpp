#pragma once

#include <memory>
#include <iostream>
#include <vector>

#include <Stream/BinaryStream.hpp>
#include <String/SubString.hpp>
#include <String/StringManip.hpp>

#include <ReferenceCounting/SmartPtr.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>

#include <HTTP/HttpMisc.hpp>

namespace tinyfcgi
{
  typedef String::SubString string_ref;
}

#include "tinyfcgi/tinyfcgi.hpp"

namespace FCGI
{
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  class InputStream: public Stream::BinaryInputStream
  {
  public:
    InputStream() noexcept;

    InputStream(const String::SubString& buf) noexcept;

    void
    set_buf(const String::SubString& buf) noexcept;

    virtual Stream::BinaryInputStream&
    read(char_type* s, streamsize n) /*throw(eh::Exception)*/;

    void
    has_body(bool val) noexcept;

  private:
    String::SubString buf_;
    mutable size_t pos_;
  };

  class HttpResponse;

  class OutputStream: public Stream::BinaryOutputStream
  {
  public:
    OutputStream(HttpResponse* owner) noexcept;

    virtual
    Stream::BinaryOutputStream&
    write(const char_type* s, streamsize n) /*throw(eh::Exception)*/;

  private:
    HttpResponse* owner_;
  };

  enum ParseRes
  {
    PARSE_OK,
    PARSE_NEED_MORE,
    PARSE_INVALID_HEADER,
    PARSE_BEGIN_REQUEST_EXPECTED,
    PARSE_INVALID_ID,
    PARSE_FRAGMENTED_STDIN
  };

  class HttpRequest
  {
  public:
    enum Method
    {
      RM_GET,
      RM_POST
    };

    class Exception : public FCGI::Exception
    {
    public:
      template <typename T>
      explicit
      Exception(const T& description, int error_code = 0)
        noexcept;

      virtual
      ~Exception() noexcept;

      int
      error_code() const noexcept;

    protected:
      Exception() noexcept;

    protected:
      int error_code_;
    };

  public:
    static void
    parse_params(const String::SubString& str, HTTP::ParamList& params)
      /*throw(String::StringManip::InvalidFormatException, eh::Exception)*/;

  public:
    HttpRequest() noexcept
      : method_(RM_GET),
        header_only_(false),
        secure_(false)
    {}

    Method
    method() const noexcept { return method_; }

    const String::SubString&
    uri() const noexcept { return uri_; }

    const String::SubString&
    args() const noexcept { return query_string_; }

    const HTTP::ParamList&
    params() const noexcept { return params_; }

    const HTTP::SubHeaderList&
    headers() const noexcept { return headers_; }

    const String::SubString&
    body() const noexcept { return body_; }

    InputStream&
    get_input_stream() const noexcept { return input_stream_; }

    bool
    secure() const noexcept { return secure_; }

    const String::SubString&
    server_name() const noexcept { return server_name_; }

    void
    set_params(HTTP::ParamList&& params) noexcept { params_ = std::move(params); }

    /**
     * HEAD request, as opposed to GET
     * @return true if HEAD request
     */
    bool
    header_only() const noexcept { return header_only_; }

  public:
    ParseRes
    parse(char* buf, size_t size);

  private:
    Method method_;
    String::SubString body_;
    String::SubString uri_;
    String::SubString server_name_;
    String::SubString query_string_;
    HTTP::ParamList params_;
    HTTP::SubHeaderList headers_;
    mutable InputStream input_stream_;
    bool header_only_;
    bool secure_;
  };

  // class HttpRequestHolder
  class HttpRequestHolder: public ReferenceCounting::AtomicImpl
  {
  public:
    int
    parse(const void* buf, unsigned long size)
    {
      buf_.assign(static_cast<const char*>(buf), size);
      return http_request_.parse(const_cast<char*>(buf_.data()), size);
    }

    const HttpRequest&
    request() const
    {
      return http_request_;
    }

    HttpRequest&
    request()
    {
      return http_request_;
    }

  protected:
    virtual
    ~HttpRequestHolder() noexcept = default;

  protected:
    std::string buf_;
    HttpRequest http_request_;
  };

  typedef ReferenceCounting::SmartPtr<HttpRequestHolder>
    HttpRequestHolder_var;

  //
  // class HttpResponse
  //
  class HttpResponse: public ReferenceCounting::AtomicImpl
  {
  public:
    HttpResponse(uint16_t id = 1) noexcept;

    void
    add_header(
      const String::SubString& name,
      const String::SubString& value)
      /*throw(eh::Exception)*/;

    void
    set_content_type(const String::SubString& value)
      /*throw(eh::Exception)*/;

    void
    add_cookie(const char* value)
      /*throw(eh::Exception)*/;

    OutputStream&
    get_output_stream() noexcept;

    ssize_t
    write(const String::SubString& str) noexcept;

    size_t
    end_response(
      std::vector<String::SubString>& res,
      int status) noexcept;

    bool
    cookie_installed() const noexcept;

  protected:
    struct MessageHolder
    {
      MessageHolder(uint16_t id)
        : buf(MAX_SIZE),
          msg(id, buf.data(), MAX_SIZE)
      {};

      static const size_t MAX_SIZE = 64 * 1024 - 1;
      std::vector<char> buf;
      tinyfcgi::message msg;
    };

  protected:
    virtual
    ~HttpResponse() noexcept = default;

  private:
    char wbuf_[32 * 1024];
    tinyfcgi::message status_msg_;
    tinyfcgi::message headers_msg_;
    std::vector<std::unique_ptr<MessageHolder> > body_messages_;
    tinyfcgi::message end_msg_;
    OutputStream output_stream_;
    size_t body_size_;
    bool cookie_installed_;
  };

  typedef ReferenceCounting::SmartPtr<HttpResponse>
    HttpResponse_var;

  //
  // class HttpResponseWriter
  //
  class HttpResponseWriter: public virtual ReferenceCounting::AtomicImpl
  {
  public:
    virtual void
    write(int code, FCGI::HttpResponse* response) = 0;
  };

  typedef ReferenceCounting::SmartPtr<HttpResponseWriter>
    HttpResponseWriter_var;
}

namespace FCGI
{
  inline bool
  HttpResponse::cookie_installed() const noexcept
  {
    return cookie_installed_;
  }
}
