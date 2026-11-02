#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

#include <Frontends/FrontendCommons/HttpResponse.hpp>

namespace
{
  void require(bool value, const char* message)
  {
    if (!value)
    {
      throw std::runtime_error(message);
    }
  }

  void check_arena_strings()
  {
    FCGI::HttpResponse_var response(new FCGI::HttpResponse());
    std::string name(100, 'n');
    std::string value(300, 'v');
    value[150] = '\0';
    const std::string expected_value = value;
    response->add_header(name, value);
    response->add_header_nocopy_name(String::SubString("X-View"),
      std::string_view(value.data(), value.size()));
    response->add_header_nocopy_name(String::SubString("X-Substring"), String::SubString(value));
    response->add_header("X-Empty", std::string_view());
    response->set_content_type(value);
    require(response->headers()[0].name.data() != name.data(), "lvalue name was borrowed");
    require(response->headers()[0].value.data() != value.data(), "lvalue value was borrowed");
    name.assign(100, 'x');
    value.assign(300, 'x');

    std::string moved_name(200, 'm');
    std::string moved_value(400, 'z');
    const auto* name_buffer = moved_name.data();
    const auto* value_buffer = moved_value.data();
    response->add_header(std::move(moved_name), std::move(moved_value));
    require(response->headers()[5].name.data() == name_buffer, "rvalue name was copied");
    require(response->headers()[5].value.data() == value_buffer, "rvalue value was copied");
    const auto* copied_value = response->headers()[0].value.data();
    for (std::size_t i = 0; i < 100; ++i)
    {
      response->add_header(name, value);
    }

    const auto headers = response->headers();
    require(headers[0].name == String::SubString(std::string(100, 'n')), "lvalue name damaged");
    for (const auto index : {0, 1, 2, 4})
    {
      require(headers[index].value == String::SubString(expected_value), "copied value damaged");
    }

    require(headers[0].value.data() == copied_value, "arena string moved during growth");
    require(headers[3].value.empty(), "empty view damaged");
    require(headers[5].value.data() == value_buffer, "moved string lost during growth");
  }

  void check_headers()
  {
    FCGI::HttpResponse_var response(new FCGI::HttpResponse());
    const std::string borrowed_name = "X-Borrowed";
    const std::string borrowed_value = "borrowed-value";
    response->add_header_nocopy(borrowed_name, borrowed_value);
    response->add_header("X-Owned", "short");
    response->add_header_nocopy_name(String::SubString("X-Value"), std::string(256, 'v'));
    response->set_content_type("text/plain");
    response->add_header("X-Owned", "duplicate");

    const auto first = response->headers();
    const auto begin = reinterpret_cast<std::uintptr_t>(response.in());
    const auto data = reinterpret_cast<std::uintptr_t>(first.data());
    require(data >= begin && data + first.size_bytes() <= begin + sizeof(FCGI::HttpResponse),
      "small header vector did not use the embedded arena");
    require(first[0].name.data() == borrowed_name.data(), "nocopy name was copied");
    require(first[0].value.data() == borrowed_value.data(), "nocopy value was copied");

    const char* owned_name = first[1].name.data();
    const char* short_value = first[1].value.data();
    const char* long_value = first[2].value.data();

    for (std::size_t i = 0; i < 100; ++i)
    {
      response->add_header("X-" + std::to_string(i), std::string(128 + i, 'a' + i % 26));
    }

    const auto headers = response->headers();
    require(headers.size() == 105, "headers lost during growth");
    require(headers[1].name.data() == owned_name, "owned name moved during growth");
    require(headers[1].value.data() == short_value, "short value moved during growth");
    require(headers[2].value.data() == long_value, "long value moved during growth");
    require(headers[0].name.data() == borrowed_name.data(), "borrowed name lost during growth");
    require(headers[0].value.data() == borrowed_value.data(), "borrowed value lost during growth");
    require(headers[1].name == "X-Owned" && headers[1].value == "short", "short string damaged");
    require(headers[2].name == "X-Value", "nocopy-name header damaged");
    require(headers[2].value == String::SubString(std::string(256, 'v')), "long string damaged");
    require(headers[3].name == "Content-Type" && headers[3].value == "text/plain",
      "content type damaged");
    require(headers[4].name == "X-Owned" && headers[4].value == "duplicate",
      "duplicate order damaged");
    for (std::size_t i = 0; i < 100; ++i)
    {
      require(headers[i + 5].name == String::SubString("X-" + std::to_string(i)),
        "owned name damaged during growth");
      require(headers[i + 5].value == String::SubString(std::string(128 + i, 'a' + i % 26)),
        "owned value damaged during growth");
    }

    response->add_cookie("a=1");
    response->write(String::SubString("body"));
    require(response->cookie_installed() && response->cookies().front() == "a=1", "cookie lost");
    require(response->body() == "body", "body lost");
  }
}

int main()
{
  try
  {
    check_headers();
    check_arena_strings();
    std::cout << "HttpResponseHeadersTest: PASS\n";
    return 0;
  }
  catch (const std::exception& ex)
  {
    std::cerr << ex.what() << '\n';
    return 1;
  }
}
