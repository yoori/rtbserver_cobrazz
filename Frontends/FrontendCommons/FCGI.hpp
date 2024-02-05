#ifndef FRONTENDCOMMONS_FCGI
#define FRONTENDCOMMONS_FCGI

// STD
#include <memory>
#include <iostream>
#include <vector>

// UNIXCOMMONS
#include <ReferenceCounting/SmartPtr.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <Stream/BinaryStream.hpp>
#include <String/SubString.hpp>
#include <String/StringManip.hpp>

// THIS
#include <Frontends/FrontendCommons/HttpRequest.hpp>
#include <Frontends/FrontendCommons/HttpResponse.hpp>
#include <Frontends/FrontendCommons/HttpResponseWriter.hpp>

namespace tinyfcgi
{

typedef String::SubString string_ref;

} // namespace tinyfcgi

#include <Frontends/FrontendCommons/tinyfcgi/tinyfcgi.hpp>

namespace FCGI
{

enum ParseRes
{
  PARSE_OK,
  PARSE_NEED_MORE,
  PARSE_INVALID_HEADER,
  PARSE_BEGIN_REQUEST_EXPECTED,
  PARSE_INVALID_ID,
  PARSE_FRAGMENTED_STDIN
};

using InputStream = FrontendCommons::InputStream;
DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

class HttpResponseFCGI final :
  public FrontendCommons::HttpResponse,
  public ReferenceCounting::AtomicImpl
{
public:
  HttpResponseFCGI(uint16_t id = 1) noexcept;

  void add_header(
    const String::SubString& name,
    const String::SubString& value) override;

  void add_header(
    std::string&& name,
    std::string&& value) override;

  void set_content_type(const String::SubString& value) override;

  void add_cookie(const char* value) override;

  void add_cookie(std::string&& value) override;

  FrontendCommons::OutputStream& get_output_stream() noexcept override;

  ssize_t write(const String::SubString& str) noexcept override;

  ssize_t write(std::string&& str) noexcept override;

  size_t end_response(
    std::vector<String::SubString>& res,
    int status) noexcept override;

  bool cookie_installed() const noexcept override;

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

private:
  ~HttpResponseFCGI() noexcept override = default;

private:
  char wbuf_[32 * 1024];

  tinyfcgi::message status_msg_;

  tinyfcgi::message headers_msg_;

  std::vector<std::unique_ptr<MessageHolder>> body_messages_;

  tinyfcgi::message end_msg_;

  FrontendCommons::OutputStream output_stream_;

  size_t body_size_;

  bool cookie_installed_;
};

class HttpResponseFactory final :
  public FrontendCommons::HttpResponseFactory,
  public ReferenceCounting::AtomicImpl
{
public:
  HttpResponseFactory() = default;

  FrontendCommons::HttpResponse_var create() override;

private:
  ~HttpResponseFactory() override = default;
};

class HttpRequestFCGI final :
  public FrontendCommons::HttpRequest,
  public ReferenceCounting::AtomicImpl
{
public:
  HttpRequestFCGI() noexcept = default;

  Method method() const noexcept;

  const String::SubString& uri() const noexcept;

  const String::SubString& args() const noexcept;

  const HTTP::ParamList& params() const noexcept;

  const HTTP::SubHeaderList& headers() const noexcept;

  const String::SubString& body() const noexcept;

  InputStream& get_input_stream() const noexcept;

  bool secure() const noexcept;

  const String::SubString& server_name() const noexcept;

  void set_params(HTTP::ParamList&& params) noexcept;

  bool header_only() const noexcept;

  ParseRes parse(char* buf, size_t size);

private:
  ~HttpRequestFCGI() override = default;

private:
  Method method_ = RM_GET;

  String::SubString body_;

  String::SubString uri_;

  String::SubString server_name_;

  String::SubString query_string_;

  HTTP::ParamList params_;

  HTTP::SubHeaderList headers_;

  mutable InputStream input_stream_;

  bool header_only_ = false;

  bool secure_ = false;
};

using HttpRequestFCGI_var = ReferenceCounting::SmartPtr<HttpRequestFCGI>;

class HttpRequestHolderFCGI final :
  public FrontendCommons::HttpRequestHolder,
  public ReferenceCounting::AtomicImpl
{
public:
  using HttpRequest = FrontendCommons::HttpRequest;
  using HttpRequestFCGI_var = ReferenceCounting::SmartPtr<HttpRequestFCGI>;

public:
  HttpRequestHolderFCGI()
    : http_request_(new HttpRequestFCGI)
  {
  }

  int parse(const void* buf, unsigned long size);

  const HttpRequest& request() const override;

  HttpRequest& request() override;

private:
  ~HttpRequestHolderFCGI() override = default;

protected:
  std::string buf_;

  HttpRequestFCGI_var http_request_;
};

using HttpRequestHolderFCGI_var = ReferenceCounting::SmartPtr<HttpRequestHolderFCGI>;

inline bool HttpResponseFCGI::cookie_installed() const noexcept
{
  return cookie_installed_;
}

inline HttpRequestFCGI::Method
HttpRequestFCGI::method() const noexcept
{
  return method_;
}

inline const String::SubString&
HttpRequestFCGI::uri() const noexcept
{
  return uri_;
}

inline const String::SubString&
HttpRequestFCGI::args() const noexcept
{
  return query_string_;
}

inline const HTTP::ParamList&
HttpRequestFCGI::params() const noexcept
{
  return params_;
}

inline const HTTP::SubHeaderList&
HttpRequestFCGI::headers() const noexcept
{
  return headers_;
}

inline const String::SubString&
HttpRequestFCGI::body() const noexcept
{
  return body_;
}

inline InputStream&
HttpRequestFCGI::get_input_stream() const noexcept
{
  return input_stream_;
}

inline bool HttpRequestFCGI::secure() const noexcept
{
  return secure_;
}

inline const String::SubString&
HttpRequestFCGI::server_name() const noexcept
{
  return server_name_;
}

inline void HttpRequestFCGI::set_params(HTTP::ParamList&& params) noexcept
{
  params_ = std::move(params);
}

inline bool HttpRequestFCGI::header_only() const noexcept
{
  return header_only_;
}

inline const HttpRequestHolderFCGI::HttpRequest&
HttpRequestHolderFCGI::request() const
{
  return *http_request_;
}

inline HttpRequestHolderFCGI::HttpRequest&
HttpRequestHolderFCGI::request()
{
  return *http_request_;
}

inline int HttpRequestHolderFCGI::parse(const void* buf, unsigned long size)
{
  buf_.assign(static_cast<const char*>(buf), size);
  return http_request_->parse(const_cast<char*>(buf_.data()), size);
}

} // namespace FCGI

#endif // FRONTENDCOMMONS_FCGI
