#include "HttpResponse.hpp"
#include <sstream>
#include <utility>

namespace
{
  const String::SubString CONTENT_TYPE_HEADER("Content-Type");
}

namespace FCGI
{

OutputStream::OutputStream(HttpResponse* owner) noexcept
  : owner_(owner)
{}

Stream::BinaryOutputStream&
OutputStream::write(const char_type* s, streamsize n)
  /*throw(eh::Exception)*/
{
  ssize_t res = owner_->write(String::SubString(s, n));

  if (res < static_cast<ssize_t>(n) && n > 0)
  {
    setstate(std::ios_base::badbit | std::ios_base::failbit);
  }
  else if (n > 0)
  {
    setstate(std::ios_base::eofbit);
  }

  return *this;
}


HttpResponse::HttpResponse(uint16_t /*id*/) noexcept
  : status_(200),
    output_stream_(this),
    cookie_installed_(false)
{}

void
HttpResponse::set_status(int status) noexcept
{
  status_ = status;
}

int
HttpResponse::status() const noexcept
{
  return status_;
}

void
HttpResponse::add_header_nocopy(
  const String::SubString& name,
  const String::SubString& value)
  /*throw(eh::Exception)*/
{
  headers_.push_back(HTTP::SubHeader(name, value));
}

void
HttpResponse::add_header_nocopy_name(
  const String::SubString& name,
  std::string value)
  /*throw(eh::Exception)*/
{
  string_holders_.emplace_back(std::move(value));
  const std::string& value_holder = string_holders_.back();
  add_header_nocopy(name, String::SubString(value_holder));
}

void
HttpResponse::add_header(
  std::string name,
  std::string value)
  /*throw(eh::Exception)*/
{
  string_holders_.emplace_back(std::move(name));
  const std::string& name_holder = string_holders_.back();
  string_holders_.emplace_back(std::move(value));
  const std::string& value_holder = string_holders_.back();
  add_header_nocopy(
    String::SubString(name_holder),
    String::SubString(value_holder));
}

void
HttpResponse::set_content_type_nocopy(const String::SubString& value)
  /*throw(eh::Exception)*/
{
  add_header_nocopy(CONTENT_TYPE_HEADER, value);
}

void
HttpResponse::set_content_type(std::string value)
  /*throw(eh::Exception)*/
{
  add_header_nocopy_name(CONTENT_TYPE_HEADER, std::move(value));
}

void
HttpResponse::add_cookie(const char* value)
  /*throw(eh::Exception)*/
{
  cookies_.emplace_back(value ? value : "");
  cookie_installed_ = true;
}

OutputStream&
HttpResponse::get_output_stream() noexcept
{
  return output_stream_;
}

ssize_t
HttpResponse::write(const String::SubString& str) noexcept
{
  body_.append(str.data(), str.size());
  return str.size();
}

const HTTP::SubHeaderList&
HttpResponse::headers() const noexcept
{
  return headers_;
}

const std::vector<std::string>&
HttpResponse::cookies() const noexcept
{
  return cookies_;
}

const std::string&
HttpResponse::body() const noexcept
{
  return body_;
}

bool
HttpResponse::cookie_installed() const noexcept
{
  return cookie_installed_;
}

}
