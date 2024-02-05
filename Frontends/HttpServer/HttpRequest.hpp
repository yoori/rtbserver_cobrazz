#ifndef FRONTENDS_HTTPSERVER_HTTPREQUEST
#define FRONTENDS_HTTPSERVER_HTTPREQUEST

// THIS
#include <FrontendCommons/HttpRequest.hpp>

namespace AdServer::Frontends::Http
{

class HttpRequest final :
  public FrontendCommons::HttpRequest,
  public ReferenceCounting::AtomicImpl
{
public:
  HttpRequest(
    const HttpRequest::Method method,
    const std::string& body,
    const std::string& uri,
    const std::string& server_name,
    const std::string& query_string,
    HTTP::HeaderList&& headers,
    const bool header_only,
    const bool secure)
    : method_(method),
      body_(body),
      uri_(uri),
      server_name_(server_name),
      query_string_(query_string),
      headers_(std::move(headers)),
      input_stream_(body_),
      header_only_(header_only),
      secure_(secure)
  {
    for (auto& header : headers)
    {
      const auto& name = header.name;
      const auto& value = header.value;

      subheaders_.emplace_back(
        String::SubString(name.data(), name.size()),
        String::SubString(value.data(), value.size()));
    }
  }

  Method method() const noexcept override
  {
    return method_;
  }

  const String::SubString& uri() const noexcept override
  {
    return String::SubString(uri_);
  }

  const String::SubString& args() const noexcept override
  {
    return String::SubString(query_string_);
  }

  const HTTP::ParamList& params() const noexcept override
  {
    return params_;
  }

  const HTTP::SubHeaderList& headers() const noexcept override
  {
    return subheaders_;
  }

  const String::SubString& body() const noexcept override
  {
    return String::SubString(body_);
  }

  FrontendCommons::InputStream& get_input_stream() const noexcept override
  {
    return input_stream_;
  }

  bool secure() const noexcept override
  {
    return secure_;
  }

  const String::SubString& server_name() const noexcept override
  {
    return String::SubString(body_);
  }

  void set_params(HTTP::ParamList&& params) noexcept override
  {
    params_ = std::move(params);
  }

  bool header_only() const noexcept override
  {
    return header_only_;
  }

private:
  ~HttpRequest() override = default;

private:
  Method method_ = RM_GET;

  std::string body_;

  std::string uri_;

  std::string server_name_;

  std::string query_string_;

  HTTP::ParamList params_;

  HTTP::HeaderList headers_;

  HTTP::SubHeaderList subheaders_;

  mutable FrontendCommons::InputStream input_stream_;

  bool header_only_ = false;

  bool secure_ = false;
};

using HttpRequest_var = ReferenceCounting::SmartPtr<HttpRequest>;

class HttpRequestHolder final :
  public FrontendCommons::HttpRequestHolder,
  public ReferenceCounting::AtomicImpl
{
public:
  HttpRequestHolder(HttpRequest* request)
    : request_(ReferenceCounting::add_ref(request))
  {
  }

  const HttpRequest& request() const
  {
    return *request_;
  }

  HttpRequest& request()
  {
    return *request_;
  }

protected:
  ~HttpRequestHolder() override = default;

private:
  HttpRequest_var request_;
};

using HttpRequestHolder_var = ReferenceCounting::SmartPtr<HttpRequestHolder>;

} // namespace AdServer::Frontends::Http

#endif //FRONTENDS_HTTPSERVER_HTTPREQUEST