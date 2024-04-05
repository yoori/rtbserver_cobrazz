#ifndef FRONTENDCOMMONS_HTTPREQUEST
#define FRONTENDCOMMONS_HTTPREQUEST

// UNIXCOMMONS
#include <HTTP/HttpMisc.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Stream/BinaryStream.hpp>
#include <String/SubString.hpp>

namespace FrontendCommons
{

DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

class InputStream final : public Stream::BinaryInputStream
{
public:
  InputStream() noexcept;

  ~InputStream() override = default;

  InputStream(const String::SubString& buf) noexcept;

  void set_buf(const String::SubString& buf) noexcept;

  virtual Stream::BinaryInputStream& read(
    char_type* s,
    streamsize n);

  void has_body(bool val) noexcept;

private:
  String::SubString buf_;
  mutable size_t pos_;
};

class HttpRequest : public virtual ReferenceCounting::Interface
{
public:
  enum Method
  {
    RM_GET,
    RM_POST
  };

  class Exception : public FrontendCommons::Exception
  {
  public:
    template <typename T>
    explicit Exception(
      const T& description,
      int error_code = 0) noexcept;

    ~Exception() noexcept override;

    int error_code() const noexcept;

  protected:
    Exception() noexcept;

  protected:
    int error_code_;
  };

public:
  HttpRequest() noexcept = default;

  virtual ~HttpRequest() = default;

  virtual Method method() const noexcept = 0;

  virtual String::SubString uri() const noexcept = 0;

  virtual String::SubString args() const noexcept = 0;

  virtual const HTTP::ParamList& params() const noexcept = 0;

  virtual const HTTP::SubHeaderList& headers() const noexcept = 0;

  virtual String::SubString body() const noexcept = 0;

  virtual InputStream& get_input_stream() const noexcept = 0;

  virtual bool secure() const noexcept = 0;

  virtual String::SubString server_name() const noexcept = 0;

  virtual void set_params(HTTP::ParamList&& params) noexcept = 0;

  virtual bool header_only() const noexcept = 0;

  static void parse_params(
    const String::SubString& str,
    HTTP::ParamList& params);
};

using HttpRequest_var = ReferenceCounting::SmartPtr<HttpRequest>;

// class HttpRequestHolder
class HttpRequestHolder : public virtual ReferenceCounting::Interface
{
public:
  HttpRequestHolder() = default;

  virtual ~HttpRequestHolder() noexcept = default;

  virtual const HttpRequest& request() const = 0;

  virtual HttpRequest& request() = 0;
};

using HttpRequestHolder_var = ReferenceCounting::SmartPtr<HttpRequestHolder>;

} // namespace FrontendCommons

#endif //FRONTENDCOMMONS_HTTPREQUEST
