#include <cstdint>
#include <iostream>
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

  void check_request(FCGI::HttpRequestHolder& holder, std::size_t count)
  {
    std::string wire = make_request(count);
    require(holder.parse(wire.data(), wire.size()) == FCGI::PARSE_OK, "parse failed");
    wire.assign(wire.size(), '!');
    const auto& request = holder.request();
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
    check_request(*holder, 8);
    check_request(*holder, 100);
    check_request(*holder, 1);
    const auto wire = make_request(20);
    require(holder->parse(wire.data(), wire.size() - 8) == FCGI::PARSE_NEED_MORE,
      "incomplete request accepted");
    require(holder->request().headers().empty(), "old headers retained after incomplete parse");
    check_request(*holder, 20);
    std::cout << "HttpRequestHeadersTest: PASS\n";
    return 0;
  }
  catch (const std::exception& ex)
  {
    std::cerr << ex.what() << '\n';
    return 1;
  }
}
