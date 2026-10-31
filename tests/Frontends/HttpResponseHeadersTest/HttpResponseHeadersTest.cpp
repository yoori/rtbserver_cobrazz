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
    std::cout << "HttpResponseHeadersTest: PASS\n";
    return 0;
  }
  catch (const std::exception& ex)
  {
    std::cerr << ex.what() << '\n';
    return 1;
  }
}
