// STD
#include <sstream>

// UNIXCOMMONS
#include <Generics/Function.hpp>

// THIS
#include <Frontends/HttpServer/HttpResponse.hpp>

namespace AdServer::Frontends::Http
{

namespace
{

const String::SubString CONTENT_TYPE("Content-Type");
const String::SubString SET_COOKIE("Set-Cookie");

} // namespace

HttpResponse::HttpResponse()
  : container_(std::make_unique<HttpResponseContainer>()),
    output_stream_(this)
{
}

void HttpResponse::add_header(
  const String::SubString& name,
  const String::SubString& value)
{
  container_->headers.emplace_back(
    std::piecewise_construct,
    std::forward_as_tuple(name.data(), name.size()),
    std::forward_as_tuple(value.data(), value.size()));
}

void HttpResponse::add_header(
  std::string&& name,
  std::string&& value)
{
  container_->headers.emplace_back(
    std::move(name),
    std::move(value));
}

void HttpResponse::set_content_type(
  const String::SubString& value)
{
  add_header(CONTENT_TYPE, value);
}

void HttpResponse::add_cookie(const char* value)
{
  cookie_installed_ = true;
  add_header(SET_COOKIE, String::SubString(value));
}

void HttpResponse::add_cookie(std::string&& value)
{
  cookie_installed_ = true;
  container_->headers.emplace_back(
    std::piecewise_construct,
    std::forward_as_tuple(SET_COOKIE.data(), SET_COOKIE.size()),
    std::forward_as_tuple(std::move(value)));
}

HttpResponse::OutputStream&
HttpResponse::get_output_stream() noexcept
{
  return output_stream_;
}

ssize_t HttpResponse::write(const String::SubString& str) noexcept
{
  const std::size_t capacity = 8 * 1024;
  size_t saved_size = 0;

  auto& body_chunks = container_->body_chunks;
  while(str.size() > saved_size)
  {
    if (body_chunks.empty() || body_chunks.back().size() >= body_chunks.back().capacity())
    {
      body_chunks.emplace_back();
      body_chunks.back().reserve(capacity);
    }

    auto& chunk = body_chunks.back();

    const std::size_t msg_size = std::min(
      str.size() - saved_size,
      chunk.capacity() - chunk.size());
    const char* const begin = str.data() + saved_size;
    chunk.append(begin, begin + msg_size);
    saved_size += msg_size;
  }

  container_->body_size += str.size();

  return str.size();
}

ssize_t HttpResponse::write(std::string&& str) noexcept
{
  const auto size = str.size();
  if (size >= 20)
  {
    container_->body_chunks.emplace_back(std::move(str));
    container_->body_size += str.size();
    return size;
  }
  else
  {
    return write(String::SubString(str.data(), size));
  }
}

size_t HttpResponse::end_response(
  std::vector<String::SubString>& /*res*/,
  int /*status*/) noexcept
{
  assert(false);
  return 0;
}

bool HttpResponse::cookie_installed() const noexcept
{
  return cookie_installed_;
}

HttpResponse::HttpResponseContainerPtr
HttpResponse::transfer() noexcept
{
  auto temp = std::make_unique<HttpResponseContainer>();
  std::swap(temp, container_);
  return temp;
}

HttpResponseFactory::HttpResponse_var
HttpResponseFactory::create()
{
  return HttpResponse_var(new HttpResponse);
}

} // namespace AdServer::Frontends::Http