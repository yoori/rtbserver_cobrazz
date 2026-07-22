#pragma once

#include <iostream>
#include <string>
#include <string_view>

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

    static void
    parse_params(std::string_view str, HTTP::ParamList& params)
      /*throw(String::StringManip::InvalidFormatException, eh::Exception)*/;

    static void
    parse_params(const std::string& str, HTTP::ParamList& params)
      /*throw(String::StringManip::InvalidFormatException, eh::Exception)*/
    {
      parse_params(std::string_view(str), params);
    }

    static void
    parse_params(const String::SubString& str, HTTP::ParamList& params)
      /*throw(String::StringManip::InvalidFormatException, eh::Exception)*/
    {
      parse_params(
        str.empty() ? std::string_view() :
          std::string_view(str.data(), str.size()),
        params);
    }

    HttpRequest() noexcept
      : method_(RM_GET),
        header_only_(false),
        secure_(false)
    {}

    Method
    method() const noexcept { return method_; }

    void
    set_method(Method method) noexcept;

    const String::SubString&
    uri() const noexcept { return uri_; }

    void
    set_uri(const String::SubString& uri) noexcept;

    const String::SubString&
    args() const noexcept { return query_string_; }

    void
    set_args(const String::SubString& query_string) noexcept;

    const HTTP::ParamList&
    params() const noexcept { return params_; }

    const HTTP::SubHeaderList&
    headers() const noexcept { return headers_; }

    void
    set_headers(HTTP::SubHeaderList&& headers) noexcept;

    std::string_view
    body() const noexcept
    {
      return body_.empty() ? std::string_view() :
        std::string_view(body_.data(), body_.size());
    }

    void
    set_body(const String::SubString& body) noexcept;

    bool
    secure() const noexcept { return secure_; }

    void
    set_secure(bool secure) noexcept { secure_ = secure; }

    const String::SubString&
    server_name() const noexcept { return server_name_; }

    void
    set_server_name(const String::SubString& server_name) noexcept;

    void
    set_params(HTTP::ParamList&& params) noexcept { params_ = std::move(params); }

    bool
    header_only() const noexcept { return header_only_; }

    void
    set_header_only(bool header_only) noexcept { header_only_ = header_only; }

  private:
    Method method_;
    String::SubString body_;
    String::SubString uri_;
    String::SubString server_name_;
    String::SubString query_string_;
    HTTP::ParamList params_;
    HTTP::SubHeaderList headers_;
    bool header_only_;
    bool secure_;
  };

  class HttpRequestHolder: public ReferenceCounting::AtomicImpl
  {
  public:
    int
    parse(const void* buf, unsigned long size);

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
}

namespace FCGI
{
  template <typename T>
  HttpRequest::Exception::Exception(const T& description, int error_code) noexcept
    : FCGI::Exception(description),
      error_code_(error_code)
  {}

  inline
  HttpRequest::Exception::~Exception() noexcept = default;

  inline int
  HttpRequest::Exception::error_code() const noexcept
  {
    return error_code_;
  }

  inline
  HttpRequest::Exception::Exception() noexcept
    : error_code_(0)
  {}

  inline void
  HttpRequest::set_method(Method method) noexcept
  {
    method_ = method;
  }

  inline void
  HttpRequest::set_uri(const String::SubString& uri) noexcept
  {
    uri_ = uri;
  }

  inline void
  HttpRequest::set_args(const String::SubString& query_string) noexcept
  {
    query_string_ = query_string;
  }

  inline void
  HttpRequest::set_headers(HTTP::SubHeaderList&& headers) noexcept
  {
    headers_ = std::move(headers);
  }

  inline void
  HttpRequest::set_body(const String::SubString& body) noexcept
  {
    body_ = body;
  }

  inline void
  HttpRequest::set_server_name(const String::SubString& server_name) noexcept
  {
    server_name_ = server_name;
  }
}
