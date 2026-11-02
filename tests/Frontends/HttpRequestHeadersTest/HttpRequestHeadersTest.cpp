#include <algorithm>
#include <cstring>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#include <HTTP/HTTPCookie.hpp>
#include <Frontends/FrontendCommons/HttpRequest.hpp>

namespace
{
  void require(bool value, const char* message)
  {
    if (!value)
    {
      throw std::runtime_error(message);
    }
  }

  void append_record(std::string& wire, unsigned char type, const std::string& content)
  {
    const unsigned char header[] = {
      1, type, 0, 1, static_cast<unsigned char>(content.size() >> 8),
      static_cast<unsigned char>(content.size()), 0, 0};
    wire.append(reinterpret_cast<const char*>(header), sizeof(header));
    wire.append(content);
  }

  void append_param(std::string& params, const std::string& name, const std::string& value)
  {
    require(name.size() < 128 && value.size() < 128, "short test parameters expected");
    params.push_back(static_cast<char>(name.size()));
    params.push_back(static_cast<char>(value.size()));
    params.append(name);
    params.append(value);
  }

  std::string make_request(std::size_t header_count)
  {
    std::string params;
    append_param(params, "REQUEST_METHOD", "POST");
    append_param(params, "REQUEST_URI", "/test");
    append_param(params, "HTTPS", "on");
    append_param(params, "REMOTE_ADDR", "127.0.0.1");
    append_param(params, "HTTP_COOKIE", "a=1; b=2");
    for (std::size_t i = 0; i < header_count; ++i)
    {
      append_param(params, "HTTP_X_TEST", std::to_string(i));
    }

    std::string wire;
    append_record(wire, FCGI_BEGIN_REQUEST, std::string(8, '\0'));
    append_record(wire, FCGI_PARAMS, params);
    append_record(wire, FCGI_PARAMS, {});
    append_record(wire, FCGI_STDIN, "body");
    append_record(wire, FCGI_STDIN, {});
    return wire;
  }

  FCGI::HttpRequestHolder::Buffer make_buffer(std::string_view wire)
  {
    std::shared_ptr<char[]> buffer(new char[wire.size()]);
    std::memcpy(buffer.get(), wire.data(), wire.size());
    return buffer;
  }

  void check_incremental_read(std::string wire, const std::string& body, std::size_t chunk)
  {
    FCGI::HttpRequestHolder_var holder(new FCGI::HttpRequestHolder());
    const auto buffer = make_buffer(wire);
    std::size_t offset = 0;
    while (offset < wire.size())
    {
      const auto size = std::min(chunk, wire.size() - offset);
      std::size_t consumed = 0;
      const int result = holder->append(buffer, offset, size, consumed);
      require(consumed == size, "incremental consumed size mismatch");
      offset += size;
      require(result == (offset == wire.size() ? FCGI::PARSE_OK : FCGI::PARSE_NEED_MORE),
        "incremental parse result mismatch");
    }

    require(holder->request().body() == std::string_view(body), "incremental body damaged");
    require(holder->request().uri() == "/test", "incremental URI damaged");
    if (wire.size() <= 8192)
    {
      const auto body_offset = wire.find(body);
      require(holder->request().body().data() == buffer.get() + body_offset,
        "socket buffer was copied before dispatch");
    }
  }

  void check_read_buffers()
  {
    const auto wire = make_request(8);
    for (std::size_t chunk : {1, 7, 8, 31, 8192})
    {
      check_incremental_read(wire, "body", chunk);
    }

    std::string large = wire.substr(0, wire.size() - 20);
    std::string body;
    for (char value : {'a', 'b', 'c'})
    {
      const std::string part(60000, value);
      append_record(large, FCGI_STDIN, part);
      large[large.size() - part.size() - 2] = 3;
      large.append(3, '\0');
      body += part;
    }

    append_record(large, FCGI_STDIN, {});
    check_incremental_read(large, body, 997);
    check_incremental_read(large, body, 262144);

    FCGI::HttpRequestHolder_var first(new FCGI::HttpRequestHolder());
    const auto combined = wire + wire;
    auto buffer = make_buffer(combined);
    std::size_t consumed = 0;
    require(first->append(buffer, 0, combined.size(), consumed) == FCGI::PARSE_OK,
      "coalesced first request failed");
    require(consumed == wire.size(), "coalesced trailing request lost");
    FCGI::HttpRequestHolder_var second(new FCGI::HttpRequestHolder());
    require(second->append(buffer, wire.size(), wire.size(), consumed) == FCGI::PARSE_OK,
      "coalesced second request failed");
    require(consumed == wire.size(), "coalesced second request size mismatch");
    buffer.reset();
    first = FCGI::HttpRequestHolder_var();
    require(second->request().body() == "body", "second request borrowed first buffer");
  }

  void check_request(std::size_t count)
  {
    FCGI::HttpRequestHolder_var holder(new FCGI::HttpRequestHolder());
    std::string wire = make_request(count);
    const auto buffer = make_buffer(wire);
    std::size_t consumed = 0;
    require(holder->append(buffer, 0, wire.size(), consumed) == FCGI::PARSE_OK, "parse failed");
    require(consumed == wire.size(), "request consumed size mismatch");
    wire.assign(wire.size(), '!');
    const auto& request = holder->request();
    const auto headers = request.headers();
    require(headers.size() == count + 3, "wrong header count");
    if (count == 8)
    {
      const auto begin = reinterpret_cast<std::uintptr_t>(&request);
      const auto data = reinterpret_cast<std::uintptr_t>(headers.data());
      require(data >= begin && data + headers.size_bytes() <= begin + sizeof(request),
        "small header vector did not use the embedded arena");
    }

    require(headers[0].name == "secure" && headers[0].value == "1", "HTTPS header lost");
    require(headers[1].name == ".remotehost", "remote address header lost");
    require(headers[1].value == "127.0.0.1", "remote address value lost");
    require(request.body() == "body" && request.uri() == "/test", "request data damaged");
    for (std::size_t i = 0; i < count; ++i)
    {
      require(headers[i + 3].name == "X_TEST", "header name damaged");
      require(headers[i + 3].value == String::SubString(std::to_string(i)),
        "duplicate header order or value damaged");
    }

    HTTP::CookieList cookies;
    cookies.load_from_headers(headers);
    require(cookies.size() == 2, "cookies were not parsed from span");
    require(cookies.begin()->name == "a" && cookies.begin()->value == "1", "cookie damaged");
  }
}

int main()
{
  try
  {
    FCGI::HttpRequestHolder_var holder(new FCGI::HttpRequestHolder());
    check_read_buffers();
    check_request(8);
    check_request(100);
    check_request(1);
    const auto wire = make_request(20);
    const auto buffer = make_buffer(wire);
    std::size_t consumed = 0;
    require(holder->append(buffer, 0, wire.size() - 8, consumed) == FCGI::PARSE_NEED_MORE,
      "incomplete request accepted");
    require(consumed == wire.size() - 8, "incomplete request consumed size mismatch");
    require(holder->request().headers().empty(), "headers exposed before completing request");
    require(holder->append(buffer, consumed, 8, consumed) == FCGI::PARSE_OK,
      "completed request rejected");
    require(consumed == 8 && holder->request().headers().size() == 23,
      "completed request headers damaged");
    check_request(20);
    std::cout << "HttpRequestHeadersTest: PASS\n";
    return 0;
  }
  catch (const std::exception& ex)
  {
    std::cerr << ex.what() << '\n';
    return 1;
  }
}
