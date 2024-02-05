#ifndef FRONTENDCOMMONS_HTTPRESPONSE
#define FRONTENDCOMMONS_HTTPRESPONSE

// STD
#include <vector>

// UNIXCOMMONS
#include <ReferenceCounting/SmartPtr.hpp>

// THIS
#include <Stream/BinaryStream.hpp>
#include <String/SubString.hpp>

namespace FrontendCommons
{

class HttpResponse;

class OutputStream final: public Stream::BinaryOutputStream
{
public:
  OutputStream(HttpResponse* owner) noexcept;

  Stream::BinaryOutputStream& write(
    const char_type* s,
    streamsize n) override;

private:
  HttpResponse* owner_;
};

class HttpResponse : public virtual ReferenceCounting::Interface
{
public:
  HttpResponse() = default;

  ~HttpResponse() noexcept override = default;

  virtual void add_header(
    const String::SubString& name,
    const String::SubString& value) = 0;

  virtual void add_header(
    std::string&& name,
    std::string&& value) = 0;

  virtual void set_content_type(const String::SubString& value) = 0;

  virtual void add_cookie(const char* value) = 0;

  virtual void add_cookie(std::string&& value) = 0;

  virtual OutputStream& get_output_stream() noexcept = 0;

  virtual ssize_t write(const String::SubString& str) noexcept = 0;

  virtual ssize_t write(std::string&& str) noexcept = 0;

  virtual size_t end_response(
    std::vector<String::SubString>& res,
    int status) noexcept = 0;

  virtual bool cookie_installed() const noexcept = 0;
};

using HttpResponse_var = ReferenceCounting::SmartPtr<HttpResponse>;

class HttpResponseFactory : public virtual ReferenceCounting::Interface
{
public:
  HttpResponseFactory() = default;

  virtual ~HttpResponseFactory() = default;

  virtual HttpResponse_var create() = 0;
};

using HttpResponseFactory_var = ReferenceCounting::SmartPtr<HttpResponseFactory>;

} // namespace FrontendCommons

#endif //FRONTENDCOMMONS_HTTPRESPONSE
