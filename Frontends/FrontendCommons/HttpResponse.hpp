#pragma once

#include <memory>
#include <iostream>
#include <list>
#include <vector>

#include <Stream/BinaryStream.hpp>
#include <String/SubString.hpp>
#include <String/StringManip.hpp>

#include <ReferenceCounting/SmartPtr.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>

#include <HTTP/HttpMisc.hpp>
#include "HttpRequest.hpp"

namespace FCGI
{
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

  //
  // class HttpResponse
  //
  class HttpResponse: public ReferenceCounting::AtomicImpl
  {
  public:
    HttpResponse(uint16_t id = 1) noexcept;

    void
    set_status(int status) noexcept;

    int
    status() const noexcept;

    void
    add_header_nocopy(const String::SubString& name, const String::SubString& value)
      /*throw(eh::Exception)*/;

    void
    add_header_nocopy_name(const String::SubString& name, std::string value)
      /*throw(eh::Exception)*/;

    void
    add_header(std::string name, std::string value)
      /*throw(eh::Exception)*/;

    void
    set_content_type_nocopy(const String::SubString& value)
      /*throw(eh::Exception)*/;

    void
    set_content_type(std::string value)
      /*throw(eh::Exception)*/;

    void
    add_cookie(const char* value)
      /*throw(eh::Exception)*/;

    OutputStream&
    get_output_stream() noexcept;

    ssize_t
    write(const String::SubString& str) noexcept;

    const HTTP::SubHeaderList&
    headers() const noexcept;

    const std::vector<std::string>&
    cookies() const noexcept;

    const std::string&
    body() const noexcept;

    bool
    cookie_installed() const noexcept;

  protected:
    ~HttpResponse() noexcept override = default;

  private:
    int status_;
    HTTP::SubHeaderList headers_;
    std::list<std::string> string_holders_;
    std::vector<std::string> cookies_;
    std::string body_;
    OutputStream output_stream_;
    bool cookie_installed_;
  };

  typedef ReferenceCounting::SmartPtr<HttpResponse>
    HttpResponse_var;

  //
  // class BaseHttpResponseWriter
  //
  class BaseHttpResponseWriter: public virtual ReferenceCounting::AtomicImpl
  {
  public:
    virtual void
    write(HttpResponse_var response) = 0;

    void
    write(int code, HttpResponse* response)
    {
      response->set_status(code == 0 ? 200 : code);
      write(HttpResponse_var(ReferenceCounting::add_ref(response)));
    }
  };

  typedef ReferenceCounting::SmartPtr<BaseHttpResponseWriter>
    BaseHttpResponseWriter_var;

}
