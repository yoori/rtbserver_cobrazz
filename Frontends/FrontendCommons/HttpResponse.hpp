#pragma once

#include <memory>
#include <iostream>
#include <vector>
#include <string_view>
#include <type_traits>
#include <utility>

#include <Stream/BinaryStream.hpp>
#include <String/SubString.hpp>
#include <String/StringManip.hpp>

#include <ReferenceCounting/SmartPtr.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>

#include <HTTP/HttpMisc.hpp>
#include <Generics/MonoAllocator.hpp>
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
    HttpResponse(uint16_t id = 1);

    void
    set_status(int status) noexcept;

    int
    status() const noexcept;

    void
    add_header_nocopy(const String::SubString& name, const String::SubString& value)
      /*throw(eh::Exception)*/;

    template<typename Value>
    void add_header_nocopy_name(const String::SubString& name, Value&& value)
      /*throw(eh::Exception)*/;

    template<typename Name, typename Value>
    void add_header(Name&& name, Value&& value)
      /*throw(eh::Exception)*/;

    void
    set_content_type_nocopy(const String::SubString& value)
      /*throw(eh::Exception)*/;

    template<typename Value>
    void set_content_type(Value&& value)
      /*throw(eh::Exception)*/;

    void
    add_cookie(const char* value)
      /*throw(eh::Exception)*/;

    OutputStream&
    get_output_stream() noexcept;

    ssize_t
    write(const String::SubString& str) noexcept;

    HTTP::SubHeaderSpan headers() const noexcept;

    const std::vector<std::string>&
    cookies() const noexcept;

    const std::string&
    body() const noexcept;

    bool
    cookie_installed() const noexcept;

  protected:
    ~HttpResponse() noexcept override = default;

  private:
    template<typename Value>
    String::SubString hold_header_string_(Value&& value);

    String::SubString copy_header_string_(std::string_view value);

    String::SubString move_header_string_(std::string&& value);

    int status_;
    static constexpr std::size_t INLINE_HEADER_COUNT = 16;
    Generics::MonoAllocatorFixedArena<INLINE_HEADER_COUNT * sizeof(HTTP::SubHeader)> header_arena_;
    Generics::MonoVector<HTTP::SubHeader> headers_;
    Generics::MonoList<std::string> string_holders_;
    std::vector<std::string> cookies_;
    std::string body_;
    OutputStream output_stream_;
    bool cookie_installed_;
  };

  template<typename Value>
  void HttpResponse::add_header_nocopy_name(const String::SubString& name, Value&& value)
  {
    add_header_nocopy(name, hold_header_string_(std::forward<Value>(value)));
  }

  template<typename Name, typename Value>
  void HttpResponse::add_header(Name&& name, Value&& value)
  {
    const auto stored_name = hold_header_string_(std::forward<Name>(name));
    const auto stored_value = hold_header_string_(std::forward<Value>(value));
    add_header_nocopy(stored_name, stored_value);
  }

  template<typename Value>
  void HttpResponse::set_content_type(Value&& value)
  {
    add_header_nocopy_name(String::SubString("Content-Type"), std::forward<Value>(value));
  }

  template<typename Value>
  String::SubString HttpResponse::hold_header_string_(Value&& value)
  {
    if constexpr (std::is_same_v<Value, std::string>)
    {
      return move_header_string_(std::move(value));
    }
    else if constexpr (std::is_same_v<std::decay_t<Value>, String::SubString>)
    {
      return copy_header_string_(std::string_view(value.data(), value.size()));
    }
    else
    {
      return copy_header_string_(std::string_view(value));
    }
  }

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
