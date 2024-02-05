#ifndef FRONTENDS_HTTPSERVER_HTTPRESPONSE
#define FRONTENDS_HTTPSERVER_HTTPRESPONSE

// STD
#include <deque>
#include <memory>

// UNIXCOMMONS
#include <eh/Exception.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>

// THIS
#include <Frontends/FrontendCommons/HttpResponse.hpp>

namespace AdServer::Frontends::Http
{

namespace internal
{

struct HttpResponseContainer final
{
  using Name = std::string;
  using Value = std::string;
  using Header = std::pair<Name, Value>;
  using Headers = std::deque<Header>;
  using BodyChunk = std::string;
  using BodyChunks = std::deque<BodyChunk>;

  Headers headers;
  BodyChunks body_chunks;
  std::size_t body_size = 0;
  int status_code = 0;
};

using HttpResponseContainerPtr = std::unique_ptr<HttpResponseContainer>;

} // namespace internal

class HttpResponse final :
  public FrontendCommons::HttpResponse,
  public ReferenceCounting::AtomicImpl
{
private:
  using HttpResponseContainer = internal::HttpResponseContainer;
  using HttpResponseContainerPtr = internal::HttpResponseContainerPtr;

public:
  using OutputStream = FrontendCommons::OutputStream;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  HttpResponse();

  ~HttpResponse() noexcept override = default;

  void add_header(
    const String::SubString& name,
    const String::SubString& value) override;

  void add_header(
    std::string&& name,
    std::string&& value) override;

  void set_content_type(const String::SubString& value) override;

  void add_cookie(const char* value) override;

  void add_cookie(std::string&& value) override;

  OutputStream& get_output_stream() noexcept override;

  ssize_t write(const String::SubString& str) noexcept override;

  ssize_t write(std::string&& str) noexcept override;

  size_t end_response(
    std::vector<String::SubString>& res,
    int status) noexcept override;

  bool cookie_installed() const noexcept override;

private:
  HttpResponseContainerPtr transfer() noexcept;

private:
  friend class HttpResponseWriterImpl;

  HttpResponseContainerPtr container_;

  OutputStream output_stream_;

  bool cookie_installed_ = false;
};

class HttpResponseFactory final :
  public FrontendCommons::HttpResponseFactory,
  public ReferenceCounting::AtomicImpl
{
public:
  using HttpResponse_var = FrontendCommons::HttpResponse_var;

public:
  HttpResponseFactory() = default;

  HttpResponse_var create() override;

private:
  ~HttpResponseFactory() override = default;
};

} // namespace AdServer::Frontends::Http

#endif //FRONTENDS_HTTPSERVER_HTTPRESPONSE
